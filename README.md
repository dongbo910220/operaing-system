## 虚拟机选择

+ bochs 
  + 内存 megs: 32mb
  + 启动方式 boot：disk 
  + logs: boots.logs
  + Mouse: enabld=0 
  + Keyboard_mapping: enabled=1
  + ata0: enabled=0 (size=60M)



## BIOS

#### ==内存布局==（实模式 1mb）

| start | end   | size        | utility                                             |
| ----- | ----- | ----------- | --------------------------------------------------- |
| FFFF0 | FFFFF | 16B         | BIOS starter (code:  ``` jmp f000:e05b``` 跳转指令) |
| F0000 | FFFFF | 64KB        | BIOS                                                |
| C8000 | EFFFF | 160KB       | 映射硬件适配器的ROM 或者 内存式映射I/O              |
| C0000 | C7FFF | 32KB        | display adaptor BIOS                                |
| B8000 | BFFFF | 32KB        | for text display                                    |
| B0000 | B7FFF | 32KB        | for black and white display                         |
| A0000 | AFFFF | 64KB        | for colorful display                                |
| 7E00  | 9FBFF | about 608KB | Extended BIOS Data Area                             |
| 7C00  | 7DFF  | 512B        | MBR loaded here by BIOS (0 盘 0 道 1 扇区)          |
| 500   | 7BFF  | about 30KB  | Can be used                                         |
| 400   | 4FF   | 256B        | BIOS Data Area                                      |
| 000   | 3FF   | 1KB         | Interrupt Vector Table                              |



开机流程的一瞬间： CS：IP 被强制初始化为 bios 开始处，随后跳转执行BIOS...

BIOS 具体做了 检查内存， 显卡等外设信息，检查通过，并初始化硬件后，在IVT 表中构建 数据结构 ，并填写中断例程。类似于spring 开始时先扫描包，随后进行依赖注入等。最后检验 (0 盘 0 道 1 扇区)处内容，满足条件后加载到0x7c00 并跳转 ``` jmp 0: 0x7c00```

写入磁盘有关指令：

``` sh
dd if=... of=... count=... seek=... conv=notrunc
```

+ if 读取位置
+ of 写入位置 
+ count 块数
+ seek 跳过的块数
+ conv=notrunc 不打断文件



==通用寄存器^(实模式下)^==

| register | 助记名称       |
| -------- | -------------- |
| ax       | 累加器         |
| bx       | 基址寄存器     |
| cx       | 计数器         |
| dx       | 数据寄存器     |
| si       | 源变址寄存器   |
| di       | 目的变址寄存器 |
| sp       | 栈指针寄存器   |
| bp       | 基址指针       |

+ 基址寻址中，默认 使用ds + bx /ss + bp. for exp: mov [bx], 0x5   put immediate 0x5 to ds: bx



==ret 与 retf 的区别==

ret 在 ss : sp 处弹出2字节 替换 ip 

retf : (return far) 在 ss : sp 处弹出4字节 替换 ip ， cs



IO接口通信方式：

+ 读：

  + 16位： in ax, dx. (dx 存放 端口号 For exp ``` mov dx, ax```)
  + 8位： in al, dx

+ 写：

  + 16位： out dx, ax  /     out immediate, ax
  + 8位： out dx, al. /     out immediate, al 

  

  

  

显卡通信方式：

这里重点关注文字： 直接在 0xb8000 ~ 0xbFFFF处写即可（会通过地址映射到显存，随后显示在屏幕）



## bochs 调试常用指令

+ set	  for exp set reg=val 设置某个寄存器的值
+ show int      中断提示（including softens extint iret）
+ c      continue 知道 断点 
+ s.  [count] 单（多）步调试
+ 断点类
  + vb [seg: off].  为虚拟地址添加断点 
  + lb [addr] 为线性地址添加断电
  + pb [addr] 为物理地址添加断点 
  + sba [count] 执行第count数指令时中断，从0开始计数 
  + watch r [phy_addr]. 如果物理地址phy_addr有读操作则停止 
  + watch w [phy_addr]. 如果物理地址phy_addr有写操作则停止 
+ cpu and memory content
  + x. /nuf [addr].  显示线性地址内容  n 显示的单元数 u 字节数 f 进制  for exp:  **x /40bx 0x13e**
  + Setpmem [phy_addr] [size] [val] 设置物理地址val 连续size个单位
  + r 显示通用寄存器的val 
  + Print-stack [num] 显示栈 （num指定条目数 caution: 低地址在上，与实际相反）
  + info + /b 查看断点
  + info cpu 查看所有寄存器的val
  + info idt 显示中断向量表 
  + Info gdt 
  + info ldt 显示局部描述符表 
  + info tss 显示任务状态段 
  + info int 显示中断向量表
  + page linea-ddr 显示线性地址到物理地址的映射 





## 操作硬盘

+ 设置要读取的扇区。  ``` mov dx, 0x1f2.   out dx, XXX ```

+ 将LBA（logical Block Address）写入 0x1f3 ~ 0x1f6 

  + ​                 

        ;LBA地址7~0位写入端口0x1f3
        mov dx,0x1f3                       
        out dx,al       
        
        ;LBA地址15~8位写入端口0x1f4
        mov cl,8
        shr eax,cl
        mov dx,0x1f4
        out dx,al
          
        ;LBA地址23~16位写入端口0x1f5
        shr eax,cl
        mov dx,0x1f5
        out dx,al
          
        shr eax,cl
        and al,0x0f	   ;lba第24~27位
        or al,0xe0	   ; 设置7～4位为1110,表示lba模式
        mov dx,0x1f6
        out dx,al

+ 写入读命令 向0x1f7端口写入读命令，0x20 

  + ``` mov dx,0x1f7
        mov al,0x20                        
        out dx,al  
    ```

+ 检测硬盘状态  同0x1f7端口 读入检查第七位 

+ 循环从 0x1f0 读数据 次数由扇区数决定





## 保护模式

+ Why?
  +  实模式下操作系统与用户在同一特权级
  +  用户程序所使用的都是真实的物理地址
  +  用户可自由修改段基址
  +  只有20条地址线，最大只能索引1mb
+ advantage
  + 寄存器扩展 ax -> eax 
  + 寻址扩展 
    + 实模式 bx/bp + si/di  + immediate 
    + 保护模式 esi/edi/ebp/esp + 通用reg (除了esp) * factor(1/2/4/8)  + immediate





开启保护模式：

+ 打开A20地址线

  +  ```in al,0x92; or al,0000_0010B; out 0x92,al```

+ 加载全局描述符表

  + 记载后访问地址时cpu硬件负责检测是否越界
  + 加载时监测 rpl <= dpl and cpl <= dpl
  + ```lgdt [ptr]``` ptr存放gdt的内存地址

+ 将cr0(控制寄存器)的pe位置1 

  + ```mov eax, cr0; or eax, 0x00000001; mov cr0, eax``` 随后通过jmp selector:XXX 刷新流水线

+ tips: 注意[bits16] [bits32]的汇编切换 

  + 加载时只有可执行的段描述符才能加载到cs寄存器中 
  + 可执行的段描述符也只能加载到cs中
  + 可写的段描述符才能加到ss中
  + 至少可读的段才能加载到ds, es,fs,gs中
  + 开启保护模式不等于开启分页了（虽然一般都会开启）
  + 段描述符加载后会在A位置1，操作系统可定期清0（此功能未实现）

  

### 分页

> 多进程使用内存，当出现整块区域不够的时候，内存只能等待or腾出空间（当物理能存过小时，就无法晕运行成功程序了，颗粒度太大）

#### 解决方案

一级页表

+ 开启前线性地址 = 物理地址 
+ 开启后线性地址 = 虚拟地址
+ 开启分页后由页部件进行相关转换
+ 问题 一级页表需要一次性的都建好 这样才能保证映射关系，无法动态创建，所以有了二级页表



二级页表创建

+ 构造页目录与页表
+ 用cr3加载页目录 
  + ```mov eax page_dir_table_pos; mov cr3 eax;```
+ 打开cr0的pg位
  + ​	```; 打开cr0的pg位(第31位)
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax```
+ 在开启分页后,用gdt新的地址重新加载 lgdt [gdt_ptr]             ; 重新加载





## 内核加载

``` ld XXX -Ttext 地址 -e 入口 -o xxx.bin```

类似这种程序入口，程序的大小（size）由程序头来记录，另外与程序体共同组成



### elf格式的二进制文件

---

由于多个section连接后形成segment，  其中 section 和 segment 大小与数量都无法确定，所以用一种新的数据结构来描述他们，program header table 程序头表(本质就是段头表)与 section header table节头表

elf 简要组成： elf头 + 程序头表 + 节头表 + 文件体（body）

elf主要包括 程序头表的大小（每个） + 文件中偏移量 + 数目等信息 以及 节头表的大小（每个entry） + 文件中偏移量 + 数目等信息 ，作用相当于一级索引 （总索引）

程序头主要记录 本段在elf中的位置， 程序中的虚拟地址， 以及文件中的大小， 程序中的大小（通常一样，例外是包含了bss节的数据段）,以及权限信息（读，写，执行等等）



| start    | end       | size            | utility                                                      |
| -------- | --------- | --------------- | ------------------------------------------------------------ |
| FFFF0    | FFFFF     | 16B             | BIOS starter (code:  ``` jmp f000:e05b``` 跳转指令)          |
| F0000    | FFFFF     | 64KB            | BIOS                                                         |
| C8000    | EFFFF     | 160KB           | 映射硬件适配器的ROM 或者 内存式映射I/O                       |
| C0000    | C7FFF     | 32KB            | display adaptor BIOS                                         |
| B8000    | BFFFF     | 32KB            | for text display                                             |
| B0000    | B7FFF     | 32KB            | for black and white display                                  |
| A0000    | AFFFF     | 64KB            | for colorful display                                         |
| **7E00** | **9FBFF** | **about 608KB** | **Extended BIOS Data Area**。（选择这里）                    |
| 9f000    |           |                 | 内核栈的栈底                                                 |
| 70000    | 9FBFF     | 190KB           | Kernel.bin(不超过100KB) 可被覆盖                             |
| 7E00     | 6FFFF     |                 | 本次未使用                                                   |
| 7C00     | 7DFF      | 512B            | MBR loaded here by BIOS (0 盘 0 道 1 扇区)                   |
| **500**  | **7BFF**  | **about 30KB**  | **can be used** （loader.bin放在这里）                       |
| 1500     | 7BFF      | about 26KB      | Kernel在内存中位置（不够可以继续使用mbr,与kernel.bin中的内存位置） |
| 10d0     | 1500      |                 | 其中一部分未使用 另一部分是文件头部分                        |
| 900      | 10d0      | 2KB             | Loader.bin                                                   |
| 400      | 4FF       | 256B            | BIOS Data Area                                               |
| 000      | 3FF       | 1KB             | Interrupt Vector Table                                       |

copy方式：

先把kernel.bin的elf文件 从硬盘拷入指定位置（0x70000）,接着根据elf头中的位置找到第一个程序头（segment）记录在ebx(通常充当基址), 然后 dx 记录每个program header 大小, cx 记录多少个 program header，开始循环，然后利用栈 ，根据头信息分别压入 原地址， 目的地址， 大小即可完成拷贝，随后选取下一个程序头，继续当前操作， 

拷贝时 先用cld 清楚flags DF 位清0， 表示esi, edi 会随着指令自增

（ mov edi, [ebp + 8]	   ; dst
   mov esi, [ebp + 12]	   ; src
   mov ecx, [ebp + 16]	   ; size
   rep movsb		   ; 逐字节拷贝）



### 特权级

---

特权级的检查发生在加载的一刹那，对访问者与受访者的特权级的匹配进行检查。

处理器在不同的特权级下，用不同的栈，否则会非常混乱。

特权级的转移方式：通过中断门，调用门等方式从低到高转移，然后通过返回指令返回到低特权级。

从低到高转移时，从tss中找到高特权级栈的地址 ss, esp, 并自动压入转移前的ss, esp， 返回时即可找到

段寄存器中的低两位为rpl(请求特权级)， cs中的rpl为cpl(表示处理器的当前特权级)， 给cs 赋值需要跳转指令如 call, jmp等

门结构，除任务门外都对应一段函数，调用门（已停用）通过call or jmp 指令 + 选择子即可调用（调用门在GDT中），而中断门与陷阱门在IDT中。当调用调用门时，CPU硬件可以实现向高特权级转移，用call的话还会在栈中留下返回地址，结束时retf可返回低特权级，（中断门使用iret）

举例调用门使用过程， 现在用户进程栈中压入有关参数，随后处理器从tss中获取相关信息进入新栈，压入之前的ss,esp 以及 自动压入用户进程栈中有关参数，并且记录原cs, eip（P240）， 返回时，retf远转移，根据cs是否变化来判断以后是否还有ss, esp的更换， retf + 参数个数自动跳过中间参数，如果返回的时候发生了特权级的转变，会对四个数据段寄存器ds, es, fs, gs进行检查，如果大于当前特权级，自动置0，到时候会有异常。

引入rpl为了防止程序恶意使用内核中的段，比如拷贝内核信息到用户的数据段里，使用调用门提升特权级时，内核会自动把提交的段选择子中的rpl设置为与用户当前特权级一样的级别，避免这种事情发生，当进行访问时，rpl的级别不够访问内核的段，导致失败.



## 实现打印函数 （对内核的进一步完善）

不同的调用约定 对应不同的调用角色回收栈中参数。对于cdcel来说eax, ecx, edx，由调用者保存，其余register由被调用者保存 系统调用时，五个以下参数传递使用reg，超过五个放入连续的内存空间中，其中小于五个：分别由ebx, ecx, edx, esi, edi放入（注意顺序）

-- --

在汇编语言与C语言进行混合编程的时候，

+ 汇编代码中道出符号的关键字global

+ C语言中只要定义符号是全局的就行（通常无需额外关键字修饰）

+ 引用外部符号都是extern XX



#### 实现打印函数

显卡上的register非常多，这里主要用CRT controller registers,其中使用方法较为：

+ 往 address register (3d4h) 输送要进行操作的端口号，接下来
+ 往data register (3d5h)输送数据即可 （读 or 写）

Put_char 实现思路如下：

+ 备份寄存器，保护现场
+ 获取光标坐标值（为下一个要打印的字符pos）
+ 获取要打印的字符 
+ 判断是否为控制字符（是的话进行处理），如果不是则默认为可见字符，进入输出处理流程 
+ 打印后通常需要判断是否进行滚屏 
+ 更新光标坐标值 
+ 恢复寄存器现场并退出



**Detail** 

+ 备份寄存器，保护现场

```  nasm
pushad (push all double);
mov ax, SELECTOR_VIDEO	       ; 不能直接把立即数送入段寄存器
mov gs, ax
   
```

+ 获取光标坐标值（为下一个要打印的字符pos）

```nasm
;先获得高8位
   mov dx, 0x03d4  ;索引寄存器
   mov al, 0x0e	   ;用于提供光标位置的高8位 (类似于控制字)
   out dx, al
   mov dx, 0x03d5  ;通过读写数据端口0x3d5来获得或设置光标位置 
   in al, dx	   ;得到了光标位置的高8位
   mov ah, al

   ;再获取低8位
   mov dx, 0x03d4
   mov al, 0x0f
   out dx, al
   mov dx, 0x03d5 
   in al, dx

```

+ 获取要打印的字符,判断是否为控制字符（是的话进行处理），如果不是则默认为可见字符，进入输出处理流程 

```nasm
mov ecx, [esp + 36]	      ;pushad压入4×8＝32字节,加上主调函数的返回地址4字节,故esp+36字节
   cmp cl, 0xd				  ;CR是0x0d,LF是0x0a
   jz .is_carriage_return
   cmp cl, 0xa
   jz .is_line_feed

   cmp cl, 0x8				  ;BS(backspace)的asc码是8
   jz .is_backspace
   jmp .put_other	   
```

+ 具体操作就是根据光标位置然后``` mov [gs:bx(光标已放这了)], cl(字符)``` 打印后通常需要判断是否进行滚屏 更新光标坐标值( 与上面类似 ,  省略) 恢复寄存器现场并退出 与上面类似 ,  

```
.put_char_done: 
   popad
   ret
```

实现put_str

+ 实现思路 利用ebx 存放字符串地址 mov ebx [esp + 12]， inc ebx，然后 mov cl, [ebx]即可，随后call put_char, 如果遇到了0，就进行结束的操作 （cmp cl, 0;   jz .str_over）



实现put_int 

+ 4位4位的进行处理 与 9 进行比较 如果大于9随后 + “A” 的ascii， 找个地方缓存一下，总体与put_str类似



## 中断

操作系统本质上是个死循环，依靠中断驱动

中断

+ 外中断 （发起时，中断向量号通过 NMI or INTR 引脚传入，执行相应处理程序）
  + 可屏蔽 from INTR引脚 	eflags 中的IF位 只能屏蔽这种类型的中断，通常有个中断代理接在此处
  + 不可屏蔽 from NMI引脚  一般是灾难性错误（电源掉电，内存读写错误，总线奇偶校验错误）
+ 内中断
  + 软（soft）中断  软件主动发起的中断
  + 异常 执行期间CPU产生的错误引起的 （/ 0）



