#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>
#include <cstring>
namespace freelook {
// Keep the existing MAX_PATH contract, but refuse truncation and an oversized
// sibling filename before writing it. A failed result is never used as a path.
inline bool siblingModulePath(HMODULE module,const wchar_t* filename,wchar_t (&out)[MAX_PATH]){
  DWORD length=GetModuleFileNameW(module,out,MAX_PATH);
  if(!length||length>=MAX_PATH){out[0]=0;return false;}
  wchar_t* slash=wcsrchr(out,L'\\');
  if(!slash){out[0]=0;return false;}
  size_t folderLength=size_t(slash+1-out),nameLength=wcslen(filename);
  if(nameLength>=MAX_PATH-folderLength){out[0]=0;return false;}
  memcpy(slash+1,filename,(nameLength+1)*sizeof(wchar_t));return true;
}
}
