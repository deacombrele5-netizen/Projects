#include "crypto.h"
#include "shell_integration.h"
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <filesystem>
#include <vector>
#include <string>

#pragma comment(lib,"comctl32.lib")
namespace fs=std::filesystem;
enum { ID_PATH=101, ID_BROWSE, ID_PASS, ID_CONFIRM, ID_SHOW, ID_ENCRYPT, ID_DECRYPT, ID_RUN, ID_STATUS, ID_LANG, ID_DARK, ID_MENU_ON, ID_MENU_OFF };
HWND gMain{},gPath{},gPass{},gConfirm{},gStatus{},gShow{},gConfirmLabel{};
bool gDark=false, gEnglish=false;
std::wstring gInitialPath;
bool gInitialDecrypt=false;
std::wstring T(const wchar_t* ru,const wchar_t* en){return gEnglish?en:ru;}
void SetStatus(const std::wstring& s){SetWindowTextW(gStatus,s.c_str());}
HWND Add(HWND parent,const wchar_t* cls,const std::wstring& text,DWORD style,int x,int y,int w,int h,int id){return CreateWindowExW(0,cls,text.c_str(),WS_CHILD|WS_VISIBLE|style,x,y,w,h,parent,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);}
std::wstring GetText(HWND h){int n=GetWindowTextLengthW(h);std::wstring s(n+1,L'\0');GetWindowTextW(h,s.data(),n+1);s.resize(n);return s;}
bool ProcessOne(const fs::path& p,bool decrypt,const std::wstring& pass,std::wstring& msg){
 std::wstring out,err; bool ok=decrypt?crypto::DecryptFile(p,pass,out,err):crypto::EncryptFile(p,pass,out,err);
 msg=(ok?T(L"Готово: ",L"Created: ")+out:T(L"Ошибка: ",L"Error: ")+err);return ok;
}
void ProcessSelection(bool decrypt){
 std::wstring path=GetText(gPath),pass=GetText(gPass),confirm=GetText(gConfirm);
 if(path.empty()||pass.empty()){MessageBoxW(gMain,T(L"Выбери файл или папку и введи пароль.",L"Choose a file or folder and enter a password.").c_str(),L"USB Encryptor",MB_ICONWARNING);return;}
 if(pass.size()<8){MessageBoxW(gMain,T(L"Пароль должен содержать не менее 8 символов.",L"Password must contain at least 8 characters.").c_str(),L"USB Encryptor",MB_ICONWARNING);return;}
 if(!decrypt && !SendMessageW(gShow,BM_GETCHECK,0,0) && pass!=confirm){MessageBoxW(gMain,T(L"Пароли не совпадают.",L"Passwords do not match.").c_str(),L"USB Encryptor",MB_ICONWARNING);return;}
 int success=0,failed=0; std::wstring last;
 try{
  fs::path p(path);
  if(fs::is_regular_file(p)){if(ProcessOne(p,decrypt,pass,last))success++;else failed++;}
  else if(fs::is_directory(p)){
   std::vector<fs::path> items;
   for(auto it=fs::recursive_directory_iterator(p,fs::directory_options::skip_permission_denied);it!=fs::recursive_directory_iterator();++it)
    if(it->is_regular_file() && (decrypt?it->path().extension()==L".locked":it->path().extension()!=L".locked")) items.push_back(it->path());
   for(const auto& f:items){if(ProcessOne(f,decrypt,pass,last))success++;else failed++;}
  } else {last=T(L"Путь не найден.",L"Path not found.");failed++;}
 }catch(const std::exception&){last=T(L"Ошибка доступа к файлам или папке.",L"File or folder access error.");failed++;}
 SecureZeroMemory(pass.data(),pass.size()*sizeof(wchar_t));SecureZeroMemory(confirm.data(),confirm.size()*sizeof(wchar_t));
 SetStatus(T(L"Успешно: ",L"Successful: ")+std::to_wstring(success)+T(L"; ошибок: ",L"; errors: ")+std::to_wstring(failed)+(last.empty()?L"":L" — "+last));
}
void Browse(){
 wchar_t file[MAX_PATH]{}; OPENFILENAMEW ofn{};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=gMain;ofn.lpstrFile=file;ofn.nMaxFile=MAX_PATH;
 ofn.lpstrFilter=L"Все файлы\0*.*\0\0";ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_EXPLORER;
 if(GetOpenFileNameW(&ofn))SetWindowTextW(gPath,file);
}
void ToggleMode(bool decrypt){ShowWindow(gConfirmLabel,decrypt?SW_HIDE:SW_SHOW);ShowWindow(gConfirm,decrypt?SW_HIDE:SW_SHOW);}
void InitControls(HWND h){
 Add(h,L"STATIC",L"USB Encryptor",SS_LEFT,18,12,300,28);
 Add(h,L"STATIC",T(L"Файл или папка:",L"File or folder:"),0,18,52,130,22);
 gPath=Add(h,L"EDIT",L"",WS_BORDER|ES_AUTOHSCROLL,18,75,390,25,ID_PATH);
 Add(h,L"BUTTON",T(L"Обзор...",L"Browse..."),BS_PUSHBUTTON,416,75,90,25,ID_BROWSE);
 Add(h,L"STATIC",T(L"Пароль:",L"Password:"),0,18,112,120,20);
 gPass=Add(h,L"EDIT",L"",WS_BORDER|ES_PASSWORD|ES_AUTOHSCROLL,18,135,230,25,ID_PASS);
 gShow=Add(h,L"BUTTON",T(L"Показать пароль",L"Show password"),BS_AUTOCHECKBOX,258,135,150,25,ID_SHOW);
 gConfirmLabel=Add(h,L"STATIC",T(L"Повторите пароль:",L"Confirm password:"),0,18,170,150,20);
 gConfirm=Add(h,L"EDIT",L"",WS_BORDER|ES_PASSWORD|ES_AUTOHSCROLL,18,193,230,25,ID_CONFIRM);
 Add(h,L"BUTTON",T(L"Зашифровать копию",L"Encrypt a copy"),BS_PUSHBUTTON,18,238,155,32,ID_ENCRYPT);
 Add(h,L"BUTTON",T(L"Расшифровать копию",L"Decrypt a copy"),BS_PUSHBUTTON,182,238,155,32,ID_DECRYPT);
 Add(h,L"BUTTON",T(L"Установить меню Проводника",L"Enable Explorer menu"),BS_PUSHBUTTON,18,282,210,26,ID_MENU_ON);
 Add(h,L"BUTTON",T(L"Удалить меню",L"Remove menu"),BS_PUSHBUTTON,238,282,130,26,ID_MENU_OFF);
 gStatus=Add(h,L"STATIC",T(L"Оригиналы не удаляются. Результат создаётся рядом.",L"Originals are kept. Output is created beside the source."),SS_LEFT,18,320,490,45,ID_STATUS);
 HWND lang=Add(h,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL,390,12,116,120,ID_LANG);
 SendMessageW(lang,CB_ADDSTRING,0,(LPARAM)L"Русский");SendMessageW(lang,CB_ADDSTRING,0,(LPARAM)L"English");SendMessageW(lang,CB_SETCURSEL,0,0);
 HWND dark=Add(h,L"BUTTON",T(L"Тёмная тема",L"Dark theme"),BS_AUTOCHECKBOX,18,370,120,22,ID_DARK);
 (void)dark;
}
LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CREATE:gMain=h;InitControls(h);if(!gInitialPath.empty())SetWindowTextW(gPath,gInitialPath.c_str());ToggleMode(gInitialDecrypt);return 0;
 case WM_COMMAND:
  switch(LOWORD(w)){
   case ID_BROWSE:Browse();break;
   case ID_SHOW:{bool show=SendMessageW(gShow,BM_GETCHECK,0,0)==BST_CHECKED;SendMessageW(gPass,EM_SETPASSWORDCHAR,show?0:L'*',0);SendMessageW(gConfirm,EM_SETPASSWORDCHAR,show?0:L'*',0);InvalidateRect(gPass,nullptr,TRUE);InvalidateRect(gConfirm,nullptr,TRUE);ShowWindow(gConfirmLabel,show?SW_HIDE:SW_SHOW);ShowWindow(gConfirm,show?SW_HIDE:SW_SHOW);break;}
   case ID_ENCRYPT:ProcessSelection(false);break;
   case ID_DECRYPT:ProcessSelection(true);break;
   case ID_MENU_ON:{std::wstring e;if(EnableShellMenu(e))SetStatus(T(L"Контекстное меню включено для текущего пользователя.",L"Explorer menu enabled for current user."));else SetStatus(e);break;}
   case ID_MENU_OFF:{std::wstring e;if(DisableShellMenu(e))SetStatus(T(L"Контекстное меню удалено.",L"Explorer menu removed."));else SetStatus(e);break;}
   case ID_LANG:gEnglish=SendMessageW((HWND)l,CB_GETCURSEL,0,0)==1;SetStatus(T(L"Язык изменён после перезапуска окна.",L"Language selection applies after restarting the window."));break;
   case ID_DARK:gDark=SendMessageW((HWND)l,BM_GETCHECK,0,0)==BST_CHECKED;InvalidateRect(h,nullptr,TRUE);break;
  }return 0;
 case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:case WM_CTLCOLORBTN:
  if(gDark){HDC dc=(HDC)w;SetBkColor(dc,RGB(38,38,38));SetTextColor(dc,RGB(235,235,235));static HBRUSH b=CreateSolidBrush(RGB(38,38,38));return (LRESULT)b;}break;
 case WM_ERASEBKGND:if(gDark){RECT r;GetClientRect(h,&r);FillRect((HDC)w,&r,CreateSolidBrush(RGB(38,38,38)));return 1;}break;
 case WM_DESTROY:PostQuitMessage(0);return 0;
 }return DefWindowProcW(h,m,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE, PWSTR, int show){
 int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 if(argv && argc>=3 && (wcscmp(argv[1],L"--encrypt")==0||wcscmp(argv[1],L"--decrypt")==0)){gInitialDecrypt=wcscmp(argv[1],L"--decrypt")==0;gInitialPath=argv[2];}
 if(argv)LocalFree(argv);
 INITCOMMONCONTROLSEX ic{sizeof(ic),ICC_STANDARD_CLASSES};InitCommonControlsEx(&ic);
 WNDCLASSW wc{};wc.lpfnWndProc=WndProc;wc.hInstance=instance;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hIcon=LoadIcon(nullptr,IDI_SHIELD);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"USBEncryptorClassicWindow";RegisterClassW(&wc);
 HWND h=CreateWindowExW(WS_EX_APPWINDOW,wc.lpszClassName,L"USB Encryptor",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,535,445,nullptr,nullptr,instance,nullptr);
 if(!h)return 1;ShowWindow(h,show);UpdateWindow(h);
 MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return (int)msg.wParam;
}
