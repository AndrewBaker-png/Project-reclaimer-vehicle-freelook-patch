.intel_syntax noprefix
.text

.global testLiftedInput
testLiftedInput:
 push r12
 push r13
 sub rsp,0x188
 mov r12,rcx
 mov r13,rdx
.global testLiftedInputSite
testLiftedInputSite:
 mov rax,[rsp+0xd0]
 mov rcx,[rsp+0xd0]
 add rsp,0x188
 pop r13
 pop r12
 ret

.global testLiftedCamera
testLiftedCamera:
 push rbp
 mov rbp,rsp
 sub rsp,0x30
 mov [rsp+0x20],rbx
 mov [rsp+0x28],r12
 mov r12,rcx
 mov ebx,edx
 mov rax,r8
.global testLiftedCameraSite
testLiftedCameraSite:
 mov rbx,[rsp+0x20]
 mov r12,[rsp+0x28]
 add rsp,0x30
 mov rsp,rbp
 pop rbp
 ret
