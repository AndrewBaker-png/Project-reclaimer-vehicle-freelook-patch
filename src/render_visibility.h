#pragma once
#include <cstdint>
namespace freelook {
enum class CockpitRenderSkip { none, pilot, vehicle };
inline bool hideVisorEffect(int user,uint32_t owner,uint32_t vehicle,bool cockpit,bool visor){
  return user==0&&cockpit&&visor&&vehicle!=0xffffffff&&owner==vehicle;
}
// Supply identities from the current render snapshot after validating the pilot
// seat and supported vehicle. Never infer ownership from an object index alone.
inline CockpitRenderSkip cockpitRenderSkip(int user,uint32_t object,uint32_t pilot,uint32_t vehicle,bool cockpit,bool visor,bool mainView){
  if(!cockpit||!mainView||user!=0||pilot==0xffffffff||vehicle==0xffffffff||pilot==vehicle)return CockpitRenderSkip::none;
  if(object==pilot)return CockpitRenderSkip::pilot;
  if(visor&&object==vehicle)return CockpitRenderSkip::vehicle;
  return CockpitRenderSkip::none;
}
inline bool hideCockpitPilot(int user,uint32_t object,uint32_t pilot,bool cockpit,bool mainView){
  return cockpit&&mainView&&user==0&&pilot!=0xffffffff&&object==pilot;
}
}