中断的本质就是来了一个中断信号后，调用相应的中断处理程序。



#### 中断描述符表

中断门包含了处理程序所在的段选择子和段内偏移地址，并且在通过此方式进入中断后，寄存器中的eflags中的IF位自动置0，也就是把中断关闭，这样避免嵌套。

中断门只存在IDT中。

实模式下，在地址0～ 0x3ff的是中断向量表IVT，而中断描述符表不限制地址，放在哪里都可以（到时候IDTR加载），与调用门类似，都是8字节描述

同样，由中断描述符表寄存器加载，lidt (load IDT)指令加载 48位数据，前16位表示段界限，后面是线性基地址。



**中断处理过程 **

对于cpu外来说，由中断代理芯片负责接收（intel 8259A）

对于cpu内部来说，会执行中断向量号对应的中断处理程序。

处理器会根据中断向量号来进行定位中断门描述符，lidt里有地址 + 8 * 中断向量号可找到对应的门描述符，随后处理器进行特权级检查

+ 如果soft int , （主要用于迈向高的特权级别）

  + 当前特权级 cpl 要大于等于门描述符的 DPL(数值上 CPL 小于等于 DPL )，相当于入门门槛，防止用户主动调用用于kernel的例程
  + 接下来判断门描述符记录的选择子所对应的中断处理程序所在的段（有点绕），要求CPL 权限 小于 该段的DPL（比如CPL 3 大于 DPL 0， 除了retf 等返回指令 只能由低向高转移）， 否则会发异常，因为都在内核里了，没必要通过中断来进行权限升级。
+ 如果是外部设备 or 异常引起的
  + 没有soft int 的第一部分
  + 只判断门描述符记录的选择子所对应的中断处理程序所在的段（有点绕），要求CPL 权限 小于 该段的DPL(数值上 CPL 大于DPL 比如 3 > 0)， 否则会发异常，因为都在内核里了，没必要通过中断来进行权限升级。，与soft int 的第二部分一样（有疑问，kernel里面不太对啊，CPL已经是0了， 我觉得这里是小于等于比较合理）



 中断发生后，假设位于CPU外部，eflags中的NT位和TF位会置0，随后IF位也会置0（防止中断嵌套）

中断用iret 进行返回，会弹出数据到cs,eip, eflags等，根据特权级是否变化(处理器用cs 中的c pl与保存的cs 中进行对比)，变化的话，直接弹出数据ss, esp （P309）

中断处理程序中可以将中断的IF位再次打开， cli 指令关中断， sti 指令打开中断，这样直接控制IF位。



 **中断压栈**

中断发生的时候，处理器收到一个中断向量号（由INTR接收到），随后找到对应的中断门描述符，加载代码段选择子（cs）与段内偏移量(esp)，完成了跳转。 同时自动把跳转前面的cs与esp压栈，同时还有eflags，当特权级发生变化的时候，还会把ss和esp进行压栈。P309

+ 如果当前cs与门描述符中所对应的目标代码段的DPL相比，需要向高级转移，也就是CPL权限比DPL要低，就会把旧的ss与esp进行保存随后压栈到新的栈中，新的栈的位置由TSS中的esp和ss确定，但如果发现不需要进行特区级转移，那么就不会压栈ss与esp（因为毫无必要），其他不变，随后压栈eflags,cs, eip,error_code(视情况而定)
+ iret 进行返回时，如果处理器发现cs中的cpl有变化，就会继续弹出栈中数据到ss和esp,在这之前，需要手动跳过error_code，因为不是所有中断处理程序都有，所以需要程序自行判断，且返回后，处理器会检查ds,es,fs,gs等段reg的内容，如果dpl比cpl要高，处理器会把0填入到对应段reg中（GDT中的第0个描述符不可用）



#### 8259A 可编程中断控制器

8259A可以负责所有来自外设的中断，相当于中断的总代理，因为cpu的INTR信号线只有1根，如果多中断同时发生，cpu只能处理1个，那么不得不维护一个中断列表，这样得不偿失，于是就有了8259A。

8259A可以用来管理和控制可屏蔽中断，并对他们进行优先级判决，同时向CPU发送中断向量号，并且可以通过编程控制。并且使这些设备（发送中断的）以为自己直接发给了CPU，不知道有中断代理，其实中断代理在多个中断同时到达后会进行优先级裁决。

多个8259A可通过级联的方式进行组合。

外设本身是不知道中断向量号，是cpu与8259A配合管理中断设备设计出来的。

流程：

+ 中断发生后，8259A会向CPU发送INTE信号，通知有中断
+ 随后CPU将手中的指令执行完成后，会回复一个响应信号，表示自己已经准备好了，这时8259A会做一些操作（略）
+ 随后CPU会通过INTR询问8259A中断号 中断向量号其实由 起始中断向量号 + IRQ接口号组成
+ 8259A发送中断号给CPU（此时通过数据总线）
  + 如果是手动模式，那么中断处理程序结束的时候要发送EOI 代码 （end of interrupt）



构造IDT + 提供中断向量号即可完成

其中构造IDT 包括了构造中断处理程序 随后填充到IDT中（对应的描述符）

##### 准备IDT表格

准备门描述符结构体

```
struct gate_desc {
   uint16_t    func_offset_low_word;
   uint16_t    selector;
   uint8_t     dcount;   //此项固定值，不用考虑
   uint8_t     attribute;
   uint16_t    func_offset_high_word;
};
static struct gate_desc idt[IDT_DESC_CNT];   // idt是中断描述符表,本质上就是个中断门描述符数组
```

随后进行初始化（即填表）

随后加载IDT

```
uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
   asm volatile("lidt %0" : : "m" (idt_operand));
```

**设置8259A**

通过向主片与从片的控制端口与数据端口发送数据 即outb,设置，同时还可通过此方式设置需要屏蔽的中断。具体例如

```nasm
outb (PIC_M_CTRL, 0x11);   // ICW1: 边沿触发,级联8259, 需要ICW4.
outb (PIC_M_DATA, 0x20);   // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
```

此处略

构造IDT中的handler，具体每个中断处理程序所特有的部分，由idt_table中所对应的代码实现,把环境准备好。

==用了何种设计模式==

```
以下是保存上下文环境,此时已经在新的栈中（如果特权级变化）
   push ds
   push es
   push fs
   push gs
   pushad			 ; PUSHAD指令压入32位寄存器,其入栈顺序是: EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI

   ; 如果是从片上进入的中断,除了往从片上发送EOI外,还要往主片上发送EOI 
   mov al,0x20                   ; 中断结束命令EOI
   out 0xa0,al                   ; 向从片发送
   out 0x20,al                   ; 向主片发送

   push %1			 ; 不管idt_table中的目标程序是否需要参数,都一律压入中断向量号,调试时很方便
   call [idt_table + %1*4]       ; 调用idt_table中的C版本中断处理函数
   jmp intr_exit
   
   
intr_exit:	     
; 以下是恢复上下文环境
   add esp, 4			   ; 跳过中断号
   popad
   pop gs
   pop fs
   pop es
   pop ds
   add esp, 4			   ; 跳过error_code
   iretd  ;跳过error_code 或 0
```



  ##### 8253（可编程技术器）

可通过设置来改变8253时钟中断发生的频率，使用硬件定时的好处在于节省处理器的时间，技术工作可由硬件处理器独立完成。设置方法同样是通过想要的频率来计算可设置的各个参数，随后outb向指定port 写入即可，没啥新鲜的（万物皆可抽象的又一次体现）



## 内存管理系统

**ASSERT**

```c
#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

/*
 * __VA_ARGS__ 是预处理器所支持的专用标识符。
 * 代表所有与省略号“...”相对应的参数. 
 * "..."表示定义的宏其参数可变.
 	__FILE__ 表示被编译的文件名，
 	__LINE__ 表示被编译文件中的行号
 	__func__ 表示被编译的函数名
 */
 															
#define PANIC(...) panic_print(__FILE__,__LINE__,__func__,__VA_ARGS__) // 此处编写个负责打印的函数即可  void panic_print(char* filename, int line, const char* func, const char* condition);

#ifdef NDEBUG    /* 可以用gcc -DNDEBUG 来制定系统变量 也可以定义在哪个文件里，但意义不大，这个应该是个灵活机动的事情      */
	#define ASSERT(CONDITION)  (void(0))
#else
	#define ASSERT(CONDITON)  \
	if (CONDITON) {  \
										\
	}	else {						\
			PANIC(#CONDITION);						\    /* 其中#CONDITION 的作用是把 CONDITION 转化为字符串常量 比如 CONDITON = x != 1 将被转化成 “x != 1”  所以panic 中的 __VA_ARGS__ 要用字符串来表达     */
	} \
#endif	

#endif
```

  

#### bitmap位图

  为了更好的利用内存资源，如果用单个字节来表示某个内存单位（页，数组的每一项占用情况等）有时候会很占位置，那么就需要以位图的形式来进行表示，通过初始基地址和位图包含的字节数来描述。

  ```c
  struct bitmap {
     uint32_t btmp_bytes_len;
  /* 在遍历位图时,整体上以字节为单位,细节上是以位为单位,所以此处位图的指针必须是单字节 */
     uint8_t* bits;
  };
  ```

位图这个“类”的相关方法编写思路，编一个根据位置查找的,以及set指定位置的bit值的

```c
//判断bit_idx位是否为1
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx)
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value) 
```

再编写一个找连续个空闲位的，本质上就是一位位的比，直到满足cnt

```c
int bitmap_scan(struct bitmap* btmp, uint32_t cnt)
```



#### 内存管理系统

  为了实现内存的“专款专用“，把内存池分为用户物理内存池，负责分配给用户进程，以及内核物理内存池，负责分配给操作系统使用。内存池中以页为基本单位。将物理内存各自分配一半给内核与用户进程（可调整）（肯定是放在内核里），相比虚拟地址内存池增加了内存容量，因为物理内存是有限的，but虚拟地址近似无限。

```c
/* 内存池结构,生成两个实例用于管理内核内存池和用户内存池 */
struct pool {
   struct bitmap pool_bitmap;	 // 本内存池用到的位图结构,用于管理物理内存
   uint32_t phy_addr_start;	 // 本内存池所管理物理内存的起始地址
   uint32_t pool_size;		 // 本内存池字节容量
};

struct pool kernel_pool, user_pool;      // 生成内核内存池和用户内存池
```



同样，为了对虚拟内存进行管理，给每个用户进程and内核都分配一个虚拟内存池（但都放在内核里，便于操作）,同样设置虚拟的起始地址，通常为8048000。

```c
/* 用于虚拟地址管理 */
struct virtual_addr {
   struct bitmap vaddr_bitmap; // 虚拟地址用到的位图结构 
   uint32_t vaddr_start;       // 虚拟地址起始地址
};

struct virtual_addr kernel_vaddr;	 // 此结构是用来给内核分配虚拟地址 起始地址为K_HEAP_START 大小和 内核物理内存池一样

/* 0xc0000000是内核从虚拟地址3G起. 0x100000意指跨过低端1M内存,使虚拟地址在逻辑上连续 */
#define K_HEAP_START 0xc0100000
```



那么，如何表达这种对应关系，于是就有了页表的概念。同样肯定是每个进程and内核都有一份属于自己的页表，申请内存时，操作系统先从用户进程自己的虚拟地址池中分配空闲地址，然后再从用户物理内存池（所有用户进程共享）分配空闲的物理内存，随后在页表中建立映射关系。

由于之前9f000（见之前的内存分配图）已经被设置为9f000了，其实可以设置为9fc00(因为到9fbff可用)，但这就意味着main的栈会少一些字节，所以还是设置为9f000, 而从**7E00** 到 **9FBFF** 约为608K, 内核也就不到100K，所以放这里够用，所以PCB（main thread）为9e000，为了给kernel 创造尽可能大的空间， 要把物理（kernel + 用户进程）位图紧挨在PCB前面，一页 是 8 * 4096 * 4K = 128MB的字节，预留4页（9e000 - 4000 = 9a000）， 最大可管理512MB的物理空间，足够了

由于低端1MB已经映射到3GB以上了，所以内核里堆的其实地址为0xc0100000。物理地址 0x100000 ~ 0x101fff已经映射为页目录表和页表了，但是0xc100000~0xc101fff并不会映射到这里，只要不在页表添加映射就好了



初始化内存池

```c
static void mem_pool_init(uint32_t all_mem) 
```

已经使用的内存不在姑那里范围之中，只管理空闲的内存，而低端的1mb 可以认为几乎都被使用了，所以不在管理范围之列，当然，提前准备好内核的1GB所用的空间（其实1023项指向 自身，所以少了4MB），也就是769 到 1022 个页目录项共指向254个页表，加上之前的页目录表本身以及低端的的4MB（0 and 768 页目录项同时指向）的页表，1023页表项指向页目录表自身，便于以后操作。所以总计1MB + 256 * 4K 被使用，分配剩余的即可。（平分）

```c
// kernel
struct pool { 
   struct bitmap pool_bitmap;	 // 其中其实地址为 c009a000
   uint32_t phy_addr_start;	 // =200000     低端1mb+ 页表 被使用
   uint32_t pool_size;		 // 约为 15MB
};

// userprog
struct pool { 
   struct bitmap pool_bitmap;	 // 其中其实地址为 c009a1E0  （kernel 占了15MB）
   uint32_t phy_addr_start;	 // 1100000     低端1mb+ 页表 + kernel占据后的其实位置
   uint32_t pool_size;		 // 约为 15MB
};
```



页表与页目录表所对应的都是物理地址，利用页目录表的最后一项指向自己本身这个特性，构造相关操作页表与页目录表的方法。

设计一个的到连续个虚拟页的方法（利用bitmap_scan 找连续个页即可,本质就是操作位图）

```c
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
```

  为了填写物理地址，需要往虚拟地址所对应的pte and pde 填写相应的地址信息，需要得到这两项所对应的虚拟地址（页部件会认为都是虚拟地址进行映射， 利用页目录表的最后一项指向自己本身这个特性 即可）

```c
/* 得到虚拟地址vaddr对应的pte指针*/
uint32_t* pte_ptr(uint32_t vaddr)

/* 得到虚拟地址vaddr对应的pde的指针 */
uint32_t* pde_ptr(uint32_t vaddr) 
```

同样分配一个物理地址，这样才能往页表里填写

```c
/* 在m_pool指向的物理内存池中分配1个物理页,
 * 成功则返回页框的物理地址,失败则返回NULL */
static void* palloc(struct pool* m_pool)
//其中：
uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
return (void*)page_phyaddr;
```

接下来的思路就是在页表添加对应的物理页地址,思路就是通过的导致的pde,pte项，如果pde不存在就分配一个物理页并填入，随后判断页的P位，填入物理页号即可。

```c
/* 页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射 */
static void page_table_add(void* _vaddr, void* _page_phyaddr) 
```

malloc申请连续多个页面,实现思路是一次性申请多个连续虚拟页，随后一个一个申请物理页，并填入using page_table_add（物理页不要求连续）

```c
/* 分配pg_cnt个页空间,成功则返回起始虚拟地址,失败时返回NULL */
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt)
```

为了方便，创建一个从内核中申请 cnt 页内存的函数。

```c
/* 从内核物理内存池中申请pg_cnt页内存,成功则返回其虚拟地址,失败则返回NULL */
void* get_kernel_pages(uint32_t pg_cnt)
```



## 线程

线程与进程类似，都是独立的控制执行流，线程本质上就是一段函数，有自己的栈，一套寄存器映像（如果是进程还有自己的内存资源），在任务调度器眼中，只有执行流才是基本的调度单元，也就是执行单元。进程 = 线程 + 资源。



##### 线程身份问题 PCB

PCB process control block 程序控制块，好似进程（or线程）的身份证，包括了进程状态，PID，优先级等等（存之前内核栈的位置esp， 因为内核中也会发生中断）。这里实现内核线程。

在PCB所在页的顶端，首先准备一个中断栈，即中断后，压进来的结构，CPU会自动压入ss(old), esp(old), eflags, cs. eip, err_code 等， 随后再加上IDT 中handler 一上来压入的那些 ，将这些做成一个结构。

```
以下是保存上下文环境,此时已经在新的栈中（如果特权级变化）
   push ds
   push es
   push fs
   push gs
   pushad			 ; PUSHAD指令压入32位寄存器,其入栈顺序是: EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
   
   ...
   push %1	  ; 此处对应的中断号
```

随后准备 一个线程栈，为切换做准备 ,根据ABI原则 P411，reg ebp, ebx, edi, esi，esp 返回后不能被改变，所以提前保存，统一恢复。==最后的 应该是 function（func_arg）==,应该是第一次调用时 强行修改eip（通过ret, 当然先要把这几个pop出去）,然后执行（eip 指向的位置）function（func_arg）

