; Inno Setup script for Minesweeper.
; Build from the repo root after `cmake --build build`:
;   "C:\Users\yoel2\AppData\Local\Programs\Inno Setup 6\ISCC.exe" /DMyAppVersion=1.0.2 installer\minesweeper.iss
; Output: build\installer\minesweeper-setup-<version>.exe

#ifndef MyAppVersion
  #define MyAppVersion "1.0.2"
#endif

#define MyAppName "Minesweeper"
#define MyAppPublisher "Yoel Hartman"
#define MyAppExeName "minesweeper.exe"

[Setup]
AppId={{6F3C2B7E-9A41-4D8B-B2E5-3C1F0A7D9E42}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/yoel2810/minesweeper
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\build\installer
OutputBaseFilename=minesweeper-setup-{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
UninstallDisplayIcon={app}\{#MyAppExeName}
SetupIconFile=..\assets\icon\minesweeper.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "..\build\bin\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\assets\fonts\VT323-Regular.ttf"; DestDir: "{app}\assets\fonts"; Flags: ignoreversion
Source: "..\assets\fonts\VT323-OFL.txt"; DestDir: "{app}\assets\fonts"; Flags: ignoreversion

[Icons]
; The game finds assets\fonts\ next to its exe, so WorkingDir is not required; it is kept for tidiness.
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent
