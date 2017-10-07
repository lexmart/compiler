// NOTE: The cradle is where things grow up.

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define Assert(Expression) if(!(Expression)) { *((int *)0) = 0; }
#define InvalidCodePath Assert(0)
#define InvalidDefault default: { InvalidCodePath; }
#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))
#define MaxTokenLength 64

static FILE *InputFileHandle = 0;
static FILE *OutputFileHandle = 0;
#define InputTarget InputFileHandle
#define OutputTarget OutputFileHandle

static char Look;
static int LabelCount = 0;

static void
GetChar()
{
    Look = getc(InputTarget);
}

static void
Error(char *Message)
{
    printf("Error %s\n", Message);
}

static void
Abort(char *Message)
{
    Error(Message);
    exit(0);
}

static void
Expected(char *String)
{
    printf("%s Expected.\n", String);
    exit(0);
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
        char ExpectedString[4] = {'\'', C, '\'', 0};
        Expected(ExpectedString);
    }
}

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

static char
GetName()
{
    if(!IsAlpha(Look))
    {
        Expected("Name");
    }
    
    char Result = Look;
    GetChar();
    
    return Result;
}

static char
GetNumber()
{
    if(!IsDigit(Look))
    {
        Expected("Number");
    }
    
    char Result = Look;
    GetChar();
    
    return Result;
}

static void
EmitNoTab(char *Str)
{
    fprintf(OutputTarget, "%s\n", Str);
}

static void
Emit(char *Str)
{
    fprintf(OutputTarget, "\t%s", Str);
}

static void
EmitLn(char *Str)
{
    Emit(Str);
    fprintf(OutputTarget, "\n");
}

