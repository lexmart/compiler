#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "cradle.h"

static char Look;
static int LabelCount = 0;

static void
GetChar()
{
    Look = getc(stdin);
    while(Look == '\n')
    {
        GetChar();
    }
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

static bool
IsBoolean(char C)
{
    C = toupper(C);
    bool Result = ((C == 'T') || (C == 'F'));
    
    return Result;
}

static bool
IsOrop(char C)
{
    bool Result = ((C == '|') || (C == '^'));
    
    return Result;
}

static bool
IsRelop(char C)
{
    bool Result = ((C == '=') || (C == '#') || (C == '<') || (C == '>'));
    
    return Result;
}

static char
GetName()
{
    if(!IsAlpha(Look))
    {
        Expected("Name");
    }
    
    char Result = toupper(Look);
    GetChar();
    
    return Result;
}

static void
GetNameWithBrackets(char *Result)
{
    char Name = GetName();
    
    Result[0] = '[';
    Result[1] = Name;
    Result[2] = ']';
}

static char
GetNum()
{
    if(!IsDigit(Look))
    {
        Expected("Integer");
    }
    
    char Result = Look;
    GetChar();
    
    return Result;
}

static bool
GetBoolean()
{
    bool Result = false;
    
    if(!IsBoolean(Look))
    {
        Expected("Boolean Literal");
    }
    else
    {
        Result = toupper(Look) == 'T';
    }
    
    GetChar();
    
    return Result;
}

static void
NewLabel(char *Label)
{
    sprintf(Label, "L%02d", LabelCount++);
}

static void
PostLabel(char *Label)
{
    printf("%s:\n", Label);
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
EmitLn(char C)
{
    printf("\t%c\n", C);
}

static void
Init()
{
    GetChar();
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
    char Name = GetName();
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
        char Num = GetNum();
        EmitInstruction("MOV", "eax", Num);
    }
}

static void
SignedFactor()
{
    if(Look == '+')
    {
        GetChar();
        Factor();
    }
    else if(Look == '-')
    {
        GetChar();
        Factor();
        // TODO: NEG is an op?
        EmitInstruction("IMUL", "eax", "-1");
    }
    else
    {
        Factor();
    }
}

static void
Multiply()
{
    Match('*');
    Factor();
    EmitLn("POP ebx");
    // NOTE: MUL means put eax*ebx into eax
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
    SignedFactor();
    
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
    Term();
    
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
Equals()
{
    Match('e');
    Expression();
    EmitInstruction("POP", "ebx");
    EmitInstruction("CMP", "eax", "ebx");
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETE", "eax");
    EmitInstruction("IMUL", "eax", "-1");
}

static void
NotEquals()
{
    Match('#');
    Expression();
    EmitInstruction("POP", "ebx");
    EmitInstruction("CMP", "eax", "ebx");
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETNE", "eax");
    EmitInstruction("IMUL", "eax", "-1");
}

static void
Less()
{
    Match('<');
    Expression();
    EmitInstruction("POP", "ebx");
    EmitInstruction("CMP", "eax", "ebx");
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETG", "eax");
    EmitInstruction("IMUL", "eax", "-1");
}

static void
Greater()
{
    Match('>');
    Expression();
    EmitInstruction("POP", "ebx");
    EmitInstruction("CMP", "eax", "ebx");
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETL", "eax");
    EmitInstruction("IMUL", "eax", "-1");
}

static void
Relation()
{
    Expression();
    if(IsRelop(Look))
    {
        EmitInstruction("PUSH", "eax");
        if(Look == '=')
        {
            Equals();
        }
        else if(Look == '#')
        {
            NotEquals();
        }
        else if(Look == '<')
        {
            Less();
        }
        else if(Look == '>')
        {
            Greater();
        }
        
        EmitInstruction("CMP", "eax", "-1");
    }
}

static void
BoolFactor()
{
    if(IsBoolean(Look))
    {
        // NOTE: We use -1 = true and 0 = false
        // NOTE: This is conveniant because -1 = 0xFFFFFFFF so NOT(-1) = 0 and NOT(0) = -1
        bool Result = GetBoolean();
        if(Result)
        {
            EmitInstruction("MOV", "eax", "-1");
        }
        else
        {
            EmitInstruction("MOV", "eax", "0");
        }
    }
    else
    {
        Relation();
    }
}

static void
NotFactor()
{
    if(Look == '!')
    {
        Match('!');
        BoolFactor();
        // NOTE: Flip the bits. Remember true = -1, false = 0.
        EmitInstruction("XOR", "eax", "-1");
    }
    else
    {
        BoolFactor();
    }
}

static void
BoolTerm()
{
    NotFactor();
    
    while(Look == '&')
    {
        Match('&');
        EmitInstruction("PUSH", "eax");
        NotFactor();
        EmitInstruction("POP", "ebx");
        EmitInstruction("AND", "eax", "ebx");
    }
}

static void
BoolXor()
{
    Match('^');
    BoolTerm();
    EmitInstruction("POP", "ebx");
    EmitInstruction("XOR", "eax", "ebx");
}

static void
BoolOr()
{
    Match('|');
    BoolTerm();
    EmitInstruction("POP", "ebx");
    EmitInstruction("OR", "eax", "ebx");
}

static void
BoolExpression()
{
    BoolTerm();
    while(IsOrop(Look))
    {
        EmitInstruction("PUSH", "eax");
        if(Look == '|')
        {
            BoolOr();
        }
        else if(Look == '^')
        {
            BoolXor();
        }
    }
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
    BoolExpression();
    if(Look != 'e')
    {
        Expected("END");
    }
    
    EmitInstruction("end", "start");
}