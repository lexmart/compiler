#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// TINY Language definition:
// <program> ::= PROGRAM <top-level decls> <main> '.'
// <main> ::= BEGIN <block> END
// <top-level decls> ::= ( <data declaration> )*
// <data declaration> ::= VAR <var-list>
// <var-list> ::= <var> ( <var> )*
// <var> ::= <ident> [ = <integer> ]
// <block> ::= (<statement>)*
// <statement> ::= <if> | <while> | <assignment>

enum token_type
{
    Token_Identifier,
    Token_If,
    Token_Else,
    Token_Endif,
    Token_While,
    Token_EndWhile,
    Token_Var,
    Token_Begin,
    Token_End,
    Token_Program
};

#define MaxTokenLength 32

static char Look;
static int LabelCount = 0;
static bool SymbolTable[26] = {};
static token_type Token;
static char Value[MaxTokenLength];
static char *Keywords[] = {0, "IF", "ELSE", "ENDIF", "WHILE", "ENDWHILE", "VAR", "BEGIN", "END", "PROGRAM"};

static FILE *InputStream = stdin;
static FILE *OutputStream = stdout;

static void GetName();
static bool InSymbolTable(char);

//
// --Input processing
//

static void
GetChar()
{
    Look = getc(InputStream);
}

static void
SkipWhite()
{
    while((Look == ' ') || (Look == '\t'))
    {
        GetChar();
    }
}

static void
Newline()
{
    while(Look == '\n')
    {
        GetChar();
    }
    
    if(Look == '\r')
    {
        GetChar();
    }
    SkipWhite();
}

static void
Abort(char *String)
{
    fprintf(stdout, "Error: %s.\n", String);
    exit(0);
}

static void
Expected(char *String)
{
    fprintf(stdout, "Expected: %s\n", String);
    exit(0);
}

static void
Match(char C)
{
    Newline();
    
    if(Look != C)
    {
        char ExpectedString[4] = {'\'', C, '\'', 0};
        Expected(ExpectedString);
    }
    GetChar();
    
    SkipWhite();
}

static void
Undefined(char Name)
{
    printf("Undefined Identifier \'%c\'\n", Name);
    exit(0);
}

//
// --Lexer
//

static int
Lookup(char **Table, int TableSize, char *Entry)
{
    int Result = 0;
    
    for(int TableIndex = 1;
        TableIndex < TableSize;
        TableIndex++)
    {
        if(!strcmp(Table[TableIndex], Entry))
            {
                Result = TableIndex;
                break;
        }
    }
    
    return Result;
}

static void
Scan()
{
    GetName();
    Token = (token_type)Lookup(Keywords, sizeof(Keywords)/sizeof(char *), Value);
    
    // TODO: Update this for multiple character tokens
    if((Value[1] == 0) && !InSymbolTable(Value[0]))
    {
        char Message[1024];
        sprintf(Message, "%s is unidentified", Value);
        Abort(Message);
    }
}

static void
MatchString(char *String)
{
    if(strcmp(String, Value))
    {
        Expected(String);
    }
}

static void
MatchToken(token_type ExpectedToken)
{
    if(Token != ExpectedToken)
    {
        Expected(Keywords[ExpectedToken]);
    }
}

//
// --"Is" Functions
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
IsAlphaNumeric(char C)
{
    bool Result = (IsAlpha(C) || IsDigit(C));
    
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

//
// --"Get" Functions
//

static void
GetName()
{
    Newline();
    
    if(!IsAlpha(Look))
    {
        Expected("Name");
    }
    
    int Index = 0;
    while(IsAlphaNumeric(Look))
        {
            Value[Index++] = toupper(Look);
            GetChar();
    }
    Value[Index] = 0;
    
    SkipWhite();
}

static int
GetNumber()
{
    Newline();
    
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
    
    SkipWhite();
    
    return Result;
}

static void
NewLabel(char *Output)
{
    sprintf(Output, "L%d", LabelCount++);
}

static void
PostLabel(char *Label)
{
    fprintf(OutputStream, "%s:\n", Label);
}

//
// --Emit Functions
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
// --Code generation
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
    
    EmitInstruction("MOV", Name, "eax");
}

static void
Not()
{
    EmitInstruction("NOT", "eax");
}

static void
PopAnd()
{
    EmitInstruction("POP", "ebx");
    EmitInstruction("AND", "eax", "ebx");
}

static void
PopOr()
{
    EmitInstruction("POP", "ebx");
    EmitInstruction("OR", "eax", "ebx");
}

static void
PopXor()
{
    EmitInstruction("POP", "ebx");
    EmitInstruction("XOR", "eax", "ebx");
}

static void
PopCompare()
{
    EmitInstruction("POP", "ebx");
    EmitInstruction("CMP", "ebx", "eax");
}

static void
SetEqual()
{
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETE", "al");
    EmitInstruction("IMUL", "eax", "-1");
    EmitInstruction("CMP", "eax", "-1");
}

static void
SetNotEqual()
{
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETNE", "al");
    EmitInstruction("IMUL", "eax", "-1");
    EmitInstruction("CMP", "eax", "-1");
}

static void
SetLessThan()
{
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETL", "al");
    EmitInstruction("IMUL", "eax", "-1");
    EmitInstruction("CMP", "eax", "-1");
}

static void
SetGreaterThan()
{
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETG", "al");
    EmitInstruction("IMUL", "eax", "-1");
    EmitInstruction("CMP", "eax", "-1");
}

static void
Branch(char *Label)
{
    EmitInstruction("JMP", Label);
}

static void
BranchFalse(char *Label)
{
    EmitInstruction("JNE", Label);
}