```c
/***********  线程栈thread_stack  ***********
 * 线程自己的栈,用于存储线程中待执行的函数
 * 此结构在线程自己的内核栈中位置不固定,
 * 用在switch_to时保存线程环境。
 * 实际位置取决于实际运行情况。
 ******************************************/
struct thread_stack {
   uint32_t ebp;
   uint32_t ebx;
   uint32_t edi;
   uint32_t esi;

/* 线程第一次执行时,eip指向待调用的函数kernel_thread  kthread_stack->eip = kernel_thread;
其它时候,eip是指向switch_to的返回地址*/
   void (*eip) (thread_func* func, void* func_arg);

/*****   以下仅供第一次被调度上cpu时使用   ****/

/* 参数unused_ret只为占位置充数为返回地址 */
   void (*unused_retaddr);
   thread_func* function;   // 由Kernel_thread所调用的函数名
   void* func_arg;    // 由Kernel_thread所调用的函数所需的参数
};

/* 由kernel_thread去执行function(func_arg) */
static void kernel_thread(thread_func* function, void* func_arg) {
   function(func_arg); 
}
```

构造线程栈

```c
/* 初始化线程栈thread_stack,将待执行的函数和参数放到thread_stack中相应的位置 */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
```

初始化PCB

```c
/* 初始化线程基本信息 */
void init_thread(struct task_struct* pthread, char* name, int prio)
```

启动线程

```c
/* 创建一优先级为prio的线程,线程名为name,线程所执行的函数是function(func_arg) */
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg) {
/* pcb都位于内核空间,包括用户进程的pcb也是在内核空间 */
   struct task_struct* thread = get_kernel_pages(1);

   init_thread(thread, name, prio);
   thread_create(thread, function, func_arg);

   asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" : : "g" (thread->self_kstack) : "memory");
   return thread;
}

```

利用当前esp（此时一定在kernel里才能使用）获取 PCB

```c
/* 获取当前线程pcb指针 */
struct task_struct* running_thread() {
   uint32_t esp; 
   asm ("mov %%esp, %0" : "=g" (esp));
  /* 取esp整数部分即pcb起始地址 */
   return (struct task_struct*)(esp & 0xfffff000);
}
```

### 实现任务调度

首先在处理时钟中断所对应的中断处理程序中获取PCB，减少它的ticks，当为0时意味着切换时刻的到来，随后进入schedule进行任务调度，选择一个合适的元素，反过来获取其PCB，进行切换（switch_to），同时schedule中进行判断是什么原因导致切换，并设置不同状态

编写特定的时钟中断处理函数

```c
/* 时钟的中断处理函数 */
static void intr_timer_handler(void) {
   struct task_struct* cur_thread = running_thread();

   ASSERT(cur_thread->stack_magic == 0x19870916);         // 检查栈是否溢出

   cur_thread->elapsed_ticks++;	  // 记录此线程占用的cpu时间嘀
   ticks++;	  //从内核第一次处理时间中断后开始至今的滴哒数,内核态和用户态总共的嘀哒数

   if (cur_thread->ticks == 0) {	  // 若进程时间片用完就开始调度新的进程上cpu
      schedule(); 
   } else {				  // 将当前进程的时间片-1
      cur_thread->ticks--;
   }
}
```

随后在注册到idt_table中的中断项即可。

Schedule 通过list中的元素,，选择一个合适的元素，反过来获取其PCB，进行切换（switch_to），同时schedule中进行判断是什么原因导致切换，并设置不同状态

```c
 /* 实现任务调度 */
void schedule() {

   ASSERT(intr_get_status() == INTR_OFF);

   struct task_struct* cur = running_thread(); 
   if (cur->status == TASK_RUNNING) { // 若此线程只是cpu时间片到了,将其加入到就绪队列尾
      ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
      list_append(&thread_ready_list, &cur->general_tag);
      cur->ticks = cur->priority;     // 重新将当前线程的ticks再重置为其priority;
      cur->status = TASK_READY;
   } else { 
      /* 若此线程需要某事件发生后才能继续上cpu运行,
      不需要将其加入队列,因为当前线程不在就绪队列中。*/
   }

   ASSERT(!list_empty(&thread_ready_list));
   thread_tag = NULL;	  // thread_tag清空
/* 将thread_ready_list队列中的第一个就绪线程弹出,准备将其调度上cpu. */
   thread_tag = list_pop(&thread_ready_list);   
   struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
   next->status = TASK_RUNNING;
  // 以后会实现用户进程， process_active(next) 更改页表 以及修改 tss中的esp0
   switch_to(cur, next);
}
```

Switch_to 先保存cur 的 ABI 所要求的寄存器， 随后 将esp指针保留好， 然后开始替换， 同样根据参数 next 拿到esp, 随后一顿pop, 然后一ret, 这里分两种情形

+ 如果是第一次调用， ret 的地方提前放好了要执行的函数kernel_thread 随后 执行function(func_arg); 
+ 如果不是第一次调用，ret 的地方其实是调用switch_to的下一行（在schedule之中），随后跳出schedule,在跳出中断处理函数，比如intr_timer_handler,来到 call [idt_table + XXX]的后面 即 jmp intr_exit，开始一波POP，即线程栈的内容，来到用户处理程序（如果构造用户进程的话，需要在这里放好cs, eip等等）。

```nasm
switch_to:
   ;栈中此处是返回地址	       
   push esi
   push edi
   push ebx
   push ebp

   mov eax, [esp + 20]		 ; 得到栈中的参数cur, cur = [esp+20]  (ebp = [esp + 0])
   mov [eax], esp                ; 保存栈顶指针esp. task_struct的self_kstack字段,
				 ; self_kstack在task_struct中的偏移为0, cur 就是 task_struct
				 ; 所以直接往thread开头处存4字节便可。
;------------------  以上是备份当前线程的环境，下面是恢复下一个线程的环境  ----------------
   mov eax, [esp + 24]		 ; 得到栈中的参数next, next = [esp+24]
   mov esp, [eax]		 ; pcb的第一个成员是self_kstack成员,用来记录0级栈顶指针,
				 ; 用来上cpu时恢复0级栈,0级栈中保存了进程或线程所有信息,包括3级栈指针
   pop ebp
   pop ebx
   pop edi
   pop esi
   ret				 ; 返回到上面switch_to下面的那句注释的返回地址,
				 ; 未由中断进入,第一次执行时会返回到kernel_thread

```

增加一个系统空闲时运行的线程,其中hlt 使系统挂起，达到真正的空闲，可通过外部中断打断，所以需要先开中断“sti”

```c
struct task_struct* idle_thread;    // idle线程

/* 系统空闲时运行的线程 */
static void idle(void* arg UNUSED) {
   while(1) {
      thread_block(TASK_BLOCKED);     
      //执行hlt时必须要保证目前处在开中断的情况下
      asm volatile ("sti; hlt" : : : "memory");
   }
}
// 另外 在schedule中， 添加
/* 如果就绪队列中没有可运行的任务,就唤醒idle */
   if (list_empty(&thread_ready_list)) {
      thread_unblock(idle_thread);
   }
```

实现简单的休眠函数。设计思路，每次yield到时候看轮到自己的时候时间够不够，不够接着让，直到下一次又到自己再看

```c
/* 以tick为单位的sleep,任何时间形式的sleep会转换此ticks形式 */
static void ticks_to_sleep(uint32_t sleep_ticks) {
   uint32_t start_tick = ticks;

   /* 若间隔的ticks数不够便让出cpu */
   while (ticks - start_tick < sleep_ticks) {
      thread_yield();
   }
}

/* 以毫秒为单位的sleep   1秒= 1000毫秒 */
void mtime_sleep(uint32_t m_seconds) {
  uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);
  ASSERT(sleep_ticks > 0);
  ticks_to_sleep(sleep_ticks); 
}
```







## 锁

**公共资源**

被任务共享的一套资源，内存，硬件，文件都可以。

**临界区**

当多个任务共同访问公共资源的时候，描述访问公共资源的代码称为临界区（也就是指令）。

**互斥**

某个时刻公共资源只能被一个任务独享。

为了保证任意时刻只能有一个任务处于临界区，引出了锁的概念 

#### 信号量

信号量本质就是个计数器。肯定是个全局共享变量，它的一系列操作（up and down）都必须是原子的（关中断保证）

-- --

当信号量up的时候：

+ 信号量+ 1
+ 唤醒在此信号量上面等待的线程

当信号量down的时候：

+ 判断是否 > 0
+ 如果 > 0 , 信号量 - 1
+ 如果 = 0， 那么 直接将自己阻塞，并在此信号量上等待（信号量有个list对列）

-- --

所以两个队列获得锁的流程为：

+ A 进入临界区前信号量 down
+ B 进入临界区前信号量 down，此时将自己阻塞，并在此信号量上等待。  （第二步）
+ A从临界区出来后信号量up, 信号量 变为 1， 随后唤醒在此信号量上面等待的线程（B）
+ B再次试图获得锁，成功则进入临界区（只有两个的话基本成功，除非A再次），否则返回第二步

信号量的结构为一个计数器value, 以及一个表示线程的队列

```c
/* 信号量结构 */
struct semaphore {
   uint8_t  value;
   struct   list waiters;
};
```

锁是由信号量组成，同时需要记录持有者，以及为了同一线程重复加锁，要记录锁的持有者重复申请（进入）的次数

```c
/* 锁结构 */
struct lock {
   struct   task_struct* holder;	    // 锁的持有者
   struct   semaphore semaphore;	    // 用二元信号量实现锁
   uint32_t holder_repeat_nr;		    // 锁的持有者重复申请锁的次数
   struct lock lock;               //稍后添加
};

```

对信号量而言，设计其初始化init(),up,down方法即可。

当信号量down的时候：

+ 判断是否 > 0
+ 如果 > 0 , 信号量 - 1
+ 如果 = 0， 那么 直接将自己阻塞(同时让调度器把自己换下)，并在此信号量上等待（信号量有个list对列）

```c
/* 信号量down操作 */
void sema_down(struct semaphore* psema) {
/* 关中断来保证原子操作 */
   enum intr_status old_status = intr_disable();
   while(psema->value == 0) {	// 若value为0,表示已经被别人持有
      ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
      /* 当前线程不应该已在信号量的waiters队列中 */
      if (elem_find(&psema->waiters, &running_thread()->general_tag)) {
	 PANIC("sema_down: thread blocked has been in waiters_list\n");
      }
/* 若信号量的值等于0,则当前线程把自己加入该锁的等待队列,然后阻塞自己 */
      list_append(&psema->waiters, &running_thread()->general_tag); 
      thread_block(TASK_BLOCKED);    // 阻塞线程,直到被唤醒
   }
/* 若value为1或被唤醒后,会执行下面的代码,也就是获得了锁。*/
   psema->value--;
   ASSERT(psema->value == 0);	    
/* 恢复之前的中断状态 */
   intr_set_status(old_status);
}
```

当信号量up的时候：

+ 信号量+ 1
+ 唤醒在此信号量上面等待的线程（通过waiters找到阻塞线程 by list_pop，放回队列即可），其实感觉再加个非空判断（&psema->waiters）比较好（list_pop下面， 如果确实空了，就psema->value++;）否则啥也不干，不过这会涉及到锁的改动，持有者变了，感觉还是应该再加个超级信号量，复用这个结构。

```c
/* 信号量的up操作 */
void sema_up(struct semaphore* psema) {
/* 关中断,保证原子操作 */
   enum intr_status old_status = intr_disable();
   ASSERT(psema->value == 0);	    
   if (!list_empty(&psema->waiters)) {
      struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));   // 注意 这里会pop
      thread_unblock(thread_blocked); 
   }
   psema->value++;
   ASSERT(psema->value == 1);	    
/* 恢复之前的中断状态 */
   intr_set_status(old_status);
}
```





**线程阻塞**

阻塞是自己发出的动作，也就是自己才能阻塞自己，而不是别人阻塞，但是是线程由“别人”进行唤醒的。（阻塞就意味着让调度器找不到“自己”，也就是从就绪队列里换下，这样就无从调度了哈哈，然后调度器进行调度，这时候会调度别的执行流）,在阻塞的过程中要关闭中断，仅仅负责换下，在哪里阻塞不由自己控制

```c
/* 当前线程将自己阻塞,标志其状态为stat. */
void thread_block(enum task_status stat) {
/* stat取值为TASK_BLOCKED,TASK_WAITING,TASK_HANGING,也就是只有这三种状态才不会被调度*/
   ASSERT(((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING)));
   enum intr_status old_status = intr_disable();
   struct task_struct* cur_thread = running_thread();
   cur_thread->status = stat; // 置其状态为stat 
   schedule();		      // 将当前线程换下处理器
/* 待当前线程被解除阻塞后才继续运行下面的intr_set_status 本次无法执行 */
   intr_set_status(old_status);
}

```

与线程阻塞相类似的事thread_yield,区别是主动让出CPU换成其他线程执行即可，也就是还会把自己添加到就绪队列里。

```c
/* 主动让出cpu,换其它线程运行 */
void thread_yield(void) {
   struct task_struct* cur = running_thread();   
   enum intr_status old_status = intr_disable();
   ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
   list_append(&thread_ready_list, &cur->general_tag);  //thread_block没有这项“活动”
   cur->status = TASK_READY;
   schedule();
   intr_set_status(old_status);
}
```



阻塞的线程唤醒是锁的持有者，只有这样才能“拿到”阻塞线程，释放了锁后，就把阻塞线程添加到ready的队列了，并且修改线程状态，以后调度器就可进行调度。

```c
/* 将线程pthread解除阻塞 */
void thread_unblock(struct task_struct* pthread) {
   enum intr_status old_status = intr_disable();
   ASSERT(((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING)));
   if (pthread->status != TASK_READY) {
      ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
      if (elem_find(&thread_ready_list, &pthread->general_tag)) {
	 PANIC("thread_unblock: blocked thread in ready_list\n");
      }
      list_push(&thread_ready_list, &pthread->general_tag);    // 放到队列的最前面,使其尽快得到调度
      pthread->status = TASK_READY;
   } 
   intr_set_status(old_status);
}
```

所以大体思路就是信号量会操作线程，锁来控制信号量，锁的获得就是信号量的down, 锁的释放对应信号量的up,当然还有标记持有者， 重入次数等，贯彻了单一原则。

获得锁

```c
/* 获取锁plock */
void lock_acquire(struct lock* plock) {
/* 排除曾经自己已经持有锁但还未将其释放的情况。*/
   if (plock->holder != running_thread()) { 
      sema_down(&plock->semaphore);    // 对信号量P操作,原子操作
      plock->holder = running_thread();
      ASSERT(plock->holder_repeat_nr == 0);
      plock->holder_repeat_nr = 1;
   } else {
      plock->holder_repeat_nr++;
   }
}
```

释放锁类似，不再赘述。

给输出端加锁，即在临界代码区前后添加获得释放锁的步骤，即可。

```c
static struct lock console_lock;    // 控制台锁

/* 初始化终端 */
void console_init() {
  lock_init(&console_lock); 
}

/* 获取终端 */
void console_acquire() {
   lock_acquire(&console_lock);
}

/* 释放终端 */
void console_release() {
   lock_release(&console_lock);
}

/* 终端中输出字符串 */
void console_put_str(char* str) {
   console_acquire(); 
   put_str(str); 
   console_release();
}

```

#### 键盘输入原理

键盘其实涉及到两个独立功能芯片的配合。键盘本身含有8048芯片，当按下键与放开键的时候，会向8042（主板上的键盘控制器）发送（通码或断码），8042会保存在自己的寄存器中（保存前有可能二次处理），随后通过8259A（中断代理）发送中断信号，随后处理器会启动对应的中断代理程序，把8042处理后的码拿出来（放在自己的寄存器中），进行处理。

其实有不同的键盘扫描码（通码和断码），但是8042类似于tomcat过滤器，把不同的键盘扫描码统一处理成同一种，即第一套扫描码，只需要关注这个即可。所以只要熟悉8042的接口寄存器即可对他进行编程对应的中断处理程序。还是读状态，写数据等等，都是类似的（万物皆可抽象）

**编写键盘驱动**

由于计算机中，硬件是由软件来交互的，所以对硬件的操作要使用软件来告诉他。硬件会给软件提供接口，也就是IO指令进行对它状态的设置与读取，同时对它产生的数据进行交换（read or write asm里就是 intb outb）等等，把这些一系列的有关IO的操作封装成一个过程，这样调用者一调用，等待返回结果就好了，这便是驱动程序。

键盘的中断处理程序每次获取数据，判断通码还是断码，如果是控制类的键，就保存起来（比如 ctrl shift等）已备和后面的一块使用，因为键很多，所以可以用个大表格存储通码所对应的ascii码，可以是二维的，因为会和shift叠加使用，

```c
/* 键盘中断处理程序 */
static void intr_keyboard_handler(void) {

/* 这次中断发生前的上一次中断,以下任意三个键是否有按下 之后有判断过程，这里略过*/
   bool ctrl_down_last = ctrl_status;	  
   bool shift_down_last = shift_status;
   bool caps_lock_last = caps_lock_status;

   bool break_code;
   uint16_t scancode = inb(KBD_BUF_PORT);  //KBD_BUF_PORT 为8042的寄存器（输入与输出）
...略

      uint8_t index = (scancode &= 0x00ff);  // 将扫描码的高字节置0,主要是针对高字节是e0的扫描码.
      char cur_char = keymap[index][shift];  // 在数组中找到对应的字符
   
      /* 只处理ascii码不为0的键 */
      if (cur_char) {
	 put_char(cur_char);    
        // 以后会替换成下面
/*        if (!ioq_full(&kbd_buf)) {
	    ioq_putchar(&kbd_buf, cur_char);
	 } */
	 return;
      }

 ...略
}
```



