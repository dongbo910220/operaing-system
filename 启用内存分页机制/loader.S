    %include "boot.inc"
    section loader vstart = LOADER_BASE_ADDR
    LOADER_STACK_TOP equ LOADER_BASE_ADDR
    ;jmp loader_start


    ;构造gdt 以及内部的描述符
    GDT_BASE:   dd     0x00000000
                dd     0x00000000

    CODE_DESC:  dd     0x0000FFFF
                dd      DESC_CODE_HIGH4

    DATA_STACK_DESC:    dd     0x0000FFFF
                        dd     DESC_DATA_HIGH4

    VIDEO_DESC:         dd     0x80000007
                        dd     DESC_VIDEO_HIGH4

    GDT_SIZE    equ        $ - GDT_BASE
    GDT_LIMIT   equ        GDT_SIZE - 1
    times 60 dq 0
    SELECTOR_CODE equ (0x0001 << 3) + TI_GDT + PRL0
    SELECTOR_DATA equ (0x0002 << 3) + TI_GDT + PRL0
    SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + PRL0

    total_mem_bytes dd 0

    gdt_ptr dw GDT_LIMIT
            dd GDT_BASE

    ards_buf times 244 db 0
    ards_nr dw 0

    loader_start:

        xor ebx, ebx
        mov edx, 0x534d4150
        mov di, ards_buf
    .e820_mem_get_loop:
        mov eax, 0x0000e820
        mov ecx, 20
        int 0x15
        jc  .e820_failed_so_try_e801
        add di, cx
        inc word [ards_nr]
        cmp ebx, 0
        jnz .e820_mem_get_loop

        ;在所有ards结构中，找出(base_add_low + length_low)的最大值，即内存的容量。
           mov cx, [ards_nr]	      ;遍历每一个ARDS结构体,循环次数是ARDS的数量
           mov ebx, ards_buf
           xor edx, edx		      ;edx为最大的内存容量,在此先清0
.find_max_mem_area:
            mov eax, [ebx]
            add eax, [ebx + 8]
            add ebx, 20
            cmp edx, eax
            jge .next_ards
            mov edx, eax
.next_ards:
            loop .find_max_mem_area
            jmp .mem_get_ok

;------  int 15h ax = E801h 获取内存大小,最大支持4G  ------
; 返回后, ax cx 值一样,以KB为单位,bx dx值一样,以64KB为单位
; 在ax和cx寄存器中为低16M,在bx和dx寄存器中为16MB到4G。
.e820_failed_so_try_e801:
;略

.mem_get_ok:
            mov [total_mem_bytes], edx



; 以下全部注释掉
        ;mov sp, LOADER_BASE_ADDR
        ;mov bp, loadermsg
        ;mov cx, 17
        ;mov ax, 0x1301
        ;mov bx, 0x001f
        ;mov dx, 0x1800
        ;int 0x10

;-------------------------  准备进入保护模式 -------------------

        ;-----------------  打开A20  ----------------
        in al, 0x92
        or al, 0000_0010B
        out 0x92,al

        ;-----------------  加载GDT  ----------------
        lgdt [gdt_ptr]

        ;-----------------  cr0第0位置1  ----------------
        mov eax, cr0
        or eax, 0x00000001
        mov cr0, eax

        jmp SELECTOR_CODE: p_mode_start

[bits 32]
p_mode_start:
    mov ax,SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    ; 创建页目录及页表并初始化页内存位图
    call setup_page

    sgdt [gdt_ptr]

    mov ebx, [gdt_ptr + 2]
    or dword [ebx + 0x18 + 4], 0xc0000000

    add dword [gdt_ptr + 2], 0xc0000000

    add esp, 0xc0000000

    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    lgdt [gdt_ptr]  ; 重新加载

    mov byte [gs:160], 'V'

    jmp $

    ;-------------   创建页目录及页表   ---------------
setup_page:
    ;先把页目录占用空间以逐个字节的方式清零
        mov ecx, 4096
        mov esi, 0
    .clear_page_dir:
        mov byte [PAGE_DIR_TABLE_POS + esi], 0
        inc esi
        loop .clear_page_dir

    .create_pde:
        mov eax, PAGE_DIR_TABLE_POS
        add eax, 0x1000
        mov ebx, eax

        or eax, PG_US_U | PG_RW_W | PG_P
        mov [PAGE_DIR_TABLE_POS + 0x0], eax
        mov [PAGE_DIR_TABLE_POS + 0xc00], eax
        sub eax, 0x1000
        mov [PAGE_DIR_TABLE_POS + 4092], eax

    ;------   创建页表项   ----------
        mov ecx, 256
        mov esi, 0
        mov edx, PG_US_U | RG_RW_W | PG_P
    .create_pte:
        mov [ebx + esi * 4], edx
        add edx, 4096
        inc esi
        loop .create_pte

    ;创建内核其它页表的PDE
        mov eax, PAGE_DIR_TABLE_POS
        add eax, 0x2000
        or eax, PG_US_U | PG_RW_W | PG_P
        mov ebx, PAGE_DIR_TABLE_POS
        mov ecx, 254
        mov esi, 769
    .create_kernel_pde:
        mov [ebx+ esi * 4], eax
        inc esi
        add eax, 0x1000
        loop .create_kernel_pde
        ret
