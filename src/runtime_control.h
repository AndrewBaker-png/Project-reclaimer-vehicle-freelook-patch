#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>
namespace freelook {
inline constexpr wchar_t stopEventPattern[]=L"Local\\Halo3VehicleFreelookOnlyMP30Stop-%lu";
inline constexpr wchar_t stoppedEventPattern[]=L"Local\\Halo3VehicleFreelookOnlyMP30Stopped-%lu";
enum class StopResult {stopped,absent,unconfirmed};
inline StopResult stopRuntime(DWORD pid,DWORD timeoutMs=15000){
  wchar_t name[100];swprintf(name,100,stopEventPattern,pid);
  HANDLE stop=OpenEventW(EVENT_MODIFY_STATE,FALSE,name);
  if(!stop)return GetLastError()==ERROR_FILE_NOT_FOUND?StopResult::absent:StopResult::unconfirmed;
  swprintf(name,100,stoppedEventPattern,pid);
  HANDLE stopped=OpenEventW(SYNCHRONIZE,FALSE,name);
  bool requested=SetEvent(stop)!=FALSE;CloseHandle(stop);
  DWORD result=requested&&stopped?WaitForSingleObject(stopped,timeoutMs):WAIT_FAILED;
  if(stopped)CloseHandle(stopped);
  return result==WAIT_OBJECT_0?StopResult::stopped:StopResult::unconfirmed;
}
}
