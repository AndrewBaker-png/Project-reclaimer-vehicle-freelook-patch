#include "../src/camera_copy.h"
#include "../src/render_visibility.h"
#include <cstdio>
#include <cstdlib>
using namespace freelook;
static void require(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
int main(){
 // Exercise the render isolation boundary across both visor settings, every
 // view/user combination, and objects sharing an index with a different salt.
 constexpr uint32_t pilot=0x12340005,vehicle=0x23450006;
 const uint32_t candidates[]={pilot,vehicle,0x12340007,0x98760005,0x98760006,0xffffffff};
 for(auto owner:candidates)for(int user=-1;user<=1;user++)for(int cockpit=0;cockpit<2;cockpit++)for(int visor=0;visor<2;visor++){
   require(hideVisorEffect(user,owner,vehicle,cockpit,visor)==(user==0&&cockpit&&visor&&owner==vehicle),"effect ownership cannot suppress other vehicles, other views, or physical cockpit effects");
 }
 require(!hideVisorEffect(0,0xffffffff,0xffffffff,true,true),"unowned effects remain visible");
 for(int user=-1;user<=1;user++)for(int cockpit=0;cockpit<2;cockpit++)
 for(int visor=0;visor<2;visor++)for(int mainView=0;mainView<2;mainView++)for(auto object:candidates){
   auto result=cockpitRenderSkip(user,object,pilot,vehicle,cockpit,visor,mainView);
   bool scoped=user==0&&cockpit&&mainView;
   auto expected=scoped&&object==pilot?CockpitRenderSkip::pilot:
     scoped&&visor&&object==vehicle?CockpitRenderSkip::vehicle:CockpitRenderSkip::none;
   require(result==expected,"visor exclusions respect full datum identity, mode, camera user and caller scope");
 }
 require(cockpitRenderSkip(0,vehicle,0xffffffff,vehicle,true,true,true)==CockpitRenderSkip::none,"missing pilot fails closed");
 require(cockpitRenderSkip(0,pilot,pilot,0xffffffff,true,true,true)==CockpitRenderSkip::none,"missing vehicle fails closed");
 require(cockpitRenderSkip(0,pilot,pilot,pilot,true,true,true)==CockpitRenderSkip::none,"inconsistent object identities fail closed");
 require(hideCockpitPilot(0,0x12340005,0x12340005,true,true),"local pilot is excluded from cockpit main-view rendering");
 require(!hideCockpitPilot(0,0x12340006,0x12340005,true,true),"other objects remain visible");
 require(!hideCockpitPilot(1,0x12340005,0x12340005,true,true),"other camera users are not overridden");
 require(!hideCockpitPilot(0,0x12340005,0x12340005,true,false),"non-render predicate calls preserve original behavior");
 require(!hideCockpitPilot(0,0x12340005,0x12340005,false,true),"pilot remains visible outside cockpit mode");
 auto* source=(unsigned char*)VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);require(source!=nullptr,"test source allocated");
 memset(source,0x5a,256);Vec3 pos={-10,0,0},f={1,0,0},up={0,0,1};memcpy(source,&pos,12);memcpy(source+0x28,&f,12);memcpy(source+0x34,&up,12);
 unsigned char before[256];memcpy(before,source,256);DWORD old;require(VirtualProtect(source,4096,PAGE_READONLY,&old),"source camera protected against writes");
 CameraResultCopy copy;require(makeCameraCopy(source,{0,0,0},{1.5707963268f,0},copy),"copy built from read-only native camera");
 Vec3 actual;memcpy(&actual,copy.bytes,12);require(std::fabs(actual.x)<0.0001f&&std::fabs(actual.y+10)<0.0001f,"copied camera orbits correctly");
 require(memcmp(before,source,256)==0,"native camera remains byte-for-byte unchanged");
 require(memcmp(copy.bytes+0x40,source+0x40,256-0x40)==0,"FOV and remaining result fields preserved");
 for(int i=0;i<1000;i++){
   CameraResultCopy cockpit;
   // Use the actual source eye as pivot, including negative and moving world positions.
   Vec3 eye;memcpy(&eye,copy.bytes,12);require(transformCameraCopy(copy,eye,{i*0.003f,-0.7f},cockpit),"fixed-eye cockpit transform");
   Vec3 moved;memcpy(&moved,cockpit.bytes,12);require(memcmp(&eye,&moved,12)==0,"cockpit freelook never orbits away from the pilot eye");
   require(memcmp(cockpit.bytes+0x40,copy.bytes+0x40,256-0x40)==0,"cockpit keeps native projection and other camera state");
 }
 require(!makeCameraCopy(nullptr,{},{},copy),"missing camera rejected");
 CameraResultCopy banked;memcpy(banked.bytes,before,256);Vec3 bankedUp={0,1,0};memcpy(banked.bytes+0x34,&bankedUp,12);
 require(transformCameraCopy(banked,pos,{1.5707963268f,0},copy,true),"cockpit freelook supports a banked aircraft");
 Vec3 bankedForward;memcpy(&bankedForward,copy.bytes+0x28,12);
 require(std::fabs(bankedForward.z+1)<0.0001f&&std::fabs(bankedForward.y)<0.0001f,"cockpit yaw follows aircraft up instead of the world horizon");
 memcpy(&actual,copy.bytes,12);require(memcmp(&actual,&pos,12)==0,"banked cockpit eye remains fixed");
 Vec3 nose={0,1,0},roof={0,0,1};
 require(cockpitCameraCopy(banked,nose,roof,{},copy),"cockpit entry replaces an unrelated native aim direction");
 memcpy(&actual,copy.bytes+0x28,12);require(memcmp(&actual,&nose,12)==0,"centered cockpit points exactly along the aircraft nose");
 memcpy(&actual,copy.bytes,12);require(memcmp(&actual,&pos,12)==0,"entry calibration preserves the native pilot eye");
 require(!cockpitCameraCopy(banked,{},{},{},copy),"missing aircraft orientation is rejected");
 require(!cockpitCameraCopy(banked,nose,nose,{},copy),"parallel aircraft orientation vectors are rejected");
 CameraResultCopy steering=banked;Vec3 nativeAim={0.8f,0.6f,0},nativeUp={0,0,1};
 memcpy(steering.bytes+0x28,&nativeAim,12);memcpy(steering.bytes+0x34,&nativeUp,12);
 require(cockpitCameraCopy(steering,nativeAim,nativeUp,{},copy),"native steering view works independently of hull direction");
 memcpy(&actual,copy.bytes+0x28,12);require(std::fabs(actual.x-nativeAim.x)<0.00001f&&std::fabs(actual.y-nativeAim.y)<0.00001f&&std::fabs(actual.z-nativeAim.z)<0.00001f,"cockpit preserves immediate native aiming feedback with F8 released");
 require(!storeCameraPose(source,copy),"guarded stores reject read-only pages without changing protection");
 require(memcmp(before,source,256)==0,"rejected store leaves read-only camera untouched");
 CameraResultCopy writable;memcpy(writable.bytes,before,256);
 require(makeCameraCopy(source,{0,0,0},{1.5707963268f,0},copy)&&storeCameraPose(writable.bytes,copy),"guarded direct pose stores succeed on writable camera");
 require(memcmp(writable.bytes+0x40,before+0x40,256-0x40)==0,"direct stores preserve FOV and unrelated state");
 LARGE_INTEGER start,end,hz;QueryPerformanceFrequency(&hz);QueryPerformanceCounter(&start);
 float checksum=0;
 for(int i=0;i<100000;i++){require(makeCameraCopy(source,{0,0,0},{i*0.001f,0.3f},copy)&&storeCameraPose(writable.bytes,copy),"repeated copy and direct pose store succeed");float x;memcpy(&x,writable.bytes,4);checksum+=x;}
 QueryPerformanceCounter(&end);require(memcmp(before,source,256)==0,"100,000 render calls never alter native state");
 printf("PASS: read-only native camera, orbit/framing, FOV preservation, 100,000 private copies; %.3f microseconds/call (checksum %.2f)\n",(end.QuadPart-start.QuadPart)*1e6/hz.QuadPart/100000,checksum);
 VirtualFree(source,0,MEM_RELEASE);
}
