#define AppVer GetVersionNumbersString('..\bin\x64\textparser.exe')

[Setup]
AppName=textparser
AppVerName=textparser version {#AppVer}
AppPublisher=Boris Barbulovski
AppPublisherURL=https://github.com/bokic/textparser
AppVersion={#AppVer}
ArchitecturesAllowed=x64compatible arm64
ArchitecturesInstallIn64BitMode=x64compatible arm64
DefaultDirName={commonpf}\textparser
DefaultGroupName=textparser
UninstallDisplayIcon={app}\textparser.exe
Compression=lzma
SolidCompression=yes
OutputBaseFilename={#ReadIni(SourcePath + "installer.ini", "installer", "target_name ")}
OutputDir=.
VersionInfoVersion={#AppVer}
VersionInfoDescription={#ReadIni(SourcePath + "installer.ini", "installer", "description")}

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Flags: preservestringtype

[Setup]
ChangesEnvironment=yes

[Dirs]
Name: "{app}";

[Files]
Source: "..\bin\x64\textparser.exe"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\ccat.exe"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\textparser.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\textparser-json.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\textparser_cfml.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\textparser_php.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\textparser_html.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\textparser_css.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\json-c.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\pcre2-8.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\pcre2-16.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\pcre2-32.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\x64\pcre2-posix.dll"; DestDir: "{app}"; Check: IsX64OS
Source: "..\bin\arm64\textparser.exe"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\ccat.exe"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\textparser.dll"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\textparser-json.dll"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\textparser_cfml.dll"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\textparser_php.dll"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\textparser_html.dll"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\textparser_css.dll"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\json-c.dll"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\pcre2-8.dll"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\pcre2-16.dll"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\pcre2-32.dll"; DestDir: "{app}"; Check: IsArm64
Source: "..\bin\arm64\pcre2-posix.dll"; DestDir: "{app}"; Check: IsArm64
