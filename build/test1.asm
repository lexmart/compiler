.386
.model flat, stdcall
option casemap:none
include C:\masm32\include\windows.inc
include C:\masm32\include\kernel32.inc
includelib C:\masm32\lib\kernel32.lib
include C:\masm32\include\msvcrt.inc
includelib C:\masm32\lib\msvcrt.lib
.data
PrintFormat db "%d", 13, 10, 0
ReadFormat db "%d", 0
FIB1 DWORD 1
FIB2 DWORD 1
TEMP DWORD ?
.code
MAIN:
L0:
	MOV eax, FIB2
	PUSH eax
	MOV eax, 432
	POP ebx
	CMP ebx, eax
	MOV eax, 0
	SETNE al
	IMUL eax, -1
	CMP eax, -1
	JNE L1
	MOV eax, FIB2
	MOV TEMP, eax
	MOV eax, FIB2
	PUSH eax
	MOV eax, FIB1
	POP ebx
	ADD eax, ebx
	MOV FIB2, eax
	MOV eax, TEMP
	MOV FIB1, eax
L2:
	MOV eax, FIB2
	PUSH eax
	MOV eax, 1000
	POP ebx
	CMP ebx, eax
	MOV eax, 0
	SETGE al
	IMUL eax, -1
	CMP eax, -1
	JNE L3
	MOV eax, FIB2
	PUSH eax
	MOV eax, 1000
	POP ebx
	SUB eax, ebx
	IMUL eax, -1
	MOV FIB2, eax
	JMP L2
L3:
L4:
	MOV eax, FIB1
	PUSH eax
	MOV eax, 1000
	POP ebx
	CMP ebx, eax
	MOV eax, 0
	SETGE al
	IMUL eax, -1
	CMP eax, -1
	JNE L5
	MOV eax, FIB1
	PUSH eax
	MOV eax, 1000
	POP ebx
	SUB eax, ebx
	IMUL eax, -1
	MOV FIB1, eax
	JMP L4
L5:
	PUSH FIB2
	LEA eax, PrintFormat
	PUSH eax
	CALL _imp__printf
	ADD ESP, 8
	JMP L0
L1:
	call ExitProcess
end MAIN
