bits 32

;for lack of a better name
global ASM_GDT_SUBMIT
ASM_GDT_SUBMIT:
; gives the location of the GDT descriptor to the cpu
;   input:
;       - pointer to the GDT descriptor
;   output:
;       - N/A

    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    lgdt [eax]

    mov eax, cr0
    or eax, 0001b
    mov cr0, eax

    ;should be 0x08
    jmp 0x08:.SEGMENT_RELOAD

    .SEGMENT_RELOAD:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax   

    mov esp, ebp
    pop ebp
ret

global ASM_IDT_SUBMIT
ASM_IDT_SUBMIT:
; gives the IDT descriptor location to the cpu
;   input:
;       - pointer to the IDT
;   output:
;       - N/A

    push ebp
    mov ebp, esp

    mov edx, [ebp + 8]
    lidt [edx]
    sti

    mov esp, ebp
    pop ebp
ret

global ASM_CPU_PAGING_ENABLE
ASM_CPU_PAGING_ENABLE:
; hopefully this enables paging correctly
;   input:
;       - pointer to the page table
;   output:
;       - N/A

    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    mov cr3, eax
   
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    mov esp, ebp
    pop ebp
ret

global ASM_CPU_INVLPG
ASM_CPU_INVLPG:
; invalidates a page
; input:
;   - physical address where the page table points to
; output
;   - N/A
    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    mov [eax], eax
    invlpg [eax]

    mov esp, ebp
    pop ebp
ret

global ASM_CHECK_CPUID
ASM_CHECK_CPUID:
; check if CPUID is supported
;   input:
;       - N/A
;   output:
;       - 1 if supported, 0 if unsupported (in CPUID_AVAILABLE)

    pushfd
     
    mov eax, DWORD [esp]   

    popfd   

    and eax, 0x200000
    cmp eax, 0x200000
    jne .done

    mov BYTE [CPUID_AVAILABLE], 1

    .done:
ret

global ASM_CPU_GETVENDOR
ASM_CPU_GETVENDOR:
; gets the vendor id of the cpu
;   input:
;       - N/A
;   output:
;       - Vendor string (in var CPUID_VENDOR_STRING)

    mov eax, 0
    cpuid

    mov DWORD [CPUID_SUPPORTED_FUNCTIONS], eax

    mov DWORD [CPUID_VENDOR + 0x00], ebx
    mov DWORD [CPUID_VENDOR + 0x04], edx
    mov DWORD [CPUID_VENDOR + 0x08], ecx 

    mov eax, CPUID_VENDOR  
    mov DWORD [CPUID_VENDOR_STRING], eax 

ret


global ASM_CPU_GETNAME
ASM_CPU_GETNAME:
; gets the cpu name string of the cpu
;   input:
;       - N/A
;   output:
;       - Vendor string (in var CPUID_CPUNAME_STRING)

    mov eax, 0x80000002
    cpuid

    mov DWORD [CPUID_CPUNAME + 0x00], eax
    mov DWORD [CPUID_CPUNAME + 0x04], ebx
    mov DWORD [CPUID_CPUNAME + 0x08], ecx
    mov DWORD [CPUID_CPUNAME + 0x0C], edx  

    mov eax, 0x80000003
    cpuid

    mov DWORD [CPUID_CPUNAME + 0x10], eax
    mov DWORD [CPUID_CPUNAME + 0x14], ebx
    mov DWORD [CPUID_CPUNAME + 0x18], ecx
    mov DWORD [CPUID_CPUNAME + 0x1C], edx  

    mov eax, 0x80000004
    cpuid

    mov DWORD [CPUID_CPUNAME + 0x20], eax
    mov DWORD [CPUID_CPUNAME + 0x24], ebx
    mov DWORD [CPUID_CPUNAME + 0x28], ecx
    mov DWORD [CPUID_CPUNAME + 0x2C], edx  

    mov eax, CPUID_CPUNAME
    mov DWORD [CPUID_CPUNAME_STRING], eax

ret


global CPUID_AVAILABLE
CPUID_AVAILABLE db 0

global CPUID_VENDOR_STRING
CPUID_VENDOR_STRING dd 0
CPUID_VENDOR times 13 db 0

global CPUID_CPUNAME_STRING
CPUID_CPUNAME_STRING dd 0
CPUID_CPUNAME times 48 db 0

global CPUID_SUPPORTED_FUNCTIONS
CPUID_SUPPORTED_FUNCTIONS dd 0

