#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// TINY Language definition:
// <program> ::= PROGRAM <top-level decls> <main> '.'
// <main> ::= BEGIN <block> END
// <top-level decls> ::= ( <data declaration> )*
// <data declaration> ::= VAR <var-list>
// <var-list> ::= <var> ( <var> )*
// <var> ::= <ident> [ = <integer> ]
// <block> ::= (Assignment)*

static char Look;
static int LabelCount = 0;
static bool SymbolTable[26] = {};

#define InputStream stdin
#define OutputStream stdout
#define MaxTokenLength 32

//
// Syntax assertions
//

static void
GetChar()
{
    Look = getc(InputStream);
}

static void
Abort(char *String)
{
    fprintf(OutputStream, "Error: %s.\n", String);
    exit(0);
}

static void
Expected(char *String)
{
    fprintf(OutputStream, "Expected: %s\n", String);
    exit(0);
}

static void
Match(char C)
{
    if(Look != C)
    {
        char ExpectedString[4] = {'\'', C, '\'', 0};
        Expected(ExpectedString);
    }
    GetChar();
}

static void
Undefined(char Name)
{
    printf("Undefined Identifier \'%c\'\n", Name);
    exit(0);
}

//
// "Is" Functions
//

static bool
IsAlpha(char C)
{
    C = toupper(C);
    
    bool Result = ((C >= 'A') && (C <= 'Z'));
    
    return Result;
}

static bool
IsDigit(char C)
{
    bool Result = ((C >= '0') && (C <= '9'));
    
    return Result;
}

static bool
IsMulop(char C)
{
    bool Result = ((C == '*') || (C == '/'));
    
    return Result;
}

static bool
IsAddop(char C)
{
    bool Result = ((C == '+') || (C == '-'));
    
    return Result;
}

static bool
InSymbolTable(char Symbol)
{
    bool Result = SymbolTable[Symbol - 'A'];
    
    return Result;
}

//
// "Get" Functions
//

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

static int
GetNumber()
{
    bool Negate = false;
    if(Look == '-')
    {
        Negate = true;
        Match('-');
    }
    
    if(!IsDigit(Look))
    {
        Expected("Integer");
    }
    
    int Result = 0;
    while(IsDigit(Look))
    {
        int Digit = Look - '0';
        Result = 10*Result + Digit;
        GetChar();
    }
    
    if(Negate)
    {
        Result = -Result;
    }
    
    return Result;
}

static void
NewLabel(char *Output)
{
    sprintf(Output, "%d", LabelCount++);
}

static void
PostLabel(char *Label)
{
    fprintf(OutputStream, "%s:\n", Label);
}

//
// Emit Functions
//

static void
EmitNoTab(char *Str)
{
    fprintf(OutputStream, "%s\n", Str);
}

static void
Emit(char *Str)
{
    fprintf(OutputStream, "\t%s", Str);
}

static void
EmitLn(char *Str)
{
    Emit(Str);
    fprintf(OutputStream, "\n");
}

