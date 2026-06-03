# Tiger Compiler Architecture Notes

## Overall Pipeline

```
.tig source
    │
    ▼
[tiger.lex → lex.yy.c]  词法分析: tokenize
    │
    ▼
[tiger.grm → y.tab.c]   语法分析: tokens → AST (A_exp)
    │
    ▼
[semant.c + translate.c] 语义分析 + IR 翻译
    │  AST → Tr_exp → T_stm/T_exp (IR tree)
    │  同时产出 F_fragList (函数体片段列表)
    ▼
[canon.c]                IR 规范化
    │  C_linearize → C_basicBlocks → C_traceSchedule
    │  消除 SEQ/ESEQ, 组织基本块, 重排块顺序
    ▼
[codegen.c / codegen_arm64.c]  代码生成 (Maximal Munch)
    │  T_stmList → AS_instrList (汇编指令列表)
    │  - codegen.c:     自定义 VM 指令 (push/mov/add/call/ret...)
    │  - codegen_arm64.c: ARM64 真机指令 (ldr/str/stp/bl/ret...)
    ▼
[regalloc.c]            寄存器分配 (线性扫描)
    │  AS_instrList → AS_instrList (虚拟寄存器 → 物理寄存器)
    │  可选 spill (不够放栈上)
    ▼
输出
    ├── VM 路径:  .txt 汇编文件 → C++ VM (all/VM/) 解释执行
    └── ARM64 路径: .s 汇编文件 → cc 编译 + runtime_arm64.c → 原生执行
```

## 关键数据结构

### AST (absyn.h)
- `A_exp`: 表达式节点 (var, int, string, call, op, let, if, while, for, array...)
- `A_dec`: 声明节点 (varDec, typeDec, functionDec)

### IR Tree (tree.h)
- `T_stm`: 语句 (SEQ, LABEL, JUMP, CJUMP, MOVE, EXP)
- `T_exp`: 表达式 (BINOP, MEM, TEMP, ESEQ, NAME, CONST, CALL)
- `Temp_temp`: 虚拟寄存器, num 从 100 递增, 在 Temp_name() 中映射到名字字符串

### Assembly (assem.h)
- `AS_instr`: 一条汇编指令
  - I_OPER: 通用操作 (template string + dst list + src list + jump targets)
  - I_LABEL: 标签
  - I_MOVE: 移动操作 (template + dst + src)
  - 格式串中用 `d0, `s0, `s1, `j0 等占位符
- `AS_instrList`: 指令链表

### Frame (frame.h)
- `F_frame`: 栈帧描述
- `F_access`: 变量存储位置 (inFrame = 栈上, inReg = 寄存器)
- `F_accessList F_formals(F_frame)`: 函数形参列表
  - formals->head = static link 的访问路径 (offset 16)
  - formals->tail->head = 第一个实参 (offset 24)
- `FRAME_WORD_SIZE = 8`: 机器字长 (64位)

### 函数调用约定 (VM/ARM64 统一)

栈帧布局:
```
高地址
  [fp+24] = 第一个实参
  [fp+16] = static link (指向外层函数的 fp)
  [fp+8]  = 返回地址 (x30/lr)
  [fp+0]  = 保存的 fp (x29)
  [fp-8]  = 第一个局部变量
  [fp-16] = 第二个局部变量
  ...
低地址
```

函数调用流程:
1. 调用者: 在栈上 push 实参, 再 push static link (= 自己的 fp)
2. `bl` 指令: 设置 x30 = 返回地址
3. 被调用者序言: 保存 x30, x29, 设置 fp=sp, 分配局部变量
4. 被调用者尾声: 恢复 sp, x29, x30, ret

对于 main 函数 (没有外部 static link):
- 序言中 push xzr (0) 作为 fake static link
- 尾声: mov x0, #0; bl _exit (直接退出)

---

# 任务一: 有限寄存器分配 (线性扫描)

## 原理

### 问题
编译器(如 codegen.c)生成的代码使用无限多个虚拟寄存器 (r100, r101, r102...).
真实 CPU 只有有限个物理寄存器 (ARM64 约 30 个, 实际可用更少).

### 核心概念: Live Interval (活跃区间)
每个虚拟寄存器有一个 "生命期": 从第一次被定义 (写入) 到最后一次被使用 (读取).
在这段时间内, 寄存器的值必须保持.

```
指令 0:  r100 = 3          ← r100 定义
指令 1:  r101 = r100 + 1   ← r100 使用, r101 定义
指令 2:  push r101         ← r101 使用 (之后 r101 死亡)
指令 3:  r102 = 10         ← r102 定义
指令 4:  print(r102)       ← r102 使用 (之后 r102 死亡)
```

Live intervals:
- r100: [0, 1]  (定义在0, 最后一次使用在1)
- r101: [1, 2]
- r102: [3, 4]

r100 和 r101 的区间有重叠 (都在指令1存活), 它们冲突, 不能共享同一个物理寄存器.
r100 和 r102 的区间不重叠, 可以共享.

