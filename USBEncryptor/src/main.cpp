#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <cstdint>
#include <cstddef>
#include <algorithm>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace fs = std::filesystem;

static constexpr wchar_t APP_TITLE[] = L"USB Encryptor";
static constexpr wchar_t ENC_EXT[] = L".usbenc";
static constexpr uint32_t KDF_ITERATIONS = 600000;
static constexpr uint64_t MAX_FILE_SIZE = 512ull * 1024ull * 1024ull;

#pragma pack(push, 1)
struct FileHeader {
    char magic[8];
    uint32_t version;
    uint32_t iterations;
    uint64_t dataSize;
    unsigned char salt[16];
    unsigned char nonce[12];
    unsigned char tag[16];
};
#pragma pack(pop)

enum : int {
    ID_FILES = 101, ID_FOLDER, ID_CLEAR, ID_ENCRYPT, ID_DECRYPT,
    ID_PASSWORD, ID_CONFIRM, ID_SHOW, ID_START, ID_LIST, ID_STATUS
};

HWND g_main = nullptr, g_list = nullptr, g_password = nullptr, g_confirm = nullptr;
HWND g_confirmLabel = nullptr, g_status = nullptr, g_show = nullptr;
HWND g_encryptRadio = nullptr, g_decryptRadio = nullptr;
std::vector<fs::path> g_selected;
HFONT g_font = nullptr;

static std::wstring WinError(const wchar_t* prefix) {
    return std::wstring(prefix);
}

static void SetStatus(const std::wstring& s) {
    SetWindowTextW(g_status, s.c_str());
}

static void AddLog(const std::wstring& s) {
    SendMessageW(g_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s.c_str()));
    const LRESULT n = SendMessageW(g_list, LB_GETCOUNT, 0, 0);
    if (n > 0) SendMessageW(g_list, LB_SETCURSEL, n - 1, 0);
}

static void RefreshList() {
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    for (const auto& p : g_selected) {
        std::wstring s = p.filename().empty() ? p.wstring() : p.filename().wstring();
        if (fs::is_directory(p)) s += L"  [папка]";
        AddLog(s);
    }
    SetStatus(L"Выбрано элементов: " + std::to_wstring(g_selected.size()));
}

static void AddPath(const fs::path& p) {
    std::error_code ec;
    fs::path full = fs::absolute(p, ec);
    if (ec) full = p;
    for (const auto& old : g_selected) {
        std::error_code e2;
        if (fs::equivalent(old, full, e2) && !e2) return;
        if (old.lexically_normal() == full.lexically_normal()) return;
    }
    g_selected.push_back(full);
}

static void PickFiles() {
    std::vector<wchar_t> buffer(65536, L'\0');
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_main;
    ofn.lpstrFilter = L"Все файлы\0*.*\0\0";
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = static_cast<DWORD>(buffer.size());
    ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Выберите файлы";
    if (!GetOpenFileNameW(&ofn)) return;

    const wchar_t* first = buffer.data();
    const wchar_t* second = first + wcslen(first) + 1;
    if (*second == L'\0') {
        AddPath(first);
    } else {
        const fs::path dir(first);
        for (const wchar_t* p = second; *p; p += wcslen(p) + 1)
            AddPath(dir / p);
    }
    RefreshList();
}

static void PickFolder() {
    BROWSEINFOW bi{};
    bi.hwndOwner = g_main;
    bi.lpszTitle = L"Выберите папку для обработки";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;
    wchar_t path[MAX_PATH]{};
    if (SHGetPathFromIDListW(pidl, path)) AddPath(path);
    CoTaskMemFree(pidl);
    RefreshList();
}

static bool ReadWholeFile(const fs::path& path, std::vector<unsigned char>& data, std::wstring& error) {
    std::error_code ec;
    const auto size = fs::file_size(path, ec);
    if (ec) { error = L"Не удалось определить размер: " + path.filename().wstring(); return false; }
    if (size > MAX_FILE_SIZE) { error = L"Файл больше 512 МБ, пропущен: " + path.filename().wstring(); return false; }
    std::ifstream in(path, std::ios::binary);
    if (!in) { error = L"Не удалось открыть: " + path.filename().wstring(); return false; }
    data.resize(static_cast<size_t>(size));
    if (size && !in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size))) {
        error = L"Ошибка чтения: " + path.filename().wstring(); return false;
    }
    return true;
}