#### 输入缓冲区

通过键盘的中断处理程序会“得到”一系列符号，需要存在某个地方，随后进行分析处理，通常shell拿走（消费者）。

缓冲区是多个线程共同使用的共享内存，但是在缓冲区的操作方法上要加以注意，保证对缓冲区是互斥访问的，同时要避免越界，设计成环形比较合理（控制好头，尾指针就可以）。同时有个锁（临界区只能有一个任务操作缓冲区），另外需要记录的是如果没有“货”或者货满了，就需要PCB指针记录没有得到满足的生产，消费者，还有头，尾指针，以及缓冲区本身。

```c
struct ioqueue {
// 生产者消费者问题
    struct lock lock;
 /* 生产者,缓冲区不满时就继续往里面放数据,
  * 否则就睡眠,此项记录哪个生产者在此缓冲区上睡眠。 第一候选人， 后续的都在锁里面*/
    struct task_struct* producer;

 /* 消费者,缓冲区不空时就继续从往里面拿数据,
  * 否则就睡眠,此项记录哪个消费者在此缓冲区上睡眠。第一候选人， 后续的都在锁里面*/
    struct task_struct* consumer;
    char buf[bufsize];			    // 缓冲区大小
    int32_t head;			    // 队首,数据往队首处写入
    int32_t tail;			    // 队尾,数据从队尾处读出
};
```

设计初始化的方法，得到下一个位置的方法，是否满，是否空的方法，以及wait的方法（为了此方法能wait 消费者与生产者，传生产者 or 消费者指针，其实这里可以单一职责，一个就是一个，比如ioq_wait_producer and  ioq_wait_consumer,线程自己阻塞）， wake的方法（和wait设计思路差不多，唤醒线程）

getchar是上述的一种综合运用，先关中断，然后如果为空就加锁，（这里感觉两把锁比较好，相当于java 的conditon）然后wait，取走元素，移动游标，（肯定有写的空间了），所以根据情况唤醒生产者，putchar类似～

```c
/* 消费者从ioq队列中获取一个字符 */
char ioq_getchar(struct ioqueue* ioq) {
   ASSERT(intr_get_status() == INTR_OFF);

/* 若缓冲区(队列)为空,把消费者ioq->consumer记为当前线程自己,
 * 目的是将来生产者往缓冲区里装商品后,生产者知道唤醒哪个消费者,
 * 也就是唤醒当前线程自己*/
   while (ioq_empty(ioq)) {
      lock_acquire(&ioq->lock);	 
      ioq_wait(&ioq->consumer);
      lock_release(&ioq->lock);
   }

   char byte = ioq->buf[ioq->tail];	  // 从缓冲区中取出
   ioq->tail = next_pos(ioq->tail);	  // 把读游标移到下一位置

   if (ioq->producer != NULL) {
      wakeup(&ioq->producer);		  // 唤醒生产者
   }

   return byte; 
}

/* 生产者往ioq队列中写入一个字符byte */
void ioq_putchar(struct ioqueue* ioq, char byte)
```

把 ioq_putchar 放在键盘的中断处理程序里即可。==到时候谁来消费打印在屏幕上呢？==



## 构造用户进程

#### 任务状态段TSS

操作系统直接和硬件打交道，硬件原生支持TSS与LDT（未用到LDT）。

TSS描述符在GDT中，（LDT也在），CPU原本计划为每个任务关联一个TSS，但实际没有用，因为要想把各种资源快照存来存去太麻烦了，不如直接用栈。但依然使用一个TSS，当向高特权级跃迁时，会从TSS取到SS0， esp0自动装载(cpu自动加载，TSS毕竟是cpu内部框架原生支持，肯定是自动的，电路确保实现)，加载TSS，ltr + 16位reg or 16位内存单元（2 bytes）

利用 make_gdt_desc 构造TSS描述符（DPL = 0，其实地址指向TSS，提前在内存中预备好，界限就是TSS的大小）， 和用户代码段， 用户数据段描述符（DPL =3，起始地址为0，界限为fffff ）,随后需要重新加载GDT表

```c
/* gdt 16位的limit 32位的段基址 */
   uint64_t gdt_operand = ((8 * 7 - 1) | ((uint64_t)(uint32_t)0xc0000900 << 16));   // 7个描述符大小
   asm volatile ("lgdt %0" : : "m" (gdt_operand));
   asm volatile ("ltr %w0" : : "r" (SELECTOR_TSS));
```

可用info gdt 一查看即可 

GDT中有7个，0为不可用，1为代码段，2为数据段和栈（kernel）,3为显存段，4为TSS段，5为代码段（user）,6为数据段和栈（user）



#### **实现用户进程**

在线程创造的最后。会由ret 指令“返回”到kernel_thread 随后kernel_thread会调用function，即function(arg),如果构造用户进程，可以把function替换为创建进程的新函数即可。

由于进程相比于线程有了自己的一个虚拟地址，所以也要有一个专属于自己的虚拟地址池，同样还要有页目录表，这些都是虚拟地址，到时候可手动将其转换为物理地址以便于加载到cr3中。

```c
/* 用于虚拟地址管理 */=
struct virtual_addr userprog_vaddr; 
struct virtual_addr {
   struct bitmap vaddr_bitmap; // 虚拟地址用到的位图结构 , 	其中包括起始位置bits and 位图中字节数
   uint32_t vaddr_start;       // 虚拟地址起始地址
};
```



**创建页表与3级（用户级）特权级栈**

构造一个可以指定vaddr的函数，也即是vaddr与pf中的物理地址相关联。设计思路为根据指定地址算出虚拟位图所在位，随后置1表示占用，申请一个物理页，再写入页表即可（by page_table_add）。

```c
/* 将地址vaddr与pf池中的物理地址关联,仅支持一页空间分配 */
void* get_a_page(enum pool_flags pf, uint32_t vaddr) 
```

得到某个虚拟地址对应的物理地址,利用pte只能得到该地址所对应的物理页，随后再加上其在页中的相对位置即可。

```c
uint32_t addr_v2p(uint32_t vaddr) {
   uint32_t* pte = pte_ptr(vaddr);
   return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}
```

**特权级栈**

为了从高特权级返回到低特权级，只能是以中断门返回的方式（调用门也可以但本系统不使用）。

+ 为了假装从高特权级返回，需要使用之前进入中断后所对应的从中断返回的程序，即intr_exit
+ Intr_exit会pop registers,可以提前构造好所需要的（在PCB的中断栈中），借着pop的机会进行装载。
+ CPU会在iretd 根据cs中的rpl判断是否发生切换以及随之而来的换栈，所以，要将用户代码段（rpl = 3）提前放好。
+ 栈中所有段选择子的rpl都要为3，否则跳回低特权级栈后，若rpl=0，段选择子会被置为0，从而报错。
+ 退出中断后，elfags中的IF位必须还得设置为1（打开状态）
+ elfags中的IOPL位必须还得设置为0，不允许用户通过IO来直接操作硬件，有事情委托操作系统。



在kernel_thread里，构造内核中断栈所用的,也就是上面的6点。其中PCB中的栈指针一开始指向线程栈，现在要跳过线程栈，指向中断栈，此时已经开始运行当前进程了，线程栈已经没啥意义了,下次再切换switch_to会再次保存新的esp进PCB的栈指针，最后强行把中断栈的指针付给esp，随后jmp intr_exit(一去不复返)。

```c
void start_process(void* filename_) {
   void* function = filename_;
   struct task_struct* cur = running_thread();
   cur->self_kstack += sizeof(struct thread_stack);
   struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;	 
   proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
   proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
   proc_stack->gs = 0;		 // 用户态用不上,直接初始为0
   proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
   proc_stack->eip = function;	 // 待执行的用户程序地址
   proc_stack->cs = SELECTOR_U_CODE;
   proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
   proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE) ; //肯定要以底部地址为基准 进行申请
   proc_stack->ss = SELECTOR_U_DATA; 
   asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (proc_stack) : "memory");
}
// #define USER_STACK3_VADDR  (0xc0000000 - 0x1000)
```

随后设计个函数激活用户进程的页表即可,把PCB的页表项页的指针进行物理地址转换，随后cr3重新加载

```c
/* 击活页表 */
void page_dir_activate(struct task_struct* p_thread)
```



process_activate 先激活页表， 随后如果是用户进程的话，再更新tss中的esp0就行，也就是PCB的栈定，这样下次该用户进程中断进来了以后会找到这里。当然，这个肯定是在shedule()中调用，而且在switch_to之前。

==平坦模式如何保证不对用户代码进行修改==

**bss节简介**

bss为未初始化数据段，在程序文件中无需存在实体，由链接器会把bss节所占用的内存空间大小和合并到数据段之中，到时候操作系统的加载器会把这一部分自动置0.

链接器把目标文件中属性相同的节合并到一起，这样操作系统就可以统一分配内存了。大致可分为：

+ 可读写的数据，数据节.data和未初始化节.bss
+ 只读可执行的代码，代码节.text和初始化代码节.init
+ 只读数据，只读数据节.rodata(字符串一般情况下在这里)

到时候加载后，对的起始地址在用户进程地址最高的段之上就行了。

-- --

构造内核页表，先申请一页，随后把内核页目录项（768～1022），拷贝就可以了，最后一项写入自己的物理地址即可（使用addr_v2p）

```c
uint32_t* create_page_dir(void)
```

接下来是创建用户虚拟地址的位图,在内核中申请页表随后填写即可

```c
/* 创建用户进程虚拟地址位图 */
void create_user_vaddr_bitmap(struct task_struct* user_prog) {
   user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
   uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8 , PG_SIZE);
   user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
   user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
   bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}

//其中 #define USER_VADDR_START 0x8048000
```

创建用户进程的主函数,就是之前的综合运用

```c
/* 创建用户进程 */
void process_execute(void* filename, char* name) { 
   /* pcb内核的数据结构,由内核来维护进程信息,因此要在内核内存池中申请 */
   struct task_struct* thread = get_kernel_pages(1);
   init_thread(thread, name, default_prio); 
   create_user_vaddr_bitmap(thread);
   thread_create(thread, start_process, filename);
   thread->pgdir = create_page_dir();
   
   enum intr_status old_status = intr_disable();
   ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
   list_append(&thread_ready_list, &thread->general_tag);

   ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
   list_append(&thread_all_list, &thread->all_list_tag);
   intr_set_status(old_status);
}
```

等到时候调用的时候，会由kernel_thread调用start_process(user_prog)进行中断栈的“填写”，随后int_exit，进入用户进程的user_prog执行！(补充说明，到时候监测的时候可在user_prog处打断点（先通过nm 查地址，随后lb 打断点 ），随后执行到这里的时候用sreg查看段选择子的后两位即可)



#### 系统调用的实现

-- --

linux中，系统调用是通过中断门来实现的，通过软中断 （soft int）来主动发起中断信号，而且只占用了一个中断号0x80(大名鼎鼎啊)，当 int 0x80执行后，任务陷入内核态（0级栈），（执行前还是用户态),系统调用的参数会放到reg中，要是放在栈里就太麻烦了。

一个系统调用分为两部分，一个暴露给用户进程的接口函数，属于内核空间，是用户进程使用的途径，只负责发需求，另一部分是与之对应的内核具体实现，属于内核空间，完成功能需求，就是系统调用的功能处理函数。

实现思路：

+ 用中断门实现系统调用（0x80号中断）
+ 在IDT中安装0x80中断对应的描述符，在该描述符中注册系统调用对应的中断处理函数。
+ 在该函数中利用eax找到对应的子功能的处理函数（要先建立系统调用的“子”功能表）
+ 用宏实现用户空间系统调用接口_syscall,支持3个参数就够了，其中eax为子功能号，ebx，ecx,edx(bcd哈哈)保存3个参数

在IDT中安装0x80中断对应的描述符,注意此处一定要把特权级DPL置为3，否则无法软中断调用（但是外部设备引发的中断就无所谓）**step 2**

```c
make_idt_desc(&idt[80], IDT_DESC_ATTR_DPL3, syscall_handler);
```

实现调用接口， 注意这里只是得到了返回值，本事没有进行返回，随后还需要return一下 比如 return _syscall3(XXX) **step 4**

```c
#define _syscall3(number, agr1, arg2, arg3) ({      \
    int retval;                                    \
    asm volatile(                               
        "int $0x80"                                \
        :"=a"(retval)                             \
        :"a"(number),"b"(arg1), "c"(arg2), "d"(arg3)  \
        :"memory"
    );
    retval;
})
```

实现syscall_handler，尽力那个使用和intrXXentry中类似的结构，不一样的地方想办法凑出来 **step 2**

```nasm
syscall_handler:
;1 保存上下文环境
   push 0			    ; 压入0, 使栈中格式统一

   push ds
   push es
   push fs
   push gs
   pushad			    ; PUSHAD指令压入32位寄存器，其入栈顺序是:
				    ; EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI 
				 
   push 0x80			    ; 此位置压入0x80也是为了保持统一的栈格式

;2 为系统调用子功能传入参数
   push edx			    ; 系统调用中第3个参数
   push ecx			    ; 系统调用中第2个参数
   push ebx			    ; 系统调用中第1个参数

;3 调用子功能处理函数
   call [syscall_table + eax*4]	    ; 编译器会在栈中根据C函数声明匹配正确数量的参数
   add esp, 12			    ; 跨过上面的三个参数,不跳过第四个是因为保持格式统一，之前intrXXentry中call完之后就直接jmp 了，所以要保持形式的统一

;4 将call调用后的返回值存入待当前内核栈中eax的位置
   mov [esp + （1 + 7）*4], eax	   ; 1+7是算出来的，这样可以把eax带回去， 带到 retval里
   jmp intr_exit		    ; intr_exit返回,恢复上下文
```

syscall_table 是个函数指针数组， 把函数放到数字中对应的项里就可以了， for example

```c
syscall_table[SYS_GETPID] = sys_getpid;
```

填写用户进程的系统调用

```c
/* 返回当前任务pid */
uint32_t getpid() {
   return _syscall0(SYS_GETPID);   // 注意要 return!!!
}

/* 所对应的中断子处理流程 返回当前任务的pid */
uint32_t sys_getpid(void) {
   return running_thread()->pid;
}
```



#### 实现用户进程打印

**可变参数**

C语言当中，有一部分参数的个数是不确定的，这称为可变参数。其实编译器需要提前确定参数个数与类型（这样才能在栈里分配空间），虽说是可变的，但是在编译的时候也能确定下来，毕竟会调用。但是被调用的函数是依据什么查找的呢，就是某种记号，比如“%”，每看到一个，就在栈中查找一次，如果不一致，也是用户自身的行为，没辙hh。(把write进行了封装)

```c
/* 格式化输出字符串format */
uint32_t printf(const char* format, ...) {
   va_list args;            // typedef char* va_list
   va_start(args, format);	       // 使args指向format  args = (char*)format
   char buf[1024] = {0};	       // 用于存储拼接后的字符串  找个地方存起来, 全部初始化为0
   vsprintf(buf, format, args);  //其实一开始传的都是format，（buf, format, format）
   va_end(args);                 //args = null
   return write(buf); 
}

/* 同printf不同的地方就是字符串不是写到终端,而是写到buf中，并返回写入了多少 */
uint32_t sprintf(char* buf, const char* format, ...) //略
```

其中vsprintf作用就是按照%挨个处理format后的参数，见到一个%就处理1个，根据“%”后面的字母来指定转换过程并写入buf里，不用加‘/0’了，因为一开始已经初始化为全0了

```c
/* 将参数ap按照格式format输出到字符串str,并返回替换后str长度 */
uint32_t vsprintf(char* str, const char* format, va_list ap) {  
  // （buf, format, format）
   char* buf_ptr = str;
   const char* index_ptr = format;
   char index_char = *index_ptr;
   int32_t arg_int;
   while(index_char) {
      if (index_char != '%') {
	 *(buf_ptr++) = index_char;
	 index_char = *(++index_ptr);
   char* arg_str;    
	 continue;
      }
      index_char = *(++index_ptr);	 // 得到%后面的字符
      switch(index_char) {
        case 'c': //略
        case 'd':  //略
    case 's':
	    arg_str = va_arg(ap, char*); //把下一个参数转化成char * 
	    strcpy(buf_ptr, arg_str);
	    buf_ptr += strlen(arg_str);
	    index_char = *(++index_ptr);
	    break;

	 case 'x':
	    arg_int = va_arg(ap, int);  // 当前ap + 4 随后转化成 int* 再* 取int值 即  *((int*)（ap += 4)) 设计思路为取下一个值（一开始为format,如果直接传下一个，万一format里没有“%”就麻烦了，所以这么设计，有点不是特别人性）
	    itoa(arg_int, &buf_ptr, 16);   //就是把int 转化成 str 并依次写进buf里
	    index_char = *(++index_ptr); // 跳过格式字符并更新index_char
	    break;
      }
   }
   return strlen(str);
}
```



#### 实现堆内存管理

目前仅仅能以页大小为单位（4K）进行分配，是非常不灵活的。为了实现malloc背后的底层支持，需要支持多种分配不同大小的块，arena是将一大块内存分配成很多小内存块的内存仓库，一开始是分配一页，随后将底部的区域作为元信息，存放指向内存块描述符的指针（相当于某一规格大小的内存块总管），另一部分划分成很多小内存块，被内存块的描述符指针一个个连起来，已备使用（一旦使用就不在指针里了）

