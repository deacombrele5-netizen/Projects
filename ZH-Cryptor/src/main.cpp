#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <cwchar>
#include <sstream>
#include <iterator>

#pragma comment(lib,"bcrypt.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"advapi32.lib")
namespace fs=std::filesystem;
static constexpr uint32_t KDF_ROUNDS=600000;
static constexpr uint64_t MAX_BYTES=256ull*1024*1024;
static constexpr char MAGIC[8]={'Z','H','C','R','Y','1','\0','\0'};
#pragma pack(push,1)
struct Header { char magic[8]; uint32_t version; uint32_t rounds; uint64_t size; BYTE salt[16]; BYTE nonce[12]; BYTE tag[16]; };
#pragma pack(pop)

static bool randomBytes(BYTE* p,ULONG n){return BCryptGenRandom(nullptr,p,n,BCRYPT_USE_SYSTEM_PREFERRED_RNG)>=0;}
static void wipe(std::vector<BYTE>& v){if(!v.empty())SecureZeroMemory(v.data(),v.size());}
static bool derive(const std::wstring& pass,const Header& h,BYTE key[32]){
 BCRYPT_ALG_HANDLE a=nullptr;if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0)return false;
 auto st=BCryptDeriveKeyPBKDF2(a,(PUCHAR)pass.data(),(ULONG)(pass.size()*sizeof(wchar_t)),(PUCHAR)h.salt,16,h.rounds,key,32,0);
 BCryptCloseAlgorithmProvider(a,0);return st>=0;
}
static bool gcm(bool enc,const Header& h,const BYTE key[32],const std::vector<BYTE>& in,std::vector<BYTE>& out,BYTE tag[16]){
 BCRYPT_ALG_HANDLE a=nullptr;BCRYPT_KEY_HANDLE k=nullptr;bool ok=false;ULONG written=0;
 if(BCryptOpenAlgorithmProvider(&a,BCRYPT_AES_ALGORITHM,nullptr,0)<0)return false;
 wchar_t mode[]=BCRYPT_CHAIN_MODE_GCM;
 if(BCryptSetProperty(a,BCRYPT_CHAINING_MODE,(PUCHAR)mode,sizeof(mode),0)<0)goto end;
 if(BCryptGenerateSymmetricKey(a,&k,nullptr,0,(PUCHAR)key,32,0)<0)goto end;
 out.resize(in.size());
 { BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO ai; BCRYPT_INIT_AUTH_MODE_INFO(ai);
   ai.pbNonce=(PUCHAR)h.nonce;ai.cbNonce=12;ai.pbAuthData=(PUCHAR)&h;ai.cbAuthData=(ULONG)offsetof(Header,tag);ai.pbTag=tag;ai.cbTag=16;
   NTSTATUS s=enc?BCryptEncrypt(k,(PUCHAR)in.data(),(ULONG)in.size(),&ai,nullptr,0,out.data(),(ULONG)out.size(),&written,0)
                 :BCryptDecrypt(k,(PUCHAR)in.data(),(ULONG)in.size(),&ai,nullptr,0,out.data(),(ULONG)out.size(),&written,0);
   ok=s>=0&&written==in.size();
 }
end: if(k)BCryptDestroyKey(k);if(a)BCryptCloseAlgorithmProvider(a,0);return ok;
}
static bool readFile(const fs::path& p,std::vector<BYTE>& v,std::wstring& err){
 std::error_code ec;auto n=fs::file_size(p,ec);if(ec||n>MAX_BYTES+sizeof(Header)){err=L"Файл недоступен или превышает лимит 256 МиБ.";return false;}
 std::ifstream f(p,std::ios::binary);if(!f){err=L"Не удалось открыть файл.";return false;}v.resize((size_t)n);
 if(n&&!f.read((char*)v.data(),(std::streamsize)n)){err=L"Ошибка чтения.";return false;}return true;
}
static bool writeNew(const fs::path& wanted,const std::vector<BYTE>& data,std::wstring& out,std::wstring& err){
 fs::path dst=wanted;
 for(unsigned i=1;fs::exists(dst);++i){dst=wanted.parent_path()/(wanted.stem().wstring()+L" ("+std::to_wstring(i)+L")"+wanted.extension().wstring());if(i>10000){err=L"Не удалось подобрать имя результата.";return false;}}
 fs::path tmp=dst;tmp+=L".zhctmp";
 {std::ofstream f(tmp,std::ios::binary|std::ios::trunc);if(!f){err=L"Не удалось создать файл результата.";return false;}
  if(!data.empty())f.write((const char*)data.data(),(std::streamsize)data.size());f.flush();if(!f){f.close();fs::remove(tmp);err=L"Ошибка записи.";return false;}}
 if(!MoveFileExW(tmp.c_str(),dst.c_str(),MOVEFILE_WRITE_THROUGH)){fs::remove(tmp);err=L"Не удалось завершить запись.";return false;}
 out=dst.wstring();return true;
}
static bool encryptFile(const fs::path& p,const std::wstring& pass,std::wstring& out,std::wstring& err){
 std::vector<BYTE> plain,cipher;if(!readFile(p,plain,err))return false;
 Header h{};memcpy(h.magic,MAGIC,8);h.version=1;h.rounds=KDF_ROUNDS;h.size=plain.size();
 if(!randomBytes(h.salt,16)||!randomBytes(h.nonce,12)){wipe(plain);err=L"Системный генератор случайных чисел недоступен.";return false;}
 BYTE key[32]{},tag[16]{};if(!derive(pass,h,key)){wipe(plain);err=L"Не удалось получить ключ.";return false;}
 bool ok=gcm(true,h,key,plain,cipher,tag);SecureZeroMemory(key,32);wipe(plain);
 if(!ok){wipe(cipher);err=L"Ошибка AES-256-GCM.";return false;}memcpy(h.tag,tag,16);SecureZeroMemory(tag,16);
 std::vector<BYTE> packed(sizeof(h)+cipher.size());memcpy(packed.data(),&h,sizeof(h));if(!cipher.empty())memcpy(packed.data()+sizeof(h),cipher.data(),cipher.size());wipe(cipher);
 fs::path dst=p;dst+=L".zhc";ok=writeNew(dst,packed,out,err);wipe(packed);return ok;
}
static bool decryptFile(const fs::path& p,const std::wstring& pass,std::wstring& out,std::wstring& err){
 std::vector<BYTE> packed,plain;if(!readFile(p,packed,err))return false;
 if(packed.size()<sizeof(Header)){wipe(packed);err=L"Формат ZH Cryptor не распознан.";return false;}
 Header h{};memcpy(&h,packed.data(),sizeof(h));
 if(memcmp(h.magic,MAGIC,8)||h.version!=1||h.rounds<100000||h.rounds>2000000||h.size>MAX_BYTES||h.size!=packed.size()-sizeof(Header)){wipe(packed);err=L"Файл повреждён или имеет неподдерживаемый формат.";return false;}
 std::vector<BYTE> cipher(packed.begin()+sizeof(Header),packed.end());wipe(packed);BYTE key[32]{},tag[16]{};memcpy(tag,h.tag,16);
 if(!derive(pass,h,key)){wipe(cipher);err=L"Не удалось получить ключ.";return false;}
 bool ok=gcm(false,h,key,cipher,plain,tag);SecureZeroMemory(key,32);SecureZeroMemory(tag,16);wipe(cipher);
 if(!ok){wipe(plain);err=L"Неверный пароль или файл повреждён. Исходник не изменён.";return false;}
 fs::path dst=p;if(dst.extension()==L".zhc")dst.replace_extension();else dst+=L".restored";
 ok=writeNew(dst,plain,out,err);wipe(plain);return ok;
}
static bool shellMenu(bool enable,std::wstring& err){
 const wchar_t* keys[]={L"Software\\Classes\\*\\shell\\ZHCryptor.Encrypt",L"Software\\Classes\\*\\shell\\ZHCryptor.Decrypt",L"Software\\Classes\\Directory\\shell\\ZHCryptor.Encrypt",L"Software\\Classes\\Directory\\shell\\ZHCryptor.Decrypt"};
 if(!enable){for(auto k:keys){LONG s=RegDeleteTreeW(HKEY_CURRENT_USER,k);if(s!=ERROR_SUCCESS&&s!=ERROR_FILE_NOT_FOUND){err=L"Не удалось удалить пункт Проводника.";return false;}}return true;}
 wchar_t exe[MAX_PATH]{};if(!GetModuleFileNameW(nullptr,exe,MAX_PATH)){err=L"Не удалось определить путь программы.";return false;}
 for(int i=0;i<4;i++){HKEY h=nullptr;LONG s=RegCreateKeyExW(HKEY_CURRENT_USER,keys[i],0,nullptr,0,KEY_WRITE,nullptr,&h,nullptr);if(s!=ERROR_SUCCESS){err=L"Не удалось добавить меню Проводника.";return false;}
  const wchar_t* label=(i%2==0)?L"Зашифровать копию — ZH Cryptor":L"Расшифровать копию — ZH Cryptor";RegSetValueExW(h,nullptr,0,REG_SZ,(const BYTE*)label,(DWORD)((wcslen(label)+1)*sizeof(wchar_t)));RegCloseKey(h);
  std::wstring cmdkey=std::wstring(keys[i])+L"\\command";if(RegCreateKeyExW(HKEY_CURRENT_USER,cmdkey.c_str(),0,nullptr,0,KEY_WRITE,nullptr,&h,nullptr)!=ERROR_SUCCESS){err=L"Не удалось настроить команду меню.";return false;}
  std::wstring cmd=L"\""+std::wstring(exe)+L"\" "+((i%2==0)?L"--encrypt ":L"--decrypt ")+L"\"%1\"";RegSetValueExW(h,nullptr,0,REG_SZ,(const BYTE*)cmd.c_str(),(DWORD)((cmd.size()+1)*sizeof(wchar_t)));RegCloseKey(h);
 }return true;
}

static bool hasEncrypted(const fs::path& folder){
 try{for(auto it=fs::recursive_directory_iterator(folder,fs::directory_options::skip_permission_denied);it!=fs::recursive_directory_iterator();++it)if(it->is_regular_file()&&it->path().extension()==L".zhc")return true;}catch(...){}return false;
}
static bool setFolderLockIcon(const fs::path& folder){
 HMODULE mod=GetModuleHandleW(nullptr);HRSRC res=FindResourceW(mod,MAKEINTRESOURCEW(102),RT_RCDATA);if(!res)return false;
 HGLOBAL loaded=LoadResource(mod,res);DWORD bytes=SizeofResource(mod,res);const void* raw=LockResource(loaded);if(!raw||!bytes)return false;
 fs::path ini=folder/L"desktop.ini",ico=folder/L".zhcryptor.ico";
 DWORD original=GetFileAttributesW(folder.c_str());if(original==INVALID_FILE_ATTRIBUTES)return false;
 if(fs::exists(ini)){std::ifstream old(ini,std::ios::binary);std::string txt((std::istreambuf_iterator<char>(old)),{});if(txt.find("ZHCRYPTOR_FOLDER_LOCK")==std::string::npos)return false;auto at=txt.find("ORIGINAL_FOLDER_ATTRS=");if(at!=std::string::npos){at+=22;try{original=(DWORD)std::stoul(txt.substr(at));}catch(...){}}}
 else if(fs::exists(ico))return false;
 fs::path tmp=ico;tmp+=L".tmp";
 {std::ofstream out(tmp,std::ios::binary|std::ios::trunc);if(!out)return false;out.write((const char*)raw,bytes);out.flush();if(!out){out.close();fs::remove(tmp);return false;}}
 if(!MoveFileExW(tmp.c_str(),ico.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){fs::remove(tmp);return false;}
 std::ofstream meta(ini,std::ios::binary|std::ios::trunc);
 if(!meta){fs::remove(ico);return false;}
 meta<<"; ZHCRYPTOR_FOLDER_LOCK\n; ORIGINAL_FOLDER_ATTRS="<<original<<"\n[.ShellClassInfo]\nIconResource=.zhcryptor.ico,0\n";meta.close();
 if(!meta){fs::remove(ini);fs::remove(ico);return false;}
 SetFileAttributesW(ico.c_str(),FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
 SetFileAttributesW(ini.c_str(),FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
 SetFileAttributesW(folder.c_str(),original|FILE_ATTRIBUTE_SYSTEM);
 SHChangeNotify(SHCNE_UPDATEITEM,SHCNF_PATH,folder.c_str(),nullptr);return true;
}
static void removeFolderLockIcon(const fs::path& folder){
 fs::path ini=folder/L"desktop.ini",ico=folder/L".zhcryptor.ico";
 std::ifstream in(ini,std::ios::binary);if(!in)return;std::string txt((std::istreambuf_iterator<char>(in)),{});
 const std::string marker="ZHCRYPTOR_FOLDER_LOCK";if(txt.find(marker)==std::string::npos)return;
 DWORD original=GetFileAttributesW(folder.c_str());auto pos=txt.find("ORIGINAL_FOLDER_ATTRS=");
 if(pos!=std::string::npos){pos+=22;try{DWORD saved=(DWORD)std::stoul(txt.substr(pos));if(original!=INVALID_FILE_ATTRIBUTES)SetFileAttributesW(folder.c_str(),saved);}catch(...){}}
 SetFileAttributesW(ini.c_str(),FILE_ATTRIBUTE_NORMAL);SetFileAttributesW(ico.c_str(),FILE_ATTRIBUTE_NORMAL);
 DeleteFileW(ini.c_str());DeleteFileW(ico.c_str());SHChangeNotify(SHCNE_UPDATEITEM,SHCNF_PATH,folder.c_str(),nullptr);
}

enum{P_PATH=101,P_PASS,P_CONFIRM,P_SHOW,P_ENCRYPT,P_DECRYPT,P_BROWSE,P_FOLDER,P_MENU,P_UNMENU,P_LANG,P_THEME,P_STATUS};
static HWND gWnd,gPath,gPass,gConfirm,gStatus,gConfirmLabel;static bool gShow=false,gDark=false,gEnglish=false;static std::wstring gArgPath;static bool gArgDecrypt=false;
static const wchar_t* tr(const wchar_t* ru,const wchar_t* en){return gEnglish?en:ru;}
static HWND ctl(HWND p,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id=0){return CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,p,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);}
static std::wstring get(HWND h){int n=GetWindowTextLengthW(h);std::wstring s(n+1,L'\0');GetWindowTextW(h,s.data(),n+1);s.resize(n);return s;}
static void status(const std::wstring& s){SetWindowTextW(gStatus,s.c_str());}
static void mode(bool dec){ShowWindow(gConfirmLabel,dec||gShow?SW_HIDE:SW_SHOW);ShowWindow(gConfirm,dec||gShow?SW_HIDE:SW_SHOW);}
static void chooseFile(){wchar_t p[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=gWnd;o.lpstrFile=p;o.nMaxFile=32768;o.lpstrFilter=L"Все файлы\0*.*\0\0";o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;if(GetOpenFileNameW(&o))SetWindowTextW(gPath,p);}
static void chooseFolder(){BROWSEINFOW b{};b.hwndOwner=gWnd;b.lpszTitle=tr(L"Выберите папку",L"Select a folder");LPITEMIDLIST id=SHBrowseForFolderW(&b);if(id){wchar_t p[MAX_PATH]{};if(SHGetPathFromIDListW(id,p))SetWindowTextW(gPath,p);CoTaskMemFree(id);}}
static void run(bool dec){
 std::wstring path=get(gPath),pass=get(gPass),confirm=get(gConfirm);
 if(path.empty()||pass.size()<8){MessageBoxW(gWnd,tr(L"Укажи файл или папку и пароль минимум из 8 символов.",L"Choose a file or folder and enter a password of at least 8 characters."),L"ZH Cryptor",MB_ICONWARNING);return;}
 if(!dec&&!gShow&&pass!=confirm){MessageBoxW(gWnd,tr(L"Пароли не совпадают.",L"Passwords do not match."),L"ZH Cryptor",MB_ICONWARNING);return;}
 int good=0,bad=0;std::wstring last;
 try{fs::path p(path);std::vector<fs::path> files;
  if(fs::is_regular_file(p))files.push_back(p);
  else if(fs::is_directory(p)){for(auto it=fs::recursive_directory_iterator(p,fs::directory_options::skip_permission_denied);it!=fs::recursive_directory_iterator();++it)if(it->is_regular_file()&&(dec?it->path().extension()==L".zhc":it->path().extension()!=L".zhc"))files.push_back(it->path());}
  else{last=L"Путь не найден.";bad++;}
  for(const auto& f:files){std::wstring out,err;bool ok=dec?decryptFile(f,pass,out,err):encryptFile(f,pass,out,err);if(ok){good++;last=out;}else{bad++;last=err;}}
  if(good>0&&fs::is_directory(p)){if(dec){if(!hasEncrypted(p))removeFolderLockIcon(p);}else setFolderLockIcon(p);}
 }catch(...){bad++;last=L"Ошибка доступа к папке или файлам.";}
 SecureZeroMemory(pass.data(),pass.size()*sizeof(wchar_t));SecureZeroMemory(confirm.data(),confirm.size()*sizeof(wchar_t));
 status(tr(L"Успешно: ",L"Done: ")+std::to_wstring(good)+tr(L"; ошибок: ",L"; errors: ")+std::to_wstring(bad)+(last.empty()?L"":L" — "+last));
}
static void controls(HWND h){
 ctl(h,L"STATIC",L"ZH Cryptor",SS_LEFT,18,12,250,30);
 ctl(h,L"STATIC",tr(L"Файл или папка",L"File or folder"),0,18,52,200,20);
 gPath=ctl(h,L"EDIT",L"",WS_BORDER|ES_AUTOHSCROLL,18,74,390,25,P_PATH);
 ctl(h,L"BUTTON",tr(L"Файл...",L"File..."),BS_PUSHBUTTON,416,74,88,25,P_BROWSE);
 ctl(h,L"BUTTON",tr(L"Папка...",L"Folder..."),BS_PUSHBUTTON,416,105,88,25,P_FOLDER);
 ctl(h,L"STATIC",tr(L"Пароль",L"Password"),0,18,112,150,20);
 gPass=ctl(h,L"EDIT",L"",WS_BORDER|ES_PASSWORD|ES_AUTOHSCROLL,18,135,240,25,P_PASS);
 ctl(h,L"BUTTON",tr(L"Показать",L"Show"),BS_AUTOCHECKBOX,270,135,100,25,P_SHOW);
 gConfirmLabel=ctl(h,L"STATIC",tr(L"Повторите пароль",L"Confirm password"),0,18,170,170,20);
 gConfirm=ctl(h,L"EDIT",L"",WS_BORDER|ES_PASSWORD|ES_AUTOHSCROLL,18,193,240,25,P_CONFIRM);
 ctl(h,L"BUTTON",tr(L"Зашифровать копию",L"Encrypt a copy"),BS_PUSHBUTTON,18,235,170,32,P_ENCRYPT);
 ctl(h,L"BUTTON",tr(L"Расшифровать копию",L"Decrypt a copy"),BS_PUSHBUTTON,196,235,170,32,P_DECRYPT);
 ctl(h,L"BUTTON",tr(L"Добавить меню Проводника",L"Add Explorer menu"),BS_PUSHBUTTON,18,280,205,27,P_MENU);
 ctl(h,L"BUTTON",tr(L"Убрать меню",L"Remove menu"),BS_PUSHBUTTON,232,280,130,27,P_UNMENU);
 gStatus=ctl(h,L"STATIC",tr(L"Оригиналы сохраняются. Результат создаётся рядом.",L"Originals are preserved. Output is created beside the source."),SS_LEFT,18,320,480,45,P_STATUS);
 HWND lang=ctl(h,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL,365,12,140,120,P_LANG);SendMessageW(lang,CB_ADDSTRING,0,(LPARAM)L"Русский");SendMessageW(lang,CB_ADDSTRING,0,(LPARAM)L"English");SendMessageW(lang,CB_SETCURSEL,0,0);
 ctl(h,L"BUTTON",tr(L"Тёмная тема",L"Dark theme"),BS_AUTOCHECKBOX,18,370,140,23,P_THEME);
}
static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_CREATE){gWnd=h;controls(h);if(!gArgPath.empty())SetWindowTextW(gPath,gArgPath.c_str());mode(gArgDecrypt);return 0;}
 if(m==WM_COMMAND){switch(LOWORD(w)){
 case P_BROWSE:chooseFile();break;case P_FOLDER:chooseFolder();break;
 case P_SHOW:gShow=SendMessageW((HWND)l,BM_GETCHECK,0,0)==BST_CHECKED;SendMessageW(gPass,EM_SETPASSWORDCHAR,gShow?0:L'*',0);SendMessageW(gConfirm,EM_SETPASSWORDCHAR,gShow?0:L'*',0);InvalidateRect(gPass,nullptr,TRUE);InvalidateRect(gConfirm,nullptr,TRUE);mode(false);break;
 case P_ENCRYPT:mode(false);run(false);break;case P_DECRYPT:mode(true);run(true);break;
 case P_MENU:{std::wstring e;if(shellMenu(true,e))status(tr(L"Пункты Проводника добавлены для текущего пользователя.",L"Explorer items added for current user."));else status(e);break;}
 case P_UNMENU:{std::wstring e;if(shellMenu(false,e))status(tr(L"Пункты Проводника удалены.",L"Explorer items removed."));else status(e);break;}
 case P_LANG:gEnglish=SendMessageW((HWND)l,CB_GETCURSEL,0,0)==1;MessageBoxW(h,tr(L"Перезапусти программу, чтобы применить язык.",L"Restart the app to apply language."),L"ZH Cryptor",MB_OK);break;
 case P_THEME:gDark=SendMessageW((HWND)l,BM_GETCHECK,0,0)==BST_CHECKED;InvalidateRect(h,nullptr,TRUE);break;
 }return 0;}
 if(m==WM_CTLCOLORSTATIC||m==WM_CTLCOLOREDIT||m==WM_CTLCOLORBTN){if(gDark){HDC dc=(HDC)w;SetBkColor(dc,RGB(40,40,40));SetTextColor(dc,RGB(240,240,240));static HBRUSH b=CreateSolidBrush(RGB(40,40,40));return (LRESULT)b;}}
 if(m==WM_ERASEBKGND&&gDark){RECT r;GetClientRect(h,&r);static HBRUSH b=CreateSolidBrush(RGB(40,40,40));FillRect((HDC)w,&r,b);return 1;}
 if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);
}
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,PWSTR,int show){
 int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(argv&&argc>=3&&(wcscmp(argv[1],L"--encrypt")==0||wcscmp(argv[1],L"--decrypt")==0)){gArgDecrypt=wcscmp(argv[1],L"--decrypt")==0;gArgPath=argv[2];}if(argv)LocalFree(argv);
 INITCOMMONCONTROLSEX ic{sizeof(ic),ICC_STANDARD_CLASSES};InitCommonControlsEx(&ic);
 WNDCLASSW wc{};wc.lpfnWndProc=proc;wc.hInstance=inst;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(inst,MAKEINTRESOURCEW(101));wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName=L"ZHCryptorWindow";RegisterClassW(&wc);
 HWND h=CreateWindowExW(WS_EX_APPWINDOW,wc.lpszClassName,L"ZH Cryptor",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,535,445,nullptr,nullptr,inst,nullptr);if(!h)return 1;
 ShowWindow(h,show);UpdateWindow(h);MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return (int)msg.wParam;
}