static void
EmitLn(char C)
{
    fprintf(OutputStream, "\t%c\n", C);
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
EmitInstruction(char *Name, char Param1, char Param2)
{
    char Line[1024];
    sprintf(Line, "%s %c, %c", Name, Param1, Param2);
    EmitLn(Line);
}

//
// Code generation
//

static void
Clear()
{
    EmitLn("MOV eax, 0");
}

static void
Negate()
{
    EmitLn("IMUL eax, -1");
}

static void
LoadConstant(int Constant)
{
    char Line[1024];
    sprintf(Line, "MOV eax, %d", Constant);
    EmitLn(Line);
}

static void
LoadVariable(char Name)
{
    EmitInstruction("MOV", "eax", Name);
}

static void
Push()
{
    EmitInstruction("PUSH", "eax");
}

static void
PopAdd()
{
    EmitInstruction("POP", "ebx");
    EmitInstruction("ADD", "eax", "ebx");
}

static void
PopSub()
{
    EmitInstruction("POP", "ebx");
    EmitInstruction("SUB", "eax", "ebx");
    EmitInstruction("IMUL", "eax", "-1");
}

static void
PopMul()
{
    EmitInstruction("POP", "ebx");
    EmitInstruction("IMUL", "eax", "ebx");
}

static void
PopDiv()
{
    EmitInstruction("POP", "ebx");
    EmitInstruction("DIV", "ebx", "eax");
    EmitInstruction("MOV", "eax", "ebx");
}

static void
Store(char Name)
{
    if(!InSymbolTable(Name))
    {
        Undefined(Name);
    }
    
    EmitInstruction("LEA", "ebx", Name);
    EmitInstruction("MOV", "[ebx]", "eax");
}

//
// Parsing - Expressions
//

// NOTE: Expression grammer
// <assignment>   :== <identifier> = <expression>
// <expression>   :== <first-term> (<addop> <term>)*
// <first-term>   :== <first-factor> <rest>
// <term>         :== <factor> <rest>
// <rest>         :== (<mulop> <factor>)*
// <first factor> :== [ <adop> ] <factor>
// <factor>       :== <var> | <number> | '(' <expression> ')'

static void Expression();

static void
Factor()
{
    if(Look == '(')
    {
        Match('(');
        Expression();
        Match(')');
    }
    else if(IsDigit(Look))
    {
         int Number = GetNumber();
        LoadConstant(Number);
    }
    else
    {
        char Name = GetName();
        LoadVariable(Name);
    }
}

static void
NegativeFactor()
{
    Match('-');
    if(IsDigit(Look))
    {
        int Number = GetNumber();
        LoadConstant(-Number);
    }
    else
    {
        Factor();
        Negate();
    }
}

static void
FirstFactor()
{
    if(Look == '+')
    {
        Match('+');
        Factor();
    }
    else if(Look == '-')
    {
        NegativeFactor();
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
    PopMul();
}

static void
Divide()
{
    Match('/');
    Factor();
    PopDiv();
}

static void
RestOfTerms()
{
    while(IsMulop(Look))
    {
        Push();
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
Term()
{
    Factor();
    RestOfTerms();
}

static void
FirstTerm()
{
    FirstFactor();
    RestOfTerms();
}

static void
Add()
{
    Match('+');
    Term();
    PopAdd();
}

static void
Subtract()
{
    Match('-');
    Term();
    PopSub();
}

static void
Expression()
{
    FirstTerm();
    
    while(IsAddop(Look))
    {
        Push();
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
    char Name = GetName();
    Match('=');
    Expression();
    Store(Name);
}

//
// Parsing - Program Structure
//

static void
Header()
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
    EmitNoTab("IntegerFormat db \"%d\", 13, 10, 0");
    EmitNoTab(".code");
}

static void
Block()
{
    while(Look != 'e')
    {
        Assignment();
    }
}

static void
Main()
{
    Match('b');
    PostLabel("MAIN");
    
    Block();
    
    Match('e');
    EmitLn("call ExitProcess");
    EmitNoTab("end MAIN");
}

static void
Alloc(char Name)
{
    if(InSymbolTable(Name))
    {
        Abort("Duplicate variable name");
    }
    SymbolTable[Name - 'A'] = true;
    
    fprintf(OutputStream, "%c DWORD ", Name);
    if(Look == '=')
    {
        Match('=');
         int Num = GetNumber();
        fprintf(OutputStream, "%d\n", Num);
    }
    else
    {
        fprintf(OutputStream, "?\n");
    }
}

static void
Decl()
{
    Match('v');
    char Name = GetName();
    Alloc(Name);
    
    while(Look == ',')
    {
        Match(',');
        char Name = GetName();
        Alloc(Name);
    }
}

static void
TopDecls()
{
    while(Look != 'b')
    {
        if(Look == 'v')
        {
            Decl();
        }
        else
        {
            char Message[1024];
            sprintf(Message, "Unrecognized Keyword \'%c\'", Look);
            Abort(Message);
        }
    }
    }

static void
Program()
{
    Match('p');
    Header();
    TopDecls();
    Main();
    Match('.');
}

static void
Init()
{
    GetChar();
}

int main()
{
    Init();
    Program();
    if(Look != '\n')
    {
        Abort("Unexpected data after \'.\'");
    }
}
