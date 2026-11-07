#pragma once
#include <windows.h>
#include <cwchar>
#include <cwctype>

namespace freelook {
// A single Windows key/button, independent of the game's action bindings.
// Empty names keep existing installations' numeric HoldVirtualKey setting.
inline int parseHoldKey(const wchar_t* name,int legacyKey){
  if(!*name)return legacyKey;
  wchar_t key[32]{};size_t length=wcslen(name);
  if(length>=32)return -1;
  for(size_t i=0;i<length;i++)key[i]=std::towupper(name[i]);
  if(length==1&&((key[0]>=L'A'&&key[0]<=L'Z')||(key[0]>=L'0'&&key[0]<=L'9')))return key[0];
  if(key[0]==L'F'&&length>=2&&length<=3){
    int number=0;
    for(size_t i=1;i<length;i++){if(key[i]<L'0'||key[i]>L'9')return -1;number=number*10+key[i]-L'0';}
    if(key[1]!=L'0'&&number>=1&&number<=24)return VK_F1+number-1;
  }
  struct NamedKey{const wchar_t* name;int code;};
  const NamedKey names[]={
    {L"MOUSE1",VK_LBUTTON},{L"MOUSE2",VK_RBUTTON},{L"MOUSE3",VK_MBUTTON},
    {L"MOUSE4",VK_XBUTTON1},{L"MOUSE5",VK_XBUTTON2},
    {L"SHIFT",VK_SHIFT},{L"LEFTSHIFT",VK_LSHIFT},{L"RIGHTSHIFT",VK_RSHIFT},
    {L"CTRL",VK_CONTROL},{L"LEFTCTRL",VK_LCONTROL},{L"RIGHTCTRL",VK_RCONTROL},
    {L"ALT",VK_MENU},{L"LEFTALT",VK_LMENU},{L"RIGHTALT",VK_RMENU},
    {L"SPACE",VK_SPACE},{L"CAPSLOCK",VK_CAPITAL},
    {L"INSERT",VK_INSERT},{L"DELETE",VK_DELETE},{L"HOME",VK_HOME},{L"END",VK_END},
    {L"PAGEUP",VK_PRIOR},{L"PAGEDOWN",VK_NEXT},
    {L"UP",VK_UP},{L"DOWN",VK_DOWN},{L"LEFT",VK_LEFT},{L"RIGHT",VK_RIGHT}
  };
  for(const auto& entry:names)if(wcscmp(key,entry.name)==0)return entry.code;
  return -1;
}
inline bool validHoldKey(int key,bool reclaimer){
  // Preserve voice-chat exclusions; menu keys always lock out the runtime.
  // F10 is reserved by the emergency stop chord.
  return key>=1&&key<=254&&key!='V'&&(!reclaimer||key!='B')&&
    key!=VK_ESCAPE&&key!=VK_RETURN&&key!=VK_TAB&&key!=VK_F10;
}
inline int loadHoldKey(const wchar_t* ini){
  wchar_t name[64]{};
  GetPrivateProfileStringW(L"Freelook",L"HoldKey",L"",name,64,ini);
  return parseHoldKey(name,GetPrivateProfileIntW(L"Freelook",L"HoldVirtualKey",VK_F8,ini));
}
}