```c
/* 内存块 */
struct mem_block {
   struct list_elem free_elem;
};

/* 内存块描述符 */
struct mem_block_desc {
   uint32_t block_size;		 // 内存块大小
   uint32_t blocks_per_arena;	 // 本arena中可容纳此mem_block的数量.
   struct list free_list;	 // 目前可用的mem_block链表
};
/* 内存仓库arena元信息 */
struct arena {
   struct mem_block_desc* desc;	 // 此arena关联的mem_block_desc
/* large为ture时,cnt表示的是页框数，否则cnt表示空闲mem_block数量 */
   uint32_t cnt;
   bool large;	  // large 为true时表示直接分配以页内存为单位的内存	   
};
```

同样,kernel里和用户PCB里均放置内存块描述符数组

```c
#define DESC_CNT 7	   // 内存块描述符个数
struct mem_block_desc k_block_descs[DESC_CNT];	// 内核内存块描述符数组
```

随后添加初始化描述符的方法block_desc_init(略)，一个返回内存块所在的arena地址（利用在页表的底部直接对地址操作即可）block2arena,以及arena2block(arena,idx)通过arena找到每个大小，再有idx就可算出来。

在堆中申请size个字节内存，sys_malloc，如果是分配页以上的，直接给就行，如果是分配小规格的，先找规格，如果找到，直接传地址（先置0），如果没找到，先分配1页，然后制作block再加进链表即可，随后和找到一样

```c
/* 在堆中申请size字节内存 */
void* sys_malloc(uint32_t size) {


/* 判断用哪个内存池*/
   if (cur_thread->pgdir == NULL) {     // 若为内核线程

   } else {				      // 用户进程pcb中的pgdir会在为其分配页表时创建

   }


   lock_acquire(&mem_pool->lock);

/* 超过最大内存块1024, 就分配页框 */
   if (size > 1024) {
      uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PG_SIZE);    // 向上取整需要的页框数

      a = malloc_page(PF, page_cnt);
			// 略
   } else {    // 若申请的内存小于等于1024,可在各种规格的mem_block_desc中去适配
      uint8_t desc_idx;
      
      /* 从内存块描述符中匹配合适的内存块规格 */
      for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
	 if (size <= descs[desc_idx].block_size) {  // 从小往大后,找到后退出
	    break;
	 }
      }

   /* 若mem_block_desc的free_list中已经没有可用的mem_block,
    * 就创建新的arena提供mem_block */
      if (list_empty(&descs[desc_idx].free_list)) {
	 a = malloc_page(PF, 1);       // 分配1页框做为arena
	 //略
	 memset(a, 0, PG_SIZE);

    /* 对于分配的小块内存,将desc置为相应内存块描述符, 
     * cnt置为此arena可用的内存块数,large置为false */
	 a->desc = &descs[desc_idx];
	 a->large = false;
	 a->cnt = descs[desc_idx].blocks_per_arena;
	 uint32_t block_idx;

	 enum intr_status old_status = intr_disable();

	 /* 开始将arena拆分成内存块,并添加到内存块描述符的free_list中 */
	 for (block_idx = 0; block_idx < descs[desc_idx].blocks_per_arena; block_idx++) {
	    b = arena2block(a, block_idx);
	    ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
	    list_append(&a->desc->free_list, &b->free_elem);	
	 }
	 intr_set_status(old_status);
      }    

   /* 开始分配内存块 */
      b = elem2entry(struct mem_block, free_elem, list_pop(&(descs[desc_idx].free_list)));
      memset(b, 0, descs[desc_idx].block_size);

      a = block2arena(b);  // 获取内存块b所在的arena
      a->cnt--;		   // 将此arena中的空闲内存块数减1 为将来free 回收页做准备
      lock_release(&mem_pool->lock);
      return (void*)b;
   }
}
```



**释放内存**

首先实现以页为单位的内存释放，既然是管理页，那么只要在内存池（本质是bitmap）把相应的位清0就可以了。

+ 虚拟地址位图
+ 物理地址位图
+ 相应的页表中把虚拟地址所对应的pte项中的P位清0即可，（pde目录项不用管）

整体用mfree_page来表示。就是把上述这三个综合下即可。先根据虚拟地址判断物理地址，然后根据是否是内核里的还是用户程序中的内存进行进一步操作。

```c
/* 释放以虚拟地址vaddr为起始的cnt个物理页框 */
void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {

   pg_phy_addr = addr_v2p(vaddr);  // 获取虚拟地址vaddr对应的物理地址

   
/* 判断pg_phy_addr属于用户物理内存池还是内核物理内存池 */
   if (pg_phy_addr >= user_pool.phy_addr_start) {   // 位于user_pool内存池
     //略
```

pfree,根据物理地址，通过内存池本身记录的初始地址计算偏移量以及对应的idx随后清零即可。

```c
/* 将物理地址pg_phy_addr回收到物理内存池 */
void pfree(uint32_t pg_phy_addr)
```

相应的页表中把虚拟地址所对应的pte项中的P位清0即可，（pde目录项不用管）,注意清除tlb（页表的高速缓存）（通过重新加载地址的指定项即可，）

```c
/* 去掉页表中虚拟地址vaddr的映射,只去掉vaddr对应的pte */
static void page_table_pte_remove(uint32_t vaddr) {
   uint32_t* pte = pte_ptr(vaddr);
   *pte &= ~PG_P_1;	// 将页表项pte的P位置0
   asm volatile ("invlpg %0"::"m" (vaddr):"memory");    //更新tlb
}
```

与pfree类似，根绝虚拟地址，通过内存池本身记录的初始地址计算偏移量以及对应的idx随后清零即可。

```c
/* 在虚拟地址池中释放以_vaddr起始的连续pg_cnt个虚拟页地址 */
static void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) 
```



**小内存回收**

对于小内存的处理称作回收，只要把该页的arena中的内存块重新放回到内存块描述符中的空闲链表即可，同时arena中记录的空闲块+1即可,如果+1后=全面的页面（记录在内存块描述符中），则要把描述符中的空闲链表里的块一个个找到并删除（list_remove），最后回收整个该页(arena就在该页中)

```c
/* 回收内存ptr */
void sys_free(void* ptr)
```

随后添加供用户调用的malloc与free即可



## 硬盘驱动程序

添加另一块硬盘作为文件系统的载体。

一系列操作后，通过xp/b 0x475即可检验硬盘数。

tips: cpu， 磁盘等等速度对比（https://blog.csdn.net/haoshan4783/article/details/114038981）

**创建磁盘分区表**

文件系统其实是运行在操作系统中的软件模块，是由操作系统提供的一套管理磁盘文件读写的方法和数据组织，存储形式，本身是程序。没有分区的磁盘是裸盘，raw disk，所以先对它进行分区。其中，柱面的大小=盘面数 乘以 每个磁道的扇区数。 一个磁道对应一个柱面，不同盘面的相同位置的多个磁道可以在同一个柱面上。

最初的磁盘分区表位于MBR中（就是LBA为0的地方，对应0盘0道1扇区），该扇区由三部分组成。

+ 主引导记录MBR（一开始写过）
+ 磁盘分区表DPT（Disk Partition Table）
+ 结束魔数55AA

每个分区表有四个表项，比较重要的有==活动分区标记==，若为1，表示该分区的引导扇区（该分区第一个扇区）含有操作系统的引导程序，为0表示不可引导，非活动分区，此外还有该分区其实偏移扇区，以及分区的容量扇区数（定位用的信息，还有起始磁头号也就是起始（以及结束）盘面号，起始（以及结束）柱面号，起始（以及结束）扇区号）

一开始硬盘只有四个分区，为了兼容，就新“扩展”了扩展分区，每个扩展分区相当于一个新硬盘，所以它的“MBR”叫EBR，它也有4个分区表项，只不过只有前两个能用，第一个就是逻辑分区（本分区的内容），第二个就是下一个扩展分区。



**ata通道（IDE通道）**

一个通道可以有两个硬盘共享，第一个ata通道连接在8259A从片上的IRQ14接口上。硬盘只是响应操作，一般是操作系统下命令之后（通过硬盘控制器中的device寄存器可以指定），进行响应，所以我们自然知道是哪个硬盘发来的响应（毕竟是我们自己指定的hh）。第2个ata通道在8259A从片上的IRQ15上，打开相应引脚即可（包括从片8259A）

定义通道，硬盘，分区结构在内存中的映像。 (caution:  一个通道一个信号量)

```c
/* 分区结构 */
struct partition {
   uint32_t start_lba;		 // 起始扇区
   uint32_t sec_cnt;		 // 扇区数
   struct disk* my_disk;	 // 分区所属的硬盘
   struct list_elem part_tag;	 // 用于队列中的标记
   char name[8];		 // 分区名称
   struct super_block* sb;	 // 本分区的超级块
   struct bitmap block_bitmap;	 // 块位图
   struct bitmap inode_bitmap;	 // i结点位图
   struct list open_inodes;	 // 本分区打开的i结点队列
};

/* 硬盘结构 */
struct disk {
   char name[8];			   // 本硬盘的名称，如sda等
   struct ide_channel* my_channel;	   // 此块硬盘归属于哪个ide通道
   uint8_t dev_no;			   // 本硬盘是主0还是从1
   struct partition prim_parts[4];	   // 主分区顶多是4个
   struct partition logic_parts[8];	   // 逻辑分区数量无限,但总得有个支持的上限,那就支持8个
};

/* ata通道结构 */
struct ide_channel {
   char name[8];		 // 本ata通道名称 
   uint16_t port_base;		 // 本通道的起始端口号
   uint8_t irq_no;		 // 本通道所用的中断号
   struct lock lock;		 // 通道锁
   bool expecting_intr;		 // 表示等待硬盘的中断
   struct semaphore disk_done;	 // 用于阻塞、唤醒驱动程序
   struct disk devices[2];	 // 一个通道上连接两个硬盘，一主一从
};
struct ide_channel channels[2];	 // 有两个ide通道
```

初始化通道，写写中断号，端口啥的，包括每个channel所使用的端口号

``` c
/* 硬盘数据结构初始化 */
void ide_init()
```

选择读写的硬盘，就是往对应的端口号发“消息（字节）”

```c
/* 选择读写的硬盘 */
static void select_disk(struct disk* hd) 
```

同理,  介绍几个基本函数，供之后包装使用

```c
/* 向硬盘控制器写入起始扇区地址及要读写的扇区数 */
static void select_sector(struct disk* hd, uint32_t lba, uint8_t sec_cnt)
  
  /* 向通道channel发命令cmd */
static void cmd_out(struct ide_channel* channel, uint8_t cmd)
  
 /* 硬盘读入sec_cnt个扇区的数据到buf */
static void read_from_sector(struct disk* hd, void* buf, uint8_t sec_cnt)
  
  /* 将buf中sec_cnt扇区的数据写入硬盘 */
static void write2sector(struct disk* hd, void* buf, uint8_t sec_cnt)
  
  /* 等待30秒 */
static bool busy_wait(struct disk* hd)
```



ide_read， 从硬盘读取sec_cnt个扇区到buf， 也就是先选盘，选择起始扇区数和要操作的扇区数，把命令（此时是读）写入，随后阻塞自己，等硬盘准备好后会发中断把自己unblock的，等unblock之后，（一般可行），读就完事了

```c
/* 从硬盘读取sec_cnt个扇区到buf */
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) { 
   lock_acquire (&hd->my_channel->lock);

/* 1 先选择操作的硬盘 */
   select_disk(hd);
   while(secs_done < sec_cnt) {
      if ((secs_done + 256) <= sec_cnt) {
	 secs_op = 256;
      } else {
	 secs_op = sec_cnt - secs_done;
      }

/* 2 写入待读入的扇区数和起始扇区号 */
      select_sector(hd, lba + secs_done, secs_op);

/* 3 执行的命令写入reg_cmd寄存器 */
      cmd_out(hd->my_channel, CMD_READ_SECTOR);	  // 准备开始读数据

   /*********************   阻塞自己的时机  ***********************
      在硬盘已经开始工作(开始在内部读数据或写数据)后才能阻塞自己,现在硬盘已经开始忙了,
      将自己阻塞,等待硬盘完成读操作后通过中断处理程序唤醒自己*/
      sema_down(&hd->my_channel->disk_done);
   /*************************************************************/

/* 4 检测硬盘状态是否可读 */
      /* 醒来后开始执行下面代码*/
      if (!busy_wait(hd)) {	 // 若失败
				//略
      }

   /* 5 把数据从硬盘的缓冲区中读出 */
      read_from_sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);
      secs_done += secs_op;
   }
   lock_release(&hd->my_channel->lock);
}
```

ide_write写也类似，只是发的是写命令，而且阻塞自己的实际不同，写入后才开始阻塞，硬盘内部自己放数据（估计）

硬盘的中断处理程序,该中断处理程序对应两个中断号（两个channel），先通过这个获得channel，随后只处理用户主动发起的（会在cmd_out把expecting_intr = true）,随后唤醒阻塞在上面的程序，接下来读取硬盘状态控制器，否则硬盘会认为当前中断没处理，不会发新的中断。

```c
/* 硬盘中断处理程序 */
void intr_hd_handler(uint8_t irq_no) {
   uint8_t ch_no = irq_no - 0x2e;
   struct ide_channel* channel = &channels[ch_no];
   ASSERT(channel->irq_no == irq_no);
/* 不必担心此中断是否对应的是这一次的expecting_intr,
 * 每次读写硬盘时会申请锁,从而保证了同步一致性 */
   if (channel->expecting_intr) {
      channel->expecting_intr = false;
      sema_up(&channel->disk_done);

/* 读取状态寄存器使硬盘控制器认为此次的中断已被处理,从而硬盘可以继续执行新的读写 */
      inb(reg_status(channel));
   }
}
```

**分区扫描开始**

分区表的扫描，要以MBR引导扇区为入口，便利所有主分区，然后找到总扩展分区，在其中递归遍历每个子扩展分区，并找出逻辑分区。其中，第一个硬盘命名规则为sda,第二个为sdb,这里只处理第二硬盘，因为第一个为裸盘，主分区占据sdb[1~4],逻辑分区占sdb[5-]， 专门有一个分区的list,一个个链接上

同样，为了便于管理，设计一个分区表项的结构，结尾的____attribute___((packed))表示结构要紧凑，不允许编译器为了对齐而填空隙。

```c
/* 用于记录总扩展分区的起始lba,初始为0,partition_scan时以此为标记 */
int32_t ext_lba_base = 0;

uint8_t p_no = 0, l_no = 0;	 // 用来记录硬盘主分区和逻辑分区的下标

struct list partition_list;	 // 分区队列

/* 构建一个16字节大小的结构体,用来存分区表项 */
struct partition_table_entry {
   uint8_t  bootable;		 // 是否可引导	
   uint8_t  start_head;		 // 起始磁头号
   uint8_t  start_sec;		 // 起始扇区号
   uint8_t  start_chs;		 // 起始柱面号
   uint8_t  fs_type;		 // 分区类型
   uint8_t  end_head;		 // 结束磁头号
   uint8_t  end_sec;		 // 结束扇区号
   uint8_t  end_chs;		 // 结束柱面号
/* 更需要关注的是下面这两项 */
   uint32_t start_lba;		 // 本分区起始扇区的lba地址
   uint32_t sec_cnt;		 // 本分区的扇区数目
} __attribute__ ((packed));	 // 保证此结构是16字节大小

```

同样，设计一个MBR或EBR的引导扇区结构。

```c
/* 引导扇区,mbr或ebr所在的扇区 */
struct boot_sector {
   uint8_t  other[446];		 // 引导代码
   struct   partition_table_entry partition_table[4];       // 分区表中有4项,共64字节
   uint16_t signature;		 // 启动扇区的结束标志是0x55,0xaa,
} __attribute__ ((packed));
```

获取磁盘参数

```c
/* 获得硬盘参数信息 */
static void identify_disk(struct disk* hd) {
   char id_info[512];
   select_disk(hd);
   cmd_out(hd->my_channel, CMD_IDENTIFY);
/* 向硬盘发送指令后便通过信号量阻塞自己,
 * 待硬盘处理完成后,通过中断处理程序将自己唤醒 */
   sema_down(&hd->my_channel->disk_done);

/* 醒来后开始执行下面代码*/
   if (!busy_wait(hd)) {     //  若失败
      char error[64];
      sprintf(error, "%s identify failed!!!!!!\n", hd->name);
      PANIC(error);
   }
   read_from_sector(hd, id_info, 1);

   //随后就根据引导扇区内部规定好的格式读就完事了 略
}
```

下面准备遍历各个区,完全是根据硬件特征读，比如MBR的话就是主分区，否则就是扩展分区，读完加到链上即可。

```
/* 扫描硬盘hd中地址为ext_lba的扇区中的所有分区 */
static void partition_scan(struct disk* hd, uint32_t ext_lba)
```

最后在硬盘初始化阶段判断，如果是dev_no != 0 就扫描该分支，（0是裸盘），完结！



## 文件系统

