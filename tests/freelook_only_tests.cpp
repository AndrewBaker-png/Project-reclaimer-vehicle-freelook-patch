// Exercise the actual isolated runtime against synthetic native TLS/object data.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static bool keys[256]{},focused=true;
static ULONGLONG clockMs=1000;
static uintptr_t testVector[1]{};
static SHORT testKey(int key){return keys[key]?SHORT(0x8000):0;}
static HWND testWindow(){return (HWND)1;}
static DWORD testWindowPid(HWND,DWORD* pid){*pid=focused?GetCurrentProcessId():0;return 0;}
static unsigned __int64 testGs(unsigned long){return (uintptr_t)testVector;}
static ULONGLONG testClock(){return clockMs;}
#define GetAsyncKeyState testKey
#define GetForegroundWindow testWindow
#define GetWindowThreadProcessId testWindowPid
#define __readgsqword testGs
#define GetTickCount64 testClock
#include "../src/freelook_only.cpp"
static void require(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
template<class T> static void put(void* base,size_t offset,T value){memcpy((char*)base+offset,&value,sizeof value);}
static void axis(float delta){alignas(16) unsigned char fx[512]{};put(fx,160,delta);onInputX(fx);float out;memcpy(&out,fx+160,4);require(out==(active?0:delta),"mouse diversion only during active hold");}
extern "C" {
 void testLiftedInput(uintptr_t image,uintptr_t context);
 uintptr_t testLiftedCamera(uintptr_t context,int user,uintptr_t camera);
 void testLiftedInputSite();
 void testLiftedCameraSite();
}
int main(){
 require(freelook::parseHoldKey(L"",VK_F8)==VK_F8,"empty binding preserves legacy key");
 require(freelook::parseHoldKey(L"f6",0)==VK_F6,"case insensitive function key");
 require(freelook::parseHoldKey(L"F24",0)==VK_F24,"last function key");
 require(freelook::parseHoldKey(L"Mouse4",0)==VK_XBUTTON1,"side mouse button");
 require(freelook::parseHoldKey(L"LeftAlt",0)==VK_LMENU,"left modifier");
 require(freelook::parseHoldKey(L"z",0)=='Z'&&freelook::parseHoldKey(L"0",0)=='0',"letter and digit bindings");
 for(const auto* invalid:{L"F0",L"F25",L"F01",L"Mouse6",L"Ctrl+F8",L"unknown",L"123456789012345678901234567890123456"})
   require(freelook::parseHoldKey(invalid,VK_F8)==-1,"invalid named binding never falls back silently");
 for(int invalid:{-1,0,255,VK_ESCAPE,VK_RETURN,VK_TAB,VK_F10,int('V')})
   require(!freelook::validHoldKey(invalid,false),"invalid/reserved key rejected");
 require(freelook::validHoldKey('B',false)&&!freelook::validHoldKey('B',true),"Reclaimer voice exclusion retained");
 wchar_t tempPath[MAX_PATH],iniPath[MAX_PATH];
 require(GetTempPathW(MAX_PATH,tempPath)&&GetTempFileNameW(tempPath,L"flb",0,iniPath),"temporary binding profile created");
 require(freelook::loadHoldKey(iniPath)==VK_F8,"missing settings default to F8");
 require(WritePrivateProfileStringW(L"Freelook",L"HoldVirtualKey",L"117",iniPath),"write legacy profile");
 require(freelook::loadHoldKey(iniPath)==VK_F6,"existing numeric setting preserved");
 require(WritePrivateProfileStringW(L"Freelook",L"HoldKey",L"Mouse4",iniPath),"write named profile");
 require(freelook::loadHoldKey(iniPath)==VK_XBUTTON1,"named binding overrides legacy setting");
 require(WritePrivateProfileStringW(L"Freelook",L"HoldKey",L"typo",iniPath),"write invalid profile");
 require(!freelook::validHoldKey(freelook::loadHoldKey(iniPath),true),"invalid profile refused");
 require(DeleteFileW(iniPath),"temporary profile removed");
 std::vector<unsigned char> module(0xa3a000),tls(0x600),users(0x20),array(0x60),headers(0x48),unit(0x200),vehicle(0x100),observer(0x100);
 haloBase=(uintptr_t)module.data();put(module.data(),0xa39f9c,uint32_t(0));testVector[0]=(uintptr_t)tls.data();
 put(tls.data(),0x110,(uintptr_t)users.data());put(tls.data(),0x38,(uintptr_t)array.data());put(tls.data(),0x578,(uintptr_t)observer.data());
 put(users.data(),0x14,uint32_t(0x10000));put(array.data(),0x20,uint32_t(3));put(array.data(),0x48,(uintptr_t)headers.data());
 put(headers.data(),0,uint16_t(1));put(headers.data(),3,uint8_t(0));put(headers.data(),0x10,(uintptr_t)unit.data());
 put(headers.data(),0x18,uint16_t(2));put(headers.data(),0x1b,uint8_t(1));put(headers.data(),0x28,(uintptr_t)vehicle.data());
 put(unit.data(),0xf4,1.f);put(unit.data(),0x10,uint32_t(0x20001));
 put(observer.data(),0x1c,freelook::Vec3{0,0,0});holdKey=VK_F8;enabled=1;smoothingSeconds=0;
 QueryPerformanceFrequency(&perfFrequency);
 freelook::CameraResultCopy original{};memset(original.bytes,0x35,sizeof original.bytes);
 put(original.bytes,0,freelook::Vec3{0,-8,3});put(original.bytes,0x28,freelook::Vec3{0,1,0});put(original.bytes,0x34,freelook::Vec3{0,0,1});
 auto camera=original;
 auto changed=[&](){applyCamera((uintptr_t)&camera);require(memcmp(&camera,&original,sizeof camera)!=0,"held camera orbits");for(size_t i=0;i<sizeof camera;i++)if(!(i<12||(i>=0x28&&i<0x40)))require(camera.bytes[i]==original.bytes[i],"FOV and non-pose bytes preserved");};
 auto restored=[&](){applyCamera((uintptr_t)&camera);require(memcmp(&camera,&original,sizeof camera)==0,"native pose restored");};
 axis(.2f);require(!active,"F8 released: pass through");keys[VK_F8]=true;axis(.2f);changed();
 keys[VK_F8]=false;axis(.1f);clockMs+=1000;restored();require(!active&&!returning,"release finishes recenter");
 holdKey=freelook::parseHoldKey(L"Mouse4",0);
 keys[VK_F8]=true;axis(.2f);require(!active,"old key no longer activates legacy runtime");keys[VK_F8]=false;
 keys[VK_XBUTTON1]=true;axis(.2f);changed();
 keys[VK_XBUTTON1]=false;axis(.1f);clockMs+=1000;restored();require(!active,"rebound mouse button releases legacy runtime");
 holdKey=VK_F8;
 keys[VK_F8]=true;axis(.2f);changed();put(unit.data(),0x10,uint32_t(0xffffffff));restored();require(!active&&!returning,"vehicle exit cancels orbit");
 put(unit.data(),0x10,uint32_t(0x20001));axis(.2f);changed();put(unit.data(),0xf4,0.f);restored();require(!active,"death cancels orbit");
 put(unit.data(),0xf4,1.f);axis(.2f);changed();focused=false;restored();require(!active,"focus loss cancels orbit");focused=true;
 axis(.2f);changed();keys[VK_ESCAPE]=true;axis(.1f);require(lockout&&!active,"menu locks out held F8");keys[VK_ESCAPE]=false;axis(.1f);require(!active,"lockout persists until release");keys[VK_F8]=false;axis(.1f);require(!lockout,"release clears menu lockout");
 keys[VK_F8]=true;axis(.2f);changed();put(headers.data(),0x18,uint16_t(3));put(unit.data(),0x10,uint32_t(0x30001));restored();require(!active,"full vehicle identity change cancels orbit");
 axis(.2f);freelook::CameraResultCopy copies[128];
 for(auto& copy:copies){copy=original;applyCamera((uintptr_t)&copy);require(memcmp(&copy,&original,sizeof copy)!=0,"camera cache survives repeated new addresses");}
 auto heldPose=copies[0];
 for(int pass=0;pass<3;pass++)for(auto& copy:copies){
   applyCamera((uintptr_t)&copy);
   require(memcmp(&copy,&heldPose,sizeof copy)==0,"revisiting older cameras never compounds the orbit");
 }
 // Every outstanding buffer must restore, including those visited after the
 // first callback has cleared the global returning flag.
 keys[VK_F8]=false;axis(.1f);clockMs+=1000;
 for(auto& copy:copies){applyCamera((uintptr_t)&copy);require(memcmp(&copy,&original,sizeof copy)==0,"all cached cameras restore after release");}
 require(!active&&!returning,"multi-camera release leaves runtime idle");
 keys[VK_F8]=true;axis(.2f);
 for(auto& copy:copies){applyCamera((uintptr_t)&copy);}
 // A native update takes ownership back; release must not restore a stale pose
 // over it, even when it arrives after another camera completed recenter.
 auto freshNative=original;put(freshNative.bytes,0,freelook::Vec3{9,-4,2});
 copies[127]=freshNative;
 keys[VK_F8]=false;axis(.1f);clockMs+=1000;
 for(auto& copy:copies){applyCamera((uintptr_t)&copy);}
 require(memcmp(&copies[127],&freshNative,sizeof freshNative)==0,"release preserves newer native camera ownership");
 // Cleared slots may be reused; dormant pointers must never be dereferenced.
 auto* transient=(freelook::CameraResultCopy*)VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 require(transient!=nullptr,"transient camera allocated");*transient=original;
 keys[VK_F8]=true;axis(.2f);applyCamera((uintptr_t)transient);
 DWORD old;require(VirtualProtect(transient,4096,PAGE_NOACCESS,&old)!=0,"retired camera made inaccessible");
 camera=original;applyCamera((uintptr_t)&camera);
 keys[VK_F8]=false;axis(.1f);clockMs+=1000;restored();
 require(VirtualProtect(transient,4096,PAGE_READWRITE,&old)!=0,"retired camera becomes valid again");
 applyCamera((uintptr_t)transient);require(memcmp(transient,&original,sizeof original)==0,"delayed camera restores only on its own valid callback");
 VirtualFree(transient,0,MEM_RELEASE);
 require(sizeof verifiedHookSites/sizeof verifiedHookSites[0]==3,"only input/camera sites");
 puts("PASS: F8 input/orbit/FOV isolation, recenter, exit/death/focus/menu/identity, 128 revisited cameras, complete restoration, native ownership and retired camera lifetime");

 module.resize(freelook::liftedFlightY+sizeof(float));
 haloBase=(uintptr_t)module.data();
 liftedEngine={1,haloBase,0};
 unsigned char context[0x2e0]{};
 const auto contextAddress=(uintptr_t)context;
 const DWORD slot=TlsAlloc();
 require(slot!=TLS_OUT_OF_INDEXES,"native context TLS slot allocated");
 unsigned char key[0x20]{};
 put(key,0x18,slot+1);
 const auto keyAddress=(uintptr_t)key;
 liftedEngine.host=(uintptr_t)&keyAddress-freelook::nativeContextKey;
 require(TlsSetValue(slot,const_cast<uintptr_t*>(&contextAddress)),"native context attached to test thread");
 auto& x=*reinterpret_cast<float*>(haloBase+freelook::liftedInputX);
 auto& y=*reinterpret_cast<float*>(haloBase+freelook::liftedInputY);
 auto& flightY=*reinterpret_cast<float*>(haloBase+freelook::liftedFlightY);
 x=.2f;y=.1f;flightY=-.1f;
 keys[VK_F8]=true;
 onLiftedInput(haloBase,0);
 require(x==.2f&&y==.1f&&!active,"uninitialized guest TLS leaves steering intact");
 put(context,0x2d8,(uintptr_t)testVector);
 onLiftedInput(haloBase,0x518);
 require(x==.2f&&y==.1f&&flightY==-.1f&&!active,"other local users keep their steering");
 onLiftedInput(haloBase,0);
 require(x==0&&y==0&&flightY==0&&active,"native input consumes both axes and flight pitch while held");
 onLiftedCamera(contextAddress,0,1,(uintptr_t)&camera);
 require(memcmp(&camera,&original,sizeof camera)==0,"lifted camera only edits local user zero");
 onLiftedCamera(contextAddress,0,0,(uintptr_t)&camera);
 require(memcmp(&camera,&original,sizeof camera)!=0,"lifted camera uses guest TLS to orbit");
 keys[VK_F8]=false;
 x=.2f;y=.1f;
 onLiftedInput(haloBase,0);
 clockMs+=1000;
 onLiftedCamera(contextAddress,0,0,(uintptr_t)&camera);
 require(x==.2f&&y==.1f&&memcmp(&camera,&original,sizeof camera)==0,"lifted release restores steering and camera");
 puts("PASS: lifted engine guest TLS, both axes, camera ownership and release");
 holdKey=freelook::parseHoldKey(L"LeftAlt",0);
 keys[VK_F8]=true;x=.2f;y=.1f;onLiftedInput(haloBase,0);
 require(!active&&x==.2f&&y==.1f,"old key leaves lifted steering intact");keys[VK_F8]=false;
 keys[VK_LMENU]=true;onLiftedInput(haloBase,0);
 require(active&&x==0&&y==0,"rebound modifier activates lifted runtime");
 keys[VK_LMENU]=false;x=.2f;y=.1f;onLiftedInput(haloBase,0);clockMs+=1000;
 onLiftedCamera(contextAddress,0,0,(uintptr_t)&camera);
 require(!active&&x==.2f&&y==.1f&&memcmp(&camera,&original,sizeof camera)==0,"rebound lifted release restores camera and steering");
 holdKey=VK_F8;
 puts("PASS: named and legacy binding profiles, invalid/reserved keys, rebound legacy/lifted hold and release");
 // Huge Grass uses a biped for the Mega Mantis and a giant for the Scarab.
 unsigned char tagMeta[0x20]{},tagTable[8]{},definition[0x260]{};
 put(module.data(),0x20ad058,(uintptr_t)tagMeta);
 put(module.data(),0x1fcf4c8,(uintptr_t)definition-4);
 put(tagMeta,0x10,uint32_t(1));
 put(tagMeta,0x18,(uintptr_t)tagTable);
 put(tagTable,2,uint16_t(0x1234));
 put(tagTable,4,uint32_t(1));
 put(vehicle.data(),0,uint32_t(0x12340000));
 put(headers.data(),0x1b,ObjectType::Biped);
 keys[VK_F8]=true;x=.2f;y=.1f;
 onLiftedInput(haloBase,0);
 require(!active&&x==.2f&&y==.1f,"parent biped without seats leaves input untouched");
 put(definition,0x25c,uint32_t(100));
 for(auto type:{ObjectType::Biped,ObjectType::Giant}){
   put(headers.data(),0x1b,type);
   put(definition,0x258,type==ObjectType::Biped?uint32_t(2):uint32_t(1));
   keys[VK_F8]=true;x=.2f;y=.1f;
   onLiftedInput(haloBase,0);
   require(active&&x==0&&y==0,"parent unit with seats consumes input");
   onLiftedCamera(contextAddress,0,0,(uintptr_t)&camera);
   require(memcmp(&camera,&original,sizeof camera)!=0,"seated unit camera orbits");
   keys[VK_F8]=false;
   onLiftedInput(haloBase,0);
   clockMs+=1000;
   onLiftedCamera(contextAddress,0,0,(uintptr_t)&camera);
   require(memcmp(&camera,&original,sizeof camera)==0,"seated unit camera recenters");
 }
 put(headers.data(),0x1b,ObjectType::Vehicle);
 puts("PASS: Mega Mantis and Scarab seat eligibility, orbit and recenter, ordinary biped passthrough");
 require(freelook::matchRemote(GetCurrentProcess(),(uintptr_t)testLiftedInputSite,freelook::nativeInputEndBytes),"input fixture matches verified native bridge instructions");
 require(freelook::matchRemote(GetCurrentProcess(),(uintptr_t)testLiftedCameraSite,freelook::cameraEndBytes),"camera fixture matches verified lifted instructions");
 require(MH_Initialize()==MH_OK,"lifted fixture hooks initialized");
 require(MH_CreateHook((void*)testLiftedInputSite,(void*)liftedInputHook,&liftedInputTrampoline)==MH_OK,"lifted input hook created");
 require(MH_CreateHook((void*)testLiftedCameraSite,(void*)liftedCameraHook,&liftedCameraTrampoline)==MH_OK,"lifted camera hook created");
 require(MH_EnableHook(MH_ALL_HOOKS)==MH_OK,"lifted fixture hooks enabled");
 keys[VK_F8]=true;x=.2f;y=.1f;
 testLiftedInput(haloBase,0);
 require(x==0&&y==0&&active,"input assembly extracts image and user offset from saved registers");
 require(testLiftedCamera(contextAddress,0,(uintptr_t)&camera)==(uintptr_t)&camera,"lifted camera hook preserves return value");
 require(memcmp(&camera,&original,sizeof camera)!=0,"lifted assembly extracts user, camera and context");
 keys[VK_F8]=false;
 testLiftedInput(haloBase,0);
 clockMs+=1000;
 testLiftedCamera(contextAddress,0,(uintptr_t)&camera);
 require(memcmp(&camera,&original,sizeof camera)==0,"lifted assembly release restores camera");
 require(MH_DisableHook(MH_ALL_HOOKS)==MH_OK&&MH_Uninitialize()==MH_OK,"lifted fixture hooks stopped");
 require(TlsFree(slot),"native context TLS slot released");
 puts("PASS: actual lifted input/camera assembly hooks, register arguments and return preservation");
}