//
// --Parsing - Expressions
//

// NOTE: Expression grammer
// <assignment>   :== <identifier> = <bool-expr>
// <bool-expr>    :== <bool-term> (<orop> <bool-term>)*
// <bool-term>    :== <not-factor> (<andop> <not-factor>)*
// <not-factor>   :== ['!'] <relation>
// <relation>     :== <expression> [ <relop> <expression> ]
// <expression>   :== <first-term> (<addop> <term>)*
// <first-term>   :== <first-factor> <rest>
// <term>         :== <factor> <rest>
// <rest>         :== (<mulop> <factor>)*
// <first factor> :== [ <adop> ] <factor>
// <factor>       :== <var> | <number> | '(' <bool-expr> ')'

static void BoolExpression();
static void Block();

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
         int Number = GetNumber();
        LoadConstant(Number);
    }
    else
    {
        GetName();
        LoadVariable(Value[0]);
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
    SkipWhite();
    Newline();
    
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
        
        SkipWhite();
        Newline();
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
    
    Newline();
    while(IsAddop(Look))
    {
        Newline();
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
Equals()
{
    Match('=');
    Expression();
    PopCompare();
    SetEqual();
}

static void
NotEquals()
{
    Match('#');
    Expression();
    PopCompare();
    SetNotEqual();
}

static void
LessThan()
{
    Match('<');
    Expression();
    PopCompare();
    SetLessThan();
}


static void
GreaterThan()
{
    Match('>');
    Expression();
    PopCompare();
    SetGreaterThan();
}

static void
Relation()
{
    Expression();
    if(IsRelop(Look))
    {
        Push();
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
            LessThan();
        }
        else if(Look == '>')
        {
            GreaterThan();
        }
    }
}

static void
NotFactor()
{
    if(Look == '!')
    {
        Match('!');
        Relation();
        Not();
    }
    else
    {
        Relation();
    }
}

static void
BoolTerm()
{
    NotFactor();
    
    Newline();
    while(Look == '&')
    {
        Newline();
        Match('&');
        Push();
        NotFactor();
        PopAnd();
    }
}

static void
BoolOr()
{
    Match('|');
    BoolTerm();
    PopOr();
}

static void
BoolXor()
{
    Match('^');
    BoolTerm();
    PopXor();
}

static void
BoolExpression()
{
    BoolTerm();
    
    SkipWhite();
    Newline();
    
    while(IsOrop(Look))
    {
        Push();
        
        Newline();
        if(Look == '|')
        {
            BoolOr();
        }
        else if(Look == '^')
        {
            BoolXor();
        }
        
        SkipWhite();
        Newline();
    }
}

//
// --Parsing - Control Structures
//

// <if>     :== IF <bool-expression> <block> [ ELSE <block> ] ENDIF
// <while>  :== WHILE <bool-expression> <block> ENDWHILE

static void
If()
{
    BoolExpression();
    
    char FalseLabel[MaxTokenLength];
    char DoneLabel[MaxTokenLength];
    NewLabel(FalseLabel);
    strncpy(DoneLabel, FalseLabel, MaxTokenLength);
    
    BranchFalse(FalseLabel);
    Block();
    
    Newline();
    if(Token == Token_Else)
    {
        Match('l');
        NewLabel(FalseLabel);
        PostLabel(FalseLabel);
        Block();
    }
    
    PostLabel(DoneLabel);
    MatchToken(Token_Endif);
}

static void
While()
{
    char ConditionLabel[MaxTokenLength];
    char DoneLabel[MaxTokenLength];
    NewLabel(ConditionLabel);
    NewLabel(DoneLabel);
    
    PostLabel(ConditionLabel);
    BoolExpression();
    BranchFalse(DoneLabel);
    Block();
    MatchToken(Token_EndWhile);
    Branch(ConditionLabel);
    
    PostLabel(DoneLabel);
}

//
// --Parsing - Program Structure
//

static void
Assignment()
{
    char Variable = Value[0];
    Match('=');
    BoolExpression();
    Store(Variable);
}

static void
Block()
{
    Scan();
    while((Token != Token_EndWhile) && (Token != Token_Else) && (Token != Token_End))
    {
        if(Token == Token_If)
        {
            If();
        }
        else if(Token == Token_While)
        {
            While();
        }
        else
        {
            Assignment();
        }
        
        Scan();
    }
}

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
}


static void
Main()
{
    MatchToken(Token_Begin);
    EmitNoTab(".code");
    PostLabel("MAIN");
    
    Block();
    
    MatchToken(Token_End);
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
    GetName();
    Alloc(Value[0]);
    
    SkipWhite();
    Newline();
    while(Look == ',')
    {
        Match(',');
        GetName();
        Alloc(Value[0]);
        
        SkipWhite();
        Newline();
    }
}

static void
TopDecls()
{
    Scan();
    while(Token != Token_Begin)
    {
        if(Token == Token_Var)
        {
            Decl();
        }
        else
        {
            char Message[1024];
            sprintf(Message, "Unrecognized Keyword \'%c\'", Look);
            Abort(Message);
        }
        
        Scan();
    }
    }

static void
Program()
{
    MatchToken(Token_Program);
    Header();
    TopDecls();
    Main();
    MatchToken(Token_End);
}

static void
Init()
{
    char OutputFileName[1024];
    sprintf(OutputFileName, "test1.asm");
    //OutputStream = fopen(OutputFileName, "w");
    GetChar();
    Scan();
}

int main()
{
    Init();
    Program();
    
    if(InputStream != stdin)
        {
            fclose(InputStream);
    }
    if(OutputStream != stdout)
    {
        fclose(OutputStream);
    }
}