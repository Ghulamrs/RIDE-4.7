; Inno Setup script for RIDE 4.7 - three languages (C, C++, Shalimar),
; four targets (x86_64-windows, x86_64-linux, arm64-darwin, tms6747), the
; VM6747 C6000 emulator, the C<->Shalimar converter, and the project's own
; assemblers for both machine targets: asm6x for the C6000 and, new in 4.0,
; masm for x86-64, which the installed settings.json names in place of ml64,
; and the project's own linkers beside them, link.exe (LINK, x86-64) and
; lnk6x.exe (LNK6x, C6000), named too: ours are the tools by default, and
; "askNative": true has a failure of ours ask for the vendor's tools.
; The product's name, once: the programs are {#PRODUCT}.exe and
; {#PRODUCT}Console.exe. The Makefile's PRODUCT, product.props and
; src/product.h spell it the same.
#define PRODUCT "RIDE"
#define MyVer  "4.7"
#define MyName PRODUCT + " " + MyVer
#ifndef Stage
#define Stage "C:\Users\GRA\ride-pkg\stage40"
#endif
#ifndef OutDir
#define OutDir "C:\Users\GRA\ride-pkg"
#endif

[Setup]
AppId={{A2A1C6E8-9837-457A-B0A0-01B3093C6DA0}
AppName={#MyName}
AppVersion={#MyVer}
AppPublisher=G. R. Akhtar
DefaultDirName={autopf}\{#MyName}
DefaultGroupName={#MyName}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\bin\{#PRODUCT}.exe
SetupIconFile=..\..\winforms\ride.ico
OutputDir={#OutDir}
OutputBaseFilename={#PRODUCT}-{#MyVer}-setup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
WizardStyle=modern
; Tell Explorer the environment changed, so a new console sees the PATH.
ChangesEnvironment=yes
; The per-user areas are meant: the PATH entry is the user's own (HKCU), and the shortcuts start in their Documents.
UsedUserAreasWarning=no
LicenseFile={#Stage}\README.md

[Files]
Source: "{#Stage}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[InstallDelete]
; The samples are the installer's own, so an upgrade replaces them whole: what an earlier
; install had and this one does not - examples\, the CCS samples that left - goes. The
; user's copies in Documents\RIDE are not touched.
Type: filesandordirs; Name: "{app}\examples"
Type: filesandordirs; Name: "{app}\projects"
Type: filesandordirs; Name: "{app}\programs"

[Dirs]
; Projects and single programs are Documents\RIDE\projects and Documents\RIDE\programs,
; filled by the editor from {app}\projects and {app}\programs on first use - the user's own,
; where a build can write. There is no examples\ since 03-10-2026, so nothing under
; Program Files needs to be writable, and the shortcuts start in Documents.

[Icons]
Name: "{group}\{#MyName}"; Filename: "{app}\bin\{#PRODUCT}.exe"; WorkingDir: "{userdocs}"
Name: "{group}\{#MyName} (console)"; Filename: "{app}\bin\{#PRODUCT}Console.exe"; WorkingDir: "{userdocs}"
Name: "{group}\Express Help"; Filename: "{app}\EXPRESS-HELP.html"; WorkingDir: "{app}"
Name: "{group}\Manual"; Filename: "{app}\help\manual.html"; WorkingDir: "{app}"
Name: "{group}\User Guide"; Filename: "{app}\help\guide.html"; WorkingDir: "{app}"
Name: "{group}\Resources"; Filename: "{app}\help\resources.html"; WorkingDir: "{app}"
Name: "{group}\Uninstall {#MyName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyName}"; Filename: "{app}\bin\{#PRODUCT}.exe"; WorkingDir: "{userdocs}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"
; bin holds a link.exe of the project's own. Appended, it sits after everything
; already on PATH, and a Developer Command Prompt puts Microsoft's first anyway.
Name: "addtopath"; Description: "Add the bin folder to PATH (c90, cpp11, shalimar, masm, asm6x, vm6747, c2s on the command line)"; Flags: unchecked

[Registry]
; The user's own PATH, HKCU\Environment, on purpose. The machine-wide one is
; not "HKLM\Environment" but HKLM\SYSTEM\CurrentControlSet\Control\Session
; Manager\Environment, so with the installer elevated (Program Files) the
; earlier "HKA" spelling wrote a key nothing reads and PATH never changed.
; Every account that installs RIDE already has an HKCU Path, which is where a
; per-user tool belongs anyway; NeedsAddPath reads the same hive.
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}\bin"; \
    Tasks: addtopath; Check: NeedsAddPath(ExpandConstant('{app}\bin'))

[Run]
Filename: "{app}\bin\{#PRODUCT}.exe"; Description: "Launch {#MyName}"; Flags: nowait postinstall skipifsilent

[Code]
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', OrigPath) then
  begin
    Result := True;
    exit;
  end;
  { true only if the bin folder is not already on PATH }
  Result := Pos(';' + Uppercase(Param) + ';', ';' + Uppercase(OrigPath) + ';') = 0;
end;

{ Take the bin folder back out of the user's PATH on uninstall; Inno restores
  nothing on its own, and leaving a dead entry is what "addtopath" would
  otherwise cost the next install into a different folder. At usUninstall,
  not usPostUninstall: tried on the box, the later step never reached this
  code and PATH kept the entry. The Log lines land in the uninstall log. }
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Bin, Path: string;
  P: Integer;
begin
  if CurUninstallStep <> usUninstall then exit;
  Bin := ExpandConstant('{app}\bin');
  if not RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', Path) then exit;
  P := Pos(';' + Uppercase(Bin) + ';', ';' + Uppercase(Path) + ';');
  Log('PATH entry ' + Bin + ' found at ' + IntToStr(P) + ' in ' + Path);
  if P = 0 then exit;
  { P counts from the leading ';' we added; the entry starts at P in Path
    when it is first, else the ';' before it is at P-1 }
  if P = 1 then
    Delete(Path, 1, Length(Bin) + 1)   { "bin;" at the front, or all of it }
  else
    Delete(Path, P - 1, Length(Bin) + 1);  { ";bin" }
  if RegWriteExpandStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', Path) then
    Log('PATH is now ' + Path)
  else
    Log('PATH could not be written');
end;
