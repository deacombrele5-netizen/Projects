#include "crypto.h"
#include <windows.h>
#include <bcrypt.h>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

#pragma comment(lib, "bcrypt.lib")

namespace {
#pragma pack(push, 1)
struct Header {
    char magic[8];
    std::uint32_t version;
    std::uint32_t iterations;
    std::uint64_t dataSize;
    unsigned char salt[16];
    unsigned char nonce[12];
    unsigned char tag[16];
};
#pragma pack(pop)
constexpr std::uint32_t KdfIterations = 600000;
constexpr char Magic[8] = {'U','S','B','E','N','C','1','\0'};

bool RandomBytes(unsigned char* p, ULONG n) {
    return BCryptGenRandom(nullptr, p, n, BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0;
}
bool DeriveKey(const std::wstring& password, const Header& h, unsigned char key[32]) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0) return false;
    const auto* bytes = reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(password.data()));
    ULONG length = static_cast<ULONG>(password.size() * sizeof(wchar_t));
    NTSTATUS s = BCryptDeriveKeyPBKDF2(alg, bytes, length,
        const_cast<PUCHAR>(h.salt), sizeof(h.salt), h.iterations, key, 32, 0);
    BCryptCloseAlgorithmProvider(alg, 0);
    return s >= 0;
}
bool CryptGcm(bool encrypt, const Header& h, const unsigned char key[32],
              const std::vector<unsigned char>& in, std::vector<unsigned char>& out,
              unsigned char tag[16]) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_KEY_HANDLE handle = nullptr;
    bool ok = false;
    ULONG written = 0;
    NTSTATUS status = 0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, nullptr, 0) < 0) return false;
    wchar_t mode[] = BCRYPT_CHAIN_MODE_GCM;
    if (BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(mode), sizeof(mode), 0) < 0) goto done;
    if (BCryptGenerateSymmetricKey(alg, &handle, nullptr, 0, const_cast<PUCHAR>(key), 32, 0) < 0) goto done;
    out.resize(in.size());
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth;
    BCRYPT_INIT_AUTH_MODE_INFO(auth);
    auth.pbNonce = const_cast<PUCHAR>(h.nonce);
    auth.cbNonce = sizeof(h.nonce);
    auth.pbAuthData = reinterpret_cast<PUCHAR>(const_cast<Header*>(&h));
    auth.cbAuthData = static_cast<ULONG>(offsetof(Header, tag));
    auth.pbTag = tag;
    auth.cbTag = 16;
    status = encrypt
        ? BCryptEncrypt(handle, const_cast<PUCHAR>(in.data()), static_cast<ULONG>(in.size()), &auth, nullptr, 0,
                        out.data(), static_cast<ULONG>(out.size()), &written, 0)
        : BCryptDecrypt(handle, const_cast<PUCHAR>(in.data()), static_cast<ULONG>(in.size()), &auth, nullptr, 0,
                        out.data(), static_cast<ULONG>(out.size()), &written, 0);
    if (status >= 0 && written == in.size()) ok = true;
done:
    if (handle) BCryptDestroyKey(handle);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    return ok;
}
bool ReadAll(const std::filesystem::path& p, std::vector<unsigned char>& data, std::wstring& error) {
    std::error_code ec;
    auto size = std::filesystem::file_size(p, ec);
    if (ec || size > crypto::MaxFileBytes + sizeof(Header)) { error = L"Файл слишком большой или недоступен (лимит 256 МиБ)."; return false; }
    std::ifstream f(p, std::ios::binary);
    if (!f) { error = L"Не удалось открыть исходный файл."; return false; }
    data.resize(static_cast<size_t>(size));
    if (size && !f.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size))) { error = L"Ошибка чтения файла."; return false; }
    return true;
}
bool WriteNew(const std::filesystem::path& wanted, const std::vector<unsigned char>& data, std::wstring& output, std::wstring& error) {
    std::filesystem::path candidate = wanted;
    for (unsigned i=1; std::filesystem::exists(candidate); ++i) {
        candidate = wanted.parent_path() / (wanted.stem().wstring() + L" (" + std::to_wstring(i) + L")" + wanted.extension().wstring());
        if (i > 10000) { error = L"Не удалось подобрать свободное имя результата."; return false; }
    }
    auto temp = candidate; temp += L".usbtmp";
    {
        std::ofstream f(temp, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!f) { error = L"Не удалось создать временный файл результата."; return false; }
        if (!data.empty()) f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        f.flush();
        if (!f) { f.close(); std::filesystem::remove(temp); error = L"Ошибка записи результата."; return false; }
    }
    if (!MoveFileExW(temp.c_str(), candidate.c_str(), MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temp); error = L"Не удалось безопасно завершить запись результата."; return false;
    }
    output = candidate.wstring();
    return true;
}
void Wipe(std::vector<unsigned char>& v) { if (!v.empty()) SecureZeroMemory(v.data(), v.size()); }
}

