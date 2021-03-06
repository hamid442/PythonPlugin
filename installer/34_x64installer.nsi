; Script generated with the Venis Install Wizard

; Define your application name
!define VERSION "0.2.11"
!define APPNAME "OBS Python Plugin"
!define APPNAMEANDVERSION "${APPNAME} ${VERSION}"

!define DLLDIR "$PROGRAMFILES64\OBS\plugins"
!define UNINSTALLDIR "$PROGRAMFILES64\OBS\plugins\PythonPlugin"
!define SCRIPTSDIR "$APPDATA\OBS\pluginData\Python"
!define SCRIPTSEXAMPLEDIR "$APPDATA\OBS\pluginData\Python\Examples"

; Main Install settings
Name "${APPNAMEANDVERSION}"
InstallDir "$PROGRAMFILES64\OBS\plugins"
InstallDirRegKey HKLM "Software\${APPNAME}" ""
OutFile "OBSPythonPlugin-${VERSION}.py3.4.x64.exe"

; Modern interface settings
!include "MUI.nsh"

!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "gplv2.txt"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Set languages (first is default language)
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_RESERVEFILE_LANGDLL

Section "OBS Python Plugin" Section1
	

	; Set Section properties
	SetOverwrite on

	; Copy DLL
	
	SetOutPath "${DLLDIR}\"
	File "..\PythonPlugin\x64\34Release\PythonPlugin.dll"
	
	;Copy Example Script
	
	SetOutPath "${SCRIPTSDIR}\"
	File "..\PythonPlugin\ScriptManager.py"	
	
	
	
	SetOutPath "${SCRIPTSEXAMPLEDIR}\"
	File "..\ExampleScripts\ColourSquare.py"
	File "..\ExampleScripts\DateTime.py"
	
	
	
	;Copy Unistaller	
	
	SetOutPath "${UNINSTALLDIR}\"	
	
	
	CreateDirectory "$SMPROGRAMS\OBS Python Plugin"
	CreateShortCut "$SMPROGRAMS\OBS Python Plugin\Uninstall.lnk" "${UNINSTALLDIR}\uninstall.exe"
	
	

SectionEnd

Section -FinishSection

	WriteRegStr HKLM "Software\${APPNAME}" "" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "${UNINSTALLDIR}\uninstall.exe"
	WriteUninstaller "${UNINSTALLDIR}\uninstall.exe"
	
	

SectionEnd

; Modern install component descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${Section1} ""
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;Uninstall section
Section Uninstall

	;Remove from registry...
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
	DeleteRegKey HKLM "SOFTWARE\${APPNAME}"

	; Delete self
	RMDir /r "${UNINSTALLDIR}"


	; Delete Shortcuts
	Delete "$SMPROGRAMS\OBS Python Plugin\Uninstall.lnk"

	; Clean up OBS Python Plugin
	Delete "${DLLDIR}\PythonPlugin.dll"


	; Remove remaining directories
	RMDir "$SMPROGRAMS\OBS Python Plugin"

	
	MessageBox MB_YESNO|MB_ICONQUESTION "Would you like to delete all user scripts in '${SCRIPTSDIR}' folder?" IDYES YesDelete IDNO NoDelete
	YesDelete:
		RMDir /r "${SCRIPTSDIR}"
	NoDelete:
	
	

SectionEnd

; eof