// Test double for the actual PowerShell loader. Never accesses game processes.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>
#include <cstring>
#include <string>
static std::wstring folder;
static std::wstring path(const wchar_t* name){return folder+name;}
static bool exists(const wchar_t* name){return GetFileAttributesW(path(name).c_str())!=INVALID_FILE_ATTRIBUTES;}
static void touch(const wchar_t* name){
  HANDLE file=CreateFileW(path(name).c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);
}
static bool awaitMarker(const wchar_t* name){
  ULONGLONG deadline=GetTickCount64()+10000;
  while(!exists(name)){if(GetTickCount64()>=deadline)return false;Sleep(10);}return true;
}
int wmain(int argc,wchar_t** argv){
  wchar_t ownPath[32768];DWORD length=GetModuleFileNameW(nullptr,ownPath,32768);
  if(!length||length>=32768)return 90;
  folder=ownPath;folder.resize(folder.find_last_of(L'\\')+1);
  char mode[64]{};DWORD count=0;
  HANDLE file=CreateFileW(path(L"mode.txt").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
  if(file==INVALID_HANDLE_VALUE)return 91;
  ReadFile(file,mode,sizeof mode-1,&count,nullptr);CloseHandle(file);
  for(int i=1;i<argc;i++)if(!wcscmp(argv[i],L"--probe")){
    touch(L"probe.marker");
    if(!strcmp(mode,"waiting"))return 1;
    if(!strcmp(mode,"refused"))return 6;
    return 0;
  }
  for(int i=1;i<argc;i++)if(!wcscmp(argv[i],L"--stop")){
    if(!exists(L"active.marker"))return 2;
    touch(L"stop-started.marker");
    if(!awaitMarker(L"allow-stop.marker"))return 23;
    if(!DeleteFileW(path(L"active.marker").c_str()))return 23;
    touch(L"stop-confirmed.marker");return 0;
  }
  touch(L"injection-started.marker");
  if(!strcmp(mode,"pending"))return 15;
  if(!awaitMarker(L"allow-load.marker"))return 15;
  touch(L"active.marker");return 0;
}
