bits 32

extern api_dispatcher
extern utilPool

global api_dispatcher_start
api_dispatcher_start:
; 'Bootstrap' function for system calls
;   input:
;       - EDI: EIP to return to after system call
;       - ESI: Pointer to system call request
;   output:
;       - N/A
push esi
push edi

call api_dispatcher

; return to function calling us
jmp api_dispatcher_return

global api_dispatcher_return
api_dispatcher_return:

; return
push ss
push esp
pushfd
push cs
push eax
iretd

.instrptr dd 0x00