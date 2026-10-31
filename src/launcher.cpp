#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <winternl.h>
#include <bcrypt.h>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include "verified_build.h"
#ifdef FREELOOK_ONLY
static const wchar_t runtimeDllName[]=L"Halo3VehicleFreelook-only-mp-r30.dll";
static const wchar_t stopEventPattern[]=L"Local\\Halo3VehicleFreelookOnlyMP30Stop-%lu";
static_assert(sizeof verifiedHookSites/sizeof verifiedHookSites[0]==3);
#else
static const wchar_t runtimeDllName[]=L"Halo3VehicleFreelook-native-r29.dll";
static const wchar_t stopEventPattern[]=L"Local\\Halo3VehicleFreelookNativeR29Stop-%lu";
#endif
static bool reclaimerMode=false;
static uintptr_t module(DWORD pid,const wchar_t* name,wchar_t* path=nullptr){uintptr_t base=0;HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);MODULEENTRY32W e={sizeof e};if(Module32FirstW(s,&e))do{if(!_wcsicmp(e.szModule,name)){base=(uintptr_t)e.modBaseAddr;if(path)wcscpy(path,e.szExePath);break;}}while(Module32NextW(s,&e));CloseHandle(s);return base;}
static DWORD findGame(){DWORD pid=0;HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);PROCESSENTRY32W e={sizeof e};if(Process32FirstW(s,&e))do{bool candidate=reclaimerMode?(wcsstr(e.szExeFile,L"project-reclaimer-client")&&module(e.th32ProcessID,L"halo3.dll")):!_wcsicmp(e.szExeFile,L"MCC-Win64-Shipping.exe");if(candidate){if(pid){CloseHandle(s);return 0;}pid=e.th32ProcessID;}}while(Process32NextW(s,&e));CloseHandle(s);return pid;}
static bool commandLine(HANDLE process,wchar_t* out,size_t count){
 using Query=LONG(NTAPI*)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);
 auto query=(Query)GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQueryInformationProcess");
 if(!query)return false;unsigned char bytes[32768];ULONG used=0;
 if(query(process,(PROCESSINFOCLASS)60,bytes,sizeof bytes,&used)<0)return false;
 auto* str=(UNICODE_STRING*)bytes;size_t n=str->Length/2;if(n>=count)return false;
 memcpy(out,str->Buffer,str->Length);out[n]=0;return true;
}
static bool hashFile(const wchar_t* path,const char* expected){
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return false;
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;bool ok=false;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0){
  unsigned char buffer[65536],digest[32];DWORD n=0;bool reads=true;
  for(;;){if(!ReadFile(file,buffer,sizeof buffer,&n,nullptr)){reads=false;break;}if(!n)break;if(BCryptHashData(hash,buffer,n,0)<0){reads=false;break;}}
  if(reads&&BCryptFinishHash(hash,digest,32,0)>=0){char hex[65];for(int i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",digest[i]);ok=_stricmp(hex,expected)==0;}
 }
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(file);return ok;
}
int wmain(int argc,wchar_t** argv){
 bool probe=false,stop=false;for(int i=1;i<argc;i++){if(!wcscmp(argv[i],L"--reclaimer"))reclaimerMode=true;else if(!wcscmp(argv[i],L"--probe"))probe=true;else if(!wcscmp(argv[i],L"--stop"))stop=true;else {puts("Usage: Halo3Freelook [--reclaimer] [--probe | --stop]");return 20;}}
 DWORD pid=findGame();if(!pid){puts(reclaimerMode?"Open one Reclaimer session and load a private Halo 3 test game, then try again.":"Start one MCC session using Anti-Cheat Disabled (Mods and Limited Services), then try again.");return 1;}
 wchar_t eventName[100];swprintf(eventName,100,stopEventPattern,pid);
 if(stop){HANDLE event=OpenEventW(EVENT_MODIFY_STATE,FALSE,eventName);if(!event){puts("No running freelook instance found.");return 2;}SetEvent(event);CloseHandle(event);puts("Stop requested. Restart the game before loading this prototype again.");return 0;}
#ifdef FREELOOK_ONLY
 // A fresh process ensures previous cockpit/render hooks and helpers are absent,
 // including a stopped revision whose DLL remains resident for callback safety.
 HANDLE loaded=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
 if(loaded==INVALID_HANDLE_VALUE){puts("Cannot verify loaded modules; no injection attempted.");return 21;}
 MODULEENTRY32W entry={sizeof entry};bool otherRuntime=false;
 if(!Module32FirstW(loaded,&entry)){CloseHandle(loaded);return 21;}
 do{if(wcsstr(entry.szModule,L"Halo3VehicleFreelook"))otherRuntime=true;}while(Module32NextW(loaded,&entry));CloseHandle(loaded);
 if(otherRuntime){puts("Freelook runtime already resident. Fully close and restart Reclaimer/MCC before this isolated MP test. No injection attempted.");return 22;}
#endif
 HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ,FALSE,pid);if(!process){printf("Cannot inspect MCC: Windows error %lu\n",GetLastError());return 3;}
 wchar_t cmd[16384];if(!commandLine(process,cmd,16384)||(reclaimerMode?!wcsstr(cmd,L"probe-engine-worker"):(!wcsstr(cmd,L"-no-eac")&&!wcsstr(cmd,L"-eac-nop-loaded")))){puts("Refused: could not verify MCC mod mode or the standalone Reclaimer engine worker.");CloseHandle(process);return 4;}
 wchar_t gamePath[MAX_PATH];DWORD size=MAX_PATH;if(!QueryFullProcessImageNameW(process,0,gamePath,&size)){CloseHandle(process);return 5;}
 if(!hashFile(gamePath,reclaimerMode?expectedReclaimerHash:expectedExeHash)){puts("Refused: client executable differs from the verified local build.");CloseHandle(process);return 6;}
 wchar_t haloPath[MAX_PATH];uintptr_t halo=module(pid,L"halo3.dll",haloPath);if(!halo){puts("Load Halo 3 first, then run the launcher again.");CloseHandle(process);return 7;}
 if(!hashFile(haloPath,expectedHaloHash)){puts("Refused: Halo 3 DLL differs from the verified local build.");return 8;}
 for(const auto& site:verifiedHookSites){unsigned char bytes[16];SIZE_T n=0;if(!ReadProcessMemory(process,(void*)(halo+site.rva),bytes,16,&n)||n!=16||memcmp(bytes,site.bytes,16)){printf("Refused: live hook site 0x%llx differs from the verified build (already modded or unsupported).\n",(unsigned long long)site.rva);CloseHandle(process);return 17;}}
 CloseHandle(process);
 printf("Verified %s PID %lu, runtime, exact client/Halo 3 SHA256 hashes and all %zu live hook sites.\n",reclaimerMode?"Reclaimer":"MCC",pid,sizeof verifiedHookSites/sizeof verifiedHookSites[0]);
 if(probe){puts("Probe only; no process memory changed.");return 0;}
 wchar_t dll[MAX_PATH];GetModuleFileNameW(nullptr,dll,MAX_PATH);wchar_t* p=wcsrchr(dll,L'\\');p[1]=0;wcscat(dll,runtimeDllName);
 if(GetFileAttributesW(dll)==INVALID_FILE_ATTRIBUTES){puts("Prototype DLL missing beside launcher.");return 9;}
 HANDLE modules=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);MODULEENTRY32W mod={sizeof mod};bool already=false;
 if(Module32FirstW(modules,&mod))do{if(!_wcsicmp(mod.szModule,runtimeDllName))already=true;}while(Module32NextW(modules,&mod));CloseHandle(modules);
 if(already){puts("Prototype already loaded. Use --stop to disable it; restart MCC to reload.");return 10;}
 process=OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid);
 if(!process){printf("Cannot access the mod-mode process: Windows error %lu\n",GetLastError());return 11;}
 SIZE_T bytes=(wcslen(dll)+1)*sizeof(wchar_t),written=0;void* remote=VirtualAllocEx(process,nullptr,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 if(!remote||!WriteProcessMemory(process,remote,dll,bytes,&written)||written!=bytes){puts("Could not copy prototype path.");if(remote)VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);return 12;}
 // Locate LoadLibraryW's containing module and translate its RVA into the remote process.
 auto loader=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");HMODULE localOwner=nullptr;
 GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)loader,&localOwner);
 wchar_t ownerPath[MAX_PATH];GetModuleFileNameW(localOwner,ownerPath,MAX_PATH);const wchar_t* ownerName=wcsrchr(ownerPath,L'\\');ownerName=ownerName?ownerName+1:ownerPath;
 uintptr_t remoteOwner=0;modules=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);mod.dwSize=sizeof mod;
 if(Module32FirstW(modules,&mod))do{if(!_wcsicmp(mod.szModule,ownerName))remoteOwner=(uintptr_t)mod.modBaseAddr;}while(Module32NextW(modules,&mod));CloseHandle(modules);
 if(!remoteOwner){puts("Cannot resolve remote loader module.");VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);return 13;}
 auto remoteLoader=(LPTHREAD_START_ROUTINE)(remoteOwner+(uintptr_t)loader-(uintptr_t)localOwner);
 HANDLE thread=CreateRemoteThread(process,nullptr,0,remoteLoader,remote,0,nullptr);if(!thread){printf("Load failed: %lu\n",GetLastError());VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);return 14;}
 DWORD wait=WaitForSingleObject(thread,15000),result=0;GetExitCodeThread(thread,&result);CloseHandle(thread);
 if(wait==WAIT_OBJECT_0)VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);
 if(wait!=WAIT_OBJECT_0||result==0){puts("DLL load failed or is still pending; check freelook.log before retrying.");return 15;}
