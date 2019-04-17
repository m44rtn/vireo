;inVBE.ASM - initializes VESA

bits 16
[map all pVESA.map]

ModuleHeader:
    jmp short main
    nop
    .Sign                       db "_VIREO_SYS", 0x00
    .Type                       db 0x03
    .Entry                      times 2 dw 0x00 ;undefined - not used by kernel
    .Size                       times 2 dw 0x00 ;undefined - not used by kernel

main:
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call getVESAControllerInfo
    
    ;Notify the kernel we're done here and wait until it responds
    mov ax, [finished]
    mov bp, ax

    mov ax, 0
    jmp infinite

getVESAControllerInfo:
    mov ax, 0x4f00
    mov di, VESAinfo
    int 0x10

    cmp ax, 0x004F
    jne .fail

    mov ds, WORD [VESAinfo.VideoModeListSegment]

    mov si, WORD [VESAinfo.VideoModeListOffset]
    mov edi, 0
    mov edx, 0

    .GetHighest:
        mov cx, [si]
        cmp cx, 0xFFFF
        je setRes

       
        push gs

        mov ax, 0x4F01
        mov di, VESAModeInfo
        int 0x10

        mov ax, 0
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax

        pop gs

        cmp DWORD [VESAModeInfo.PhysBasePtr], 0x00
        je .next

        push edx
        mov eax, 0
        mov ebx, 0
        mov WORD ax, [VESAModeInfo.YResolution]
        mov word bx, [VESAModeInfo.BytesPerScanline]
        mul ebx
        pop edx

        cmp eax, edx
        jna .next

        mov gs, cx
        mov edx, eax

.next:
    add si, 2
    jmp .GetHighest

    .fail:
       jmp $

setRes:
    mov cx, gs
    mov ax, 0x4F01
    mov di, VESAModeInfo
    int 0x10

    mov ax, 0x4F02
    mov bx, cx
    int 0x10

    jmp infinite
;ret


pVESA_SetDiffMode:
    pop eax
    pop ebx
ret
    ;todo and all else stuff todo withhhh this function


infinite:
    jmp $

VESAinfo: ;VESA
.VBEsignature db "VBE2"
.VBEversionMinor db 0x00	;Could be one DWORD?
.VBEversionMajor db 0x00
.OEMstrOffset dw 0x0000
.OEMstrSegment dw 0x0000
.Capabilities times 4 db 0x00
.VideoModeListOffset dw 0x0000
.VideoModeListSegment dw 0x0000
.TotalMemory dw 0x0000 	;number of 64KB blocks
.OEMSoftwareRev dw 0x0000
.OEMVendorNameOffset dw 0x0000
.OEMVendorNameSegment dw 0x0000
.OEMProductNameOffset dw 0x0000
.OEMProductNameSegment dw 0x0000
.OEMProductRevOffset dw 0x0000
.OEMProductRevSegment dw 0x0000
.Reserved times 222 db 0
.OEMDatatimes times 256 db 0

VESAModeInfo:
.ModeAttributes dw 0
.WinAAttributes	db 0
.WinBAttributes	db 0
.WinGranularity	dw 0
.WinSize		dw 0
.WinASegment	dw 0
.WinBSegment	dw 0
.WinFuncPtr		dd 0
.BytesPerScanline dw 0
.XResolution dw 0
.YResolution dw 0
.XCharSize	db 0
.YCharSize	db 0
.NumberOfPlanes	db 0
.BitsPerPixel	db 0
.NumberOfBanks	db 0
.MemoryModel db 0
.BankSize db 0
.NumberOfImagePages	db 0
.ReservedA			db 0
.RedMaskSize	db 0
.RedFieldPosition db 0
.GreenMaskSize db 0
.GreenFieldPosition db 0
.BlueMaskSize db 0
.BlueFieldPosition db 0
.RsvdMaskSize db 0
.RsvdFieldPosition db 0
.DirectColorModeInfo db 0
.PhysBasePtr dd 0
.OffScreenMemOffset dd 0
.OffScreenMemSize dw 0
.ReservedB times 206 db 0x00

finished db '_VIREO-DONE'

size_marker: