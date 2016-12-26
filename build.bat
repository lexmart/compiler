@echo off

set WarningFlags=-nologo -WX -W4 -wd4201 -wd4189 -wd4505 -wd4238 -wd4100 -wd4026 -wd4244 -wd4996 -wd4127 -wd4101

REM -FC = Full path of source file in diagnostics (so emacs can parse errors/warning)
REM -Zi = Creates debug information for Visual Studio debugger (Do I need to turn this off is release builds?)
REM -LD = Build DLL file
REM -Od = Turn off all optimizations
REM -incremental:no = To stop annyoing full link message
REM Zp[1|2|4|8|16] pad struct on x-byte boundries

set CompilerFlags=%WarningFlags% %VariableFlags% -FC -Zi -Od
set LinkerFlags=-incremental:no
set ExternalLibraries=

pushd build

cl %CompilerFlags% ../lexer_and_parser.cpp /link /SUBSYSTEM:CONSOLE

popd