static bool DeriveKey(const std::wstring& password, const unsigned char salt[16],
                      uint32_t iterations, unsigned char key[32]) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    NTSTATUS st = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (!BCRYPT_SUCCESS(st)) return false;
    st = BCryptDeriveKeyPBKDF2(hAlg,
        reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(password.data())),
        static_cast<ULONG>(password.size() * sizeof(wchar_t)),
        const_cast<PUCHAR>(salt), 16, iterations, key, 32, 0);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return BCRYPT_SUCCESS(st);
}

static bool AesGcm(bool encrypt, const std::vector<unsigned char>& input,
                   std::vector<unsigned char>& output, FileHeader& header,
                   const unsigned char key[32]) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    NTSTATUS st = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(st)) return false;

    wchar_t mode[] = BCRYPT_CHAIN_MODE_GCM;
    st = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(mode),
                           sizeof(mode), 0);
    if (!BCRYPT_SUCCESS(st)) { BCryptCloseAlgorithmProvider(hAlg, 0); return false; }

    st = BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0,
                                    const_cast<PUCHAR>(key), 32, 0);
    if (!BCRYPT_SUCCESS(st)) { BCryptCloseAlgorithmProvider(hAlg, 0); return false; }

    output.resize(input.size());
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth{};
    BCRYPT_INIT_AUTH_MODE_INFO(auth);
    auth.pbNonce = header.nonce;
    auth.cbNonce = sizeof(header.nonce);
    auth.pbTag = header.tag;
    auth.cbTag = sizeof(header.tag);
    auth.pbAuthData = reinterpret_cast<PUCHAR>(&header);
    auth.cbAuthData = static_cast<ULONG>(offsetof(FileHeader, tag));

    ULONG done = 0;
    unsigned char dummy = 0;
    PUCHAR inPtr = input.empty() ? &dummy : const_cast<PUCHAR>(input.data());
    PUCHAR outPtr = output.empty() ? &dummy : output.data();
    if (encrypt) {
        st = BCryptEncrypt(hKey, inPtr, static_cast<ULONG>(input.size()), &auth,
                           nullptr, 0, outPtr, static_cast<ULONG>(output.size()), &done, 0);
    } else {
        st = BCryptDecrypt(hKey, inPtr, static_cast<ULONG>(input.size()), &auth,
                           nullptr, 0, outPtr, static_cast<ULONG>(output.size()), &done, 0);
    }
    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return BCRYPT_SUCCESS(st) && done == input.size();
}

static fs::path UniqueEncryptedPath(const fs::path& source) {
    fs::path candidate(source.wstring() + ENC_EXT);
    for (unsigned i = 1; fs::exists(candidate); ++i)
        candidate = fs::path(source.wstring() + L" (" + std::to_wstring(i) + L")" + ENC_EXT);
    return candidate;
}

static fs::path UniqueDecryptedPath(const fs::path& source) {
    fs::path base = source;
    if (base.extension() == ENC_EXT) base.replace_extension();
    else base += L".decrypted";
    if (!fs::exists(base)) return base;
    fs::path candidate = base.parent_path() /
        (base.stem().wstring() + L"_decrypted" + base.extension().wstring());
    for (unsigned i = 2; fs::exists(candidate); ++i)
        candidate = base.parent_path() /
            (base.stem().wstring() + L"_decrypted_" + std::to_wstring(i) + base.extension().wstring());
    return candidate;
}

static bool WriteAtomic(const fs::path& destination, const FileHeader* header,
                        const std::vector<unsigned char>& data, std::wstring& error) {
    fs::path temp = destination;
    temp += L".usbtmp";
    if (fs::exists(temp)) {
        error = L"Временный файл уже существует: " + temp.filename().wstring();
        return false;
    }
    {
        std::ofstream out(temp, std::ios::binary | std::ios::out);
        if (!out) { error = L"Не удалось создать выходной файл."; return false; }
        if (header) out.write(reinterpret_cast<const char*>(header), sizeof(FileHeader));
        if (!data.empty()) out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        out.flush();
        if (!out) {
            out.close();
            std::error_code ec; fs::remove(temp, ec);
            error = L"Ошибка записи. Исходник не изменён.";
            return false;
        }
    }
    if (!MoveFileExW(temp.c_str(), destination.c_str(), MOVEFILE_WRITE_THROUGH)) {
        std::error_code ec; fs::remove(temp, ec);
        error = L"Не удалось завершить запись результата. Исходник сохранён.";
        return false;
    }
    return true;
}