由于文件是由多个“块”组成的，那么如何把这些块组织到一起呢，一种是类似链表的那种方式（FAT），每个块都存储下一块的地址，这样文件可以不连续存储，但是缺点是要想读某个指定块，就要把它前面的都读了才能找到他，而硬盘的速度是很慢的...那么，很容易想到，就需要有一个“index”,也就是块地址数组，每个元素的值是块的地址，数组下标表示第几块，而且要为每个文件都配备这样一种结构胶，就是index node(inode),另外考虑到文件过大，会在第13块建立一级间接块索引表指针，它指向的块都是块指针，从13开始，这样inode结构就可以存储更多块了。那么为了在inode中囊括文件的所有信息，还会有一些权限，属主（属于谁），时间（创建，修改，访问），文件大小等信息（但是，没有文件名）。inode就是文件通向实体数据块的大门。inode就是文件在文件系统的元信息，要想通过文件系统获得文件的实体，就要找到文件的inode。

linux中，文件系统是针对各个分区（partition）来进行管理的，inode本身也必须存储在磁盘的某个位置，由于inode结构是固定的，所以本分区中所有的inode可以通过一个数组来维护（inode_table），数组元素就是文件inode的编号。

由于inode本身不含文件名，有inode就可以找到文件，但是人也不能生记inode号吧hh，而我们都是通过目录进行查找的，所以把文件名有关的信息和inode节点号相关的工作就交给目录了。目录本身也是文件（inode）。区别是如果inode表示的是普通文件，此inode指向的数据块的内容就是普通文件的内容。如果inode表示的是目录文件，此inode指向的数据块的内容应该是该目录下的目录项，目录项包含目录文件和普通文件等。当然，根目录是所有目录的父目录。

```c
/* inode结构 */
struct inode {
   uint32_t i_no;    // inode编号

/* 当此inode是文件时,i_size是指文件大小,
若此inode是目录,i_size是指该目录下所有目录项大小之和*/
   uint32_t i_size;

   uint32_t i_open_cnts;   // 记录此文件被打开的次数, 为关闭做准备
   bool write_deny;	   // 写文件不能并行,进程写文件前检查此标识

/* i_sectors[0-11]是直接块, i_sectors[12]用来存储一级间接块指针 */
   uint32_t i_sectors[13];
   struct list_elem inode_tag;
};
```



所以，在设计一个目录项的时候，他应该包括inode编号，文件名（要对应上，绑定关联），以及该目录想的类型（还是目录or普通文件）。所以通过文件名找文件实体数据块的流程是：

+ 通过目录找到文件名所在的目录项
+ 从目录项中获取inode编号
+ 用此inode号作为数组索引下标
+ 从该inode中获得数据块的地址，加载得到数据块。

```c
/* 目录结构, 只在内存里存在，不在磁盘上存在，便于管理，到时候遍历好使 */
struct dir {
   struct inode* inode;   // 指向内存中的inode
   uint32_t dir_pos;	  // 记录在目录内的偏移，相当于遍历中的指针
   uint8_t dir_buf[512];  // 目录的数据缓存
};

/* 目录项结构 */
struct dir_entry {
   char filename[MAX_FILE_NAME_LEN];  // 普通文件或目录名称
   uint32_t i_no;		      // 普通文件或目录对应的inode编号
   enum file_types f_type;	      // 文件类型
};
```



任何分区都有一个根目录，在各个分区中肯定有一个固定的地方记载了根目录的位置（哪个扇区）（see more details in P600），inode数组和位图，空闲块位图，空闲块区域（有一个肯定被根目录占了），这个地方就是超级快，在操作系统引导块（引导块可能占据多个扇区）之后。

```c
/* 超级块 */
struct super_block {
   uint32_t magic;		    // 用来标识文件系统类型,支持多文件系统的操作系统通过此标志来识别文件系统类型
   uint32_t sec_cnt;		    // 本分区总共的扇区数
   uint32_t inode_cnt;		    // 本分区中inode数量
   uint32_t part_lba_base;	    // 本分区的起始lba地址

   uint32_t block_bitmap_lba;	    // 块位图本身起始扇区地址
   uint32_t block_bitmap_sects;     // 扇区位图本身占用的扇区数量

   uint32_t inode_bitmap_lba;	    // i结点位图起始扇区lba地址
   uint32_t inode_bitmap_sects;	    // i结点位图占用的扇区数量

   uint32_t inode_table_lba;	    // i结点表起始扇区lba地址
   uint32_t inode_table_sects;	    // i结点表占用的扇区数量

   uint32_t data_start_lba;	    // 数据区开始的第一个扇区号
   uint32_t root_inode_no;	    // 根目录所在的I结点号 根目录放在第一个扇区比较合适
   uint32_t dir_entry_size;	    // 目录项大小 为了管理方便 感觉非必要

   uint8_t  pad[460];		    // 加上460字节,凑够512字节1扇区大小
} __attribute__ ((packed));
```

完成格式化分区的函数， 创建文件系统，本质上就是创建文件系统所需要的元信息，包括超级块的pos & size, 空闲块 pos & size, inode_bitmap pos & size,  inode_table pos & size, 空闲块 pos, 根目录 pos。创建步骤如下：

+ 根据分区part大小，计算分区文件系统各个部分（元信息）需要的扇区数和位置。
+ 在内存中创建超级块，将以上各步骤的元信息写入超级块
+ 将超级块写入磁盘
+ 将于那信息写入磁盘上各自的位置
+ 将根目录写入磁盘

```c
* 格式化分区,也就是初始化分区的元信息,创建文件系统 */
static void partition_format(struct partition* part)
```

**挂载分区**

挂载分区的实质就是把该文件系统的元信息从硬盘上读取出来加载到内存中，这样硬盘资源的变化都用内存中元信息进行跟踪，如果有写操作，就把内存中的元信息同步写入到硬盘来持久化即可。

表示一个当前操作的分区

```c
struct partition* cur_part;	 // 默认情况下操作的是哪个分区
```

```c
#define offset(struct_type,member) (int)(&((struct_type*)0)->member)

#define elem2entry(struct_type, struct_member_name, elem_ptr) \
	 (struct_type*)((int)elem_ptr - offset(struct_type, struct_member_name))
```

就是把bitmap, block_map，超级块等信息读出来挂在内存中的分区结构上

```c
/* 在分区链表中找到名为part_name的分区,并将其指针赋值给cur_part */
static bool mount_partition(struct list_elem* pelem, int arg)

//最后会在filesys_init 中挂载sdb1分区 ， 以后都默认操作这个分区
/* 确定默认操作的分区 */
 char default_part[8] = "sdb1";
/* 挂载分区 */
list_traversal(&partition_list, mount_partition, (int)default_part);
```

#### 文件描述符

inode是操作系统为自己的文件系统准备的数据结构，与用户的关系不大。用户使用息息相关的是文件描述符。操作系统允许一个进程同时，多次，打开统一文件，也允许该文件被不同的进程同时打开，那么需要为这种方式设立一种独特的描述符，表示每次读写的位置等，以实现互不影响。每次打开就会产生一个文件结构，多次打开就会产生多个文件结构，并把每次文件的操作偏移量记在不同的文件结构中，此外还有属于哪个inode(一个文件仅有一个inode),以及操作权限。

这些文件描述符被放在一个数组中（也叫文件表）统一管理，不同进程PCB会保存使用的数组下标即可。

```c
/* 文件结构 */
struct file {
   uint32_t fd_pos;      // 记录当前文件操作的偏移地址,以0为起始,最大为文件大小-1
   uint32_t fd_flag;
   struct inode* fd_inode;
};
```

所以，linux中通过文件描述符查找文件数据块的过程中涉及到一下三种数据结构。

+ PCB中的文件描述符数组
+ 存储所有文件结构的文件表
+ inode队列，也就是inode缓存。

所以， OPEN操作的本质就是在上述三个结构里进行操作。

+ 在全局inode队列里新建一个inode，然后返回inode地址
+ 在文件表中找空位，填入该文件结构，使其中的fd_inode指针指向返回的inode地址
+ 在pcb中的文件描述符数组中也找个位置存放上一步返回的文件表中的下标即可。

下面开始实现，先实现一些基础的函数。获取inode所在的扇区和扇区内的偏移量,随后放在inode_position里面

```c
/* 获取inode所在的扇区和扇区内的偏移量 */
static void inode_locate(struct partition* part, uint32_t inode_no, struct inode_position* inode_pos)
/* 用来存储inode位置 */
struct inode_position {
   bool	 two_sec;	// inode是否跨扇区
   uint32_t sec_lba;	// inode所在的扇区号
   uint32_t off_size;	// inode在扇区内的字节偏移量
};

```

先利用inode_locate，定位，随后从硬盘中读入有关扇区，再写入即可

```c
/* 将inode写入到分区part */
void inode_sync(struct partition* part, struct inode* inode, void* io_buf)
```

根据inode_no 找到对应的inode 先在内存里找，找到后返回，否则就在磁盘里找（利用inode_locate），找到后加到列表里，并且在将该inode打开数置1

```c
/* 根据i结点号返回相应的i结点 */
struct inode* inode_open(struct partition* part, uint32_t inode_no)
```

关闭inode或减少inode的打开数，如果打开书为0，就关闭（也就是在链表中list_remove）, 初始化inode,给inode结构中把inode_no赋值即可，略

```c
/* 关闭inode或减少inode的打开数 */
void inode_close(struct inode* inode)
  
  /* 初始化new_inode */
void inode_init(uint32_t inode_no, struct inode* new_inode)
```

==新增inode_sync, 先inode_locat,==

```
inode_sync(uint32_t inode_no, struct inode* new_inode)
```



**文件相关的函数**

全局文件表在内存中,构造一些基本的函数

```c
/* 文件表 */
struct file file_table[MAX_FILE_OPEN];

/* 从文件表file_table中获取一个空闲位,成功返回下标,失败返回-1 */
int32_t get_free_slot_in_global(void)
  
  /* 将全局描述符下标安装到进程或线程自己的文件描述符数组fd_table中,
 * 成功返回下标,失败返回-1 */
int32_t pcb_fd_install(int32_t globa_fd_idx)
  
 /* 分配一个i结点,返回i结点号 */
int32_t inode_bitmap_alloc(struct partition* part)
  
  /* 分配1个扇区,返回其扇区地址 */
int32_t block_bitmap_alloc(struct partition* part)
  
  /* 将内存中bitmap第bit_idx位所在的512字节同步到硬盘 找到该位所在的那个扇区（扇区字节的倍数） 仅同步 */
void bitmap_sync(struct partition* part, uint32_t bit_idx, uint8_t btmp_type)
```

**目录有关的函数**

```c
struct dir root_dir;             // 根目录
```

Inode_open打开目录项即可，设置root_dir项

```c
/* 打开根目录 根目录的inode_no是固定的 */
void open_root_dir(struct partition* part)
  
  /* 在分区part上打开i结点为inode_no的目录并返回目录指针  */
struct dir* dir_open(struct partition* part, uint32_t inode_no)
```

把该目录对应的inode节点中的所有块全扫描一遍，一块一块扫描，如果有，就copy到dir_e中

```c
/* 在part分区内的pdir目录内寻找名为name的文件或目录,
 * 找到后返回true并将其目录项存入dir_e,否则返回false */
bool search_dir_entry(struct partition* part, struct dir* pdir, \
		     const char* name, struct dir_entry* dir_e)
```

套了一层inode_close，（根目录直接返回 不处理）

```c
/* 关闭目录 */
void dir_close(struct dir* dir)
```

初始化目录项

```c
/* 在内存中初始化目录项p_de */
void create_dir_entry(char* filename, uint32_t inode_no, uint8_t file_type, struct dir_entry* p_de) 
```

把父目录对应的inode块一个一个读入，看有没有地方，有就copy，如果最后inode块不够了，就新申请，申请后要同步block_bitmap（本身就会在内存中有一份），同步到磁盘上（根据idx可以算出是哪个分区的， 把对应的扇区字节bitmap_sync 一写就行了）（如果是第12项，间接块会麻烦些，先要再申请一块，把再申请的lba写入间接块表之中，随后同步block using ide_write，最后同步bitmap_sync(相当于两次)）,这里待改进，如果是inode中的直接块发生变化，应该inode_sync一下，这里这么写其实有问题（应该先检查有没有间接块表（12项），如果有直接读入到管路各个lba的数组中）

```c
/* 将目录项p_de写入父目录parent_dir中,io_buf由主调函数提供 */
bool sync_dir_entry(struct dir* parent_dir, struct dir_entry* p_de, void* io_buf) 
```

**路径解析**

先定义一个中间使用的结构，要把查找过程中最后走过的地方记录下来，将来作为对比就知道找到哪了（当然也知道找没找到），此外还有目录所在的直接父目录（便于操作），以及找到的类型（没找到也有类型hh）'/a/b/c' 如果是c不存在， searched_path 就是'/a/b/c', 如果是 b 不存在 searched_path 就是 '/a/b'

```c
/* 用来记录查找文件过程中已找到的上级路径,也就是查找文件过程中"走过的地方" */
struct path_search_record {
   char searched_path[MAX_PATH_LEN];	    // 查找过程中的父路径
   struct dir* parent_dir;		    // 文件或目录所在的直接父目录
   enum file_types file_type;		    // 找到的是普通文件还是目录,找不到将为未知类型(FT_UNKNOWN)
};
```

先写一些小的函数, 可以把 ‘/a/b/c 经过解析 pathname变为 ‘/b/c 而name_store变为a, （一般上一步会跟一个操作 strcat / + 'a' 形成路径），方法很简单 就是把前面一开始‘/ 去掉（while 循环搞定） 接着开始copy到name_store中，直到'/ 为止 or 0）

```c
/* 将最上层路径名称解析出来 仅仅解析的是不带‘/’的 */
static char* path_parse(char* pathname, char* name_store)
```

就是循环调用 path_parse 算路径即可

```c
/* 返回路径深度,比如/a/b/c,深度为3 */
int32_t path_depth_cnt(char* pathname) 
```

思路是这样的， 每次把当前查找的路径 添加到 searched_path 中， strcat /  + strcat parse_parse解析后的文件名（）， 随后在上级目录中寻找,search_dir_entry , 如果没有直接返回 -1， 有的话根据找到的目录项类型判断，是普通文件直接返回，目录的话就把当前查找的父目录更新成自己， 再次循环

```c
/* 搜索文件pathname,若找到则返回其inode号,否则返回-1 */
static int search_file(const char* pathname, struct path_search_record* searched_record)
```

#### 创建文件

创建文件步骤：

+ 申请从inode_bitmap 获得bit_idx,随后更新bitmap_table,(随后把这个落盘以及bitmap)

+ Inode->i->sectors 是文件内容存储在的扇区地址，要申请（from block_bitmap）随后写入（一定在data_start_lba之后，同样涉及bitmap落盘）但是创建文件的时候由于不知道写多少，暂时不分配
+ 还有在文件表中填写，并添加到PCB中的文件描述符数组中
+ 考虑父目录，新增的inode对应的目录项要写到 父目录的inode->sectors 中（可能因为目前已有的写不下而新增块），同时要把inode->i_size增加，同样要把inode 落盘（父目录的）
+ 如果失败，就回滚。

```c
/* 创建文件,若成功则返回文件描述符,否则返回-1 */
int32_t file_create(struct dir* parent_dir, char* filename, uint8_t flag) 
```

**初步实现sys_open的功能**

这里实现的，相当于open（const char* pathname, O_CREATE）功能, 根据search_file查找，查找后的结果在path_search_record 中，包括父目录，通过一系列判断，最后file_create() （父目录已在结构path_search_record中）

```c
/* 打开或创建文件成功后,返回文件描述符,否则返回-1 */
int32_t sys_open(const char* pathname, uint8_t flags)
  
  file_create(searched_record.parent_dir, (strrchr(pathname, '/') + 1), flags);
```

测试：查root目录所在的块的目录项即可（xxd.sh查看）

**文件打开与关闭**

根据inode号注册进文件描述符，随后加到PCB中，（如果flag带有写的操作 O_WRONLY or O_RDWR）,那么进入临界区前先关中断，随后判断fd指向的inode中的write_deny,若没有其他进程占用，设为false,出临界区，关中断。

```c
/* 打开编号为inode_no的inode对应的文件,若成功则返回文件描述符,否则返回-1 */
int32_t file_open(uint32_t inode_no, uint8_t flag)
```

随后修改sys_open, 除了create以为均为打开文件

```c
default:
   /* 其余情况均为打开已存在文件:
    * O_RDONLY,O_WRONLY,O_RDWR */
	 fd = file_open(inode_no, flags);
```

-- --

**文件关闭**

接下来关闭文件描述符表里的file，这里应先通过判断file->fd_flag 看是否包含写，（如果包含就把fd_inode->write_deny置为false，如果不包含就不操作，参考文献有问题，没做判断处理），

```c
/* 关闭文件 */
int32_t file_close(struct file* file) {
   if (file == NULL) {
      return -1;
   }
   file->fd_inode->write_deny = false;  // if (file->fd_flag = O_REWR or O_O_WRONLY) then 应该加判断
   inode_close(file->fd_inode);
   file->fd_inode = NULL;   // 使文件结构可用
   return 0;
}
```

fd_local2global 见名知意

