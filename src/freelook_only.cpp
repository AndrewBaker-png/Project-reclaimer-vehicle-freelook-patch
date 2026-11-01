// Isolated F8-only runtime. Build with FREELOOK_ONLY; no renderer or cockpit modules.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <share.h>
#include <intrin.h>
#include <tlhelp32.h>
#include <new>
#include "MinHook.h"
#include "freelook_math.h"
#include "camera_copy.h"
#include "verified_build.h"
#include "runtime_control.h"
#ifndef FREELOOK_ONLY
#error Build this isolated runtime with FREELOOK_ONLY.
#endif
extern "C" { void inputXHook();void inputYHook();void cameraHook();void cameraResultHook();void* inputXTrampoline=nullptr;void* inputYTrampoline=nullptr;void* cameraTrampoline=nullptr; }
static uintptr_t haloBase=0;
static SRWLOCK stateLock=SRWLOCK_INIT;
static freelook::Look look;
static freelook::Look displayed,returnFrom;
static bool returning=false;
static ULONGLONG returnAt=0,displayAt=0;
static float smoothingSeconds=0.04f,returnSeconds=0.18f;
static uint32_t heldVehicle=0xffffffff;
static bool active=false,lockout=false;
static int holdKey=0;
static float sensitivity=1.f;
static bool reclaimer=false;
static bool invertVertical=true;
static DWORD uiParent=0;
static LONG denyReason=0;
static LONG64 heldChecks=0,cameraWrites=0;
static LONG64 nonzeroX=0,nonzeroY=0;
static float maxX=0,maxY=0;
static LONG64 cameraPerfTicks=0,cameraPerfCalls=0;
static LARGE_INTEGER perfFrequency;
static HANDLE stopEvent=nullptr;
static HANDLE stoppedEvent=nullptr;
static bool hooksMayBeEnabled=false;
static FILE* logFile=nullptr;
static HMODULE selfModule=nullptr;
static LONG enabled=0;
static LONG64 mouseFrames=0,cameraFrames=0,activeFrames=0;
// These references are resolved from the current engine thread's TLS during its
// own callback, not from guessed process-wide addresses. Engine ownership keeps
// the snapshot alive until the callback returns. Never query process memory here.
template<class T> static bool read(uintptr_t p,T& value){if(!p)return false;memcpy(&value,(void*)p,sizeof value);return true;}
static bool down(int key){return (GetAsyncKeyState(key)&0x8000)!=0;}
static void log(const char* text){if(logFile){fprintf(logFile,"%llu pid=%lu %s\n",GetTickCount64(),GetCurrentProcessId(),text);fflush(logFile);}}
static uintptr_t tlsSlot(size_t offset){
  uint32_t index=0;uintptr_t block=0,value=0;
  if(!read(haloBase+0xa39f9c,index)||index>1023)return 0;
  uintptr_t vector=__readgsqword(0x58);
  if(!read(vector+index*8,block)||!read(block+offset,value))return 0;
  return value;
}
static uintptr_t object(uint32_t datum,uint8_t type){
  if(datum==0xffffffff)return 0;
  uintptr_t array=tlsSlot(0x38),table=0;uint32_t maximum=0;
  if(!array||!read(array+0x48,table)||!read(array+0x20,maximum)||(datum&0xffff)>=maximum)return 0;
  uintptr_t header=table+uintptr_t(datum&0xffff)*0x18;
  uint16_t salt=0;uint8_t actual=255;uintptr_t address=0;
  if(!read(header,salt)||salt!=(datum>>16)||!read(header+3,actual)||actual!=type||!read(header+0x10,address))return 0;
  return address;
}
static bool localVehicle(uint32_t& vehicle,uintptr_t* pilot=nullptr){
  // Native input-user zero -> unit. Resolve on the calling engine thread so
  // simulation and render snapshots each use their own valid object table.
  uintptr_t users=tlsSlot(0x110);uint32_t datum=0xffffffff;
  if(!users||!read(users+0x14,datum)){denyReason=5;return false;}
  uintptr_t biped=object(datum,0);float health=0;
  if(!biped){denyReason=7;return false;}
  if(!read(biped+0xf4,health)||!std::isfinite(health)||health<=0){denyReason=8;return false;}
  if(!read(biped+0x10,vehicle)||!object(vehicle,1)){denyReason=10;return false;}
  if(pilot)*pilot=biped;
  denyReason=0;
  return true;
}
static bool foregroundGame(){DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);return foreground==GetCurrentProcessId()||(reclaimer&&foreground==uiParent);}
static bool eligible(uint32_t& vehicle){
  if(!InterlockedCompareExchange(&enabled,0,0)||holdKey==0||!down(holdKey)){denyReason=2;return false;}
  InterlockedIncrement64(&heldChecks);
  if(!foregroundGame()){denyReason=3;return false;}
  if(down(VK_ESCAPE)||down(VK_RETURN)||down(VK_TAB)){lockout=true;denyReason=4;return false;}
  if(lockout){denyReason=4;return false;}
  return localVehicle(vehicle);
}
static void reset(){if(active){char message[220];snprintf(message,sizeof message,"released yaw=%g pitch=%g nonzeroX=%lld nonzeroY=%lld maxX=%g maxY=%g",look.yaw,look.pitch,nonzeroX,nonzeroY,maxX,maxY);log(message);returnFrom=displayed;returnAt=GetTickCount64();returning=true;}active=false;}
static void onAxis(void* savedFx,bool horizontal){
  InterlockedIncrement64(&mouseFrames);
  AcquireSRWLockExclusive(&stateLock);

  if(!down(holdKey))lockout=false;
  uint32_t vehicle;
  if(!eligible(vehicle)){reset();ReleaseSRWLockExclusive(&stateLock);return;}
  if(!active||heldVehicle!=vehicle){if(heldVehicle!=vehicle){look={};displayed={};}else if(returning){look=displayed;}heldVehicle=vehicle;active=true;returning=false;displayAt=GetTickCount64();log("hold: vehicle orbit enabled");}
  // FXSAVE64 stores XMM0 at +160, with 16 bytes per XMM register.
  float delta=0;auto* fx=(unsigned char*)savedFx;
  memcpy(&delta,fx+160,4);
  if(horizontal){if(delta!=0)nonzeroX++;if(std::fabs(delta)>maxX)maxX=std::fabs(delta);}
  else {if(delta!=0)nonzeroY++;if(std::fabs(delta)>maxY)maxY=std::fabs(delta);}
  if(std::isfinite(delta))look.move(horizontal?delta:0,horizontal?0:delta,sensitivity,invertVertical);
  float zero=0;memcpy(fx+160,&zero,4);
  InterlockedIncrement64(&activeFrames);
  ReleaseSRWLockExclusive(&stateLock);
}
extern "C" void onInputX(void* savedFx){onAxis(savedFx,true);}
extern "C" void onInputY(void* savedFx){onAxis(savedFx,false);}
struct CameraCache {uintptr_t address=0;freelook::Vec3 basePosition{},baseForward{},baseUp{},writtenPosition{},writtenForward{},writtenUp{};bool modified=false;};
struct CameraCacheBlock {CameraCache entries[8];CameraCacheBlock* next=nullptr;};
static CameraCacheBlock firstCameraBlock;
// Only inspect an incoming, engine-owned camera. Never dereference old addresses
// to reclaim entries: an outstanding pose may belong to another thread/snapshot.
static CameraCache* cameraCache(uintptr_t address,bool create){
  CameraCache* reusable=nullptr;
  CameraCacheBlock* last=&firstCameraBlock;
  for(auto* block=&firstCameraBlock;block;block=block->next){
    last=block;
    for(auto& entry:block->entries){
      if(entry.address==address)return &entry;
      if(!entry.modified&&!reusable)reusable=&entry;
    }
  }
  if(!create)return nullptr;
  if(!reusable){
    auto* block=new(std::nothrow) CameraCacheBlock{};
    if(!block)return nullptr; // Leave the native pose untouched on allocation failure.
    last->next=block;reusable=&block->entries[0];
  }
  *reusable={};reusable->address=address;return reusable;
}
static bool same(freelook::Vec3 a,freelook::Vec3 b){return memcmp(&a,&b,sizeof a)==0;}
static uintptr_t applyCamera(uintptr_t camera){
  InterlockedIncrement64(&cameraFrames);
  if(!camera)return camera;
  AcquireSRWLockExclusive(&stateLock);
  CameraCache* cache=cameraCache(camera,false);
  bool idle=!down(holdKey)&&!active&&!returning;
  if(idle&&(!cache||!cache->modified)){ReleaseSRWLockExclusive(&stateLock);return camera;}
  LARGE_INTEGER perfStart;QueryPerformanceCounter(&perfStart);
  if(!cache)cache=cameraCache(camera,true);
  if(!cache){ReleaseSRWLockExclusive(&stateLock);return camera;}
  freelook::Vec3 position,forward,up;
  if(!read(camera,position)||!read(camera+0x28,forward)||!read(camera+0x34,up)){ReleaseSRWLockExclusive(&stateLock);return camera;}
  bool ours=cache->modified&&same(position,cache->writtenPosition)&&same(forward,cache->writtenForward)&&same(up,cache->writtenUp);
  if(!ours)cache->modified=false;
  if(idle){
    if(ours){memcpy((void*)camera,&cache->basePosition,12);memcpy((void*)(camera+0x28),&cache->baseForward,12);memcpy((void*)(camera+0x34),&cache->baseUp,12);}
    cache->modified=false;
    ReleaseSRWLockExclusive(&stateLock);return camera;
  }
  // Cancel the orbit across focus loss, death, exit or a vehicle identity change.
  // Restore only a camera pose that is still exactly our own last output.
  uint32_t currentVehicle=0xffffffff;
  if(!foregroundGame()||!localVehicle(currentVehicle)||currentVehicle!=heldVehicle){
    if(ours){memcpy((void*)camera,&cache->basePosition,12);memcpy((void*)(camera+0x28),&cache->baseForward,12);memcpy((void*)(camera+0x34),&cache->baseUp,12);}
    cache->modified=false;active=false;returning=false;look={};displayed={};heldVehicle=0xffffffff;
    ReleaseSRWLockExclusive(&stateLock);return camera;
  }
  uint32_t vehicle;
  if(!down(holdKey))lockout=false;
  bool held=eligible(vehicle)&&active&&vehicle==heldVehicle;
  if(!held)reset();
  ULONGLONG now=GetTickCount64();
  if(returning){
    float t=freelook::clamp((now-returnAt)/(returnSeconds*1000.f),0.f,1.f),remaining=1.f-t*t*(3.f-2.f*t);
    displayed={returnFrom.yaw*remaining,returnFrom.pitch*remaining};
    if(t>=1){returning=false;displayed={};look={};}
  }else if(held){
    float dt=displayAt?(now-displayAt)/1000.f:0.f;displayAt=now;
    displayed=freelook::smooth(displayed,look,dt,smoothingSeconds);
  }
  if(!held&&!returning){
    if(ours){memcpy((void*)camera,&cache->basePosition,12);memcpy((void*)(camera+0x28),&cache->baseForward,12);memcpy((void*)(camera+0x34),&cache->baseUp,12);}
    cache->modified=false;
    ReleaseSRWLockExclusive(&stateLock);return camera;
  }
  freelook::Vec3 pivot;uintptr_t observer=tlsSlot(0x578);
  if(!observer||!read(observer+0x1c,pivot)){ReleaseSRWLockExclusive(&stateLock);return camera;}
  freelook::Look offset=displayed;
  freelook::CameraResultCopy baseline,copy;
  memcpy(baseline.bytes,(void*)camera,sizeof baseline.bytes);
  if(ours){position=cache->basePosition;forward=cache->baseForward;up=cache->baseUp;memcpy(baseline.bytes,&position,12);memcpy(baseline.bytes+0x28,&forward,12);memcpy(baseline.bytes+0x34,&up,12);}
  cache->basePosition=position;cache->baseForward=forward;cache->baseUp=up;
  bool transformed=freelook::transformCameraCopy(baseline,pivot,offset,copy);
  if(!transformed){ReleaseSRWLockExclusive(&stateLock);return camera;}
  memcpy((void*)camera,copy.bytes,12);memcpy((void*)(camera+0x28),copy.bytes+0x28,12);memcpy((void*)(camera+0x34),copy.bytes+0x34,12);
  memcpy(&cache->writtenPosition,copy.bytes,12);memcpy(&cache->writtenForward,copy.bytes+0x28,12);memcpy(&cache->writtenUp,copy.bytes+0x34,12);cache->modified=true;
  ReleaseSRWLockExclusive(&stateLock);
  InterlockedIncrement64(&cameraWrites);
  LARGE_INTEGER perfEnd;QueryPerformanceCounter(&perfEnd);
  InterlockedAdd64(&cameraPerfTicks,perfEnd.QuadPart-perfStart.QuadPart);InterlockedIncrement64(&cameraPerfCalls);
  return camera;
}
extern "C" void onCamera(void*){} // retained only for the isolated assembly-stub tests
// Halo's internal caller relies on volatile R11 and XMM3-5 surviving this
// function. A normal C++ detour violates that private calling convention.
// The assembly wrapper runs the original first and preserves its complete
// register state while this callback edits only the local user's camera pose.
extern "C" void onCameraResult(uintptr_t camera,int user){
  if(user==0)applyCamera(camera);
}

