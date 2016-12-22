#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "cradle.h"

static char Look;

static void
GetChar()
{
    Look = getc(stdin);
}

static void
Error(char *Str)
{
    printf("Error: %s.\n", Str);
}

static void
Abort(char *Str)
{
    Error(Str);
    exit(0);
}


static void
Expected(char *Str)
{
    printf("%s Expected.\n", Str);
    exit(0);
}

static void
Expected(char C)
{
    char Str[4] = {'\'', C, '\'', 0};
    Expected(&Str[0]);
}

static void
Match(char C)
{
    if(Look == C)
    {
        GetChar();
        SkipBlank();
    }
    else
    {
        Expected(C);
    }
}

static bool
IsAlpha(char C)
{
    bool Result = (((C >= 'a') && (C <= 'z')) ||
                   ((C >= 'A') && (C <= 'Z')));
    
    return Result;
}

static bool
IsDigit(char C)
{
    bool Result = ((C >= '0') && (C <= '9'));
    
    return Result;
}

static bool
IsAlphaNumeric(char C)
{
    bool Result = (IsAlpha(C) || IsDigit(C));
    
    return Result;
}

static bool
IsBlank(char C)
{
    bool Result = ((C == '\t') || (C == ' '));
    
    return Result;
}

static bool
IsAddop(char C)
{
    bool Result = ((C == '+') || (C == '-'));
    
    return Result;
}

static bool
IsMulop(char C)
{
    bool Result = ((C == '*') || (C == '/'));
    
    return Result;
}

static void
SkipBlank()
{
    while(IsBlank(Look))
    {
        GetChar();
    }
}

static void
GetName(char *Token)
{
    if(!IsAlpha(Look))
    {
        Expected("Name");
    }
    
    int InsertIndex = 0;
    while(IsAlphaNumeric(Look))
    {
        Assert(InsertIndex < MaxTokenLength);
        Token[InsertIndex++] = toupper(Look);
        GetChar();
    }
    Token[InsertIndex] = 0;
    
    SkipBlank();
}

static void
GetNum(char *Token)
{
    if(!IsDigit(Look))
    {
        Expected("Integer");
    }
    
    int InsertIndex = 0;
    while(IsDigit(Look))
    {
        Assert(InsertIndex < MaxTokenLength);
        Token[InsertIndex++] = Look;
        GetChar();
    }
    Token[InsertIndex] = 0;
    
    SkipBlank();
}

static void
EmitNoTab(char *Str)
{
    printf("%s\n", Str);
}

static void
Emit(char *Str)
{
    printf("\t%s", Str);
}

static void
EmitLn(char *Str)
{
    Emit(Str);
    printf("\n");
}

static void
Init()
{
    GetChar();
    SkipBlank();
}

static void
EmitInstruction(char *Name, char *Param1)
{
    char Line[1024];
    sprintf(Line, "%s %s", Name, Param1);
    EmitLn(Line);
}

static void
EmitInstruction(char *Name, char Param1)
{
    char Line[1024];
    sprintf(Line, "%s %c", Name, Param1);
    EmitLn(Line);
}

static void
EmitInstruction(char *Name, char *Param1, char *Param2)
{
    char Line[1024];
    sprintf(Line, "%s %s, %s", Name, Param1, Param2);
    EmitLn(Line);
}

static void
EmitInstruction(char *Name, char *Param1, char Param2)
{
    char Line[1024];
    sprintf(Line, "%s %s, %c", Name, Param1, Param2);
    EmitLn(Line);
}

static void
EmitInstruction(char *Name, char Param1, char *Param2)
{
    char Line[1024];
    sprintf(Line, "%s %c, %s", Name, Param1, Param2);
    EmitLn(Line);
}

static void
Identifier()
{
    char Name[MaxTokenLength];
    GetName(Name);
    if(Look == '(')
    {
        Match('(');
        Match(')');
        EmitInstruction("CALL", Name);
    }
    else
    {
        EmitInstruction("MOV", "eax", Name);
    }
}

static void
Factor()
{
    if(Look == '(')
    {
        Match('(');
        Expression();
        Match(')');
    }
    else if(IsAlpha(Look))
    {
        Identifier();
    }
    else
    {
        char Num[MaxTokenLength];
        GetNum(Num);
        EmitInstruction("MOV", "eax", Num);
    }
}

static void
Multiply()
{
    Match('*');
    Factor();
    EmitLn("POP ebx");
    // NOTE: MUL means put eax/ebx into eax
    EmitLn("MUL ebx");
}

static void
Divide()
{
    Match('/');
    Factor();
    EmitLn("POP ebx");
    // NOTE: MUL means put eax/ebx into eax
    EmitLn("DIV ebx");
}

static void
Term()
{
    Factor();
    
    while(IsMulop(Look))
    {
        EmitLn("PUSH eax");
        if(Look == '*')
        {
            Multiply();
        }
        else if(Look == '/')
        {
            Divide();
        }
    }
}

static void
Add()
{
    Match('+');
    Term();
    
    EmitInstruction("POP", "ebx");
    EmitInstruction("ADD", "eax", "ebx");
}

static void
Subtract()
{
    Match('-');
    Term();
    
    EmitInstruction("POP", "ebx");
    EmitInstruction("SUB", "eax", "ebx");
    EmitInstruction("NEG", "eax");
}

static void
Expression()
{
    if(IsAddop(Look))
    {
        EmitInstruction("MOV", "eax", "0");
    }
    else
    {
        Term();
    }
    
    while(IsAddop(Look))
    {
        EmitInstruction("PUSH", "eax");
        if(Look == '+')
        {
            Add();
        }
        else if(Look == '-')
        {
            Subtract();
        }
    }
}

static void
Assignment()
{
    char Name[1024];
    GetName(Name);
    Match('=');
    Expression();
    EmitInstruction("MOV", Name, "eax");
}

void
main(int NumArguments, char **Arguments)
{
    EmitNoTab(".386");
    EmitNoTab(".model flat, stdcall");
    EmitNoTab("option casemap:none");
    EmitNoTab("include C:\\masm32\\include\\windows.inc");
    EmitNoTab("include C:\\masm32\\include\\kernel32.inc");
    EmitNoTab("includelib C:\\masm32\\lib\\kernel32.lib");
    EmitNoTab("include C:\\masm32\\include\\msvcrt.inc");
    EmitNoTab("includelib C:\\masm32\\lib\\msvcrt.lib");
    EmitNoTab(".data");
    EmitNoTab(".code");
    EmitNoTab("start:");
    
    Init();
    Assignment();
    if(Look != '\n')
    {
        Expected("Newline");
    }
    
    EmitLn("call ExitProcess");
    EmitNoTab("end start");
}