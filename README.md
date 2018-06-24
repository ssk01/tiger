# Modern CompilerImplementation in C
现代编译原理（即虎书）
# In me the tiger sniffe the rose
虎书前12章是基础部分，完成一个简单的编译器tiger，后面涉及tiger语言为基础的GC、函数式、OO、多态等高级功能。   
我完成了前10chap的大多数功能，10,11章是关于寄存器分配的。我选择了自己实现一个弱鸡的VM来跳过这两章。该VM可以的寄存器个数可以有无数个，所以不用考虑寄存器溢出的问题，不过变量还是事实上存活在虚拟的栈或堆上。
# 未完的坑
内存相关，程序输入输出，字符串相关处理，程序部分类型推导功能的验证。
# 程序例子
八皇后
~~~
/* A program to solve the 8-queens problem */

let
    var N := 8
	var Num := 0
    type intArray = array of int

    var row :=  [ N ] of 0:intArray
    var col :=  [ N ] of 0:intArray
    var diag1 :=  [N+N-1] of 0:intArray
    var diag2 :=  [N+N-1] of 0:intArray

    function printboard() =
       (for i := 0 to N-1
	 do (for j := 0 to N-1 
	      do print(if col[i]=j then "O" else ".");
	     print("newline"));
         print("newline"); 
		 Num := Num+1)

  
    function try(c:int) = 
( 
     if c=N
     then printboard()
     else for r := 0 to N-1
	   do if row[r]=0 & diag1[r+c]=0 & diag2[r+N-1-c]=0
	           then (row[r]:=1; diag1[r+c]:=1; diag2[r+N-1-c]:=1;
		         col[c]:=r;
	                 try(c+1);
			 row[r]:=0; diag1[r+c]:=0; diag2[r+N-1-c]:=0)

)
 in try(0);
 printInt(Num);
end

~~~
链表合并
~~~
let 

 type any = {any : int}
 var buffer := getchar()

function readint(any: any) : int =
 let var i := 0
     function isdigit(s : string) : int = 
		  ord(s)>=ord("0") & ord(s)<=ord("9")
     function skipto() =
       while buffer=" " | buffer="\n"
         do buffer := getchar()
  in skipto();
     any.any := isdigit(buffer);
     while isdigit(buffer)
       do (i := i*10+ord(buffer)-ord("0"); buffer := getchar());
     i
 end

 type list = {first: int, rest: list}

 function readlist() : list =
    let var any := any{any=0}
        var i := readint(any)
     in if any.any
         then list{first=i,rest=readlist()}
         else nil
    end

 function merge(a: list, b: list) : list =
   if a=nil then b
   else if b=nil then a
   else if a.first < b.first 
      then list{first=a.first,rest=merge(a.rest,b)}
      else list{first=b.first,rest=merge(a,b.rest)}

 function printint(i: int) =
  let function f(i:int) = if i>0 
	     then (f(i/10); print(chr(i-i/10*10+ord("0"))))
   in if i<0 then (print("-"); f(-i))
      else if i>0 then f(i)
      else print("0")
  end

 function printlist(l: list) =
   if l=nil then print("\n")
   else (printint(l.first); print(" "); printlist(l.rest))

   var list1 := readlist()
   var list2 := (buffer:=getchar(); readlist())


  /* BODY OF MAIN PROGRAM */
 in printlist(merge(list1,list2))
end
~~~
