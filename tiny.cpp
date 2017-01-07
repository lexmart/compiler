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

#define Assert(Expression) if(!(Expression)) { *((int *)0) = 0; }
#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))

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
    Token_Program,
    Token_Read,
    Token_Write,
    Token_Number,
    Token_Operator
};

#define MaxTokenLength 32

static char Look;
static int LabelCount = 0;
static int NumSymbols = 1;
static char *SymbolTable[4096] = {};
static token_type Token;
static char Value[MaxTokenLength];
static char *Keywords[] = {0, "IF", "ELSE", "ENDIF", "WHILE", "ENDWHILE", "VAR", "BEGIN", "END", "PROGRAM", "READ", "WRITE"};

static FILE *InputStream = stdin;
static FILE *OutputStream = stdout;

static void GetName();
static bool InSymbolTable(char *);
static bool IsWhite(char);
static void Next();

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
    while(IsWhite(Look))
    {
        GetChar();
    }
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
    fprintf(stdout, "Expected: %s, Got: %s\n", String, Value);
    exit(0);
}

static void
Match(char C)
{
    if(Value[0] != C)
    {
        char ExpectedString[4] = {'\'', C, '\'', 0};
        Expected(ExpectedString);
    }
    Next();
}

static void
Undefined(char *Name)
{
    printf("Undefined Identifier \'%s\'\n", Name);
    exit(0);
}

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
MatchString(char *String)
{
    if(strcmp(String, Value) != 0)
    {
        Expected(String);
    }
    
    Next();
}

static void
MatchToken(token_type ExpectedToken)
{
    if(Token != ExpectedToken)
    {
        Expected(Keywords[ExpectedToken]);
    }
    
    Next();
}

//
// --"Is" Functions
//