### 线性扫描算法

1. 计算所有虚拟寄存器的 live interval
2. 按 interval 的起点排序
3. 维护 active 列表 (当前存活的 interval, 按终点排序)
4. 逐个处理:
   - 从 active 中移除已经结束的 (终点 < 当前起点)
   - 如果有空闲物理寄存器 → 分配给当前 interval
   - 如果物理寄存器满了 → 比较当前 interval 和 active 中最远的终点
     - 当前更远: spill 当前 (不分配寄存器, 放到栈上)
     - active 中某个更远: spill 那个, 把寄存器给当前

### Spill (溢出)
当物理寄存器不够时, 部分变量的值必须"溢出"到栈上保存:
- 使用前: `mov Rx, [fp+offset]` (从栈加载到寄存器)
- 定义后: `mov [fp+offset], Rx` (从寄存器存回栈)

常用一个专门的 "spill scratch register" 来做临时中转.

## 实现细节

### 文件: regalloc.c / regalloc.h

```
RA_linearScan(frame, ilist) → AS_instrList (重写后的指令列表)
```

步骤:
1. 遍历指令列表, 统计每个虚拟寄存器第一次和最后一次出现的位置 → live interval
2. 过滤: 特殊寄存器 (ebp/esp/eax/void) 和标签 (L开头) 不参与分配
3. 线性扫描: 8个物理寄存器 (6可分配 + 2 spill), 分配或 spill
4. 改写指令:
   - 非 spill 的虚拟寄存器 → 物理寄存器名 (R0-R5 或 x19-x24)
   - spill 的: 插入 load/store 指令, 用 spill 寄存器中转
5. 更新 Temp_name() 映射: 虚拟寄存器 → 物理寄存器名字符串
6. 扩展栈帧: 更新 `sub sp` 指令, 为 spill slot 腾出空间

### 配置结构 (RA_Config)

```c
typedef struct {
    int num_regs;      // 物理寄存器总数 (含 spill)
    int num_alloc;     // 可分配的数量
    char **reg_names;  // 寄存器名字符串数组
} RA_Config;
```

VM 模式: `{"R0","R1","R2","R3","R4","R5","R6","R7"}`, num_alloc=6

ARM64 模式: `{"x19","x20","x21","x22","x23","x24","x25","x26"}`, num_alloc=6

### ARM64 寄存器选择说明

ARM64 模式使用 **callee-saved 寄存器 (x19-x24)** 作为分配寄存器, 而非 VM 模式的 caller-saved 寄存器 (x9-x14).
原因: 编译器生成的代码会调用 C 函数 (如 `malloc`, `printInt`, `stringEqual`). AAPCS64 调用约定规定 caller-saved 寄存器 (x0-x18) 在函数调用后可能被破坏. 使用 callee-saved 寄存器可以确保程序变量在 bl 指令前后保持其值, 无需额外的 save/restore.

x25-x26 作为 spill scratch 寄存器 (不被 callee-saved 保证, 但 spill 的 load/store 不跨越函数调用).

### 已知问题
- 多条 spilled 操作数在同一指令中时, 共享 spill 寄存器导致值覆盖 (需第二 spill 寄存器或重组指令)
- 当前 8-queens 程序在 6 个分配寄存器下产生 0 spill, 所以 spill 机制未充分测试

---

# 任务二: ARM64 原生代码生成

## 原理

将 Tiger 编译器后端从自定义 VM 指令切换到 ARM64 真机指令.
流水线前端 (lex/parse/semant/translate/canon) 完全不变, 只改 codegen 和运行时.

### 关键变更

#### 1. 指令映射

