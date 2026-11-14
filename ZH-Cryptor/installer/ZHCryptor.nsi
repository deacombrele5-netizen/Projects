!include "MUI2.nsh"
Name "ZH Cryptor"
OutFile "ZH-Cryptor-Setup.exe"
InstallDir "$LOCALAPPDATA\Programs\ZH Cryptor"
RequestExecutionLevel user
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Russian"
Section "ZH Cryptor"
  SetOutPath "$INSTDIR"
  File "..\..\dist\ZHCryptor.exe"
  CreateShortCut "$SMPROGRAMS\ZH Cryptor.lnk" "$INSTDIR\ZHCryptor.exe"
SectionEnd
Section "Uninstall"
  Delete "$SMPROGRAMS\ZH Cryptor.lnk"
  Delete "$INSTDIR\ZHCryptor.exe"
  RMDir "$INSTDIR"
SectionEnd
