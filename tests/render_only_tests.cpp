// Exercise the actual 0.10.3 detour with isolated engine/render state.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static bool keys[256]{},focused=true;
static ULONGLONG clockMs=1000;
static SHORT testKey(int key){return keys[key]?SHORT(0x8000):0;}
static HWND testWindow(){return (HWND)1;}
static DWORD testWindowPid(HWND,DWORD* pid){*pid=focused?GetCurrentProcessId():0;return 0;}
static ULONGLONG testClock(){return clockMs;}
#define GetAsyncKeyState testKey
#define GetForegroundWindow testWindow
#define GetWindowThreadProcessId testWindowPid
#define GetTickCount64 testClock
static uintptr_t testTlsVector[1];
#define __readgsqword(offset) ((uintptr_t)testTlsVector)
#include "../src/freelook_only.cpp"
static void require(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
template<class T>static void put(void* base,size_t offset,T value){memcpy((char*)base+offset,&value,sizeof value);}
static unsigned bridgeCalls=0;
extern "C" void testLiftedInput(uintptr_t,uintptr_t),testLiftedInputSite();
extern "C" void testLiftedInput0104(uintptr_t,uintptr_t),testLiftedInput0104Site();
extern "C" __declspec(noinline) void renderBridgeFixture(uintptr_t context){
  uintptr_t output=0,source=0;
  memcpy(&output,(void*)(context+8),8);memcpy(&source,(void*)(context+0x10),8);
  memset((void*)output,0x35,0x100);
  memcpy((void*)output,(void*)source,12);
  memcpy((void*)(output+12),(void*)(source+0x28),24);
  ++bridgeCalls;
}
extern "C" void testLiftedInput0108(uintptr_t,uintptr_t),testLiftedInput0108Site();
int main(int argc,char** argv){
  std::vector<unsigned char> module(freelook::liftedFlightY+4),tls(0x600),users(0x20),array(0x60),headers(0x48),unit(0x200),vehicle(0x100),observer(4*0x3d0);
  uintptr_t vector[1]={(uintptr_t)tls.data()};unsigned char context[0x2e0]{};
  haloBase=(uintptr_t)module.data();liftedEngine={1,haloBase,0};liftedEngine.update0102=true;liftedEngine.update0103=argc==1;liftedEngine.update0104=argc>1 && strcmp(argv[1],"--0104")==0;liftedEngine.update0108=argc>1 && strcmp(argv[1],"--0108")==0;
  put(module.data(),0xa39f9c,uint32_t(0));put(context,0x2d8,(uintptr_t)vector);
  put(tls.data(),0x110,(uintptr_t)users.data());put(tls.data(),0x38,(uintptr_t)array.data());put(tls.data(),0x578,(uintptr_t)observer.data());
  put(users.data(),0x14,uint32_t(0x10000));put(array.data(),0x20,uint32_t(3));put(array.data(),0x48,(uintptr_t)headers.data());
  put(headers.data(),0,uint16_t(1));put(headers.data(),3,uint8_t(0));put(headers.data(),0x10,(uintptr_t)unit.data());
  put(headers.data(),0x18,uint16_t(2));put(headers.data(),0x1b,uint8_t(1));put(headers.data(),0x28,(uintptr_t)vehicle.data());
  put(unit.data(),0xf4,1.f);put(unit.data(),0x10,uint32_t(0x20001));
  put(observer.data(),0x1c,freelook::Vec3{10,20,30});
  auto source=(uintptr_t)observer.data()+0x11c;
  put((void*)source,0,freelook::Vec3{10,12,33});put((void*)source,0x28,freelook::Vec3{0,1,0});put((void*)source,0x34,freelook::Vec3{0,0,1});
  const auto originalObserver=observer,originalUnit=unit,originalVehicle=vehicle,originalUsers=users;
  unsigned char output[0x100];put(context,8,(uintptr_t)output);put(context,0x10,source);
  holdKey=VK_F8;enabled=1;smoothingSeconds=0;keys[VK_F8]=true;
  // The new version uses the native input TLS key, not a guessed render thread.
  DWORD slot=TlsAlloc();require(slot!=TLS_OUT_OF_INDEXES,"test TLS allocated");
  unsigned char key[0x20]{};put(key,0x18,slot+1);const uintptr_t keyAddress=(uintptr_t)key,contextAddress=(uintptr_t)context;
  liftedEngine.host=(uintptr_t)&keyAddress-liftedEngine.contextKey();
  require(TlsSetValue(slot,const_cast<uintptr_t*>(&contextAddress)),"input context assigned");
  auto& x=*(float*)(haloBase+freelook::liftedInputX);auto& y=*(float*)(haloBase+freelook::liftedInputY);auto& flight=*(float*)(haloBase+freelook::liftedFlightY);
  std::vector<unsigned char> staticTls(0xbd18+sizeof(uintptr_t));
  uint32_t moduleIndex=0;
  if(liftedEngine.update0108){
    liftedEngine.host=(uintptr_t)&moduleIndex-0xda53148;
    testTlsVector[0]=(uintptr_t)staticTls.data();
    put(staticTls.data(),0xbd18,contextAddress);
  }
  auto input=liftedEngine.update0108?testLiftedInput0108:liftedEngine.update0104?testLiftedInput0104:testLiftedInput;
  auto inputSite=liftedEngine.update0108?testLiftedInput0108Site:liftedEngine.update0104?testLiftedInput0104Site:testLiftedInputSite;
  require(MH_Initialize()==MH_OK,"MinHook initialized");
  require(MH_CreateHook((void*)inputSite,(void*)liftedInputHook,&liftedInputTrampoline)==MH_OK,"version-specific input epilogue detour created");
  require(MH_EnableHook((void*)inputSite)==MH_OK,"native input detour enabled");
  x=.3f;y=.1f;flight=-.1f;input(haloBase,0);
  require(active&&x==0&&y==0&&flight==0,"selected version diverts steering axes while held");
  renderBridgeFixture(contextAddress);unsigned char baseline[0x100];memcpy(baseline,output,sizeof output);
  require(MH_CreateHook((void*)renderBridgeFixture,(void*)onRenderCamera,&liftedCameraTrampoline)==MH_OK,"native render bridge detour created");
  require(MH_EnableHook((void*)renderBridgeFixture)==MH_OK,"native render bridge detour enabled");
  void(*volatile invoke)(uintptr_t)=renderBridgeFixture;
  for(int i=0;i<128;i++){
    invoke(contextAddress);
    require(memcmp(output,baseline,36)!=0,"render pose changes");
    require(memcmp(output+36,baseline+36,sizeof output-36)==0,"FOV/projection and all non-pose bytes unchanged");
    require(observer==originalObserver&&unit==originalUnit&&vehicle==originalVehicle&&users==originalUsers,"observer/aim/simulation fixture unchanged");
  }
  require(bridgeCalls==129,"native bridge executes exactly once per callback");
  require(!applyRenderCamera(source,source,contextAddress),"observer alias refused");
  require(!applyRenderCamera(source-10,source,contextAddress),"partial observer overlap refused");
  require(!applyRenderCamera((uintptr_t)output,source+0x3d0,contextAddress),"other split-screen user refused");
  require(observer==originalObserver,"alias checks never write observer");
  focused=false;invoke(contextAddress);require(memcmp(output,baseline,sizeof output)==0&&!active,"focus loss immediately returns native output");focused=true;
  x=.3f;input(haloBase,0);invoke(contextAddress);
  keys[VK_ESCAPE]=true;x=.1f;input(haloBase,0);clockMs+=1000;invoke(contextAddress);
  require(lockout&&!active&&memcmp(output,baseline,sizeof output)==0,"menu lockout recenters");keys[VK_ESCAPE]=false;keys[VK_F8]=false;input(haloBase,0);
  keys[VK_F8]=true;x=.3f;input(haloBase,0);invoke(contextAddress);
  keys[VK_F8]=false;x=.2f;y=.1f;flight=-.1f;input(haloBase,0);clockMs+=1000;invoke(contextAddress);
  require(x==.2f&&y==.1f&&flight==-.1f&&memcmp(output,baseline,sizeof output)==0,"release restores normal steering and render pose");
  wchar_t tempDir[MAX_PATH],ini[MAX_PATH];
  require(GetTempPathW(MAX_PATH,tempDir)&&GetTempFileNameW(tempDir,L"flk",0,ini),"temporary settings file");
  keys[VK_F8]=true;x=.3f;input(haloBase,0);require(active,"old binding active before rebind");
  require(WritePrivateProfileStringW(L"Freelook",L"HoldVirtualKey",L"5",ini),"save side button binding");
  keys[VK_XBUTTON1]=true;refreshBinding(ini);
  x=.3f;input(haloBase,0);require(!active&&x==.3f,"held replacement waits for release");
  keys[VK_XBUTTON1]=false;input(haloBase,0);require(!active&&x==.3f,"old held key no longer diverts input");
  keys[VK_XBUTTON1]=true;input(haloBase,0);require(active&&x==0,"side button activates after fresh press");
  require(WritePrivateProfileStringW(L"Freelook",L"HoldVirtualKey",L"119",ini),"restore F8");
  keys[VK_F8]=false;refreshBinding(ini);keys[VK_XBUTTON1]=false;DeleteFileW(ini);
  keys[VK_F8]=true;x=.3f;input(haloBase,0);put(unit.data(),0xf4,0.f);invoke(contextAddress);
  require(!active&&memcmp(output,baseline,sizeof output)==0,"death cancels render orbit");put(unit.data(),0xf4,1.f);
  x=.3f;input(haloBase,0);put(unit.data(),0x10,uint32_t(0xffffffff));invoke(contextAddress);
  require(!active&&memcmp(output,baseline,sizeof output)==0,"vehicle exit cancels render orbit");put(unit.data(),0x10,uint32_t(0x20001));
  x=.3f;input(haloBase,0);enabled=0;invoke(contextAddress);
  require(memcmp(output,baseline,sizeof output)==0,"disabled runtime leaves native output");
  require(observer==originalObserver&&unit==originalUnit&&vehicle==originalVehicle&&users==originalUsers,"all non-render state remains original");
  require(MH_DisableHook(MH_ALL_HOOKS)==MH_OK&&MH_Uninitialize()==MH_OK,"hooks disabled");TlsFree(slot);
  printf("PASS: actual %s input/render detours, 128 frames, observer/aim isolation, alias rejection, FOV preservation, release/menu/focus/death/exit/disable\n",liftedEngine.update0108?"0.10.8":liftedEngine.update0104?"0.10.4":"0.10.3");
}
