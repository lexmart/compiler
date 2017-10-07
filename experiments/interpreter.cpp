#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static char Look;
static char Table[26];

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

static int
GetNum()
{
    if(!IsDigit(Look))
    {
        Expected("Integer");
    }
    
    
    int Value = 0;
    while(IsDigit(Look))
    {
        Value = (10*Value) + (Look - '0');
        GetChar();
    }
    
    return Value;
}

static void
Init()
{
    GetChar();
    
    for(int i = 0; i < 26; i++)
    {
        Table[i] = 0;
    }
}

static int Expression();

static int Factor()
{
    int Result;
    
    if(Look == '(')
    {
        Match('(');
        Result = Expression();
        Match(')');
    }
    else if(IsAlpha(Look))
    {
        Result = Table[GetName() - 'A'];
    }
    else
    {
        Result = GetNum();
    }
    
    return Result;
}

static int
Term()
{
    int Value = Factor();
    while(IsMulop(Look))
    {
        if(Look == '*')
        {
            Match('*');
            Value *= Factor();
        }
        else if(Look == '/')
        {
            Match('/');
            Value /= Factor();
        }
    }
    
    return Value;
}

static int
Expression()
{
    int Value;
    
    if(IsAddop(Look))
    {
        Value = 0;
    }
    else
    {
        Value = Term();
    }
    
    while(IsAddop(Look))
    {
        if(Look == '+')
        {
            Match('+');
            Value += Term();
        }
        else if(Look == '-')
        {
            Match('-');
            Value -= Term();
        }
    }
    
    return Value;
}

static void
Assignment()
{
    char Name = GetName();
    Match('=');
    Table[Name - 'A'] = Expression();
}

static void
Output()
{
    Match('!');
    int Value = Table[GetName() - 'A'];
    printf("%d\n", Value);
}

void
main(int NumArguments, char **Arguments)
{
    Init();
    
    while(Look != '.')
    {
        if(Look == '!')
        {
            Output();
        }
        else
        {
            Assignment();
        }
        
        Match('\n');
    }
    
    
    printf("%d\n", Expression());
}