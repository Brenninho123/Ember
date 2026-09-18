MBALIGN     equ  1 << 0
MEMINFO     equ  1 << 1
VIDINFO     equ  1 << 2
FLAGS       equ  MBALIGN | MEMINFO
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 65536
stack_top:

align 4096
boot_page_directory:
    resb 4096
boot_page_table1:
    resb 4096

section .text
global _start
extern kernel_main
extern kernel_early_init

_start:
    cli

    mov esp, stack_top
    mov ebp, esp

    push ebx
    push eax

    call kernel_early_init

    pop eax
    pop ebx

    push ebx
    push eax
    call kernel_main
    add esp, 8

    cli
.hang:
    hlt
    jmp .hang

global gdt_flush_asm
gdt_flush_asm:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.reload_cs
.reload_cs:
    ret

global idt_flush_asm
idt_flush_asm:
    mov eax, [esp + 4]
    lidt [eax]
    ret
