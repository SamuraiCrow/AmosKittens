
extern int stack;

extern void _num( int num );
extern void setStackDecimal( double decimal );
extern void setStackStr( char *str );
extern void setStackStrDup(const char *str);

extern bool stackStrAddValue(struct kittyData *item0, struct kittyData *item1);
extern bool stackStrAddDecimal(struct kittyData *item0, struct kittyData *item1);

extern bool stackStrAddStr(struct kittyData *item0,	struct kittyData *item1);

extern bool stackMoreStr(struct kittyData *item0,	struct kittyData *item1);
extern bool stackLessStr(struct kittyData *item0,	struct kittyData *item1);
extern bool stackEqualStr(struct kittyData *item0, struct kittyData *item1);

extern void popStack(int n);

void flushCmdParaStack();

extern int _stackInt( int n );
extern char *_stackString( int n );

