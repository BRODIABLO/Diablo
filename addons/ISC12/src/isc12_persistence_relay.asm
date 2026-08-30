OPTION CASEMAP:NONE

EXTERN ISC12PrepareNativeStoreRead:PROC
EXTERN ISC12PrepareNativeStoreWrite:PROC

EXTERN gISC12PersistenceReaderContinueExit:QWORD
EXTERN gISC12PersistenceReaderRejectedExit:QWORD
EXTERN gISC12PersistenceWriterVanillaExit:QWORD
EXTERN gISC12PersistenceWriterCommittedExit:QWORD
EXTERN gISC12PersistenceWriterRejectedExit:QWORD

PUBLIC ISC12PersistenceReaderMidHook
PUBLIC ISC12PersistenceWriterMidHook
PUBLIC ISC12PersistenceRelayTemplateBegin
PUBLIC ISC12PersistenceRelayTemplateWriterEntry
PUBLIC ISC12PersistenceRelayTemplateReaderContinueExit
PUBLIC ISC12PersistenceRelayTemplateReaderRejectedExit
PUBLIC ISC12PersistenceRelayTemplateWriterVanillaExit
PUBLIC ISC12PersistenceRelayTemplateWriterCommittedExit
PUBLIC ISC12PersistenceRelayTemplateWriterRejectedExit
PUBLIC ISC12PersistenceRelayTemplatePlayerSaveFinalizeEntry
PUBLIC ISC12PersistenceRelayTemplateStatePointer
PUBLIC ISC12PersistenceRelayTemplateEnd

.code

; Both future save hooks share one process-lifetime RX block and one separate
; RW state block. All branches and the state-slot reference are internal, so
; this complete block remains position-independent after it is copied near the
; two save seams. P3b prepares it but deliberately publishes neither hook.
ALIGN 16
ISC12PersistenceRelayTemplateBegin LABEL BYTE
    mov r11, qword ptr [ISC12PersistenceRelayTemplateStatePointer]
    lock inc dword ptr [r11]
    cmp dword ptr [r11+4], 0
    je PersistenceRelayFailClosed
    jmp qword ptr [r11+10h]

ISC12PersistenceRelayTemplateReaderContinueExit:
    mov r11, qword ptr [ISC12PersistenceRelayTemplateStatePointer]
    lock dec dword ptr [r11]
    lea rcx, [rsp+28h]
    jmp qword ptr [r11+20h]

ISC12PersistenceRelayTemplateReaderRejectedExit:
    mov r11, qword ptr [ISC12PersistenceRelayTemplateStatePointer]
    lock dec dword ptr [r11]
    jmp qword ptr [r11+28h]

ALIGN 16
ISC12PersistenceRelayTemplateWriterEntry:
    mov r11, qword ptr [ISC12PersistenceRelayTemplateStatePointer]
    lock inc dword ptr [r11]
    cmp dword ptr [r11+4], 0
    je PersistenceRelayFailClosed
    jmp qword ptr [r11+18h]

ISC12PersistenceRelayTemplateWriterVanillaExit:
    mov r11, qword ptr [ISC12PersistenceRelayTemplateStatePointer]
    lock dec dword ptr [r11]
    lea rcx, [rsp+28h]
    jmp qword ptr [r11+30h]

ISC12PersistenceRelayTemplateWriterCommittedExit:
    mov r11, qword ptr [ISC12PersistenceRelayTemplateStatePointer]
    lock dec dword ptr [r11]
    jmp qword ptr [r11+38h]

ISC12PersistenceRelayTemplateWriterRejectedExit:
    mov r11, qword ptr [ISC12PersistenceRelayTemplateStatePointer]
    lock dec dword ptr [r11]
    jmp qword ptr [r11+40h]

PersistenceRelayFailClosed:
    ; An installed persistence seam may never fall back into vanilla after
    ; its DLL handler is inactive: doing so could decode an ISC12 payload as
    ; nine-bit data or destructively rewrite its canonical file.
    mov ecx, 7
    int 29h
    jmp PersistenceRelayFailClosed

ALIGN 16
ISC12PersistenceRelayTemplatePlayerSaveFinalizeEntry:
    ; Leaf replacement for the native bit-writer used-end helper. RCX owns
    ; the writer. RAX preserves the native used-end result while EDX carries
    ; the sticky overflow flag to the caller's final status publication.
    cmp qword ptr [rcx+18h], 0
    mov rax, qword ptr [rcx+10h]
    je PlayerSaveFinalizeNoIncrement
    inc rax
PlayerSaveFinalizeNoIncrement:
    mov edx, dword ptr [rcx+20h]
    ret

ALIGN 8
ISC12PersistenceRelayTemplateStatePointer QWORD 0
ISC12PersistenceRelayTemplateEnd LABEL BYTE

ALIGN 16
ISC12PersistenceReaderMidHook PROC FRAME
    ; At 0x9FC654 RSP is aligned, RBX owns the locked save object, RDI is the
    ; announced file length, EAX is the native read status and [RSP+30h] is
    ; the native DWORD bytes-read slot.
    ; FRAME metadata covers this stub's allocation, not generic unwind through
    ; the tail-entered native frame or the copied relay. The noexcept callback
    ; contains expected failures and fast-fails every unexpected exception.
    sub rsp, 20h
    .allocstack 20h
    .endprolog
    mov rcx, rbx
    mov rdx, rdi
    mov r8d, dword ptr [rsp+50h]
    mov r9d, eax
    call ISC12PrepareNativeStoreRead
    add rsp, 20h
    cmp eax, 2
    je ReaderRejected
    cmp eax, 1
    jbe ReaderContinue
    mov ecx, 7
    int 29h

ReaderContinue:
    jmp qword ptr [gISC12PersistenceReaderContinueExit]

ReaderRejected:
    jmp qword ptr [gISC12PersistenceReaderRejectedExit]
ISC12PersistenceReaderMidHook ENDP

ALIGN 16
ISC12PersistenceWriterMidHook PROC FRAME
    ; At 0x9F95A2 RSP is aligned, RBX owns the locked save object and the
    ; already-built NUL-terminated UTF-8 canonical path begins at [RSP+40h].
    ; The stolen native constructor at 0x11C7E20 performs exactly this
    ; INVALID_HANDLE_VALUE initialization. Both target continuations may then
    ; execute the native close helper safely without opening the final path.
    ; As above, no recoverable exception may unwind across this tail-entered
    ; boundary; registered/chained unwind would be required before allowing it.
    sub rsp, 20h
    .allocstack 20h
    .endprolog
    mov qword ptr [rsp+48h], -1
    mov rcx, rbx
    lea rdx, [rsp+60h]
    call ISC12PrepareNativeStoreWrite
    add rsp, 20h
    test eax, eax
    je WriterVanilla
    cmp eax, 1
    je WriterCommitted
    cmp eax, 2
    je WriterRejected
    mov ecx, 7
    int 29h

WriterVanilla:
    jmp qword ptr [gISC12PersistenceWriterVanillaExit]

WriterCommitted:
    jmp qword ptr [gISC12PersistenceWriterCommittedExit]

WriterRejected:
    jmp qword ptr [gISC12PersistenceWriterRejectedExit]
ISC12PersistenceWriterMidHook ENDP

END