| VM 指令 | ARM64 指令 | 说明 |
|---------|-----------|------|
| push src | sub sp, sp, #8; str src, [sp] | 入栈 (拆为两步, 避免 str writeback 破坏对齐) |
| mov dst, src | mov dst, src | 寄存器复制 |
| mov dst, #N | mov dst, #N | 加载立即数 |
| mov dst, [src+N] | ldr dst, [src, #N] | 从内存加载 |
| mov [dst+N], src | str src, [dst, #N] | 存储到内存 |
| add/sub/mul/div | add/sub/mul/sdiv | 算术 |
| cmp a, b | cmp a, b | 比较 |
| je/jne/jl/jg/jle/jge | b.eq/b.ne/b.lt/b.gt/b.le/b.ge | 条件跳转 |
| jmp label | b label | 无条件跳转 |
| call label | bl label | 函数调用 |
| ret | ret | 返回 |

栈帧保存/恢复: 不使用 `stp/ldp` 写回模式 (如 `stp x29, x30, [sp, #-16]!`), 而是统一使用 `sub sp, sp, #N` 预分配, 再用普通 `str` 逐条存储.
原因: `stp` 的 offset 必须 8 字节对齐, 当局部变量不是 16 字节整数倍时会产生对齐问题. 拆分为 `sub sp` + `str` 可以精确控制偏移量.

#### 2. 栈帧

ARM64 严格要求 16 字节对齐. 使用 `sub sp, sp, #N` 统一分配, 避免多次 `str ... !` 造成的对齐破坏.

#### 3. 外部 C 函数调用

VM 中外部函数 (printInt, print, malloc...) 由 VM 解释器特殊处理.
ARM64 原生模式下, 外部函数是真实的 C 函数, 通过 AAPCS64 调用约定传递参数:
- 参数在 x0-x7 中
- `bl _printInt` 直接调用

实现: munchArgs 将参数推到栈上, 然后 `emit_extern_bridge()` 从栈加载到 x0-x7, 最后 `bl`.

C 函数名在 C 编译后自动加 `_` 前缀: `printInt` → `_printInt`.

#### 4. 字符串标签

Tiger 中 `"hello"` 在 IR 中作为 T_NAME 节点 (标签).
ARM64 中需要加载标签地址: `adrp xD, _label@PAGE; add xD, xD, _label@PAGEOFF`.

### 文件

- `codegen_arm64.c/h`: ARM64 代码生成器 (~450 行)
- `runtime_arm64.c`: C 运行时 (~30 行)
- `regalloc_arm64.h`: ARM64 寄存器配置

### 测试状态

- `add.tig` (1*3+2*4): 通过, 退出码 0
- `sl.tig` (printInt(11)): 通过, 正确输出 11
- `king.tig` (8皇后): 通过, 全部正确运行
- 基准数据 (king.tig, 8-queens): VM 解释执行约 0.93s. ARM64 原生执行约 1.2s (待实际测量确认).

### 实现过程中发现的 Bug 及修复

#### Bug 1: RA 帧调整破坏了 sub 指令

**症状**: 寄存器分配阶段扩展 `sub sp, sp, #N` 为帧预留 spill slot 时, 匹配模式 `sub \`d0` 过于宽泛, 也匹配了 `sub xD, xA, xB` 形式的算术减法指令, 错误地将其改写为 `sub x11, 0`.

**修复**: 限制模式匹配只匹配 `sub sp, sp, #N` 的形式, 不匹配 `sub xA, xB, xC` 的三操作数减法.

#### Bug 2: T_MOVE MEM 存储的双重偏移

**症状**: `munchExp(MEM)` 在计算内存地址时已经包含了 offset (生成 `[base, #offset]`), 然后 T_MOVE 的 emit 又叠加了一次 offset, 导致实际存储地址为 `base + 2*offset`, 写入错误的内存位置.

**修复**: T_MOVE MEM 存储时不再额外加 offset, 直接使用 munchExp 已计算好的地址.

#### Bug 3: munchArgs 参数顺序反转

**症状**: `munchArgs` 递归处理实参列表时, 先处理 tail 再处理 head (递归尾优先), 结果压栈顺序与 IR 期望顺序相反. IR 期望第一个实参在栈顶 (最先被 push), 但递归实现将最后一个实参先压栈.

**修复**: 使用计数器确定栈偏移量, 将每个实参写入正确的位置, 而不是依赖递归调用的自然顺序.

#### Bug 4: C 函数调用破坏寄存器

**症状**: 程序在调用 `bl _printInt` 等 C 函数后, 之前计算的值被破坏. AAPCS64 规定 x0-x18 为 caller-saved 寄存器, `bl` 调用后的 C 代码可以随意修改它们. 最初使用 x9-x14 作为分配寄存器, 这些寄存器在 `bl` 后内容不可靠.

**修复**: 将分配寄存器从 caller-saved (x9-x14) 切换为 callee-saved (x19-x24). callee-saved 寄存器由被调用者保证恢复原值, 跨函数调用安全.

#### Bug 5: ExternCall 丢失 static link

**症状**: 外部 C 函数 (如 `initArray`, `malloc`, `stringEqual`) 没有 static link 概念, Tiger 调用约定要求在实参之前 push static link. 但 codegen 在调用外部函数时跳过了第一个参数 (static link), 导致实参偏移量整体错位.

**修复**: 外部函数调用时, 从 formal 实参列表的第二个元素开始 (跳过 static link), 只传递真正的函数参数给 C 函数.

#### Bug 6: 帧指针指向位置错误

**症状**: `fp` 指向帧底部 (栈高地址方向) 而非 saved-registers 区域, 导致局部变量的 `[fp-N]` 偏移计算出错, 变量读取/写入错误的栈位置.

**修复**: 调整序言中 `fp` 的设置, 使其指向保存的 `x29/x30` 区域顶部 (即 `[fp+0]` = 保存的 fp, `[fp+8]` = 返回地址), 与帧布局约定一致.

### 已知问题
- ~~for 循环语法运行在每个 Tiger 程序时报告 `Bad file descriptor`~~ (已修复)
- ~~`king.tig` (8皇后) 输出错误~~ (已修复, 即 Bug 1-Bug 6 综合修复后全部通过)
- 多条 spilled 操作数在同一指令中时, 共享 spill 寄存器导致值覆盖
