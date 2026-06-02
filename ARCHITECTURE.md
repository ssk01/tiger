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
   - 非 spill 的虚拟寄存器 → 物理寄存器名 (R0-R5 或 x9-x14)
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
ARM64 模式: `{"x9","x10","x11","x12","x13","x14","x25","x26"}`, num_alloc=6

## 已知问题
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
| push src | str src, [sp, #-8]! | 入栈 (改为 sub sp + str 避免对齐问题) |
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
- `king.tig` (8皇后): 有已知问题 ("sub xN, 0" 指令)
