.386
.model flat, stdcall
option casemap:none
include C:\masm32\include\windows.inc
include C:\masm32\include\kernel32.inc
includelib C:\masm32\lib\kernel32.lib
include C:\masm32\include\msvcrt.inc
includelib C:\masm32\lib\msvcrt.lib
.data
.code
start:
        MOV eax, 0
        PUSH eax
        MOV eax, 0
        PUSH eax
        MOV eax, 2
        PUSH eax
        MOV eax, 2
        POP ebx
        MUL ebx
        POP ebx
        SUB eax, ebx
        NEG eax
        PUSH eax
        MOV eax, 4
        POP ebx
        ADD eax, ebx
        PUSH eax
        MOV eax, 3
        POP ebx
        SUB eax, ebx
        NEG eax
        PUSH eax
        MOV eax, 2
        POP ebx
        ADD eax, ebx
        POP ebx
        SUB eax, ebx
        NEG eax
        call ExitProcess
end start