static void
EmitLn(char C)
{
    fprintf(OutputTarget, "\t%c\n", C);
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

static void
NewLabel(char *Label)
{
    sprintf(Label, "L%02d", LabelCount++);
}

static void
PostLabel(char *Label)
{
    fprintf(OutputTarget, "%s:\n", Label);
}

static void
PostLabel(char Label)
{
    fprintf(OutputTarget, "%c:\n", Label);
}


//
//
//

/*

<program> ::= <program-header> <block> '.'
     <program-header> ::= PROGRAM <ident>
     <block> ::= <declarations> <statements>
     
     <declarations> ::= 
     (<label list>   |
                          <constant list> |
                          <type list>     |
                          <variable list> |
                          <procedure>     |
                          <function>      )*
                          
                               <statements> ::= <compound statement>
                               
     <compound statement> ::= 
     BEGIN
     <statement>
     (';' <statement>) 
     END
     
     <statement> ::= <simple statement> | <structured statement>
     
    <simple statement> ::= <assignment> | <procedure call> | null
    
    <structured statement> ::= <compound statement> |
                               <if statement>       |
                               <case statement>     |
                               <while statement>    |
                               <repeat statement>   |
                               <for statement>      |
                               <with statement>
                               
     */

static void BoolExpression();

static void
Factor()
{
    if(Look == '(')
    {
        Match('(');
        BoolExpression();
        Match(')');
    }
    else if(IsDigit(Look))
    {
        char C = GetNumber();
        EmitInstruction("MOV", "eax", C);
    }
    else
    {
        char C = GetName();
        EmitInstruction("MOV", "eax", C);
    }
}

static void
SignedFactor()
{
    if((Look == '+') || (Look == '-'))
    {
        if(Look == '-')
        {
            GetChar();
            Factor();
            EmitInstruction("IMUL", "eax", "-1");
        }
        else
        {
            GetChar();
            Factor();
        }
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
    EmitInstruction("POP", "ebx");
    EmitInstruction("IMUL", "eax", "ebx");
}

static void
Divide()
{
    Match('/');
    Factor();
    EmitInstruction("POP", "ebx");
    EmitInstruction("DIV", "eax", "ebx");
}

static void
Term()
{
    SignedFactor();
    while((Look == '*') || (Look == '/'))
    {
        EmitInstruction("PUSH", "eax");
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
    Match('+');
    Term();
    EmitInstruction("POP", "ebx");
    EmitInstruction("ADD", "eax", "ebx");
}

static void
Expression()
{
    Term();
    while((Look == '+') || (Look == '-'))
    {
        EmitInstruction("PUSH", "eax");
        if(Look == '+')
        {
            Add();
        }
        else if(Look == '0')
        {
            Subtract();
        }
    }
}

static void
LessThan()
{
    Match('<');
    Expression();
    EmitInstruction("POP", "ebx");
    EmitInstruction("CMP", "ebx", "eax");
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETL", "al");
    EmitInstruction("IMUL", "eax", "-1");
    EmitInstruction("CMP", "eax", "-1");
}

static void
GreaterThan()
{
    Match('>');
    Expression();
    EmitInstruction("POP", "ebx");
    EmitInstruction("CMP", "ebx", "eax");
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETG", "al");
    EmitInstruction("IMUL", "eax", "-1");
    EmitInstruction("CMP", "eax", "-1");
}

static void
Equals()
{
    Match('=');
    Expression();
    EmitInstruction("POP", "ebx");
    EmitInstruction("CMP", "ebx", "eax");
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETE", "al");
    EmitInstruction("IMUL", "eax", "-1");
    EmitInstruction("CMP", "eax", "-1");
}

static void
Relation()
{
    Expression();
    if((Look == '<') || (Look == '>') || (Look == '='))
    {
        EmitInstruction("PUSH", "eax");
        if(Look == '<')
        {
            LessThan();
        }
        else if(Look == '>')
        {
            GreaterThan();
        }
        else
        {
            Equals();
        }
    }
}

static void
BoolFactor()
{
    if(Look == 't')
    {
        EmitInstruction("MOV", "eax", "-1");
    }
    else if(Look == 'f')
    {
        EmitInstruction("MOV", "eax", "0");
    }
    else
    {
        Relation();
    }
}

static void
BoolNotFactor()
{
    if(Look == '!')
    {
        Match('!');
        BoolFactor();
        EmitInstruction("XOR", "eax", "-1");
    }
    else
    {
        BoolFactor();
    }
}

static void
And()
{
    Match('&');
    BoolNotFactor();
    EmitInstruction("POP", "ebx");
    EmitInstruction("AND", "eax", "ebx");
}

static void
BoolTerm()
{
    BoolNotFactor();
    while(Look == '&')
    {
        EmitInstruction("PUSH", "eax");
        And();
    }
}

static void
Or()
{
    Match('|');
    BoolNotFactor();
    EmitInstruction("POP", "ebx");
    EmitInstruction("OR", "eax", "ebx");
}

static void
Xor()
{
    Match('^');
    BoolNotFactor();
    EmitInstruction("POP", "ebx");
    EmitInstruction("XOR", "eax", "ebx");
}

static void
BoolExpression()
{
    BoolTerm();
    while((Look == '|') || (Look == '^'))
    {
        EmitInstruction("PUSH", "eax");
        if(Look == '|')
        {
            Or();
        }
        else if(Look == '^')
        {
            Xor();
        }
    }
}

static void
SimpleStatement()
{
    char C = GetName();
    if(Look == '(')
    {
        Match('(');
        char Var = GetName();
        char VarWithBrackets[4] = {'[', Var, ']', 0};
        EmitInstruction("PUSH", VarWithBrackets);
        EmitInstruction("LEA", "eax", "[IntegerFormat]");
        EmitInstruction("PUSH", "eax");
        EmitInstruction("CALL", "_imp__printf");
        EmitInstruction("ADD", "esp", "08h");
        Match(')');
    }
    else
    {
        Match('=');
        BoolExpression();
        EmitInstruction("MOV", C, "eax");
    }
}

static void
If()
{
    Match('i');
    BoolExpression();
    
    char FalseLabel[MaxTokenLength];
    NewLabel(FalseLabel);
    
    EmitInstruction("JNE", FalseLabel);
    SimpleStatement();
    
    PostLabel(FalseLabel);
    Match('e');
}

static void CompoundStatement();

static void
While()
{
    Match('w');
    
    char ConditionLabel[MaxTokenLength];
    char DoneLabel[MaxTokenLength];
    NewLabel(ConditionLabel);
    NewLabel(DoneLabel);
    
    PostLabel(ConditionLabel);
    BoolExpression();
    EmitInstruction("JNE", DoneLabel);
    
    CompoundStatement();
    EmitInstruction("JMP", ConditionLabel);
    
    PostLabel(DoneLabel);
}

static void
StructuredStatement()
{
    if(Look == 'b')
    {
        CompoundStatement();
    }
    else if(Look == 'i')
    {
        If();
    }
    else if(Look == 'w')
    {
        While();
    }
    else
    {
        InvalidCodePath;
    }
}

static void
Statement()
{
    // TODO: begin/if/while
    if((Look == 'b') || (Look == 'i') || (Look == 'w'))
    {
        StructuredStatement();
    }
    else
    {
        SimpleStatement();
    }
}

static void
Labels()
{
    Match('l');
}

static void
Constants()
{
    Match('c');
}

static void
Types()
{
    Match('t');
}

static void
Variables()
{
    Match('v');
    char Var = GetName();
    Match('=');
    char Value = GetNumber();
    
    EmitInstruction("MOV", Var, Value);
}

static void
Procedure()
{
    Match('p');
}

static void
Function()
{
    Match('f');
}

static void
Declarations()
{
    while((Look == 'l') || (Look == 'c') || (Look == 't') || (Look == 'v') || (Look == 'p') ||
          (Look == 'f'))
    {
        switch(Look)
        {
            case 'l':
            {
                Labels();
            } break;
            
            case 'c':
            {
                Constants();
            } break;
            
            case 't':
            {
                Types();
            } break;
            
            case 'v':
            {
                Variables();
            } break;
            
            case 'p':
            {
                Procedure();
            } break;
            
            case 'f':
            {
                Function();
            } break;
            
            InvalidDefault;
        }
    }
}

static void
CompoundStatement()
{
    Match('b');
    Statement();
    while(Look == ';')
    {
        Match(';');
        Statement();
    }
    Match('e');
}

static void
Statements()
{
    CompoundStatement();
}

static void
Block(char Name)
{
    Declarations();
    PostLabel(Name);
    Statements();
}

static void Prolog()
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
    EmitNoTab("X DWORD ?");
    EmitNoTab("Y DWORD ?");
    EmitNoTab("Z DWORD ?");
    EmitNoTab(".code");
}

static void Epilog(char Name)
{
    EmitInstruction("call", "ExitProcess");
    EmitInstruction("end", Name);
}

static void
Program()
{
    Prolog();
    
    Match('p');
    char Name = GetName();
    Block(Name);
    Match('.');
    
    Epilog(Name);
}

static void
Init()
{
    GetChar();
}

int main(int NumArguments, char **Arguments)
{
    if(InputTarget != stdin)
    {
        InputFileHandle = fopen(Arguments[1], "r");
    }
    
    if(OutputTarget != stdout)
    {
        char OutputFileName[1024];
        char *InputFileChar = Arguments[1];
        char *OutputFileChar = OutputFileName;
        while(*InputFileChar != '.')
        {
            *OutputFileChar++ = *InputFileChar++;
        }
        *OutputFileChar++ = '.';
        *OutputFileChar++ = 'a';
        *OutputFileChar++ = 's';
        *OutputFileChar++ = 'm';
        *OutputFileChar++ = 0;
        
    OutputFileHandle = fopen(OutputFileName, "w");
    }
    
    Init();
    Program();
}