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
int main(){
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
 keys[VK_F8]=true;axis(.2f);changed();put(unit.data(),0x10,uint32_t(0xffffffff));restored();require(!active&&!returning,"vehicle exit cancels orbit");
 put(unit.data(),0x10,uint32_t(0x20001));axis(.2f);changed();put(unit.data(),0xf4,0.f);restored();require(!active,"death cancels orbit");
 put(unit.data(),0xf4,1.f);axis(.2f);changed();focused=false;restored();require(!active,"focus loss cancels orbit");focused=true;
 axis(.2f);changed();keys[VK_ESCAPE]=true;axis(.1f);require(lockout&&!active,"menu locks out held F8");keys[VK_ESCAPE]=false;axis(.1f);require(!active,"lockout persists until release");keys[VK_F8]=false;axis(.1f);require(!lockout,"release clears menu lockout");
 keys[VK_F8]=true;axis(.2f);changed();put(headers.data(),0x18,uint16_t(3));put(unit.data(),0x10,uint32_t(0x30001));restored();require(!active,"full vehicle identity change cancels orbit");
 axis(.2f);freelook::CameraResultCopy copies[12];for(auto& copy:copies){copy=original;applyCamera((uintptr_t)&copy);require(memcmp(&copy,&original,sizeof copy)!=0,"camera cache survives repeated new addresses");}
 require(sizeof verifiedHookSites/sizeof verifiedHookSites[0]==3,"only input/camera sites");
 puts("PASS: actual F8 runtime input diversion, orbit/FOV isolation, recenter, exit, death, focus loss, menu lockout, vehicle identity and camera turnover");
}
