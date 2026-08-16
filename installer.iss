[Setup]
; General Configuration
AppName=USBIP Client
AppVersion=1.0
AppPublisher=Tyler Long
DefaultDirName={autopf}\USBIP Client
DefaultGroupName=USBIP Client
OutputDir=C:\workspace\build
OutputBaseFilename=USBIPClient_Installer_v1.0
Compression=lzma2
SolidCompression=yes

; CRITICAL: Force the installer to require Admin rights 
; This ensures it can install to Program Files and that the user 
; has permission to execute your driver SetupAPI code later.
PrivilegesRequired=admin

; Optional UI tweaks
WizardStyle=modern
UninstallDisplayIcon={app}\USBIPClient.exe

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Copy the main executable
Source: "C:\workspace\build\Release\USBIPClient.exe"; DestDir: "{app}"; Flags: ignoreversion

; Recursively copy all Qt DLLs, dependencies, and the entire 'driver' folder
Source: "C:\workspace\build\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start Menu Shortcut
Name: "{group}\USBIP Client"; Filename: "{app}\USBIPClient.exe"
; Desktop Shortcut
Name: "{autodesktop}\USBIP Client"; Filename: "{app}\USBIPClient.exe"; Tasks: desktopicon

[Run]
; Option to launch the application immediately after installation finishes
Filename: "{app}\USBIPClient.exe"; Description: "{cm:LaunchProgram,USBIP Client}"; Flags: nowait postinstall skipifsilent