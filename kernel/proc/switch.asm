[bits 64]

global context_switch
global enter_user_mode
global resume_user_from_saved_frame

section .text

context_switch:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov [rdi], rsp

    mov rsp, rsi

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ret

; Переход в user-mode через iretq.
; Вход: rdi = указатель на слот, куда сохранить текущий kernel rsp,
;       rsi = адрес iretq-фрейма (rip, cs, rflags, rsp, ss).
; Сохраняет регистры ядра на kernel-стеке, чтобы exit_process мог вернуться
; обратно через context_switch(saved_rsp, resume_rsp).
enter_user_mode:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov [rdi], rsp
    mov rsp, rsi
    iretq

; Возврат в user-mode из сохранённого InterruptFrame на стеке.
; Вход: rdi = указатель на InterruptFrame (с размеченностью idt::InterruptFrame)
resume_user_from_saved_frame:
    mov rsp, rdi
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16
    iretq
