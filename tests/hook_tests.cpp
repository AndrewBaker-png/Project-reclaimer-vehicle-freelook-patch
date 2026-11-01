#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "MinHook.h"
extern "C" {
void inputXHook();void syntheticInputSite();void exerciseInputHook();
void* inputXTrampoline=nullptr;void* inputYTrampoline=nullptr;void* cameraTrampoline=nullptr;
float testStored=0;uint64_t testRcx=0,testFlags=0;int calls=0;
void onInputX(void* fx){calls++;float zero=0;memcpy((char*)fx+160,&zero,4);}
void onInputY(void*){}
void onCamera(void*){}
void exerciseCameraResult();void syntheticCameraResult();
uint64_t cameraReturned=0,cameraR11=0,cameraCallbackPointer=0;
uint32_t cameraXmm3=0,cameraXmm4=0,cameraXmm5=0;
int cameraCallbackUser=-1;
}
static void require(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
int main(){
 cameraTrampoline=(void*)syntheticCameraResult;
 for(int i=0;i<10000;i++){
  exerciseCameraResult();
  require(cameraReturned==0x12345678,"camera result preserved");
  require(cameraR11==1,"Halo player loop increment R11 preserved");
  require(cameraXmm3==0x3f800000&&cameraXmm4==0x40000000&&cameraXmm5==0x40400000,"Halo distance registers XMM3-5 preserved");
  require(cameraCallbackPointer==0x12345678&&cameraCallbackUser==0,"camera callback receives original result and user index");
 }
 cameraTrampoline=nullptr;
 exerciseInputHook();require(testStored==1.f,"unhooked input writes original value");
 require(MH_Initialize()==MH_OK,"MinHook initialize");
 require(MH_CreateHook((void*)syntheticInputSite,(void*)inputXHook,&inputXTrampoline)==MH_OK,"create RIP-relative mid-hook");
 require(MH_EnableHook((void*)syntheticInputSite)==MH_OK,"enable mid-hook");
 for(int i=0;i<10000;i++){exerciseInputHook();require(testStored==0.f,"callback replaces only consumed axis");require(testRcx==0x1122334455667788ULL,"volatile GPR preserved");require((testFlags&1)==1,"carry flag preserved");}
 require(calls==10000,"one callback per input");
 require(MH_DisableHook((void*)syntheticInputSite)==MH_OK,"disable hook");exerciseInputHook();require(testStored==1.f,"original behavior restored");
 MH_Uninitialize();puts("PASS: 10,000 native mid-hook calls, RIP-relative relocation, axis replacement, GPR/flags preservation, clean disable");
}
