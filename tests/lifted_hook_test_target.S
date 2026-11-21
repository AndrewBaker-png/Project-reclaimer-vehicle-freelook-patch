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

.global testLiftedInput0104
testLiftedInput0104:
 sub rsp,0xe8
 movaps [rsp+0xc0],xmm6
 movaps [rsp+0xd0],xmm7
 mov rax,rdx
.global testLiftedInput0104Site
testLiftedInput0104Site:
 movaps xmm6,[rsp+0xc0]
 movaps xmm7,[rsp+0xd0]
 add rsp,0xe8
 ret

.global testLiftedInput0108
testLiftedInput0108:
 push rbp
 sub rsp,0x80
 mov rbp,rsp
 movaps [rbp+0x50],xmm6
 movaps [rbp+0x60],xmm7
 mov rax,rdx
.global testLiftedInput0108Site
testLiftedInput0108Site:
 movaps xmm6,[rbp+0x50]
 movaps xmm7,[rbp+0x60]
 add rsp,0x80
 pop rbp
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

.global testLiftedCamera0102
testLiftedCamera0102:
 push rbp
 mov rbp,rsp
 sub rsp,0x30
 mov [rsp+0x20],rbx
 mov [rsp+0x28],r12
 mov rbx,rcx
 mov r12d,edx
 mov rax,r8
.global testLiftedCamera0102Site
testLiftedCamera0102Site:
 mov rbx,[rsp+0x20]
 mov r12,[rsp+0x28]
 add rsp,0x30
 mov rsp,rbp
 pop rbp
 ret
