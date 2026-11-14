#pragma once
#include <filesystem>
#include <string>

namespace crypto {
constexpr unsigned long long MaxFileBytes = 256ull * 1024ull * 1024ull;
bool EncryptFile(const std::filesystem::path& input, const std::wstring& password, std::wstring& output, std::wstring& error);
bool DecryptFile(const std::filesystem::path& input, const std::wstring& password, std::wstring& output, std::wstring& error);
}