static bool checkBytes(uintptr_t address,const unsigned char* expected,size_t size){unsigned char bytes[32];return size<=sizeof bytes&&ReadProcessMemory(GetCurrentProcess(),(void*)address,bytes,size,nullptr)&&memcmp(bytes,expected,size)==0;}
static DWORD runWorker(){
  QueryPerformanceFrequency(&perfFrequency);
  wchar_t path[MAX_PATH];GetModuleFileNameW(selfModule,path,MAX_PATH);
  wchar_t* slash=wcsrchr(path,L'\\');if(slash)slash[1]=0;
  wchar_t ini[MAX_PATH],logPath[MAX_PATH];swprintf(ini,MAX_PATH,L"%lsfreelook.ini",path);swprintf(logPath,MAX_PATH,L"%lsfreelook.log",path);
  logFile=_wfsopen(logPath,L"a",_SH_DENYNO);log("freelook-only MP test revision 30 (audit fixes) starting");
  reclaimer=wcsstr(GetCommandLineW(),L"probe-engine-worker")!=nullptr;
  if(reclaimer){HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);PROCESSENTRY32W pe={sizeof pe};if(Process32FirstW(snapshot,&pe))do{if(pe.th32ProcessID==GetCurrentProcessId()){uiParent=pe.th32ParentProcessID;break;}}while(Process32NextW(snapshot,&pe));CloseHandle(snapshot);}
  if(!reclaimer&&!wcsstr(GetCommandLineW(),L"-no-eac")&&!wcsstr(GetCommandLineW(),L"-eac-nop-loaded")){log("REFUSED: use MCC Anti-Cheat Disabled launch option");return 1;}
  holdKey=GetPrivateProfileIntW(L"Freelook",L"HoldVirtualKey",119,ini);
  invertVertical=GetPrivateProfileIntW(L"Freelook",L"InvertVertical",1,ini)!=0;
  sensitivity=freelook::clamp(GetPrivateProfileIntW(L"Freelook",L"SensitivityMilli",1000,ini)/1000.f,.01f,10.f);
  smoothingSeconds=freelook::clamp(GetPrivateProfileIntW(L"Freelook",L"SmoothingMs",40,ini)/1000.f,0.f,.3f);
  returnSeconds=freelook::clamp(GetPrivateProfileIntW(L"Freelook",L"ReturnMs",180,ini)/1000.f,.01f,1.f);
  if(holdKey<1||holdKey>254||holdKey==86||(reclaimer&&holdKey==66)){log("REFUSED: invalid hold key or voice-chat conflict");return 2;}
  while(!(haloBase=(uintptr_t)GetModuleHandleW(L"halo3.dll"))){if(WaitForSingleObject(stopEvent,100)==WAIT_OBJECT_0)return 0;}
  if(WaitForSingleObject(stopEvent,0)==WAIT_OBJECT_0)return 0;
  static_assert(sizeof verifiedHookSites/sizeof verifiedHookSites[0]==3);
  for(const auto& site:verifiedHookSites)if(!checkBytes(haloBase+site.rva,site.bytes,16)){log("REFUSED: unsupported or modified hook sites");return 4;}
  if(MH_Initialize()!=MH_OK)return 5;
  struct Callback{void* hook;void** trampoline;};
  const Callback callbacks[]={{(void*)inputXHook,&inputXTrampoline},{(void*)inputYHook,&inputYTrampoline},{(void*)cameraResultHook,&cameraTrampoline}};
  static_assert(sizeof callbacks/sizeof callbacks[0]==sizeof verifiedHookSites/sizeof verifiedHookSites[0]);
  for(size_t i=0;i<3;i++)if(MH_CreateHook((void*)(haloBase+verifiedHookSites[i].rva),callbacks[i].hook,callbacks[i].trampoline)!=MH_OK){log("hook creation failed");MH_Uninitialize();return 6;}
  if(WaitForSingleObject(stopEvent,0)==WAIT_OBJECT_0){MH_Uninitialize();return 0;}
  InterlockedExchange(&enabled,1);
  for(const auto& site:verifiedHookSites)MH_QueueEnableHook((void*)(haloBase+site.rva));
  hooksMayBeEnabled=true;
  if(MH_ApplyQueued()!=MH_OK){InterlockedExchange(&enabled,0);hooksMayBeEnabled=MH_DisableHook(MH_ALL_HOOKS)!=MH_OK;log("hook activation failed");return 7;}
  log("ACTIVE: exactly 3 input/camera hooks; F8 orbit only; no F7/F9 controls, cockpit, visor, HUD, graphics hooks or asset loading");
  ULONGLONG lastStatus=GetTickCount64();
  while(WaitForSingleObject(stopEvent,33)==WAIT_TIMEOUT){
    if((uintptr_t)GetModuleHandleW(L"halo3.dll")!=haloBase)break;
    if(down(VK_F10)&&down(VK_CONTROL)&&down(VK_SHIFT))break;
    auto now=GetTickCount64();if(now-lastStatus>=5000){lastStatus=now;char text[300];snprintf(text,sizeof text,"input=%lld camera=%lld diverted=%lld f8checks=%lld cameraWrites=%lld deny=%ld",mouseFrames,cameraFrames,activeFrames,heldChecks,cameraWrites,denyReason);log(text);}
  }
  InterlockedExchange(&enabled,0);
  // Retain trampolines/module until process exit: native callbacks can be in flight.
  bool disabled=true;
  if((uintptr_t)GetModuleHandleW(L"halo3.dll")==haloBase)for(const auto& site:verifiedHookSites){
    auto status=MH_DisableHook((void*)(haloBase+site.rva));
    if(status!=MH_OK&&status!=MH_ERROR_DISABLED)disabled=false;
  }
  hooksMayBeEnabled=!disabled;
  if(!disabled){log("ERROR: could not confirm every hook was disabled; restart client");return 8;}
  log("freelook-only stopped; restart client before loading again");
  // Log remains open until process exit so an already-running callback is safe.
  return 0;
}
static DWORD WINAPI worker(void*){
  DWORD result=runWorker();
  if(!hooksMayBeEnabled)SetEvent(stoppedEvent);
  return result;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID){
  if(reason==DLL_PROCESS_ATTACH){
    selfModule=module;DisableThreadLibraryCalls(module);
    // Publish stop handles synchronously: a successful LoadLibrary result must
    // never precede the runtime becoming stoppable. Do not wait in DllMain.
    wchar_t name[100];
    swprintf(name,100,freelook::stopEventPattern,GetCurrentProcessId());
    stopEvent=CreateEventW(nullptr,TRUE,FALSE,name);
    swprintf(name,100,freelook::stoppedEventPattern,GetCurrentProcessId());
    stoppedEvent=CreateEventW(nullptr,TRUE,FALSE,name);
    if(!stopEvent||!stoppedEvent){if(stopEvent)CloseHandle(stopEvent);if(stoppedEvent)CloseHandle(stoppedEvent);return FALSE;}
    HANDLE t=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
    if(!t){CloseHandle(stopEvent);CloseHandle(stoppedEvent);return FALSE;}
    CloseHandle(t);
  }
  return TRUE;
}