#ifndef FREELOOK_ONLY
 wchar_t helper[MAX_PATH],ini[MAX_PATH];GetModuleFileNameW(nullptr,helper,MAX_PATH);wchar_t* slash=wcsrchr(helper,L'\\');slash[1]=0;wcscpy(ini,helper);wcscat(ini,L"freelook.ini");wcscat(helper,L"Halo3VisorHud.exe");
 if(GetPrivateProfileIntW(L"Freelook",L"HudEnabled",1,ini)){
   wchar_t args[MAX_PATH+80];swprintf(args,MAX_PATH+80,L"\"%ls\" --pid %lu",helper,pid);
   STARTUPINFOW startup={sizeof startup};startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION child{};
   if(CreateProcessW(helper,args,nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child)){CloseHandle(child.hThread);CloseHandle(child.hProcess);}
   else printf("HUD companion could not start: Windows error %lu. Camera and visor remain available.\n",GetLastError());
 }
#endif
#ifdef FREELOOK_ONLY
 puts("Freelook-only MP test build loaded: three input/camera hooks, no cockpit, visor, HUD or rendering hooks. Hold F8 in a vehicle; release to recenter.\nCheck freelook.log. Stop with this launcher --stop or Ctrl+Shift+F10.");
#else
 puts("Prototype loaded. Check freelook.log and visor-hud.log for status. Hold F8 while in a Halo 3 vehicle.\nStop with this launcher --stop, or Ctrl+Shift+F10. Game files remain unchanged.");
#endif
 return 0;
}