```c
/* 将文件描述符转化为文件表的下标 */
static uint32_t fd_local2global(uint32_t local_fd)
```

注意此处操作的是PCB中的文件描述符表，找到对应的全局文件描述符，进行file_close操作

```c
/* 关闭文件描述符fd指向的文件,成功返回0,否则返回-1 */
int32_t sys_close(int32_t fd) {
   int32_t ret = -1;   // 返回值默认为-1,即失败
   if (fd > 2) {
      uint32_t _fd = fd_local2global(fd);
      ret = file_close(&file_table[_fd]);
      running_thread()->fd_table[fd] = -1; // 使该文件描述符位可用
   }
   return ret;
}
```



**实现文件写入**

首先还是实现file_write，然后再根据文件描述符判断是否写在屏幕上还是文件中,

先检查 file + count 字节后会不会超(file->fd_inode->i_size + count) > (BLOCK_SIZE * 140))，如果文件第一次写就分配一块到（i_sectors[0]），之后同步到硬盘，随后算一下要写多少块（fd_inode->i_size + count 可以算出最终块数，与之前的一做减法就知道），提前准备好块，如果不够提前分配,每分配一次就要立刻bitmap_sync(落盘)，如果需要还要分配间接块（也要落盘），随后开始写入，先算第一块的余量，然后开始写，一块块写，写完一块落盘一块（ide_write），直到写完，最后同步file->fd_inode(因为可能会分配直接块or 一级间接块，)

```c
/* 把buf中的count个字节写入file,成功则返回写入的字节数,失败则返回-1 */
int32_t file_write(struct file* file, const void* buf, uint32_t count)
```

之后修改sys_write，之前是直接写在屏幕上（最终调用put_str）,现在判断，如果是标准输出，那么和以前一样，否则检查对应文件的打开时的权限，如果可以写，那么就开始file_write了，否则报错(目前只有追加写)

```c
/* 将buf中连续count个字节写入文件描述符fd,成功则返回写入的字节数,失败返回-1 */
int32_t sys_write(int32_t fd, const void* buf, uint32_t count) {
   if (fd < 0) {
      printk("sys_write: fd error\n");
      return -1;
   }
   if (fd == stdout_no) {  
      char tmp_buf[1024] = {0};
      memcpy(tmp_buf, buf, count);
      console_put_str(tmp_buf);
      return count;
   }
   uint32_t _fd = fd_local2global(fd);
   struct file* wr_file = &file_table[_fd];
   if (wr_file->fd_flag & O_WRONLY || wr_file->fd_flag & O_RDWR) {
      uint32_t bytes_written  = file_write(wr_file, buf, count);
      return bytes_written;
   } else {
      console_put_str("sys_write: not allowed to write file without flag O_RDWR or O_WRONLY\n");
      return -1;
   }
}
```

修改printf()

```c
write(1, buf, strlen(buf)); // 原来是 write(buf); 之前的sys_write 直接对应console_put_str,最终调用put_str
```

测试：首先在file_write里提前print出写到哪个lba上了，直接xxd脚本查看即可。

-- --

 **读取文件**

起始位置由file->fd_pos给出，随后与写入类似（比写入简单，不用考虑落盘的事，结尾更新fd_pos）, 同样sys_read略。

**修改指针（fd_pos）位置**

修改指针位置即可

```c
/* 重置用于文件读写操作的偏移指针,成功时返回新的偏移量,出错时返回-1 */
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence)
```



#### 文件删除功能

涉及到的主要有 inode位图，inode_table(对应项清0即可)， inode中i_sectors[0-11]【直接块】以及第12间接块（还要把12块中的一个个直接块一一回收，也就是block_map回收），最后删除父目录中的对应项（清零即可， 如果该sectors就一块也涉及回收， size还要减去一个目录项的大小，回收与之前类似，还要把父目录的inode更新。）由于这些很多，先添加两个辅助函数。

完成 inode_table(对应项清0即可) 的功能， 以及回收inode的数据块和inode本身

```c
/* 将硬盘分区part上的inode清空 */
void inode_delete(struct partition* part, uint32_t inode_no, void* io_buf)
  /* 回收inode的数据块和inode本身 */
void inode_release(struct partition* part, uint32_t inode_no) 
```

删除父目录中的对应项（清零即可， 如果该sectors就一块也涉及回收， size还要减去一个目录项的大小，回收与之前类似，还要把父目录的inode更新。）

```c
/* 把分区part目录pdir中编号为inode_no的目录项删除 */
bool delete_dir_entry(struct partition* part, struct dir* pdir, uint32_t inode_no, void* io_buf)
```

最后实现sys_unlink即可，先search_file 找到， 把parent_dir,存好，随后调用上面三个辅助函数即可,还要检查是否在已打开文件列表(文件表)中。



#### 创建目录

创建目录，首先申请一个inode，（操作inode-bitmap inode_table），随后找到父目录（没找到的话报错），随后添加目录项（找位置写即可），随后在自己的i_sectors[0]（先分配）中把‘.’ 与 ‘..’ 写上，其中一个对应自己，一个对应父目录即可，以上操作都需要落盘（同步到硬盘）。

```c
/* 创建目录pathname,成功返回0,失败返回-1 */
int32_t sys_mkdir(const char* pathname)
```

测试时，创建目录，随后查看根目录中对应的inode号，再在inode_table中查找对应项即可，查找i_sectors[0]的对应lba,找到后，查看查看信息即可（xxd lba * 512）

**遍历目录**

要先打开目录， 随后查看目录项即可，最后再关闭。为此，先写打开与关闭目录的函数。

```c
/* 目录打开成功后返回目录指针,失败返回NULL */
struct dir* sys_opendir(const char* name) // 其实就是一系列包装了 dir_opendir_open
  
  /* 成功关闭目录dir返回0,失败返回-1 */
int32_t sys_closedir(struct dir* dir)  //包装了 dir_close
```

就是在目录里找，找到一个返回一个，并在dir_pos中记录位置,方法是把一个扇区读入到dir->dir_buf中，随后查找并更新dir_pos即可

```c
/* 读取目录,成功返回1个目录项,失败返回NULL */
struct dir_entry* dir_read(struct dir* dir)
```

实现目录回绕（rewinddir）,其实就是使dir_pos回到0，

```c
/* 把目录dir的指针dir_pos置0 */
void sys_rewinddir(struct dir* dir) {
   dir->dir_pos = 0;
}
```

**删除目录**

当目录项中只有“.” 以及‘’..‘’才可以进行删除，先扫描各个块，随后，在父目录parent_dir中删除子目录child_dir对应的目录项，最后再删除子目录的inode

```c
/* （之前已经验证过子目录为 目录项中只有“.” 以及‘’..‘’才可以进行删除）在父目录parent_dir中删除child_dir */
int32_t dir_remove(struct dir* parent_dir, struct dir* child_dir)
  
  /* 删除空目录,成功时返回0,失败时返回-1 里面会判断是否 只有两个目录项 随后再删除（调用）dir_remove*/
int32_t sys_rmdir(const char* pathname)
```

测试，看看父目录下的inode节点中所对应的的sector块中是否还有子目录就可知道，（如果用inode_bitmap中对应节点是否在来查看就太麻烦了）

**任务的工作目录**

为实现pwd功能，先要实现一些基础功能。把该目录的第一个扇区读出来，随后返回第二个目录项的inode即可（也就是“.”）

```c
/* 获得父目录的inode编号 */
static uint32_t get_parent_dir_inode_nr(uint32_t child_inode_nr, void* io_buf) 
```

根据inode号取名字，因为名字在父目录里，所以只能在父目录里找

```c
/* 在inode编号为p_inode_nr的目录中查找inode编号为c_inode_nr的子目录的名字,
 * 将名字存入缓冲区path.成功返回0,失败返-1 */
static int get_child_dir_name(uint32_t p_inode_nr, uint32_t c_inode_nr, char* path, void* io_buf)
```

获取当前工作路径，首先会在pcb中添加一个当前进程工作目录的inode编号，（肯定要在进程中有一个位置进行记录，放在pcb比较合适）,递归调用get_parent_dir_inode_nr 以及get_child_dir_name 获取当前路径的名字，随后更新parent_dir为当前dir继续调用，知道根目录，注意此时得到的是反的，再调整顺序即可（把最后一个加到一个新的buf中来，每次执行即可）

```c
/* 把当前工作目录绝对路径写入buf, size是buf的大小.
 当buf为NULL时,由操作系统分配存储工作路径的空间并返回地址
 失败则返回NULL */
char* sys_getcwd(char* buf, uint32_t size)
```

**改变当前工作目录**

使用chdir来改变当前工作目录(直接search_file 一搜就行， 随后再一设置 running_thread()->cwd_inode_nr = inode_no;)

```c
/* 更改当前工作目录为绝对路径path,成功则返回0,失败返回-1 */
int32_t sys_chdir(const char* path) 
```

**获得文件属性**

实现类似ls -l功能， 为此，要先有一个结构来保存这个状态stat，

```c
/* 文件属性结构体 */
struct stat {
   uint32_t st_ino;		 // inode编号
   uint32_t st_size;		 // 尺寸
   enum file_types st_filetype;	 // 文件类型
};
```

路径有了，直接search_file 一搜就行，把stat 的属性根据查找结果一设置就行

```c
/* 在buf中填充文件结构相关信息,成功时返回0,失败返回-1 */
int32_t sys_stat(const char* path, struct stat* buf)
```



## 构建交互系统

**实现fork**

经过fork后，原来的一个进程变成了两个，两个进程实行的是独立且相同的代码，只不过子进程是在fork之后才开始执行的。父子进程的进程体（数据段以及代码段）是一摸一样的。为了实现它就要先把父进程的进程资源复制一份，随后跳过去进行执行。那么需要复制的有：

+ 进程的pcb（肯定得有独立的）step1
+ 程序体（数据段以及代码段）.  Step2
+ 用户栈（编译器会把局部变量在栈中创建）. Step3
+ 内核栈（进入内核后要保存上下文环境）.     Step4
+ 虚拟地址池，每个进程都有自己的独立的内存空间，其地址是虚拟地址池来管理的。 Step5
+ 页表，肯定每个进程都有独立的内存空间      step6

为此，首先在PCB中要新增属性parent_pid,并在开始赋值为-1，还要新增辅助函数。

根据虚拟地址分配一个物理地址给它，并绑定页表（为fork准备）

```c
/* 安装1页大小的vaddr,专门针对fork时虚拟地址位图无须操作的情况 */
void* get_a_page_without_opvaddrbitmap(enum pool_flags pf, uint32_t vaddr)
```

 将父进程的pcb、虚拟地址位图拷贝给子进程, step1 and step 5, 复制pcb所在的整个页,里面包含进程pcb信息及特级0极的栈,里面包含了返回地址, 然后再单独修改个别部分, 复制父进程的虚拟地址池的位图(重新申请若干页get_kernel_pages，然后memcpy)

```c
/* 将父进程的pcb、虚拟地址位图拷贝给子进程 */
static int32_t copy_pcb_vaddrbitmap_stack0(struct task_struct* child_thread, struct task_struct* parent_thread)
```

从父进程的虚拟内存池中，一页页拷贝到子进程中，迭代方式可以由父进程的parent_thread->userprog_vaddr.vaddr_start + idx * 4K算出每页地址，随后直接拷贝到内核中转站 buf_page中，然后切换子进程（本质上是有了子进程的pcb之后，有了页表地址， 随后再次调用cr3 重新加载，）之后（已是子进程的页表了）直接在子进程中申请（使用get_a_page_without_opvaddrbitmap）1页以父进程的当前拷贝页的虚拟地址，随后memcpy搞定，然后再恢复成父进程的页表即可（与子进程类似），继续迭代下一页即可

```c
/* 复制子进程的进程体(代码和数据)及用户栈 */
static void copy_body_stack3(struct task_struct* child_thread, struct task_struct* parent_thread, void* buf_page)
```

为子进程构造中断栈，以备从中断栈返回时用到，跳回指定位置，同时修改返回值（修改eax就是修改返回值），此函数即修改中断栈页修改线程栈，直接不废话了，直接就构造switch_to 所需参数 ebp, ebx, edi, esi, 以及ret时需要的返回地址，随后直接挑转，中断栈直接copy的是父进程的，所以返回地址不用调整（还是fork之后的，而此时对应的虚拟地址都准备好了，且拷贝好了）

```c
/* 为子进程构建thread_stack和修改返回值 */
static int32_t build_child_stack(struct task_struct* child_thread) {
/* a 使子进程pid返回值为0 */
   /* 获取子进程0级栈栈顶 */
   struct intr_stack* intr_0_stack = (struct intr_stack*)((uint32_t)child_thread + PG_SIZE - sizeof(struct intr_stack));
   /* 修改子进程的返回值为0 */
   intr_0_stack->eax = 0;

/* b 为switch_to 构建 struct thread_stack,将其构建在紧临intr_stack之下的空间， 开始修改中断栈 */
   uint32_t* ret_addr_in_thread_stack  = (uint32_t*)intr_0_stack - 1;

   /***   这三行不是必要的,只是为了梳理thread_stack中的关系 ***/
   uint32_t* esi_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 2; 
   uint32_t* edi_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 3; 
   uint32_t* ebx_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 4; 
   /**********************************************************/

   /* ebp在thread_stack中的地址便是当时的esp(0级栈的栈顶),
   即esp为"(uint32_t*)intr_0_stack - 5" */
   uint32_t* ebp_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 5; 

   /* switch_to的返回地址更新为intr_exit,直接从中断返回 */
   *ret_addr_in_thread_stack = (uint32_t)intr_exit;

   /* 下面这两行赋值只是为了使构建的thread_stack更加清晰,其实也不需要,
    * 因为在进入intr_exit后一系列的pop会把寄存器中的数据覆盖 */
   *ebp_ptr_in_thread_stack = *ebx_ptr_in_thread_stack =\
   *edi_ptr_in_thread_stack = *esi_ptr_in_thread_stack = 0;
   /*********************************************************/

   /* 把构建的thread_stack的栈顶做为switch_to恢复数据时的栈顶 */
   child_thread->self_kstack = ebp_ptr_in_thread_stack;	    
   return 0;
}

```

把父进程的每个打开的文件中的打开次数都+1

```c
/* 更新inode打开数 */
static void update_inode_open_cnts(struct task_struct* thread)
```

拷贝父进程本身所占资源给子进程, 就是前面的综合

```c
/* 拷贝父进程本身所占资源给子进程 */
static int32_t copy_process(struct task_struct* child_thread, struct task_struct* parent_thread) {
   /* 内核缓冲区,作为父进程用户空间的数据复制到子进程用户空间的中转 */
   void* buf_page = get_kernel_pages(1);
   if (buf_page == NULL) {
      return -1;
   }

   /* a 复制父进程的pcb、虚拟地址位图、内核栈到子进程 */
   if (copy_pcb_vaddrbitmap_stack0(child_thread, parent_thread) == -1) {
      return -1;
   }

   /* b 为子进程创建页表,此页表仅包括内核空间 */
   child_thread->pgdir = create_page_dir();
   if(child_thread->pgdir == NULL) {
      return -1;
   }

   /* c 复制父进程进程体及用户栈给子进程 */
   copy_body_stack3(child_thread, parent_thread, buf_page);

   /* d 构建子进程thread_stack和修改返回值pid */
   build_child_stack(child_thread);

   /* e 更新文件inode的打开数 */
   update_inode_open_cnts(child_thread);

   mfree_page(PF_KERNEL, buf_page, 1);
   return 0;
}

```

最后的sys_fork就是包装调用copy_process, 最后再把子进程加到线程队列里

```c
/* fork子进程,内核线程不可直接调用 */
pid_t sys_fork(void) 
```

**修改sys_read,使其能从键盘获取输入**

判断fd号，如果是标准输入stdin_no, 则调用 ioq_getchar(&kbd_buf) 从键盘缓冲区中获取，（之前如果按键了键盘的中断处理程序会在缓冲区里放置处理好的char的），否则就调用file_read

```c
/* 从文件描述符fd指向的文件中读取count个字节到buf,若成功则返回读出的字节数,到文件尾则返回-1 */
int32_t sys_read(int32_t fd, void* buf, uint32_t count)
```

实现清屏的汇编代码，就是重新加载现存段，随后把 80 * 25 的字节往清零并往寄存器里输出就行了（mov word [gs:ebx], 0x0720	），最后再重新设置光标就行

```nasm
cls_screen
```



#### 初步实现shell

要把输入的命令找个地方先存好， 以及当前目录

```c
/* 存储输入的命令 */
static char cmd_line[cmd_len] = {0};

/* 用来记录当前目录,是当前目录的缓存,每次执行cd命令时会更新此内容 */
char cwd_cache[64] = {0};
```

同样要把输出提示符准备好 也就是xx@ip + 当前目录的缓存

```c
/* 输出提示符 */
void print_prompt(void) {
   printf("[xx@ip %s]$ ", cwd_cache);
}
```

shell无限循环调用read_line, 每次先进行print_prompt，随后再进行read_line,(其中底层调用read(stdin_no), 读进缓冲区cmd_line， 进行分析处理) 最后shell 清空 cmd_line 再次进入循环

Readline 更多的是读入进cmd_line缓冲区中，同时put_char打印到屏幕上

