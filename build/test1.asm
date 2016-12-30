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
X DWORD ?
Y DWORD ?
Z DWORD ?
.code
x:
	MOV eax, 4
	MOV X, eax
	MOV eax, 1
	MOV X, eax
	MOV eax, 1
	MOV Y, eax
L00:
	MOV eax, X
	PUSH eax
	MOV eax, 5
	POP ebx
	CMP ebx, eax
	MOV eax, 0
	SETL al
	IMUL eax, -1
	CMP eax, -1
	JNE L01
L02:
	MOV eax, Y
	PUSH eax
	MOV eax, 5
	POP ebx
	CMP ebx, eax
	MOV eax, 0
	SETL al
	IMUL eax, -1
	CMP eax, -1
	JNE L03
	MOV eax, X
	PUSH eax
	MOV eax, Y
	POP ebx
	IMUL eax, ebx
	MOV Z, eax
	MOV eax, Y
	PUSH eax
	MOV eax, 1
	POP ebx
	ADD eax, ebx
	MOV Y, eax
	PUSH [Z]
	LEA eax, [IntegerFormat]
	PUSH eax
	CALL _imp__printf
	ADD esp, 08h
	JMP L02
L03:
	MOV eax, X
	PUSH eax
	MOV eax, 1
	POP ebx
	ADD eax, ebx
	MOV X, eax
	MOV eax, 1
	MOV Y, eax
	JMP L00
L01:
	call ExitProcess
	end x
