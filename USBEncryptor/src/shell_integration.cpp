#include "shell_integration.h"
#include <windows.h>
#include <filesystem>

namespace {
bool SetVerb(HKEY root, const wchar_t* base, const wchar_t* verb, const wchar_t* label,
             const std::wstring& exe, const wchar_t* action, std::wstring& error) {
    std::wstring keyPath=std::wstring(base)+L"\\shell\\"+verb;
    HKEY key=nullptr;
    if (RegCreateKeyExW(root,keyPath.c_str(),0,nullptr,0,KEY_WRITE,nullptr,&key,nullptr)!=ERROR_SUCCESS) { error=L"Не удалось создать пункт контекстного меню."; return false; }
    RegSetValueExW(key,nullptr,0,REG_SZ,reinterpret_cast<const BYTE*>(label),static_cast<DWORD>((wcslen(label)+1)*sizeof(wchar_t)));
    RegCloseKey(key);
    keyPath+=L"\\command";
    if (RegCreateKeyExW(root,keyPath.c_str(),0,nullptr,0,KEY_WRITE,nullptr,&key,nullptr)!=ERROR_SUCCESS) { error=L"Не удалось записать команду контекстного меню."; return false; }
    std::wstring command=L"\""+exe+L"\" "+action+L" \"%1\"";
    RegSetValueExW(key,nullptr,0,REG_SZ,reinterpret_cast<const BYTE*>(command.c_str()),static_cast<DWORD>((command.size()+1)*sizeof(wchar_t)));
    RegCloseKey(key); return true;
}
}
bool EnableShellMenu(std::wstring& error) {
    wchar_t path[MAX_PATH]{};
    if (!GetModuleFileNameW(nullptr,path,MAX_PATH)) { error=L"Не удалось определить путь программы."; return false; }
    std::wstring exe=path;
    HKEY root=HKEY_CURRENT_USER;
    const wchar_t* bases[]={L"Software\\Classes\\*",L"Software\\Classes\\Directory"};
    for (auto base:bases) {
        if (!SetVerb(root,base,L"USBEncryptor.Encrypt",L"Зашифровать копию (USB Encryptor)",exe,L"--encrypt",error)) return false;
        if (!SetVerb(root,base,L"USBEncryptor.Decrypt",L"Расшифровать копию (USB Encryptor)",exe,L"--decrypt",error)) return false;
    }
    return true;
}
bool DisableShellMenu(std::wstring& error) {
    const wchar_t* paths[]={L"Software\\Classes\\*\\shell\\USBEncryptor.Encrypt",L"Software\\Classes\\*\\shell\\USBEncryptor.Decrypt",
      L"Software\\Classes\\Directory\\shell\\USBEncryptor.Encrypt",L"Software\\Classes\\Directory\\shell\\USBEncryptor.Decrypt"};
    for (auto p:paths) { LONG s=RegDeleteTreeW(HKEY_CURRENT_USER,p); if (s!=ERROR_SUCCESS && s!=ERROR_FILE_NOT_FOUND) { error=L"Не удалось удалить один из пунктов меню."; return false; } }
    return true;
}
