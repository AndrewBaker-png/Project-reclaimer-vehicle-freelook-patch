.intel_syntax noprefix
.text
# Save the interrupted machine state, including flags, x87/SSE state and all GPRs.
# MinHook relocates the overwritten instructions; these stubs run before them.
.macro MIDHOOK name, callback, trampoline, lifted=0
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
.if \lifted
 mov rcx,[rax+0x18]
 mov rdx,[rax+0x10]
 mov r8,[rax+0x58]
 mov r9,[rax+0x70]
.else
 lea rcx,[rsp+0x20]
.endif
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
#ifdef FREELOOK_LIFTED_HOOKS
# Pass saved R12, R13, RBX and RAX to the lifted-engine callbacks.
MIDHOOK liftedInputHook, onLiftedInput, liftedInputTrampoline, 1
MIDHOOK liftedCameraHook, onLiftedCamera, liftedCameraTrampoline, 1
#endif

# Preserve the original function's private ABI, including volatile registers
# used by callers (R11 is a player-loop increment; XMM3-5 hold distances).
# Save the user index across the original call, then save its output state.
.global cameraResultHook
cameraResultHook:
 sub rsp,0x28
 mov [rsp+0x20],ecx
 call [rip+cameraTrampoline]
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
 mov rcx,[rax+0x70]
 mov edx,[rax+0xA0]
 call onCameraResult
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
 lea rsp,[rsp+0x28]
 ret