static bool EncryptOne(const fs::path& source, const std::wstring& password, std::wstring& message) {
    if (source.extension() == ENC_EXT) { message = L"Уже имеет расширение .usbenc: " + source.filename().wstring(); return false; }
    std::vector<unsigned char> plain, cipher;
    if (!ReadWholeFile(source, plain, message)) return false;

    FileHeader h{};
    memcpy(h.magic, "USBENC01", 8);
    h.version = 1;
    h.iterations = KDF_ITERATIONS;
    h.dataSize = plain.size();
    if (!BCRYPT_SUCCESS(BCryptGenRandom(nullptr, h.salt, sizeof(h.salt), BCRYPT_USE_SYSTEM_PREFERRED_RNG)) ||
        !BCRYPT_SUCCESS(BCryptGenRandom(nullptr, h.nonce, sizeof(h.nonce), BCRYPT_USE_SYSTEM_PREFERRED_RNG))) {
        message = L"Не удалось получить криптографически случайные данные."; return false;
    }
    unsigned char key[32]{};
    if (!DeriveKey(password, h.salt, h.iterations, key)) { message = L"Ошибка подготовки ключа."; return false; }
    const bool ok = AesGcm(true, plain, cipher, h, key);
    SecureZeroMemory(key, sizeof(key));
    if (!ok) { message = L"Ошибка шифрования: " + source.filename().wstring(); return false; }

    const fs::path destination = UniqueEncryptedPath(source);
    if (!WriteAtomic(destination, &h, cipher, message)) return false;
    message = L"Зашифрована копия: " + destination.filename().wstring() + L" (оригинал сохранён)";
    return true;
}

static bool DecryptOne(const fs::path& source, const std::wstring& password, std::wstring& message) {
    std::ifstream in(source, std::ios::binary);
    if (!in) { message = L"Не удалось открыть: " + source.filename().wstring(); return false; }
    FileHeader h{};
    if (!in.read(reinterpret_cast<char*>(&h), sizeof(h)) ||
        memcmp(h.magic, "USBENC01", 8) != 0 || h.version != 1 ||
        h.iterations < 100000 || h.iterations > 2000000 || h.dataSize > MAX_FILE_SIZE) {
        message = L"Это не файл USB Encryptor или его заголовок повреждён: " + source.filename().wstring();
        return false;
    }
    std::vector<unsigned char> cipher(static_cast<size_t>(h.dataSize)), plain;
    if (h.dataSize && !in.read(reinterpret_cast<char*>(cipher.data()), static_cast<std::streamsize>(h.dataSize))) {
        message = L"Файл повреждён или обрезан: " + source.filename().wstring(); return false;
    }
    char extra;
    if (in.read(&extra, 1)) { message = L"Размер файла не совпадает с заголовком: " + source.filename().wstring(); return false; }

    unsigned char key[32]{};
    if (!DeriveKey(password, h.salt, h.iterations, key)) { message = L"Ошибка подготовки ключа."; return false; }
    const bool ok = AesGcm(false, cipher, plain, h, key);
    SecureZeroMemory(key, sizeof(key));
    if (!ok) { message = L"Неверный пароль или файл повреждён: " + source.filename().wstring(); return false; }

    const fs::path destination = UniqueDecryptedPath(source);
    if (!WriteAtomic(destination, nullptr, plain, message)) return false;
    message = L"Восстановлена копия: " + destination.filename().wstring() + L" (зашифрованный оригинал сохранён)";
    return true;
}

