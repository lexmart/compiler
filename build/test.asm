.386
.model flat, stdcall
option casemap:none
include C:\masm32\include\windows.inc
include C:\masm32\include\kernel32.inc
includelib C:\masm32\lib\kernel32.lib
include C:\masm32\include\msvcrt.inc
includelib C:\masm32\lib\msvcrt.lib
.data
X DWORD 8
.code
start:
        MOV eax, X
        PUSH eax
        MOV eax, 2
        POP ebx
        ADD eax, ebx
        MOV X, eax
        call ExitProcess
end start