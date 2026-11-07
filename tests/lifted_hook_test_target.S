.intel_syntax noprefix
.text

.global testLiftedInput
testLiftedInput:
 sub rsp,0xb8
 movaps [rsp+0x90],xmm6
 movaps [rsp+0xa0],xmm7
 mov rax,rdx
.global testLiftedInputSite
testLiftedInputSite:
 movaps xmm6,[rsp+0x90]
 movaps xmm7,[rsp+0xa0]
 add rsp,0xb8
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