static void CollectFiles(const fs::path& item, std::vector<fs::path>& files) {
    std::error_code ec;
    if (fs::is_regular_file(item, ec)) { files.push_back(item); return; }
    if (!fs::is_directory(item, ec)) return;
    fs::recursive_directory_iterator it(item, fs::directory_options::skip_permission_denied, ec), end;
    for (; it != end; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        std::error_code e2;
        if (it->is_regular_file(e2) && !e2) files.push_back(it->path());
    }
}

static void ToggleMode() {
    const bool encrypt = SendMessageW(g_encryptRadio, BM_GETCHECK, 0, 0) == BST_CHECKED;
    ShowWindow(g_confirmLabel, encrypt ? SW_SHOW : SW_HIDE);
    ShowWindow(g_confirm, encrypt ? SW_SHOW : SW_HIDE);
    SetWindowTextW(GetDlgItem(g_main, ID_START), encrypt ? L"Зашифровать копии" : L"Расшифровать копии");
}

static void TogglePassword() {
    const bool show = SendMessageW(g_show, BM_GETCHECK, 0, 0) == BST_CHECKED;
    SendMessageW(g_password, EM_SETPASSWORDCHAR, show ? 0 : L'*', 0);
    SendMessageW(g_confirm, EM_SETPASSWORDCHAR, show ? 0 : L'*', 0);
    InvalidateRect(g_password, nullptr, TRUE);
    InvalidateRect(g_confirm, nullptr, TRUE);
}

static void StartWork() {
    if (g_selected.empty()) { MessageBoxW(g_main, L"Сначала добавь файлы или папку.", APP_TITLE, MB_OK | MB_ICONINFORMATION); return; }
    wchar_t pass[1024]{}, confirm[1024]{};
    GetWindowTextW(g_password, pass, 1024);
    GetWindowTextW(g_confirm, confirm, 1024);
    std::wstring password(pass);
    SecureZeroMemory(pass, sizeof(pass));
    SecureZeroMemory(confirm, sizeof(confirm));
    if (password.size() < 8) { MessageBoxW(g_main, L"Пароль должен содержать не менее 8 символов.", APP_TITLE, MB_OK | MB_ICONWARNING); SecureZeroMemory(password.data(), password.size() * sizeof(wchar_t)); return; }
    const bool encrypt = SendMessageW(g_encryptRadio, BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (encrypt) {
        wchar_t confirmText[1024]{};
        GetWindowTextW(g_confirm, confirmText, 1024);
        if (password != confirmText) {
            SecureZeroMemory(confirmText, sizeof(confirmText));
            SecureZeroMemory(password.data(), password.size() * sizeof(wchar_t));
            MessageBoxW(g_main, L"Пароли не совпадают.", APP_TITLE, MB_OK | MB_ICONWARNING);
            return;
        }
        SecureZeroMemory(confirmText, sizeof(confirmText));
    }

    std::vector<fs::path> files;
    for (const auto& item : g_selected) CollectFiles(item, files);
    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());
    if (files.empty()) {
        SecureZeroMemory(password.data(), password.size() * sizeof(wchar_t));
        MessageBoxW(g_main, L"В выбранных элементах не найдено файлов.", APP_TITLE, MB_OK | MB_ICONINFORMATION);
        return;
    }

    EnableWindow(GetDlgItem(g_main, ID_START), FALSE);
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    size_t success = 0, failed = 0;
    for (const auto& file : files) {
        std::wstring msg;
        const bool ok = encrypt ? EncryptOne(file, password, msg) : DecryptOne(file, password, msg);
        if (ok) ++success; else ++failed;
        AddLog((ok ? L"[OK] " : L"[ОШИБКА] ") + msg);
        SetStatus(L"Обработано: " + std::to_wstring(success + failed) + L" из " + std::to_wstring(files.size()));
        UpdateWindow(g_main);
    }
    SecureZeroMemory(password.data(), password.size() * sizeof(wchar_t));
    EnableWindow(GetDlgItem(g_main, ID_START), TRUE);
    SetStatus(L"Готово. Успешно: " + std::to_wstring(success) + L"; ошибок: " + std::to_wstring(failed) +
              L". Исходные файлы не удалялись.");
    MessageBoxW(g_main, (L"Обработка завершена.\nУспешно: " + std::to_wstring(success) +
        L"\nОшибок: " + std::to_wstring(failed) +
        L"\nИсходные файлы программой не удаляются.").c_str(), APP_TITLE, MB_OK | MB_ICONINFORMATION);
}

