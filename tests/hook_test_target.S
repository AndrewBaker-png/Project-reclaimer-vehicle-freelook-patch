.intel_syntax noprefix
.text
.global exerciseInputHook
exerciseInputHook:
 sub rsp,40
 mov rcx,0x1122334455667788
 mov eax,0x3f800000
 movd xmm0,eax
 stc
 call syntheticInputSite
 add rsp,40
 ret
.global syntheticInputSite
syntheticInputSite:
 movss [rip+testStored],xmm0
 mov [rip+testRcx],rcx
 pushfq
 pop rax
 mov [rip+testFlags],rax
 ret
