# USB Encryptor

Portable, offline Windows file/folder encryption utility with a classic Win32 interface.

## Current target
- Windows 7 SP1 and newer (x86/x64 builds planned).
- Windows XP and Vista are **not supported by this build**.
- No installation is required to run the executable. Explorer context-menu entries are optional and registered for the current Windows user only.

## Security and behavior
- AES-256-GCM authenticated encryption via Windows CNG.
- PBKDF2-HMAC-SHA-256, 600,000 iterations, random salt and nonce.
- Each file is encrypted independently into a sibling `.locked` file.
- Original files are never deleted or overwritten.
- Decryption writes a separate restored file and never removes the encrypted source.
- Folder mode processes files recursively; it does not encrypt directory metadata or create a single opaque folder container.
- The password is not recoverable. Losing it means losing access to the encrypted data.
- The application does not claim to decrypt files made by arbitrary third-party encryption programs. Only supported formats can be decrypted.

## Important limits
The first implementation buffers one file in memory and therefore caps a file at 256 MiB. This is a deliberate safety limit until streaming authenticated encryption is implemented and tested. Windows, antivirus software, indexing and shell history may leave operating-system traces; portable does not mean zero traces.

## Build
Use Visual Studio 2022 or newer with the Desktop development with C++ workload and CMake:
```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```
The GitHub Actions workflow builds x64 and Win32 artifacts.

## Explorer integration
Open the program and choose **Enable Explorer menu**. It writes per-user registry entries under HKCU; administrator rights are not required. Use **Remove Explorer menu** to undo them. The context menu is optional and does not run the program elevated.

## Status
A successful CI compilation confirms that the source compiles. It is not a security audit or proof of correct runtime behavior. Test only with disposable copies until the documented test matrix has passed.