static HWND MakeControl(DWORD ex, const wchar_t* cls, const wchar_t* text, DWORD style,
                        int x, int y, int w, int h, int id) {
    HWND hwnd = CreateWindowExW(ex, cls, text, WS_CHILD | WS_VISIBLE | style,
        x, y, w, h, g_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
    if (g_font) SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
    return hwnd;
}

static void CreateControls(HWND hwnd) {
    g_font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    MakeControl(0, L"STATIC", L"Режим:", WS_VISIBLE, 16, 14, 60, 22, -1);
    g_encryptRadio = MakeControl(0, L"BUTTON", L"Шифрование", BS_AUTORADIOBUTTON | WS_GROUP, 78, 12, 120, 24, ID_ENCRYPT);
    g_decryptRadio = MakeControl(0, L"BUTTON", L"Расшифровка", BS_AUTORADIOBUTTON, 205, 12, 130, 24, ID_DECRYPT);
    SendMessageW(g_encryptRadio, BM_SETCHECK, BST_CHECKED, 0);

    MakeControl(0, L"STATIC", L"Пароль:", 0, 16, 48, 90, 22, -1);
    g_password = MakeControl(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_AUTOHSCROLL | ES_PASSWORD | WS_TABSTOP, 110, 45, 240, 24, ID_PASSWORD);
    g_confirmLabel = MakeControl(0, L"STATIC", L"Повтор пароля:", 0, 365, 48, 100, 22, -1);
    g_confirm = MakeControl(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_AUTOHSCROLL | ES_PASSWORD | WS_TABSTOP, 465, 45, 210, 24, ID_CONFIRM);
    g_show = MakeControl(0, L"BUTTON", L"Показать пароль", BS_AUTOCHECKBOX | WS_TABSTOP, 110, 75, 150, 22, ID_SHOW);

    MakeControl(0, L"BUTTON", L"Выбранные файлы и папки", BS_GROUPBOX, 12, 106, 680, 275, -1);
    MakeControl(0, L"BUTTON", L"Добавить файлы...", BS_PUSHBUTTON | WS_TABSTOP, 28, 130, 145, 28, ID_FILES);
    MakeControl(0, L"BUTTON", L"Добавить папку...", BS_PUSHBUTTON | WS_TABSTOP, 182, 130, 145, 28, ID_FOLDER);
    MakeControl(0, L"BUTTON", L"Очистить список", BS_PUSHBUTTON | WS_TABSTOP, 336, 130, 145, 28, ID_CLEAR);
    g_list = MakeControl(WS_EX_CLIENTEDGE, L"LISTBOX", L"", LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | WS_TABSTOP, 28, 166, 645, 198, ID_LIST);

    g_status = MakeControl(0, L"STATIC", L"Выбери файлы или папку. Оригиналы всегда сохраняются.", SS_LEFT, 16, 390, 660, 24, ID_STATUS);
    MakeControl(0, L"BUTTON", L"Зашифровать копии", BS_DEFPUSHBUTTON | WS_TABSTOP, 435, 425, 150, 32, ID_START);
    MakeControl(0, L"BUTTON", L"Выход", BS_PUSHBUTTON | WS_TABSTOP, 595, 425, 90, 32, IDCANCEL);
    ToggleMode();
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_main = hwnd;
        CreateControls(hwnd);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_FILES: PickFiles(); return 0;
        case ID_FOLDER: PickFolder(); return 0;
        case ID_CLEAR: g_selected.clear(); RefreshList(); return 0;
        case ID_ENCRYPT:
        case ID_DECRYPT: ToggleMode(); return 0;
        case ID_SHOW: TogglePassword(); return 0;
        case ID_START: StartWork(); return 0;
        case IDCANCEL: DestroyWindow(hwnd); return 0;
        }
        break;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"USBEncryptorClassicWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    if (!RegisterClassW(&wc)) { CoUninitialize(); return 1; }

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, APP_TITLE,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 720, 510, nullptr, nullptr, instance, nullptr);
    if (!hwnd) { CoUninitialize(); return 1; }
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
