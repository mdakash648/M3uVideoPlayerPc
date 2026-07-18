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
UninstallDisplayIcon={app}\M3uVideoPlayerPc.exe
PrivilegesRequired=lowest

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Copy the main executable and all DLLs/folders from the release directory
Source: "M3uVideoPlayerPc_Release\M3uVideoPlayerPc.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "M3uVideoPlayerPc_Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start Menu Shortcut
Name: "{group}\M3u Video Player PC"; Filename: "{app}\M3uVideoPlayerPc.exe"
; Desktop Shortcut (if user checked the box)
Name: "{autodesktop}\M3u Video Player PC"; Filename: "{app}\M3uVideoPlayerPc.exe"; Tasks: desktopicon

[Run]
; Option to launch the app immediately after installation
Filename: "{app}\M3uVideoPlayerPc.exe"; Description: "{cm:LaunchProgram,M3u Video Player PC}"; Flags: nowait postinstall skipifsilent
