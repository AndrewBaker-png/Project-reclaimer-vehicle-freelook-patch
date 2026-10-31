#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cwchar>
#include <cstdio>
#include <cstdlib>
static std::wstring executablePath;
static bool failQuery=false;
static DWORD testModulePath(HMODULE,wchar_t* out,DWORD capacity){
  if(failQuery)return 0;
  size_t count=executablePath.size();if(count>=capacity)count=capacity-1;
  wmemcpy(out,executablePath.c_str(),count);out[count]=0;
  return executablePath.size()>=capacity?capacity:DWORD(count);
}
#define GetModuleFileNameW testModulePath
#include "../src/module_path.h"
static void require(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
int main(){
  const wchar_t* name=L"Halo3VehicleFreelook-only-mp-r30.dll";
  struct {wchar_t path[MAX_PATH];wchar_t guard[32];} memory{};
  auto check=[&](bool expected){
    for(auto& c:memory.guard)c=L'!';
    require(freelook::siblingModulePath(nullptr,name,memory.path)==expected,"correct path acceptance");
    for(auto c:memory.guard)require(c==L'!',"path append never overwrites adjacent memory");
    if(!expected)require(memory.path[0]==0,"failure does not expose a partial path");
  };
  executablePath=L"C:\\Mods\\Halo3Freelook.exe";check(true);
  require(std::wstring(memory.path)==L"C:\\Mods\\"+std::wstring(name),"normal sibling DLL resolved");
  executablePath=L"C:\\"+std::wstring(226,L'a')+L"\\Halo3Freelook.exe";check(false);
  size_t nameSize=wcslen(name),folderSize=MAX_PATH-1-nameSize;
  std::wstring folder=L"C:\\"+std::wstring(folderSize-4,L'a')+L"\\";
  executablePath=folder+L"Halo3Freelook.exe";check(true);
  require(wcslen(memory.path)==MAX_PATH-1,"maximum fitting DLL path accepted");
  executablePath=L"C:\\"+std::wstring(folderSize-3,L'a')+L"\\Halo3Freelook.exe";check(false);
  executablePath=L"C:\\"+std::wstring(300,L'a')+L"\\Halo3Freelook.exe";check(false);
  executablePath=L"Halo3Freelook.exe";check(false);
  failQuery=true;check(false);
  puts("PASS: normal, exact-boundary, overflowing, truncated, malformed and failed module paths");
}
