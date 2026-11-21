#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>
#include "module_path.h"

static wchar_t config[MAX_PATH];
static HWND label, hint, captureButton;
static int selected=VK_F8;
static bool capturing=false;
static bool allowed(int key){
    return key>=1&&key<=254&&key!=VK_LBUTTON&&key!=VK_RBUTTON&&
      key!=VK_ESCAPE&&key!=VK_RETURN&&key!=VK_TAB&&key!=VK_LWIN&&key!=VK_RWIN&&
      key!=VK_F7&&key!=VK_F10&&key!='V'&&key!='B'&&key!='N';
}
static void refresh(){
    wchar_t name[80]{},text[140];
    if(selected==VK_MBUTTON)wcscpy(name,L"Middle mouse");
    else if(selected==VK_XBUTTON1)wcscpy(name,L"Mouse side button 1");
    else if(selected==VK_XBUTTON2)wcscpy(name,L"Mouse side button 2");
    else {
      UINT scan=MapVirtualKeyW(selected,MAPVK_VK_TO_VSC);
      if(selected==VK_INSERT||selected==VK_DELETE||selected==VK_HOME||selected==VK_END||selected==VK_PRIOR||selected==VK_NEXT||
         selected==VK_LEFT||selected==VK_RIGHT||selected==VK_UP||selected==VK_DOWN||selected==VK_DIVIDE||selected==VK_NUMLOCK)scan|=0x100;
      if(!GetKeyNameTextW(LONG(scan<<16),name,80))swprintf(name,80,L"Key %d",selected);
    }
    swprintf(text,140,L"Hold to freelook: %ls",name);SetWindowTextW(label,text);
}
static void choose(int key){
    if(key==VK_ESCAPE){capturing=false;SetWindowTextW(hint,L"Selection cancelled. Your saved binding has not changed.");return;}
    if(key==VK_F7){SetWindowTextW(hint,L"F7 opens Reclaimer's performance monitors. Choose another key.");return;}
    if(!allowed(key)){SetWindowTextW(hint,L"That key is reserved for menus, voice, firing, or stopping freelook. Choose another.");return;}
    selected=key;capturing=false;refresh();
    SetWindowTextW(hint,L"Click Save to apply. Avoid keys already used for driving or aiming.");
}
static LRESULT CALLBACK windowProc(HWND window,UINT message,WPARAM w,LPARAM l){
    switch(message){
    case WM_CREATE:
      label=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,22,22,480,28,window,nullptr,nullptr,nullptr);
      captureButton=CreateWindowW(L"BUTTON",L"Change binding...",WS_CHILD|WS_VISIBLE|WS_TABSTOP,22,60,175,32,window,(HMENU)101,nullptr,nullptr);
      CreateWindowW(L"BUTTON",L"Reset to F8",WS_CHILD|WS_VISIBLE|WS_TABSTOP,210,60,130,32,window,(HMENU)102,nullptr,nullptr);
      hint=CreateWindowW(L"STATIC",L"Choose one keyboard key, middle mouse, or a side button.\nHold activates freelook; release recenters.",WS_CHILD|WS_VISIBLE,22,110,480,55,window,nullptr,nullptr,nullptr);
      CreateWindowW(L"STATIC",L"Save to apply the binding to this installation, including a running game.",WS_CHILD|WS_VISIBLE,22,178,480,42,window,nullptr,nullptr,nullptr);
      CreateWindowW(L"BUTTON",L"Save",WS_CHILD|WS_VISIBLE|WS_TABSTOP,285,233,95,32,window,(HMENU)1,nullptr,nullptr);
      CreateWindowW(L"BUTTON",L"Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP,392,233,95,32,window,(HMENU)2,nullptr,nullptr);
      refresh();
      if(selected==VK_F7)SetWindowTextW(hint,L"Your saved F7 binding conflicts with Reclaimer's performance monitors.\nChoose another key, or reset to F8, then save.");
      return 0;
    case WM_COMMAND:
      if(LOWORD(w)==101){capturing=true;SetFocus(window);SetWindowTextW(hint,L"Press a key, or click middle/side mouse here. Escape cancels.\nOnly press the control you want to use; combinations are not supported.");}
      if(LOWORD(w)==102){capturing=false;selected=VK_F8;refresh();SetWindowTextW(hint,L"F8 selected. Click Save to apply.");}
      if(LOWORD(w)==2)DestroyWindow(window);
      if(LOWORD(w)==1){
        if(capturing){SetWindowTextW(hint,L"Choose a binding first, or press Escape to cancel capture.");return 0;}
        if(!allowed(selected)){MessageBoxW(window,L"Select a supported binding first.",L"Freelook controls",MB_OK|MB_ICONWARNING);return 0;}
        wchar_t value[16];swprintf(value,16,L"%d",selected);
        if(!WritePrivateProfileStringW(L"Freelook",L"HoldVirtualKey",value,config)){MessageBoxW(window,L"Could not save freelook.ini. Use a writable local mod folder.",L"Freelook controls",MB_OK|MB_ICONERROR);return 0;}
        MessageBoxW(window,L"Binding saved. Hold the selected control while in a vehicle.",L"Freelook controls",MB_OK);DestroyWindow(window);
      }return 0;
    case WM_CLOSE:DestroyWindow(window);return 0;
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,wchar_t*,int show){
    if(!freelook::siblingModulePath(nullptr,L"freelook.ini",config))return 1;
    selected=GetPrivateProfileIntW(L"Freelook",L"HoldVirtualKey",VK_F8,config);
    if(selected<1||selected>254)selected=VK_F8;
    WNDCLASSW type{};type.lpfnWndProc=windowProc;type.hInstance=instance;type.lpszClassName=L"FreelookControls3051";
    type.hCursor=LoadCursor(nullptr,IDC_ARROW);type.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);
    RegisterClassW(&type);
    HWND window=CreateWindowW(type.lpszClassName,L"Freelook controls",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,530,320,nullptr,nullptr,instance,nullptr);
    if(!window)return 1;ShowWindow(window,show);
    MSG msg;
    while(GetMessageW(&msg,nullptr,0,0)>0){
      if(capturing){
        if(msg.message==WM_KEYDOWN||msg.message==WM_SYSKEYDOWN){choose(int(msg.wParam));continue;}
        if(msg.message==WM_MBUTTONDOWN){choose(VK_MBUTTON);continue;}
        if(msg.message==WM_XBUTTONDOWN){choose(HIWORD(msg.wParam)==XBUTTON1?VK_XBUTTON1:VK_XBUTTON2);continue;}
      }
      if(IsDialogMessageW(window,&msg))continue;
      TranslateMessage(&msg);DispatchMessageW(&msg);
    }
    return 0;
}