namespace crypto {
bool EncryptFile(const std::filesystem::path& input, const std::wstring& password, std::wstring& output, std::wstring& error) {
    std::vector<unsigned char> plain, cipher;
    if (!ReadAll(input, plain, error)) return false;
    Header h{}; memcpy(h.magic, Magic, sizeof(Magic)); h.version=1; h.iterations=KdfIterations; h.dataSize=plain.size();
    if (!RandomBytes(h.salt, sizeof(h.salt)) || !RandomBytes(h.nonce, sizeof(h.nonce))) { error=L"Не удалось получить случайные данные от Windows."; Wipe(plain); return false; }
    unsigned char key[32]{};
    if (!DeriveKey(password, h, key)) { error=L"Не удалось подготовить ключ шифрования."; Wipe(plain); return false; }
    unsigned char tag[16]{};
    bool ok = CryptGcm(true, h, key, plain, cipher, tag);
    SecureZeroMemory(key, sizeof(key)); Wipe(plain);
    if (!ok) { error=L"Ошибка AES-GCM при шифровании."; Wipe(cipher); return false; }
    memcpy(h.tag, tag, sizeof(tag)); SecureZeroMemory(tag, sizeof(tag));
    std::vector<unsigned char> packed(sizeof(h)+cipher.size());
    memcpy(packed.data(), &h, sizeof(h));
    if (!cipher.empty()) memcpy(packed.data()+sizeof(h), cipher.data(), cipher.size());
    Wipe(cipher);
    auto dest=input; dest += L".locked";
    bool result=WriteNew(dest, packed, output, error); Wipe(packed); return result;
}
bool DecryptFile(const std::filesystem::path& input, const std::wstring& password, std::wstring& output, std::wstring& error) {
    std::vector<unsigned char> packed, plain;
    if (!ReadAll(input, packed, error)) return false;
    if (packed.size()<sizeof(Header)) { error=L"Это не файл USB Encryptor или он повреждён."; Wipe(packed); return false; }
    Header h{}; memcpy(&h, packed.data(), sizeof(h));
    if (memcmp(h.magic, Magic, sizeof(Magic)) || h.version!=1 || h.iterations<100000 || h.iterations>2000000 ||
        h.dataSize>MaxFileBytes || h.dataSize!=packed.size()-sizeof(Header)) {
        error=L"Неподдерживаемый или повреждённый формат файла."; Wipe(packed); return false;
    }
    std::vector<unsigned char> cipher(packed.begin()+sizeof(Header), packed.end()); Wipe(packed);
    unsigned char key[32]{}, tag[16]{}; memcpy(tag,h.tag,sizeof(tag));
    if (!DeriveKey(password,h,key)) { error=L"Не удалось подготовить ключ расшифровки."; Wipe(cipher); return false; }
    bool ok=CryptGcm(false,h,key,cipher,plain,tag);
    SecureZeroMemory(key,sizeof(key)); SecureZeroMemory(tag,sizeof(tag)); Wipe(cipher);
    if (!ok) { error=L"Неверный пароль или файл повреждён. Исходник не изменён."; Wipe(plain); return false; }
    std::filesystem::path dest=input;
    if (dest.extension()==L".locked") dest.replace_extension();
    else dest += L".restored";
    bool result=WriteNew(dest,plain,output,error); Wipe(plain); return result;
}
}
