[Setup]
; App Information
AppName=M3u Video Player PC
AppVersion=1.0
AppPublisher=Akash
DefaultDirName={autopf}\M3uVideoPlayerPc
DefaultGroupName=M3u Video Player PC
OutputDir=.\Installer
OutputBaseFilename=M3uVideoPlayer_Setup
Compression=lzma
SolidCompression=yes
SetupIconFile=src\assets\favicon.ico
UninstallDisplayIcon={app}\M3uVideoPlayerPc.exe
PrivilegesRequired=lowest

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Copy the main executable and all DLLs/folders from the release directory
Source: "M3uVideoPlayerPc_Release\M3uVideoPlayerPc.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "M3uVideoPlayerPc_Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "src\assets\favicon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; Start Menu Shortcut
Name: "{group}\M3u Video Player PC"; Filename: "{app}\M3uVideoPlayerPc.exe"; IconFilename: "{app}\favicon.ico"
; Desktop Shortcut (if user checked the box)
Name: "{autodesktop}\M3u Video Player PC"; Filename: "{app}\M3uVideoPlayerPc.exe"; IconFilename: "{app}\favicon.ico"; Tasks: desktopicon

[Run]
; Option to launch the app immediately after installation
Filename: "{app}\M3uVideoPlayerPc.exe"; Description: "{cm:LaunchProgram,M3u Video Player PC}"; Flags: nowait postinstall skipifsilent

[Registry]
; Register the application to be available in the "Open With" list
Root: HKCU; Subkey: "Software\Classes\Applications\M3uVideoPlayerPc.exe\SupportedTypes"; ValueType: string; ValueName: ".m3u"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\M3uVideoPlayerPc.exe\SupportedTypes"; ValueType: string; ValueName: ".m3u8"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\M3uVideoPlayerPc.exe\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\M3uVideoPlayerPc.exe"" ""%1"""; Flags: uninsdeletekey
; Set FriendlyAppName and Icon so the "Open With" dialog looks nice
Root: HKCU; Subkey: "Software\Classes\Applications\M3uVideoPlayerPc.exe"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "M3U Video Player PC"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\M3uVideoPlayerPc.exe\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\M3uVideoPlayerPc.exe,0"; Flags: uninsdeletekey