```c
/* 从键盘缓冲区中最多读入count个字节到buf。*/
static void readline(char* buf, int32_t count)
```

读入进 cmd_line后, 开始调用 cmd_parse进行分析处理（``` argc = cmd_parse(cmd_line, argv, ' ');```），为下一步的分析做准备 

```c
/* 分析字符串cmd_str中以token为分隔符的单词,将各单词的指针存入argv数组 */
static int32_t cmd_parse(char* cmd_str, char** argv, char token)
```

本质上就是 调用sprintf

```c
/* 以填充空格的方式输出buf */
static void pad_print(char* buf, int32_t buf_len, void* ptr, char format) {
  // 略
  switch(format) {
      case 's':
	 out_pad_0idx = sprintf(buf, "%s", ptr);
	 break;
      case 'd':
	 out_pad_0idx = sprintf(buf, "%d", *((int16_t*)ptr));
      case 'x':
	 out_pad_0idx = sprintf(buf, "%x", *((uint32_t*)ptr));
   }
  // 填空格
  sys_write(stdout_no, buf, buf_len - 1);
}
```

本质上就是打印各个线程信息，调用pad_print 打印即可

```c
/* 用于在list_traversal函数中的回调函数,用于针对线程队列的处理 */
static bool elem2thread_info(struct list_elem* pelem, int arg UNUSED)
```

利用list_traversal 调用 elem2thread_info 一一打印即可

```c
/* 打印任务列表 */
void sys_ps(void) {
```

把“..” 和 “.” 进行路径转换

```c
/* 将路径old_abs_path中的..和.转换为实际路径后存入new_abs_path */
static void wash_path(char* old_abs_path, char* new_abs_path)
```

处理path， 如果不是绝对路径就先得到绝对路径随后进行拼接，最后再进行转换（利用上个函数）

```c
/* 将path处理成不含..和.的绝对路径,存储在final_path */
void make_clear_abs_path(char* path, char* final_path) {
   char abs_path[MAX_PATH_LEN] = {0};
   /* 先判断是否输入的是绝对路径 */
   if (path[0] != '/') {      // 若输入的不是绝对路径,就拼接成绝对路径
      memset(abs_path, 0, MAX_PATH_LEN);
      if (getcwd(abs_path, MAX_PATH_LEN) != NULL) {
	 if (!((abs_path[0] == '/') && (abs_path[1] == 0))) {	     // 若abs_path表示的当前目录不是根目录/
	    strcat(abs_path, "/");
	 }
      }
   }
   strcat(abs_path, path);
   wash_path(abs_path, final_path);
}
```

实现pwd, 底层调用getcwd 写入缓冲区 再打印就好了

```c
/* pwd命令的内建函数 */
void buildin_pwd(uint32_t argc, char** argv UNUSED)
```

实现cd, 先调用make_clear_abs_path 对 参数进行处理，随后在chdir（需要绝对目录）即可

```c
/* cd命令的内建函数 */
char* buildin_cd(uint32_t argc, char** argv) 
```

ls相对复杂，先看参数有没有路径，没有的话就用当前路径，如果是目录就readdir搞定，如果要详细信息(ls -l)那就得到名字以后再调用stat()进行检索，如果是文件就直接输出了

```c
/* ls命令的内建函数 */
void buildin_ls(uint32_t argc, char** argv) {
  //略
  if (pathname == NULL) {	 // 若只输入了ls 或 ls -l,没有输入操作路径,默认以当前路径的绝对路径为参数.
      if (NULL != getcwd(final_path, MAX_PATH_LEN)) {
	 pathname = final_path;
      } else {
	 printf("ls: getcwd for default path failed\n");
	 return;
      }
   } else {
      make_clear_abs_path(pathname, final_path);
      pathname = final_path;
   }
  //略
  
  if (file_stat.st_filetype == FT_DIRECTORY) {
    while((dir_e = readdir(dir))) {
	    printf("%s ", dir_e->filename);
	 }
  }
}
```

以下三个都是先make_clear_abs_path 再调用相应底层函数

```c
/* mkdir命令内建函数 */
int32_t buildin_mkdir(uint32_t argc, char** argv) 

/* rmdir命令内建函数 */
int32_t buildin_rmdir(uint32_t argc, char** argv)

/* rm命令内建函数 */
int32_t buildin_rm(uint32_t argc, char** argv)
```



#### 加载用户进程

通过exec 把一个可执行文件的绝对路径当作参数，把正在运行的用户进程的程序体（数据段，代码段，堆，栈，进行替换 by 可执行文件的文件体）

读入段，ELF文件中会有每段的加载地址，大小，以及本段在ELF中的位置，这里实现读入段的功能，首先计算读入的页数，随后一页一页判断页在不在，不在就分配页，都准备好后，把这个虚拟地址当成缓冲区，直接读入filesz（也就是段大小即可）

```c
/* 将文件描述符fd指向的文件中,偏移为offset,大小为filesz的段加载到虚拟地址为vaddr的内存 */
static bool segment_load(int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr)
```

Load 功能是从文件系统中加载用户程序pathname，成功则返回程序的起始地址，否则返回-1

先通过file_open打开获得文件inode, 并得到文件描述符，随后sys_read文件ELF头（一定在一开始，写死了）,校验elf头, 随后根据elf头找到 段头表的位置，以及每个段头的描述符的大小，进行遍历，根据段头的描述符的信息，如果是可加载的，就把它加载到指定位置就行了（segment_load），最后返回elf_header.e_entry

```c
/* 从文件系统上加载用户程序pathname,成功则返回程序的起始地址,否则返回-1 */
static int32_t load(const char* pathname) {
   int32_t ret = -1;
   struct Elf32_Ehdr elf_header;
   struct Elf32_Phdr prog_header;
   memset(&elf_header, 0, sizeof(struct Elf32_Ehdr));

   int32_t fd = sys_open(pathname, O_RDONLY);
   if (fd == -1) {
      return -1;
   }

   if (sys_read(fd, &elf_header, sizeof(struct Elf32_Ehdr)) != sizeof(struct Elf32_Ehdr)) {
      ret = -1;
      goto done;
   }

   /* 校验elf头 */
   if (memcmp(elf_header.e_ident, "\177ELF\1\1\1", 7) \
      || elf_header.e_type != 2 \
      || elf_header.e_machine != 3 \
      || elf_header.e_version != 1 \
      || elf_header.e_phnum > 1024 \
      || elf_header.e_phentsize != sizeof(struct Elf32_Phdr)) {
      ret = -1;
      goto done;
   }

   Elf32_Off prog_header_offset = elf_header.e_phoff; 
   Elf32_Half prog_header_size = elf_header.e_phentsize;

   /* 遍历所有程序头 */
   uint32_t prog_idx = 0;
   while (prog_idx < elf_header.e_phnum) {
      memset(&prog_header, 0, prog_header_size);
      
      /* 将文件的指针定位到程序头 */
      sys_lseek(fd, prog_header_offset, SEEK_SET);

     /* 只获取程序头 */
      if (sys_read(fd, &prog_header, prog_header_size) != prog_header_size) {
	 ret = -1;
	 goto done;
      }

      /* 如果是可加载段就调用segment_load加载到内存 */
      if (PT_LOAD == prog_header.p_type) {
	 if (!segment_load(fd, prog_header.p_offset, prog_header.p_filesz, prog_header.p_vaddr)) {
	    ret = -1;
	    goto done;
	 }
      }

      /* 更新下一个程序头的偏移 */
      prog_header_offset += elf_header.e_phentsize;
      prog_idx++;
   }
   ret = elf_header.e_entry;
done:
   sys_close(fd);
   return ret;
}
```

最后用path指向的程序替换当前进程, 把各个段load进来后得到起始地址,接着把参数，和参数个数，其实地址等信息都放到中断栈的指定寄存器，以及栈地址，随后直接内联汇编赋值，把中断栈赋值给esp，然后jmp intr_exit跳出中断，最后的return 0其实不会执行

```c
/* 用path指向的程序替换当前进程 */
int32_t sys_execv(const char* path, const char* argv[]) {
   uint32_t argc = 0;
   while (argv[argc]) {
      argc++;
   }
   int32_t entry_point = load(path);     
   if (entry_point == -1) {	 // 若加载失败则返回-1
      return -1;
   }
   
   struct task_struct* cur = running_thread();
   /* 修改进程名 */
   memcpy(cur->name, path, TASK_NAME_LEN);
   cur->name[TASK_NAME_LEN-1] = 0;

   struct intr_stack* intr_0_stack = (struct intr_stack*)((uint32_t)cur + PG_SIZE - sizeof(struct intr_stack));
   /* 参数传递给用户进程 */
   intr_0_stack->ebx = (int32_t)argv;
   intr_0_stack->ecx = argc;
   intr_0_stack->eip = (void*)entry_point;
   /* 使新用户进程的栈地址为最高用户空间地址 */
   intr_0_stack->esp = (void*)0xc0000000;

   /* exec不同于fork,为使新进程更快被执行,直接从中断返回 */
   asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (intr_0_stack) : "memory");
   return 0;
}
```

实现的时候需要先把待执行的文件写入到文件系统中，再执行，可以先找块裸盘，把elf文件写入裸盘中，随后创立同名文件，接着把裸盘中的文件读到内存中，再把内存的文件拷贝写到（ide_write）同名文件中即可，但目前还无法传参，只写了个不含参的函数（生成elf）

目前为止，init进程（pid=1）执行了init函数，生成了子进程(pid=4)执行shell(死循环)，main主进程（pid=2, 特意把init进程放到main中抢先分配pid,虽然是现有的main进程，但是后分配的pid）在main() 函数中while(1);了， 还有生成了idle线程（pid=3）。

**实现用户进程支持参数**

之前编写用户进程的时候都有个main函数，其中int main(argc, argv), 这说明main函数即有参数传进来，又有返回值，所以main函数只是其中一个“中间环节”，为了简化开发，要把“头”和“尾”都准备好，于是有了各种库，比如C标准库和C运行时库，其中C标准库是与操作系统平台无关的，用户在不同操作系统上调用同样的函数接口，获得同样的结果，但总有些是离不开操作系统本身独特性的，就是运行时库，基于C标准库开发的，但是补充拓展了一些C标准库中没有的功能，所以它本身肯定不是通用的。

CRT最主要的就是初始化运行环境，在main函数之前做好自己的工作，传递参数等等，等待条件都准备好之后再调用main函数，等main函数结束后，还要回收用户进程的资源（exit）。

简易CRT,把之前保存的参数压给main，在自己写的main里 要while(1);卡住。 Call main 之后

```nasm
[bits 32]
extern	 main
section .text
global _start
_start:
   ;下面这两个要和execv中load之后指定的寄存器一致
   push	 ebx	  ;压入argv
   push  ecx	  ;压入argc
   call  main
  ;见下文
```

**exit与wait**

之前的CRT未实现exit,之后实现，主要功能是回收资源。wait与之相似，但wait的作用是阻塞父进程，知道任意一个子进程结束运行。如果没有子进程，那么wait会返回-1(kernel可以通过PCB中的parent_pid 挨个查看)，如果有子进程，那么就会阻塞自己，直到某个子进程退出了（exit）然后把返回值传递给父进程，随后将父进程唤醒。wait的主要作用就是使父进程阻塞，同时获得子进程的返回值。也就是让子进程先执行。

具体来说，当子进程执行完main函数，会回到CRT中，CRT负责把进程return的返回值通过系统调用exit提交给内核（还是在PCB中预留一个空间），也可以main函数里 直接调用exit(int *status),为此进程在调用exit时，表示他的生命周期结束了，但是预留空间的PCB不能被回收，直到父进程取走返回值（内核负责），才可以回收。但是有两种特殊情况：

+ 父进程提前退出时，所有子进程还在执行，那么init进程会成为这些进程新的父亲，回收PCB。 -- 孤儿进程
+ 如果父进程压根没有调用wait等待子进程的返回值，那么当子进程exit退出了以后，没有人负责接收返回值，相当于僵尸进程，只剩子进程的PCB了，影响倒是不大，但是毕竟占着pid,可能会影响之后的分配。



实现一些辅助函数。完成回收资源相关的工作。回收物理页。

```c
/* 根据物理页框地址pg_phy_addr在相应的内存池的位图清0,不改动页表*/
void free_a_phy_page(uint32_t pg_phy_addr)
```

pid分配回收相关

```c
/* pid池 */
struct pid_pool {
   struct bitmap pid_bitmap;  // pid位图
   uint32_t pid_start;	      // 起始pid
   struct lock pid_lock;      // 分配pid锁
}pid_pool;

/* 初始化pid池 */
static void pid_pool_init(void)
  /* 分配pid */
static pid_t allocate_pid(void)
  /* 释放pid */
void release_pid(pid_t pid) 
```

回收掉页表和pcb所在页

```c
* 回收thread_over的pcb和页表,并将其从调度队列中去除 */
void thread_exit(struct task_struct* thread_over, bool need_schedule)
```

找到pid对应的PCB，list_travesal会调用 

```c
/* 根据pid找pcb,若找到则返回该pcb,否则返回NULL */
struct task_struct* pid2thread(int32_t pid)
```

回收除PCB以外的资源，一页一页回收内存（如果页目录项存在（P位为1），就进去检查页表项），回收用户虚拟地址池所占的物理内存, 再把打开的文件一一关闭就好了

```c
/* 释放用户进程资源: 
 * 1 页表中对应的物理页
 * 2 虚拟内存池占物理页框
 * 3 关闭打开的文件 */
static void release_prog_resource(struct task_struct* release_thread)
```

查找子进程相关

```c
/* list_traversal的回调函数,
 * 查找pelem的parent_pid是否是ppid,成功返回true,失败则返回false */
static bool find_child(struct list_elem* pelem, int32_t ppid)
  
  /* list_traversal的回调函数,
 * 查找状态为TASK_HANGING的任务 */
static bool find_hanging_child(struct list_elem* pelem, int32_t ppid)
  
  /* list_traversal的回调函数,
 * 将一个子进程过继给init */
static bool init_adopt_a_child(struct list_elem* pelem, int32_t pid)
```

sys_wait, 就是之前描述的代码实现,先看有没有合适的（挂起）子进程，如果没有就检查有没有子进程，都没有就直接wait，否则就挂起，等待子进程exit后叫醒

```c
/* 等待子进程调用exit,将子进程的退出状态保存到status指向的变量.
 * 成功则返回子进程的pid,失败则返回-1 */
pid_t sys_wait(int32_t* status) {
   struct task_struct* parent_thread = running_thread();

   while(1) {
      /* 优先处理已经是挂起状态的任务 */
      struct list_elem* child_elem = list_traversal(&thread_all_list, find_hanging_child, parent_thread->pid);
      /* 若有挂起的子进程 */
      if (child_elem != NULL) {
	 struct task_struct* child_thread = elem2entry(struct task_struct, all_list_tag, child_elem);
	 *status = child_thread->exit_status; 

	 /* thread_exit之后,pcb会被回收,因此提前获取pid */
	 uint16_t child_pid = child_thread->pid;

	 /* 2 从就绪队列和全部队列中删除进程表项*/
	 thread_exit(child_thread, false); // 传入false,使thread_exit调用后回到此处
	 /* 进程表项是进程或线程的最后保留的资源, 至此该进程彻底消失了 */

	 return child_pid;
      } 

      /* 判断是否有子进程 */
      child_elem = list_traversal(&thread_all_list, find_child, parent_thread->pid);
      if (child_elem == NULL) {	 // 若没有子进程则出错返回
	 return -1;
      } else {
      /* 若子进程还未运行完,即还未调用exit,则将自己挂起,直到子进程在执行exit时将自己唤醒 */
	 thread_block(TASK_WAITING); 
      }
   }
}
```



```c
/* 子进程用来结束自己时调用 */
void sys_exit(int32_t status) {
   struct task_struct* child_thread = running_thread();
   child_thread->exit_status = status; 
   if (child_thread->parent_pid == -1) {
      PANIC("sys_exit: child_thread->parent_pid is -1\n");
   }

   /* 将进程child_thread的所有子进程都过继给init */
   list_traversal(&thread_all_list, init_adopt_a_child, child_thread->pid);

   /* 回收进程child_thread的资源 */
   release_prog_resource(child_thread); 

   /* 如果父进程正在等待子进程退出,将父进程唤醒 */
   struct task_struct* parent_thread = pid2thread(child_thread->parent_pid);
   if (parent_thread->status == TASK_WAITING) {
      thread_unblock(parent_thread);
   }

   /* 将自己挂起,等待父进程获取其status,并回收其pcb */
   thread_block(TASK_HANGING);
}
```



完善crt

```nasm
[bits 32]
extern	 main
extern	 exit 
section .text
global _start
_start:
   ;下面这两个要和execv中load之后指定的寄存器一致
   push	 ebx	  ;压入argv
   push  ecx	  ;压入argc
   call  main

   ;将main的返回值通过栈传给exit,gcc用eax存储返回值,这是ABI规定的
   push  eax
   call exit
   ;exit不会返回
```

实现cat 就是把文件打开（open），随后读出来读到缓冲区，再写到屏幕上（write 中的fd 为stdin_no 即可）,现在可以把之前的while(1);全部替换exit了，可释放pid号，过程略
