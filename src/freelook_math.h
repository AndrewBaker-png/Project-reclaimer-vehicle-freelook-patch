#pragma once
#include <cmath>
namespace freelook {
struct Vec3 { float x,y,z; };
inline Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 mul(Vec3 a,float f){return {a.x*f,a.y*f,a.z*f};}
inline float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline Vec3 unit(Vec3 a){float l=std::sqrt(dot(a,a));return l>0.00001f?mul(a,1/l):Vec3{0,0,0};}
inline Vec3 rotate(Vec3 v,Vec3 axis,float angle){float c=std::cos(angle),s=std::sin(angle);return add(add(mul(v,c),mul(cross(axis,v),s)),mul(axis,dot(axis,v)*(1-c)));}
inline float clamp(float x,float lo,float hi){return x<lo?lo:(x>hi?hi:x);}
inline float wrap(float a){return std::remainder(a,6.28318530718f);}
struct Look { float yaw=0,pitch=0; void move(float x,float y,float sensitivity,bool invertVertical=false){yaw=wrap(yaw+x*sensitivity);pitch=clamp(pitch+(invertVertical?y:-y)*sensitivity,-1.3962634f,1.3962634f);} };
inline void orient(Vec3 baseForward,Vec3 baseUp,Look look,Vec3& forward,Vec3& up,bool localYaw=false){
  Vec3 z=localYaw?unit(baseUp):Vec3{0,0,1};
  forward=rotate(baseForward,z,look.yaw);up=rotate(baseUp,z,look.yaw);
  Vec3 right=unit(cross(forward,up));
  forward=unit(rotate(forward,right,look.pitch));up=unit(rotate(up,right,look.pitch));
}
inline Vec3 orbit(Vec3 position,Vec3 pivot,Vec3 baseForward,Vec3 baseUp,Look look){
  Vec3 z={0,0,1};
  Vec3 right=unit(cross(rotate(baseForward,z,look.yaw),rotate(baseUp,z,look.yaw)));
  Vec3 relative=add(position,mul(pivot,-1));
  return add(pivot,rotate(rotate(relative,z,look.yaw),right,look.pitch));
}
inline Look smooth(Look current,Look target,float dt,float seconds){
  float alpha=seconds<=0?1.f:1.f-std::exp(-clamp(dt,0.f,0.1f)/seconds);
  return {wrap(current.yaw+wrap(target.yaw-current.yaw)*alpha),current.pitch+(target.pitch-current.pitch)*alpha};
}
}
