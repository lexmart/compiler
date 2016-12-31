.386
.model flat, stdcall
option casemap:none
include C:\masm32\include\windows.inc
include C:\masm32\include\kernel32.inc
includelib C:\masm32\lib\kernel32.lib
include C:\masm32\include\msvcrt.inc
includelib C:\masm32\lib\msvcrt.lib
.data
IntegerFormat db "%d", 13, 10, 0
X DWORD 2
Z DWORD 7
.code
MAIN:
L0:
        MOV eax, X
        PUSH eax
        MOV eax, Z
        POP ebx
        CMP ebx, eax
        MOV eax, 0
        SETL al
        IMUL eax, -1
        CMP eax, -1
        JNE L1
        MOV eax, X
        PUSH eax
        MOV eax, 1
        POP ebx
        ADD eax, ebx
        MOV X, eax
        JMP L0
L1:
        MOV eax, X
        PUSH eax
        MOV eax, 1
        POP ebx
        ADD eax, ebx
        MOV X, eax
        call ExitProcess
end MAIN