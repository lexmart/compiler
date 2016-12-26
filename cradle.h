#define MaxTokenLength 64

#define Assert(Expression) if(!(Expression)) { *((int *)0) = 0; }

static void SkipBlank();
static void Factor();
static void Multiply();
static void Divide();
static void Term();
static void Add();
static void Subtract();
static void Expression();
static void Identifer();
static void If(char *);
static void Block(char *);
static void BoolExpression();