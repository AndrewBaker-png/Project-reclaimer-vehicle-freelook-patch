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
.global exerciseCameraResult
exerciseCameraResult:
 sub rsp,40
 xor ecx,ecx
 mov r11d,1
 mov eax,0x3f800000
 movd xmm3,eax
 mov eax,0x40000000
 movd xmm4,eax
 mov eax,0x40400000
 movd xmm5,eax
 call cameraResultHook
 mov [rip+cameraReturned],rax
 mov [rip+cameraR11],r11
 movd [rip+cameraXmm3],xmm3
 movd [rip+cameraXmm4],xmm4
 movd [rip+cameraXmm5],xmm5
 add rsp,40
 ret
.global syntheticCameraResult
syntheticCameraResult:
 mov eax,0x12345678
 ret
.global onCameraResult
onCameraResult:
 mov [rip+cameraCallbackPointer],rcx
 mov [rip+cameraCallbackUser],edx
 # Deliberately destroy registers that a normal C++ callback may clobber.
 mov r11d,0xf739634a
 xorps xmm3,xmm3
 xorps xmm4,xmm4
 xorps xmm5,xmm5
 xor eax,eax
 ret
.global syntheticInputSite
syntheticInputSite:
 movss [rip+testStored],xmm0
 mov [rip+testRcx],rcx
 pushfq
 pop rax
 mov [rip+testFlags],rax
 ret
