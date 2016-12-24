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
	mov eax, 6
	push eax
l1:
	mov eax, [esp]
	sub eax, 1
	mov [esp], eax
	mov eax, 0
	cmp [esp], eax
	jl l2
	jmp l1
l2:
	add esp, 4

	call ExitProcess
end start