#include "../src/freelook_math.h"
#include <cstdio>
#include <cstdlib>
using namespace freelook;
static void require(bool condition,const char* why){if(!condition){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
static bool near(float a,float b){return std::fabs(a-b)<0.0001f;}
int main(){
 Vec3 f,u;orient({1,0,0},{0,0,1},{},f,u);require(near(f.x,1)&&near(u.z,1),"zero offset preserves camera");
 orient({1,0,0},{0,0,1},{1.5707963268f,0},f,u);require(near(f.y,1)&&near(f.x,0),"yaw turns camera 90 degrees");
 orient({1,0,0},{0,0,1},{0,0.5f},f,u);require(f.z>0,"positive pitch looks up");
 for(int i=-100;i<=100;i++){orient({1,0,0},{0,0,1},{i*0.1f,i*0.01f},f,u);require(near(dot(f,f),1)&&near(dot(u,u),1)&&near(dot(f,u),0),"camera basis remains orthonormal");}
 Look l;l.move(100000,100000,0.003f);require(std::fabs(l.yaw)<=3.141593f&&l.pitch>=-1.396264f,"angles wrap and pitch clamps");
 Look normal,flipped;normal.move(2,3,0.1f);flipped.move(2,3,0.1f,true);require(near(normal.yaw,flipped.yaw)&&near(normal.pitch,-flipped.pitch),"vertical inversion changes pitch only");
 Vec3 position=orbit({-10,0,0},{0,0,0},{1,0,0},{0,0,1},{1.5707963268f,0});
 require(near(position.x,0)&&near(position.y,-10)&&near(dot(position,position),100),"freelook orbits the vehicle and preserves camera distance");
 orient({1,0,0},{0,0,1},{1.5707963268f,0},f,u);require(near(dot(unit(mul(position,-1)),f),1),"orbit keeps vehicle in the original framing");
 Look smoothed=smooth({3.1f,0},{-3.1f,1},0.01f,0.04f);require(smoothed.yaw>3.1f&&smoothed.pitch>0&&smoothed.pitch<1,"smoothing follows shortest yaw path and does not jump");
 puts("PASS: identity, yaw, pitch, 201 orthonormal bases, angle limits");
}
