global gdt_flush
global idt_flush

section .text

gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush
.flush:
    ret

idt_flush:
    mov eax, [esp + 4]
    lidt [eax]
    ret

global load_page_directory
global enable_paging

load_page_directory:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

enable_paging:
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret

global io_wait

io_wait:
    out 0x80, al
    ret
