# ZH Cryptor

ZH Cryptor is a small, offline, portable Win32 utility for encrypting copies of files and the files inside folders.

## Features
- Classic Windows controls; Russian and English selector; light/dark theme toggle.
- AES-256-GCM authenticated encryption using Windows CNG.
- PBKDF2-HMAC-SHA-256 with 600,000 rounds and a random salt; each independently authenticated 4 MiB data block uses its own random AES-GCM nonce.
- Password confirmation when hidden; a single entry when password visibility is enabled.
- Optional per-user Explorer context-menu entries for files and folders (HKCU; no administrator rights).
- Encrypted `.zhc` files receive the embedded gold-lock file icon through a per-user Windows file association. Folder encryption adds a gold-lock folder icon using a hidden `desktop.ini` and `.zhcryptor.ico`; existing custom `desktop.ini` files are left untouched.
- Encrypts to a sibling `.zhc` file. Decrypts to a separate restored file.
- Never deletes or overwrites the source.
- No network access, cloud service, telemetry, settings files or application logs. Folder-icon decoration does add two hidden metadata files to the selected folder.
- x64 and Win32 builds; statically linked MSVC runtime.

## Compatibility
The supported target for this build is Windows 10 and Windows 11. Windows 7, Vista and XP are not claimed as compatible. Do not infer OS compatibility solely from the executable architecture.

## Limits and security notes
- File contents are processed in 4 MiB authenticated blocks, rather than loading the entire file into memory. There is no artificial 256 MiB application limit; practical limits are the filesystem, available disk space, and Windows APIs.
- Folder mode recursively encrypts eligible files one by one; it does not create one encrypted folder container. The original files remain in place.
- Only ZH Cryptor's own `.zhc` format is supported for decryption. A program cannot generically remove passwords from arbitrary formats made by unrelated encryption software.
- Passwords of one or more characters are accepted. Very short passwords are much easier to guess; use a long, unique passphrase for important data.
- Losing the password means the data cannot be recovered.
- Portable does not mean zero traces: Windows, Defender, file indexing and Explorer may record normal OS activity.
- Explorer integration is optional and writes only under the current user's registry hive. Use Remove menu to undo it.
- The app does not require elevation. Do not run it as administrator unless a specific file permission requires it.

## Build
Visual Studio 2022 with Desktop development with C++ and CMake:
```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```
For 32-bit: use `-A Win32`.

## Build the optional installer
1. Put the downloaded `ZHCryptor.exe` in the repository's `dist` folder (create the folder if needed).
2. Open the NSIS compiler and compile `installer/ZHCryptor.nsi`.
3. The script creates a per-user installer and a registered uninstaller; it does not request administrator privileges.

## Runtime validation
See [tests/README.md](tests/README.md). A successful CI build is only compile validation, not a cryptographic audit or proof of runtime correctness. Use disposable copies until the full test matrix has passed.
