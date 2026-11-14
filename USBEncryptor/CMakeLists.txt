cmake_minimum_required(VERSION 3.20)
project(USBEncryptor VERSION 0.1.0 LANGUAGES CXX)
if(NOT WIN32)
  message(FATAL_ERROR "USB Encryptor is a Windows-only application.")
endif()
add_executable(USBEncryptor WIN32
  src/main.cpp
  src/crypto.cpp
  src/shell_integration.cpp
)
target_compile_features(USBEncryptor PRIVATE cxx_std_17)
target_compile_definitions(USBEncryptor PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601 WINVER=0x0601)
target_compile_options(USBEncryptor PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(USBEncryptor PRIVATE bcrypt comctl32 comdlg32 shell32 advapi32)
set_property(TARGET USBEncryptor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
