#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "cradle.h"

#define EndOfFileChar '|'

static char Look;

static char Value[MaxTokenLength];
static char Token;
static int LabelCount = 0;
// NOTE: There must be a one to one relationship between Keywords and KeywordCodes
static char *Keywords[] = {"IDENT", "IF", "ELSE", "ENDIF", "END"};
static char KeywordCodes[] = {'x', 'i', 'l', 'e', 'e'};

static int
Lookup(char *Table[], char *Value, int TableLength)
{
    int Result = 0;
    
    for(int Index = 1; Index < TableLength; Index++)
    {
        char *TableEntry = Table[Index];
        if(!strcmp(TableEntry, Value))
        {
            Result = Index;
            break;
        }
    }
    
    return Result;
}

static void
GetChar()
{
    if(Look != EndOfFileChar)
    {
        Look = getc(stdin);
        while(Look == '\n')
        {
            GetChar();
        }
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
IsWhite(char C)
{
    bool Result = ((C == '\n') || (C == ' ') || (C == '\t'));
    
    return Result;
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
SkipComma()
{
    SkipWhite();
    if(Look == ',')
    {
        GetChar();
        SkipWhite();
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
IsAlphaNum(char C)
{
    bool Result = (IsAlpha(C) || IsDigit(C));
    
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

static bool
IsOp(char C)
{
    // TODO: These handle multicharacter operators. Single character operators are
    // handled in the else branch in Scan procedure.
    bool Result = ((C == '+') || (C == '-') || (C == '*') || (C == '/') ||
                   (C == '<') || (C == '>') || (C == ':') || (C == '='));
    
    return Result;
}

static void
GetName()
{
    if(!IsAlpha(Look))
    {
        Expected("Name");
    }
    
    int CharIndex = 0;
    while(IsAlphaNum(Look))
    {
        Assert(CharIndex < MaxTokenLength);
        Value[CharIndex++] = toupper(Look);
        GetChar();
    }
    Value[CharIndex] = 0;
    
    int KeywordIndex = Lookup(Keywords, Value, ArrayCount(Keywords));
    Token = KeywordCodes[KeywordIndex];
}

static void
GetNum()
{
    if(!IsDigit(Look))
    {
        Expected("Integer");
    }
    
    int CharIndex = 0;
    while(IsDigit(Look))
    {
        Assert(CharIndex < MaxTokenLength);
        Value[CharIndex++] = Look;
        GetChar();
    }
    Value[CharIndex] = 0;
    
    Token = '#';
}

static void
GetOp()
{
    if(!IsOp(Look))
    {
        Expected("Operator");
    }
    
    int CharIndex = 0;
    while(IsOp(Look))
    {
        Assert(CharIndex < MaxTokenLength);
        Value[CharIndex++] = Look;
        GetChar();
    }
    Value[CharIndex] = 0;
    
    Token = '?';
}

static void
Scan()
{
    if(IsAlpha(Look))
    {
        GetName();
    }
    else if(IsDigit(Look))
    {
        GetNum();
    }
    else if(IsOp(Look))
    {
        GetOp();
    }
    else
    {
        // NOTE: These are single character operators
        Value[0] = Look;
        Value[1] = 0;
        Token = '?';
        GetChar();
    }
    
    SkipWhite();
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

void
main(int NumArguments, char **Arguments)
{
    Init();
    
#if 1
    
    while(strcmp(Value, "|"))
    {
        Scan();
        switch(Token)
        {
            case 'x':
            {
                printf("Identifier: ", Token);
            } break;
            case '#':
            {
                printf("Number: ", Token);
            } break;
            case '?':
            {
                printf("Operator: ", Token);
            } break;
            case 'i':
            case 'l':
            case 'e':
            {
                printf("Keyword: ", Token);
            } break;
            default:
            {
                printf("lolwat: ");
            }
        }
        printf("%s\n", Value);
    }
#endif
    
#if 0
    char Token[MaxTokenLength];
    Scan(Token);
    printf("%d\n", Lookup(Keywords, Token, ArrayCount(Keywords)));
#endif
}