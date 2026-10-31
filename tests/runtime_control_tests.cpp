#include "../src/runtime_control.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
struct Signals {HANDLE stop,stopped;};
static DWORD WINAPI acknowledge(void* argument){
  auto& signals=*(Signals*)argument;
  if(WaitForSingleObject(signals.stop,2000)!=WAIT_OBJECT_0)return 1;
  Sleep(30);SetEvent(signals.stopped);return 0;
}
int main(){
  using freelook::StopResult;
  DWORD pid=GetCurrentProcessId();
  require(freelook::stopRuntime(pid,50)==StopResult::absent,"absent runtime distinguished from stopped runtime");
  wchar_t name[100];swprintf(name,100,freelook::stopEventPattern,pid);
  Signals signals{CreateEventW(nullptr,TRUE,FALSE,name),nullptr};require(signals.stop!=nullptr,"stop event created");
  require(freelook::stopRuntime(pid,50)==StopResult::unconfirmed,"legacy runtime without acknowledgment never reports confirmed shutdown");
  require(WaitForSingleObject(signals.stop,0)==WAIT_OBJECT_0,"legacy runtime still receives stop request");
  swprintf(name,100,freelook::stoppedEventPattern,pid);
  signals.stopped=CreateEventW(nullptr,TRUE,FALSE,name);require(signals.stopped!=nullptr,"completion event created");
  ResetEvent(signals.stop);
  require(freelook::stopRuntime(pid,30)==StopResult::unconfirmed,"timeout is not reported as stopped");
  ResetEvent(signals.stop);
  HANDLE thread=CreateThread(nullptr,0,acknowledge,&signals,0,nullptr);require(thread!=nullptr,"simulated worker created");
  require(freelook::stopRuntime(pid,2000)==StopResult::stopped,"stop waits for worker acknowledgment");
  require(WaitForSingleObject(signals.stopped,0)==WAIT_OBJECT_0,"acknowledgment preceded return");
  require(WaitForSingleObject(thread,2000)==WAIT_OBJECT_0,"worker completed");
  DWORD code=1;GetExitCodeThread(thread,&code);require(code==0,"worker observed the stop request");
  require(freelook::stopRuntime(pid,50)==StopResult::stopped,"repeated stop remains confirmed");
  CloseHandle(thread);CloseHandle(signals.stopped);CloseHandle(signals.stop);
  puts("PASS: absent/legacy runtime, shutdown timeout, delayed acknowledgment and repeated stop");
}
