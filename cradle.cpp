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
Other()
{
    EmitLn(GetName());
}

static void
Condition()
{
    EmitLn("<condition>");
}

static void
If(char *OuterDoneLabel)
{
    Match('i');
    Condition();
    
    char FalseLabel[MaxTokenLength];
    char DoneLabel[MaxTokenLength];
    
    NewLabel(FalseLabel);
    strcpy(DoneLabel, FalseLabel);
    
    // NOTE: If ZF = 0 (ie the condition was not met), then jump to FalseLabel
    EmitInstruction("JNE", FalseLabel);
    Block(OuterDoneLabel);
    
    if(Look == 'l')
    {
        Match('l');
        
        NewLabel(DoneLabel);
        EmitInstruction("JMP", DoneLabel);
        
        PostLabel(FalseLabel);
        Block(OuterDoneLabel);
    }
    
    Match('e');
    PostLabel(DoneLabel);
}

static void
While()
{
    Match('w');
    char ConditionLabel[MaxTokenLength];
    char DoneLabel[MaxTokenLength];
    NewLabel(ConditionLabel);
    NewLabel(DoneLabel);
    
    PostLabel(ConditionLabel);
    Condition();
    EmitInstruction("JNE", DoneLabel);
    Block(DoneLabel);
    Match('e');
    EmitInstruction("JMP", ConditionLabel);
    PostLabel(DoneLabel);
}

// NOTE: LOOP <block ENDLOOP
static void
Loop()
{
    Match('p');
    char TopLabel[MaxTokenLength];
    char DoneLabel[MaxTokenLength];
    NewLabel(TopLabel);
    NewLabel(DoneLabel);
    PostLabel(TopLabel);
    Block(DoneLabel);
    Match('e');
    EmitInstruction("JMP", TopLabel);
    PostLabel(DoneLabel);
}

// NOTE: REPEAT <block> UNTIL <condition>
static void
RepeatUntil()
{
    Match('r');
    char TopLabel[MaxTokenLength];
    NewLabel(TopLabel);
    PostLabel(TopLabel);
    Block(0);
    Match('u');
    Condition();
    EmitInstruction("JNE", TopLabel);
}

// NOTE: FOR <ident> = <expr1> TO <expr2> <block> ENDFOR
static void
For()
{
    Match('f');
    
    char LoopLabel[MaxTokenLength];
    char DoneLabel[MaxTokenLength];
    NewLabel(LoopLabel);
    NewLabel(DoneLabel);
    
    char Name[MaxTokenLength];
    GetNameWithBrackets(Name);
    Match('=');
    Expression();
    
    EmitInstruction("MOV", Name, "eax");
    EmitInstruction("SUB", Name, "1");
    
    Match('t');
    Expression();
    EmitInstruction("PUSH", "eax");
    
    PostLabel(LoopLabel);
    EmitInstruction("ADD", Name, "1");
    EmitInstruction("MOV", "eax", Name);
    EmitInstruction("CMP", "eax", "[esp]");
    EmitInstruction("JG", DoneLabel);
    Block(0);
    EmitInstruction("JMP", LoopLabel);
    Match('e');
    
    PostLabel(DoneLabel);
    EmitInstruction("ADD", "esp", "4");
}

// NOTE: DO <expression> <block> ENDDO
static void
Do()
{
    Match('d');
    
    char LoopLabel[MaxTokenLength];
    char DoneLabel[MaxTokenLength];
    NewLabel(LoopLabel);
    NewLabel(DoneLabel);
    
    Expression();
    EmitInstruction("PUSH", "eax");
    
    PostLabel(LoopLabel);
    EmitInstruction("MOV", "eax", "[esp]");
    EmitInstruction("SUB", "eax", "1");
    EmitInstruction("MOV", "[esp]", "eax");
    
    Block(DoneLabel);
    
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("CMP", "[esp]", "eax");
    EmitInstruction("JGE", LoopLabel);
    
    PostLabel(DoneLabel);
    EmitInstruction("ADD", "esp", "4");
    Match('e');
}

static void
Expression()
{
    printf("<expression>\n");
}

static void
Break(char *DoneLabel)
{
    Match('b');
    
    if(DoneLabel)
    {
    EmitInstruction("JMP", DoneLabel);
}
else
{
    Abort("No loop to break from");
}
}

static void
Block(char *DoneLabel)
{
    // TODO: push ebp
    // TODO: mov ebp, esp
    
    while((Look != 'e') && (Look != 'l') && (Look != 'u'))
    {
        if(Look == 'i')
            {
                If(DoneLabel);
            }
            else if(Look == 'w')
            {
                While();
            }
            else if(Look == 'p')
            {
                Loop();
            }
            else if(Look == 'r')
            {
                RepeatUntil();
            }
            else if(Look == 'f')
            {
                For();
            }
            else if(Look == 'd')
            {
                Do();
            }
            else if(Look == 'b')
            {
                Break(DoneLabel);
            }
            else
            {
                Other();
            }
    }
    
    // TODO: mov esp, ebp
    // TODO: pop ebp
}

static void
Program()
{
    Block(0);
    if(Look != 'e')
    {
        Expected("End");
    }
    EmitLn("call ExitProcess");
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
    Program();
    
    EmitInstruction("end", "start");
}