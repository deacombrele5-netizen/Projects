!include "MUI2.nsh"
Name "ZH Cryptor"
OutFile "ZH-Cryptor-Setup.exe"
InstallDir "$LOCALAPPDATA\Programs\ZH Cryptor"
RequestExecutionLevel user
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Russian"

Section "ZH Cryptor" SEC_MAIN
  SetOutPath "$INSTDIR"
  File "..\..\dist\ZHCryptor.exe"
  CreateDirectory "$SMPROGRAMS\ZH Cryptor"
  CreateShortCut "$SMPROGRAMS\ZH Cryptor\ZH Cryptor.lnk" "$INSTDIR\ZHCryptor.exe"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZH Cryptor" "DisplayName" "ZH Cryptor"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZH Cryptor" "UninstallString" '"$INSTDIR\Uninstall.exe"'
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\ZH Cryptor\ZH Cryptor.lnk"
  RMDir "$SMPROGRAMS\ZH Cryptor"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$INSTDIR\ZHCryptor.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZH Cryptor"
SectionEnd