static bool
IsWhite(char C)
{
    bool Result = ((C == ' ') || (C == '\t') || (C == '\n') || (C == '\r'));
    
    return Result;
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
InSymbolTable(char *Symbol)
{
    bool Result = (Lookup(SymbolTable, NumSymbols, Symbol) > 0);
    
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
    SkipWhite();
    
    if(!IsAlpha(Look))
    {
        Expected("Identifier");
    }
    
    int Index = 0;
    while(IsAlphaNumeric(Look))
        {
            Value[Index++] = toupper(Look);
            GetChar();
    }
    Value[Index] = 0;
    
    Token = Token_Identifier;
}

static void
GetNumber()
{
    SkipWhite();
    
    if(!IsDigit(Look))
    {
        Expected("Number");
    }
    
    int Index = 0;
    while(IsDigit(Look))
    {
        Value[Index++] = Look;
        GetChar();
    }
    Value[Index] = 0;
    
    Token = Token_Number;
}

static void
GetOp()
{
    SkipWhite();
    
    Token = Token_Operator;
    Value[0] = Look;
    Value[1] = 0;
    
    GetChar();
}

static void
Scan()
{
    if(Token == Token_Identifier)
    {
        Token = (token_type)Lookup(Keywords, sizeof(Keywords)/sizeof(char *), Value);
    }
}

static void
Next()
{
    SkipWhite();
    if(IsAlpha(Look))
    {
        GetName();
    }
    else if(IsDigit(Look))
    {
        GetNumber();
    }
    else
    {
        GetOp();
    }
    
    Scan();
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
LoadConstant(bool Negative)
{
    char Line[1024];
    if(!Negative)
    {
    sprintf(Line, "MOV eax, %s", Value);
    }
    else
    {
        sprintf(Line, "MOV eax, -%s", Value);
    }
    EmitLn(Line);
}

static void
LoadVariable(char *Name)
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
Store(char *Name)
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
SetLessThanOrEqual()
{
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETLE", "al");
    EmitInstruction("IMUL", "eax", "-1");
    EmitInstruction("CMP", "eax", "-1");
}

static void
SetGreaterThanOrEqual()
{
    EmitInstruction("MOV", "eax", "0");
    EmitInstruction("SETGE", "al");
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

static void
EmitRead()
{
    EmitInstruction("LEA", "eax", Value);
    EmitInstruction("PUSH", "eax");
    EmitInstruction("LEA", "eax", "ReadFormat");
    EmitInstruction("PUSH", "eax");
    EmitInstruction("CALL", "_imp__scanf");
    EmitInstruction("ADD", "ESP", "8");
}

static void
EmitWrite()
{
    EmitInstruction("PUSH", Value);
    EmitInstruction("LEA", "eax", "PrintFormat");
    EmitInstruction("PUSH", "eax");
    EmitInstruction("CALL", "_imp__printf");
    EmitInstruction("ADD", "ESP", "8");
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
    if(!strcmp(Value, "("))
    {
        MatchString("(");
        BoolExpression();
        MatchString(")");
    }
    else
    {
        if(Token == Token_Number)
    {
        LoadConstant(false);
    }
    else if(Token == Token_Identifier)
    {
        LoadVariable(Value);
    }
    else
    {
        Expected("Math factor");
    }
    
    Next();
}
}

static void
NegativeFactor()
{
    MatchString("-");
    if(IsDigit(Value[0]))
    {
        GetNumber();
        LoadConstant(true);
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
    if(!strcmp(Value, "+"))
    {
        MatchString("+");
        Factor();
    }
    else if(!strcmp(Value, "-"))
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
    MatchString("*");
    Factor();
    PopMul();
}

static void
Divide()
{
    MatchString("/");
    Factor();
    PopDiv();
}

static void
RestOfTerms()
{
    while(IsMulop(Value[0]))
    {
        Push();
        if(!strcmp(Value, "*"))
        {
            Multiply();
        }
        else if(!strcmp(Value, "/"))
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
    MatchString("+");
    Term();
    PopAdd();
}

static void
Subtract()
{
    MatchString("-");
    Term();
    PopSub();
}

static void
Expression()
{
    FirstTerm();
    
    while(IsAddop(Value[0]))
    {
        Push();
        if(!strcmp(Value, "+"))
        {
            Add();
        }
        else if(!strcmp(Value, "-"))
        {
            Subtract();
        }
    }
}

static void
Equals()
{
    Next();
    Expression();
    PopCompare();
    SetEqual();
}

static void
NotEquals()
{
    Next();
    Expression();
    PopCompare();
    SetNotEqual();
}

static void
LessThanOrEqual()
{
    Next();
    Expression();
    PopCompare();
    SetLessThanOrEqual();
}

static void
GreaterThanOrEqual()
{
    Next();
    Expression();
    PopCompare();
    SetGreaterThanOrEqual();
}

static void
LessThan()
{
    Next();
    if(!strcmp(Value, "="))
    {
        LessThanOrEqual();
    }
    else if(!strcmp(Value, ">"))
    {
        NotEquals();
    }
    else
    {
        Expression();
        PopCompare();
        SetLessThan();
    }
}


static void
GreaterThan()
{
    Next();
    if(!strcmp(Value, "="))
    {
        GreaterThanOrEqual();
    }
    else
    {
        Expression();
        PopCompare();
        SetGreaterThan();
    }
}

static void
Relation()
{
    Expression();
    if(IsRelop(Value[0]))
    {
        Push();
        if(!strcmp(Value, "="))
        {
            Equals();
        }
        else if(!strcmp(Value, "<"))
        {
            LessThan();
        }
        else if(!strcmp(Value, ">"))
        {
            GreaterThan();
        }
    }
}

static void
NotFactor()
{
    if(!strcmp(Value, "!"))
    {
        MatchString("!");
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
    
    while(!strcmp(Value, "&"))
    {
        MatchString("&");
        Push();
        NotFactor();
        PopAnd();
    }
}

static void
BoolOr()
{
    MatchString("|");
    BoolTerm();
    PopOr();
}

static void
BoolXor()
{
    MatchString("^");
    BoolTerm();
    PopXor();
}

static void
BoolExpression()
{
    BoolTerm();
    
    while(IsOrop(Value[0]))
    {
        Push();
        
        if(!strcmp(Value, "|"))
        {
            BoolOr();
        }
        else if(!strcmp(Value, "^"))
        {
            BoolXor();
        }
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
    
    if(Token == Token_Else)
    {
        MatchString("l");
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
    Next();
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
Read()
{
    GetName();
    EmitRead();
    Next();
}

static void
Write()
{
    GetName();
    EmitWrite();
    Next();
}

static void
Assignment()
{
    int SymbolIndex = Lookup(SymbolTable, NumSymbols, Value);
    Assert(SymbolIndex != 0);
    char *Variable = SymbolTable[SymbolIndex];
    
    Next();
    MatchString("=");
    BoolExpression();
    Store(Variable);
}

static void
Block()
{
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
        else if(Token == Token_Read)
        {
            Read();
        }
        else if(Token == Token_Write)
        {
            Write();
        }
        else
        {
            Assignment();
        }
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
    EmitNoTab("PrintFormat db \"%d\", 13, 10, 0");
    EmitNoTab("ReadFormat db \"%d\", 0");
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
AddEntry(char *Name)
{
    char *Symbol = (char *)malloc(strlen(Name) + 1);
    memcpy(Symbol, Name, strlen(Name) + 1);
    
    Assert(NumSymbols < sizeof(SymbolTable)/sizeof(char *));
    SymbolTable[NumSymbols++] = Symbol;
}

static void
Alloc(char *Name)
{
    if(InSymbolTable(Name))
    {
        Abort("Duplicate variable name");
    }
    
    AddEntry(Name);
    
    fprintf(OutputStream, "%s DWORD ", Name);
    
    Next();
    if(!strcmp(Value, "="))
    {
         GetNumber();
        fprintf(OutputStream, "%s\n", Value);
        Next();
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
    Alloc(Value);
    
    while(!strcmp(Value, ","))
    {
        GetName();
        Alloc(Value);
    }
}

static void
TopDecls()
{
    while(Token != Token_Begin)
    {
        if(Token == Token_Var)
        {
            Decl();
        }
        else
        {
            char Message[1024];
            sprintf(Message, "Unrecognized Keyword \'%s\"", Value);
            Abort(Message);
        }
    }
    }

static void
Program()
{
    MatchToken(Token_Program);
    Header();
    TopDecls();
    Main();
}

static void
Init()
{
    char OutputFileName[1024];
    sprintf(OutputFileName, "test1.asm");
    OutputStream = fopen(OutputFileName, "w");
    
    GetChar();
    Next();
}

int
main()
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