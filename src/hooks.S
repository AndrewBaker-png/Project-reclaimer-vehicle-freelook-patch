.intel_syntax noprefix
.text
# Save the interrupted machine state, including flags, x87/SSE state and all GPRs.
# MinHook relocates the overwritten instructions; these stubs run before them.
.macro MIDHOOK name, callback, trampoline
.global \name
\name:
 pushfq
 push rax
 push rcx
 push rdx
 push rbx
 push rbp
 push rsi
 push rdi
 push r8
 push r9
 push r10
 push r11
 push r12
 push r13
 push r14
 push r15
 mov rax,rsp
 and rsp,-16
 sub rsp,0x230
 mov [rsp+0x220],rax
 fxsave64 [rsp+0x20]
 lea rcx,[rsp+0x20]
 call \callback
 fxrstor64 [rsp+0x20]
 mov rsp,[rsp+0x220]
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
 pop rbx
 pop rdx
 pop rcx
 pop rax
 popfq
 jmp [rip+\trampoline]
.endm
MIDHOOK inputXHook, onInputX, inputXTrampoline
MIDHOOK inputYHook, onInputY, inputYTrampoline
MIDHOOK cameraHook, onCamera, cameraTrampoline
