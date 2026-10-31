#include "../src/runtime_control.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
int wmain(int argc,wchar_t** argv){
  require(argc>=2,"runtime DLL path supplied");
  require(GetModuleHandleW(L"halo3.dll")==nullptr,"test process has no game engine");
  HMODULE runtime=LoadLibraryW(argv[1]);require(runtime!=nullptr,"actual runtime loads into isolated test process");
  wchar_t name[100];swprintf(name,100,freelook::stopEventPattern,GetCurrentProcessId());
  HANDLE stop=OpenEventW(SYNCHRONIZE,FALSE,name);
  require(stop!=nullptr,"stop event exists synchronously when LoadLibrary returns");CloseHandle(stop);
  require(freelook::stopRuntime(GetCurrentProcessId(),3000)==freelook::StopResult::stopped,"actual runtime acknowledges stop during startup without an engine");
  puts("PASS: real DLL publishes stop handle before load returns and confirms cancellation during startup");
  // The runtime deliberately remains mapped until process exit.
}
