#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#include "freelook_math.h"
namespace freelook {
inline bool accessible(const void* address,size_t size,bool writable){
  uintptr_t current=(uintptr_t)address,end=current+size;if(!current||end<current)return false;
  while(current<end){MEMORY_BASIC_INFORMATION info;if(!VirtualQuery((void*)current,&info,sizeof info)||info.State!=MEM_COMMIT||(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    DWORD protection=info.Protect&0xff;
    if(writable&&protection!=PAGE_READWRITE&&protection!=PAGE_WRITECOPY&&protection!=PAGE_EXECUTE_READWRITE&&protection!=PAGE_EXECUTE_WRITECOPY)return false;
    uintptr_t next=(uintptr_t)info.BaseAddress+info.RegionSize;if(next<=current)return false;current=next;
  }return true;
}
inline bool safeCopy(void* out,const void* in,size_t size){
  if(!accessible(in,size,false))return false;memcpy(out,in,size);return true;
}
inline bool safeStore(void* out,const void* in,size_t size){
  if(!accessible(out,size,true))return false;memcpy(out,in,size);return true;
}
struct alignas(16) CameraResultCopy { unsigned char bytes[0x100]; };
inline bool storeCameraPose(void* native,const CameraResultCopy& copy){
  auto* out=(unsigned char*)native;
  if(!accessible(out,0x40,true))return false;
  memcpy(out,copy.bytes,12);memcpy(out+0x28,copy.bytes+0x28,12);memcpy(out+0x34,copy.bytes+0x34,12);return true;
}
inline bool transformCameraCopy(const CameraResultCopy& source,Vec3 pivot,Look offset,CameraResultCopy& copy,bool localYaw=false){
  copy=source;
  Vec3 position,forward,up,newForward,newUp;
  memcpy(&position,copy.bytes,sizeof position);memcpy(&forward,copy.bytes+0x28,sizeof forward);memcpy(&up,copy.bytes+0x34,sizeof up);
  float norm=dot(forward,forward);if(!std::isfinite(norm)||norm<0.8f||norm>1.2f)return false;
  orient(forward,up,offset,newForward,newUp,localYaw);
  position=orbit(position,pivot,forward,up,offset);
  memcpy(copy.bytes,&position,sizeof position);memcpy(copy.bytes+0x28,&newForward,sizeof newForward);memcpy(copy.bytes+0x34,&newUp,sizeof newUp);
  return true;
}
inline bool makeCameraCopy(const void* native,Vec3 pivot,Look offset,CameraResultCopy& copy){
  CameraResultCopy source;if(!safeCopy(source.bytes,native,sizeof source.bytes))return false;
  return transformCameraCopy(source,pivot,offset,copy);
}
inline bool cockpitCameraCopy(const CameraResultCopy& source,Vec3 vehicleForward,Vec3 vehicleUp,Look offset,CameraResultCopy& copy){
  float f=dot(vehicleForward,vehicleForward),u=dot(vehicleUp,vehicleUp),d=dot(vehicleForward,vehicleUp);
  if(!std::isfinite(f)||!std::isfinite(u)||!std::isfinite(d)||f<0.8f||f>1.2f||u<0.8f||u>1.2f||std::fabs(d)>0.05f)return false;
  CameraResultCopy aligned=source;Vec3 eye;
  memcpy(&eye,source.bytes,12);vehicleForward=unit(vehicleForward);vehicleUp=unit(vehicleUp);
  memcpy(aligned.bytes+0x28,&vehicleForward,12);memcpy(aligned.bytes+0x34,&vehicleUp,12);
  return transformCameraCopy(aligned,eye,offset,copy,true);
}
}
