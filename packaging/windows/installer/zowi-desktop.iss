; Zowi Desktop Inno Setup script
; Requires: Inno Setup 6+ (https://jrsoftware.org/isinfo.php)
;
; Build: iscc.exe /DVERSION=x.y.z zowi-desktop.iss
;   VERSION is injected by the build script or CI pipeline.

#ifndef VERSION
  #define VERSION "0.0.0"
#endif

#define APP_NAME    "Zowi Desktop"
#define APP_EXE     "ZowiDesktop.exe"
#define APP_PUBLISHER "Eduardo"
#define APP_URL     "https://eduardomillan.github.io/ZowiDesktop/docs/"
#define APP_ID      "ZowiDesktop"

[Setup]
AppId={{7B3E4A2D-9F81-4E6C-A5D0-2C8B1F7E3A94}
AppName={#APP_NAME}
AppVersion={#VERSION}
AppPublisher={#APP_PUBLISHER}
AppPublisherURL={#APP_URL}
AppSupportURL={#APP_URL}
DefaultDirName={autopf}\{#APP_NAME}
DefaultGroupName={#APP_NAME}
LicenseFile=..\..\..\LICENSE
OutputDir=..\..\..\dist-installer
OutputBaseFilename=ZowiDesktop-{#VERSION}-setup-x64
SetupIconFile=..\..\..\images\app_icon.ico
UninstallDisplayIcon={app}\{#APP_EXE}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "french";  MessagesFile: "compiler:Languages\French.isl"

[Files]
; The portable dist/ directory — everything windeployqt collected
Source: "..\..\..\dist\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#APP_NAME}";        Filename: "{app}\{#APP_EXE}"
Name: "{group}\Uninstall {#APP_NAME}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#APP_NAME}";  Filename: "{app}\{#APP_EXE}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; Flags: checkedonce

[Run]
Filename: "{app}\{#APP_EXE}"; Description: "Launch {#APP_NAME}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
