
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/dos.h>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "errors.h"


bool every_on = true;
int every_timer = 0;
char *on_every_gosub_location = NULL;
char *on_every_proc_location = NULL;
struct timeval every_before, every_after;


int timer_offset = 0;

static struct timeval timer_before, timer_after;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern void setStackStr( char *str );
extern void setStackStrDup( const char *str );
extern int findVarPublic( char *name );

using namespace std;

extern char *findLabel( char *name );

void	input_mode( char *tokenBuffer );

// dummy not used, see code in cmdNext
char *_for( struct glueCommands *data )
{
	return NULL;
}

// dummy not used, we need to know what to do on "else if" and "else", "If" and "else if" does not know where "end if" is.

char *_ifSuccess( struct glueCommands *data ) 
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	setError(22);	// shoud not be executed
	return NULL;
}


char *_ifThenSuccess( struct glueCommands *data ) 
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	return NULL;
}


char *_procedure( struct glueCommands *data )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return  data -> tokenBuffer ;
}

char *_endProc( struct glueCommands *data )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (kittyStack[stack].type)
	{
		case type_int:
			var_param_num = kittyStack[stack].value;
			break;
		case type_float:
			var_param_decimal = kittyStack[stack].decimal;
			break;
		case type_string:
			if (var_param_str) free(var_param_str);
			var_param_str = strdup(kittyStack[stack].str);
			break;
	}

	return  data -> tokenBuffer ;
}

char *_procAndArgs( struct glueCommands *data )
{
	int oldStack;
	struct reference *ref = (struct reference *) (data->tokenBuffer);

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((ref -> ref) && (data -> tokenBuffer2))
	{
		int idx = ref->ref-1;

		switch (globalVars[idx].var.type & 7)
		{
			case type_proc:

				oldStack = data -> stack;

				printf("****\n");

				stackCmdProc( _procedure, data -> tokenBuffer2);	//  data->tokenBuffer+sizeof(struct reference)+ref->length ) ;

				cmdTmp[cmdStack-1].stack = oldStack;	// carry stack.

				printf("Goto %08x\n", globalVars[idx].var.tokenBufferPos);

				tokenMode = mode_store;
				return globalVars[idx].var.tokenBufferPos  ;
		}
	}

	return  data -> tokenBuffer ;
}

char *_gosub( struct glueCommands *data )
{
	char *ptr = data -> tokenBuffer ;
	ptr-=2;
	if (NEXT_TOKEN( ptr ) == 0x0006)
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		ptr += (2 + sizeof(struct reference) + ref -> length) ;
	}

	return ptr ;
}

char *_step( struct glueCommands *data )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_loop ))
	{
		cmdTmp[cmdStack-1].step = kittyStack[stack].value;
	}

	return NULL;
}



char *_if( struct glueCommands *data )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	unsigned short token;
	char *ptr;

	if (kittyStack[data->stack].value == 0)	// 0 is FALSE always -1 or 1 can be TRUE
	{
		int offset = *((unsigned short *) data -> tokenBuffer);

		if (offset) 
		{
			proc_names_printf("IF is FALSE --  read from %08x jump to %08x - %04x\n" ,data->tokenBuffer ,data->tokenBuffer+(offset*2), offset);
			ptr = data->tokenBuffer+(offset*2) ;
			return ptr-4;
		}
	}
	else 	stackIfSuccess();

	return NULL;
}

char *_else_if( struct glueCommands *data )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	unsigned short token;
	char *ptr;

	dump_stack();

	if (kittyStack[data->stack].value == 0)	// 0 is FALSE always -1 or 1 can be TRUE
	{
		int offset = *((unsigned short *) data -> tokenBuffer);

		printf("FALSE offset %d\n", offset * 2);

		if (offset) 
		{
			printf("IF is FALSE --  read from %08x jump to %08x \n" ,data->tokenBuffer ,data->tokenBuffer+(offset*2));
			ptr = data->tokenBuffer+(offset*2) ;
			return ptr-4 ;
		}
	}
	else 	stackIfSuccess();

	return NULL;
}

char *_while( struct glueCommands *data )	// jumps back to the token.
{
	return data -> tokenBuffer-2;
}

char *_whileCheck( struct glueCommands *data )		
{
	int offset = 0;

	proc_names_printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	// two command should be stacked, while loop, and while check.
	// while loop is removed from stack, if check is false
	// and we jump over the wend

	if (kittyStack[data->stack].value == 0)
	{
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _while ) 
		{
			cmdStack --;
			offset = *((unsigned short *) cmdTmp[cmdStack].tokenBuffer);
			if (offset) 
			{
				proc_names_printf("Jump over\n");

				return data->tokenBuffer+(offset*2);
			}
		}
	}
	return NULL;
}




char *_do( struct glueCommands *data )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	return data -> tokenBuffer-2;
}

char *_repeat( struct glueCommands *data )
{
	if (kittyStack[stack].value == 0) return data -> tokenBuffer-2;
	return 0;
}

char *_not_equal( struct glueCommands *data )
{
	proc_names_printf("%s\n",__FUNCTION__);

	if (stack==0) 
	{
		proc_names_printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	proc_names_printf("(%d == %d) == %d \n", kittyStack[stack].value , kittyStack[stack+1].value, (kittyStack[stack].value != kittyStack[stack+1].value) );

	switch (kittyStack[stack].type & 3)
	{
		case 0:	_num( kittyStack[stack].value != kittyStack[stack+1].value) ;
				break;
		case 1:	_num (kittyStack[stack].decimal != kittyStack[stack+1].decimal);
				break;
//		case 2:	_not_equalStr( data );
//				break;
	}

	correct_for_hidden_sub_data();

	return NULL;
}


BOOL setVarInt( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_int:
			var->value = kittyStack[stack].value;
			return TRUE;

		case type_float:
			var->value = (int) kittyStack[stack].decimal;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarDecimal( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_int:
			var->decimal = (double) kittyStack[stack].value;
			return TRUE;

		case type_float:
			var->decimal = kittyStack[stack].decimal;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarString( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_string:
			if (var->str) free(var->str);

			if (kittyStack[stack].str)
			{
				var->str = strdup(kittyStack[stack].str);
				var->len = kittyStack[stack].len;
			}
			else
			{
				var->str = NULL;
				var->len = 0;
			}

			return TRUE;
	}

	return FALSE;
}

BOOL setVarIntArray( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_int:
			var->int_array[var -> index] = kittyStack[stack].value;
			return TRUE;

		case type_float:
			var->int_array[var -> index] = (int) kittyStack[stack].decimal;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarDecimalArray( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_int:
			var->float_array[var -> index] = (double) kittyStack[stack].value;
			return TRUE;

		case type_float:
			var->float_array[var -> index] = kittyStack[stack].decimal;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarStringArray( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_string:
			if (var->str_array[var -> index] ) free(var->str_array[var->index]);
			var->str_array[var -> index] = strdup(kittyStack[stack].str);	
			return TRUE;
	}

	return FALSE;
}

char *_setVar( struct glueCommands *data );

char *(*_do_set) ( struct glueCommands *data ) = _setVar;

char *_setVar( struct glueCommands *data )
{
	BOOL success;
	struct kittyData *var;

	proc_names_printf("%s:%d -- set var %d\n",__FUNCTION__,__LINE__, data -> lastVar-1);

	proc_names_printf("SET var %s \n",globalVars[ data->lastVar-1].varName);

	var = &globalVars[data -> lastVar-1].var;

	success = FALSE;

	switch (var->type)
	{
		case type_int:
			success = setVarInt( var );
			break;
		case type_float:
			success = setVarDecimal( var );
			break;
		case type_string:
			success = setVarString( var );
			break;
		case type_int | type_array:
			success = setVarIntArray( var );
			break;
		case type_float | type_array:
			success = setVarDecimalArray( var );
			break;
		case type_string | type_array:
			success = setVarStringArray( var );
			break;
	}

	if (success == FALSE)
	{
		proc_names_printf("kittyStack[%d].type= %d, (globalVars[%d].var.type & 7)=%d\n",
				stack, 
				kittyStack[stack].type, 
				data -> lastVar, 
				var -> type & 7);

		setError(ERROR_Type_mismatch);
	}

	return NULL;
}

char *_setVarReverse( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d data: lastVar %d\n",__FUNCTION__,__LINE__, data -> lastVar);

	// if variable is string it will have stored its data on the stack.

	if (kittyStack[stack].str) free(kittyStack[stack].str);
	kittyStack[stack].str = NULL;
	stack --;

	data->lastVar = last_var;	// we did know then, but now we know,
	return _setVar( data );
}

//--------------------------------------------------------

char *nextArg(struct nativeCommand *cmd, char *tokenBuffer)
{
	flushCmdParaStack();
	
	if (do_input) do_input( cmd, tokenBuffer );	// read from keyboad or disk.

	stack++;
	return tokenBuffer;
}

char *subCalc(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].str = NULL;
	kittyStack[stack].state = state_subData;

	stack++;

	if (kittyStack[stack].str) proc_names_printf("%s::Unexpcted data %08x on new stack pos %d\n",__FUNCTION__, kittyStack[stack].str,stack);

	return tokenBuffer;
}

char *subCalcEnd(struct nativeCommand *cmd, char *tokenBuffer)
{
	flushCmdParaStack();
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	flushCmdParaStack();
	unLockPara();
	flushCmdParaStack();

	return tokenBuffer;
}

char *breakData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	if (do_breakdata) do_breakdata( cmd, tokenBuffer );
	return tokenBuffer;
}


char *cmdNotEqual(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	if (tokenMode == mode_logical)
	{
		stackCmdParm(_not_equal, tokenBuffer);
		stack++;
	}
	else
	{
		proc_names_printf("Syntax error\n");
	}

	return tokenBuffer;
}

char *setVar(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	if (tokenMode == mode_logical)
	{
		stackCmdParm(_equalData, tokenBuffer);
		stack++;
	}
	else
	{
		stackCmdNormal( _do_set, tokenBuffer);
		if (tokenMode == mode_standard) tokenMode = mode_logical;		// first equal is set, next equal is logical
	}
	return tokenBuffer;
}

char *cmdIf(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	printf("at line %d\n", getLineFromPointer(tokenBuffer) );

	_num(0);	// stack reset.
	stackCmdNormal(_if, tokenBuffer);

	proc_names_printf("set mode_logical\n");
	tokenMode = mode_logical;
	return tokenBuffer;
}

char *cmdThen(struct nativeCommand *cmd, char *tokenBuffer)
{
	void *fn;
	char *ret = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	
	// empty the stack for what ever is inside the IF.

	while ((cmdStack)&&(stack))
	{
		if (cmdTmp[cmdStack-1].cmd == _if ) break;
		cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _if ) ret=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);


	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _ifSuccess) 
		{
			cmdTmp[cmdStack-1].cmd = _ifThenSuccess;
			cmdTmp[cmdStack-1].flag = cmd_eol;			// should run at end of line
		}
	}

	if (ret) tokenBuffer = ret;
	tokenMode = mode_standard;

	return tokenBuffer;
}

char *cmdElse(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	dump_prog_stack();

	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _ifSuccess)		// if success jump over else
		{
			char *ptr;
			int offset = *((unsigned short *) tokenBuffer);

			if (offset) 
			{
				ptr = tokenBuffer+(offset*2) -4;
				return ptr;
			}
		}
	}

	return tokenBuffer;
}

char *cmdElseIf(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	dump_prog_stack();

	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _ifSuccess)		// if success jump over else if
		{
			char *ptr;
			int offset = *((unsigned short *) tokenBuffer);

			if (offset) 
			{
				ptr = tokenBuffer+(offset*2) -2;
				return ptr;
			}
		}
	}

	_num(0);	// stack reset.
	stackCmdNormal(_else_if, tokenBuffer);
	proc_names_printf("set mode_logical\n");
	tokenMode = mode_logical;

	return tokenBuffer;
}

char *cmdEndIf(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _ifSuccess)
		{
			printf("removed 'If Success'\n");
			cmdStack--;
			getchar();
		}
	}

	return tokenBuffer;
}

char *cmdGoto(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	jump_mode = jump_mode_goto;
	return tokenBuffer;
}

char *cmdGosub(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	jump_mode = jump_mode_gosub;
	return tokenBuffer;
}

char *cmdDo(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	stackCmdLoop( _do, tokenBuffer );
	return tokenBuffer;
}

char *cmdRepeat(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	stackCmdLoop( _repeat, tokenBuffer );
	return tokenBuffer;
}

char *cmdLoop(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

//	dump_prog_stack();

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _do ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	return tokenBuffer;
}

char *cmdWhile(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	proc_names_printf("set mode_logical\n");
	tokenMode = mode_logical;

	stackCmdLoop( _while, tokenBuffer );
	stackCmdNormal( _whileCheck, tokenBuffer );
	
	return tokenBuffer;
}

char *cmdWend(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _while ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	return tokenBuffer;
}

char *cmdUntil(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	// we are changin the stack from loop to normal, so when get to end of line or next command, it be executed after the logical tests.

	proc_names_printf("set mode_logical\n");
	tokenMode = mode_logical;

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _repeat ) cmdTmp[cmdStack-1].flag = cmd_first;
	return tokenBuffer;
}

char *cmdTrue(struct nativeCommand *cmd, char *tokenBuffer)
{
	_num(-1);
	return tokenBuffer;
}

char *cmdFalse(struct nativeCommand *cmd, char *tokenBuffer)
{
	_num(0);
	return tokenBuffer;
}

char *cmdFor(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _for, tokenBuffer );
	cmdTmp[cmdStack-1].step = 1;		// set default counter step
	tokenMode = mode_for;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	return tokenBuffer;
}

char *cmdTo(struct nativeCommand *cmd, char *tokenBuffer )
{
	int flag;
	bool is_for_to = false;

	if (tokenMode == mode_for)
	{
		if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

		if (cmdStack) if ( cmdTmp[cmdStack-1].cmd == _setVar ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

		if (cmdStack) 
		{
			// We loop back to "TO" not "FOR", we are not reseting COUNTER var.

			if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_first ))
			{
				cmdTmp[cmdStack-1].tokenBuffer2 = tokenBuffer ;
				cmdTmp[cmdStack-1].flag = cmd_loop;
				is_for_to = true;
			}
		}
	}

	if (is_for_to == false) stack ++;

	return tokenBuffer;
}

char *cmdStep(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _step, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

extern char *executeToken( char *ptr, unsigned short token );

#undef NEXT_INT


int NEXT_INT( char *tokenBuffer , char **new_ptr )
{
	unsigned short token;
	char *ptr = tokenBuffer;

	token = *( (unsigned short *) ptr);
	ptr +=2;

	do 
	{
		ptr = executeToken( ptr, token );
		
		if (ptr == NULL) 
		{
			proc_names_printf("NULL\n");
			break;
		}

		last_token = token;
		token = *( (short *) ptr);
		ptr += 2;

	} while ((token != 0) && (token != 0x0356 ));

	*new_ptr = ptr - 2;

	return _stackInt(stack);
}

char *cmdNext(struct nativeCommand *cmd, char *tokenBuffer )
{
	char *ptr = tokenBuffer ;
	char *new_ptr = NULL;
	int idx_var = -1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (NEXT_TOKEN(ptr) == 0x0006 )	// Next var
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		idx_var = ref -> ref -1;
	}
	else 	// For var=
	{
		if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_loop ))
		{
			char *ptr = cmdTmp[cmdStack-1].tokenBuffer + 2  ;	// first short is JMP address, next after is token.

			if (NEXT_TOKEN(ptr) == 0x0006 )	// next is variable
			{
				struct reference *ref = (struct reference *) (ptr + 2);
				idx_var = ref -> ref -1;
			}
		}
	}
		
	if (idx_var>-1)
	{
		if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_loop ))
		{
			ptr = cmdTmp[cmdStack-1].tokenBuffer2;

			if (globalVars[idx_var].var.value < NEXT_INT(ptr, &new_ptr)  )
			{
				globalVars[idx_var].var.value +=cmdTmp[cmdStack-1].step; 
				tokenBuffer = new_ptr;
			}
			else
			{
				cmdStack--;
			}
		}
	}

	return tokenBuffer;
}

char *cmdEnd(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	return NULL;
}

char *cmdReturn(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _gosub ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	return tokenBuffer;
}

char *cmdProcedure(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	struct procedure *proc = (struct procedure *) tokenBuffer;

	proc_names_printf("Goto %08x\n",proc -> EndOfProc);

	return proc -> EndOfProc - sizeof(struct procedure);
}

char *cmdProcAndArgs(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	struct reference *ref = (struct reference *) (tokenBuffer);

	stackCmdNormal( _procAndArgs, tokenBuffer );
	tokenBuffer += ref -> length ;

	return tokenBuffer;
}

char *cmdProc(struct nativeCommand *cmd, char *tokenBuffer )
{
// this is dummy does not do anything, silly thing 
	return tokenBuffer;
}

char *cmdEndProc(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack)
	{
		proc_names_printf("%04x\n",NEXT_TOKEN(tokenBuffer));

		if (NEXT_TOKEN(tokenBuffer) == 0x0084 )	//  End Proc[ return value ]
		{
			proc_names_printf("yes we are here\n");

			// changes function pointer only so that ']' don't think its end of proc by accident.
			// we aslo push result of stack into Param.
			if (cmdTmp[cmdStack-1].cmd == _procedure ) cmdTmp[cmdStack-1].cmd = _endProc;
		}
		else 	// End Proc
		{
			if (cmdTmp[cmdStack-1].cmd == _procedure ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		}
	}

	return tokenBuffer;
}


char *cmdBracket(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _procedure ) 
	{
		struct reference *ref;
		struct glueCommands data;
		char *ptr;
		int args = 0;
		int n;
		unsigned short token;

		proc_names_printf("go down this path ....\n");

		args = stack - cmdTmp[cmdStack-1].stack +1;

		stack -= (args-1);
	
		n=0;
		for (ptr = tokenBuffer; (*((unsigned short *) ptr) != 0x008C) &&(n<args) ;ptr+=2)
		{
			token = *((unsigned short *) ptr);

			switch ( token )
			{
				case 0x0006:	

					ref = (struct reference *) (ptr+2);

					data.lastVar = ref->ref;
					_setVar( &data );

					ptr += sizeof(struct reference ) + ref -> length;
					n++;
					stack ++;
					break;

				case 0x005C:
					break;
			}

			ptr += 2;	 // next token
		}

		popStack( stack - cmdTmp[cmdStack-1].stack  );

		return ptr;
	}

	return tokenBuffer;
}

char *cmdBracketEnd(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _procAndArgs )
	{
		cmdTmp[cmdStack-1].tokenBuffer2 = tokenBuffer;
	}

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _endProc ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	return tokenBuffer;
}

char *cmdShared(struct nativeCommand *cmd, char *tokenBuffer )
{
	// we should not need to do anything here, but maybe good idea to jump over few tokens.
	return tokenBuffer;
}

char *cmdGlobal(struct nativeCommand *cmd, char *tokenBuffer )
{
	// we should not need to do anything here, but maybe good idea to jump over few tokens.
	return tokenBuffer;
}

char *cmdParamStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	setStackStrDup(var_param_str ? var_param_str : "" );
	return tokenBuffer;
}

char *cmdParamFloat(struct nativeCommand *cmd, char *tokenBuffer )
{
	setStackDecimal( var_param_decimal );
	return tokenBuffer;
}

char *cmdParam(struct nativeCommand *cmd, char *tokenBuffer )
{
	_num( var_param_num );
	return tokenBuffer;
}

char *cmdPopProc(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	// flush loops, all other stuff

	if (cmdStack)
	{
		while (cmdTmp[cmdStack-1].cmd != _procedure ) 
		{
			cmdStack--;
			if (cmdStack==0) break;
		}
	}

	return cmdEndProc( cmd, tokenBuffer );
}

char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );

char *_cmdRead( struct glueCommands *data )
{
	short token;
	unsigned short _len;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack  );


	if (data_read_pointer)
	{
		token = *((short *) data_read_pointer);

		switch (token)
		{
			case 0x003E: _num ( *((int *) (data_read_pointer + 2)) );
						data_read_pointer +=6;
						break;
	
			case 0x0026:	_len = *((unsigned short *) (data_read_pointer + 2));
						_len = _len + (_len & 1);
						if (kittyStack[stack].str) free(kittyStack[stack].str);
						kittyStack[stack].str = strndup( data_read_pointer + 4, _len );
						kittyStack[stack].len = strlen( kittyStack[stack].str );
						data_read_pointer +=4 + _len;
						break;

			default:
					proc_names_printf("--- token %04x ---\n",token);
					getchar();

		}

		token = *((short *) data_read_pointer);

		if (token == 0x005C)
		{
			data_read_pointer +=2;
			proc_names_printf("Next item\n");
		}
		else if (token == 0x0000 )
		{
			data_read_pointer +=4;
			proc_names_printf("end of data\n");

			data_read_pointer = FinderTokenInBuffer( data_read_pointer, 0x404, 0xFFFF, 0xFFFF,_file_end_ );	// this is really bad code.

			if (data_read_pointer)
			{
				token = *((short *) data_read_pointer);
				proc_names_printf("--- token %04x ---\n",token);
				if (token =0x0404) data_read_pointer+= 4; // it has data
			}
			else
			{
				proc_names_printf("nothing to be found, sorry\n");
			}

			// we need to seek to next "data" command
		}

	}

	stack ++;

	_setVarReverse( data );

	return NULL;
}


char *cmdRead(struct nativeCommand *cmd, char *tokenBuffer )
{

	dump_global();

	proc_names_printf("read %04x\n", *((short *) tokenBuffer) );

	stackCmdNormal( _cmdRead, tokenBuffer );

	return tokenBuffer;
}

char *cmdData(struct nativeCommand *cmd, char *tokenBuffer )
{
	return tokenBuffer;
}


char *cmdOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	int num = 0;
	unsigned short ref_num = 0;
	unsigned short token = 0;

	tokenBuffer += 4;	// skip crap, no idea what use this for... :-P

	if (NEXT_TOKEN(tokenBuffer) == 0x0006 )	// next is variable
	{
		struct reference *ref = (struct reference *) (tokenBuffer + 2);
		int idx = ref->ref-1;

		proc_names_printf("works\n");

		switch ( globalVars[idx].var.type )
		{
			case type_int:
				num = globalVars[idx].var.value;
				break;
		}

		tokenBuffer += (2 + ref -> length + sizeof(struct reference ));

		token = NEXT_TOKEN(tokenBuffer);
		if ( (token==0x02A8) || (token==0x02B2) || (token==0x0386) ) 
		{
			tokenBuffer += 2;	// we know this tokens..

			proc_names_printf("success I think :-) num is %d\n", num);

			for(;;)
			{			
				switch (NEXT_TOKEN(tokenBuffer))
				{
					case 0x0006:
						tokenBuffer +=2;
						ref = (struct reference *) (tokenBuffer);
						num--;
						if (num == 0)	ref_num = ref -> ref;
						tokenBuffer += sizeof(struct reference) + ref -> length;
						break;
					case 0x005C:
						tokenBuffer +=2;
						break;
					default: 
						goto exit_on_for_loop;
				}
			}

exit_on_for_loop:

			if (ref_num>0)
			{
				proc_names_printf("name: %s\n",globalVars[ref_num-1].varName);

				switch (token)
				{
					case 0x02A8:	// goto
							tokenBuffer = findLabel(globalVars[ref_num-1].varName);
							break;
					case 0x02B2:	// gosub
							stackCmdLoop( _gosub, tokenBuffer + 2);
							tokenBuffer = findLabel(globalVars[ref_num-1].varName);
							break;
					case 0x0386:	// proc
							break;
				}

			}
		}
	}

	tokenBuffer -= 4;	// yes right, 4 will be added by main program.

	return tokenBuffer;
}

void _input_arg( struct nativeCommand *cmd, char *tokenBuffer );
void _inputLine_arg( struct nativeCommand *cmd, char *tokenBuffer );

int input_count = 0;
std::string input_str;

char *_Input(struct glueCommands *data)
{
	int args = stack - data -> stack +1;
	_input_arg( NULL, NULL );
	popStack( stack - data -> stack  );
	do_input = NULL;
	do_breakdata = NULL;
	return NULL;
}

char *_LineInput(struct glueCommands *data)
{
	int args = stack - data -> stack +1;
	_inputLine_arg( NULL, NULL );
	popStack( stack - data -> stack  );
	do_input = NULL;
	do_breakdata = NULL;
	return NULL;
}

void _input_arg( struct nativeCommand *cmd, char *tokenBuffer )
{
	int args = 0;
	int index = 0;
	int idx;
	size_t i;
	std::string arg = "";
	struct glueCommands data;
	bool success = false;
	int num;
	double des;

	if (cmd == NULL)
	{
		args = stack - cmdTmp[cmdStack].stack + 1;
	}
	else
	{
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _Input)
		{
			args = stack - cmdTmp[cmdStack-1].stack + 1;
		}
	}
	
	if ((input_count == 0)&&(stack))		// should be one arg.
	{
		char *str = _stackString( stack-args+1 );
		if (str) printf("%s", str);
	}
	else if (input_str.empty())
	{
		proc_names_printf("??? ");
	}

	do
	{
		do
		{
			while (input_str.empty()) getline(cin, input_str);

			i = input_str.find(",");	
			if (i != std::string::npos)
			{
				arg = input_str.substr(0,i); input_str.erase(0,i+1);
			}
			else	
			{
				arg = input_str; input_str = "";
			}
		}
		while ( arg.empty() );

		if (last_var)
		{
			switch (globalVars[last_var -1].var.type & 7)
			{	
				case type_string:
					success = true; break;
				case type_int:
					success = arg.find_first_not_of( "-0123456789" ) == std::string::npos; break;
				case type_float:
					success = arg.find_first_not_of( "-0123456789." ) == std::string::npos; break;
			}
		}
	}
	while (!success);

	switch (globalVars[last_var -1].var.type & 7)
	{	
		case type_string:
			setStackStrDup(arg.c_str()); break;

		case type_int:
			sscanf(arg.c_str(),"%d",&num); _num(num); break;

		case type_float:
			sscanf(arg.c_str(),"%lf",&des); setStackDecimal(des); break;
	}

	data.lastVar = last_var;
	_setVar( &data );
	input_count ++;
}

void _inputLine_arg( struct nativeCommand *cmd, char *tokenBuffer )
{
	int args = 0;
	int index = 0;
	int idx;
	size_t i;
	std::string arg = "";
	struct glueCommands data;
	bool success = false;
	int num;
	double des;

	if (cmd == NULL)
	{
		args = stack - cmdTmp[cmdStack].stack + 1;
	}
	else
	{
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _Input)
		{
			args = stack - cmdTmp[cmdStack-1].stack + 1;
		}
	}
	
	if ((input_count == 0)&&(stack))		// should be one arg.
	{
		char *str = _stackString( stack-args+1 );
		if (str) proc_names_printf("%s", str);
	}
	else if (input_str.empty())
	{
		proc_names_printf("??? ");
	}

	do
	{
		do
		{
			while (input_str.empty()) getline(cin, input_str);
			arg = input_str; input_str = "";
		}
		while ( arg.empty() );

		if (last_var)
		{
			switch (globalVars[last_var -1].var.type & 7)
			{	
				case type_string:
					success = true; break;
				case type_int:
					success = arg.find_first_not_of( "-0123456789" ) == std::string::npos; break;
				case type_float:
					success = arg.find_first_not_of( "-0123456789." ) == std::string::npos; break;
			}
		}
	}
	while (!success);

	switch (globalVars[last_var -1].var.type & 7)
	{	
		case type_string:
			setStackStrDup(arg.c_str()); break;

		case type_int:
			sscanf(arg.c_str(),"%d",&num); _num(num); break;

		case type_float:
			sscanf(arg.c_str(),"%lf",&des); setStackDecimal(des); break;
	}

	data.lastVar = last_var;
	_setVar( &data );
	input_count ++;
}

void breakdata_inc_stack( struct nativeCommand *cmd, char *tokenBuffer )
{
	stack++;
}

char *cmdInput(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	input_count = 0;
	input_str = "";

	do_input = _input_arg;
	do_breakdata = breakdata_inc_stack;
	stackCmdNormal( _Input, tokenBuffer );

	return tokenBuffer;
}

char *cmdLineInput(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	input_count = 0;
	input_str = "";

	do_input = _inputLine_arg;
	do_breakdata = breakdata_inc_stack;
	stackCmdNormal( _LineInput, tokenBuffer );

	return tokenBuffer;
}

char *_cmdExit(struct glueCommands *data)
{
	int exit_loops = 1;
	unsigned short token;
	char *ptr;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data -> stack +1;

	if (args==1) exit_loops = _stackInt(stack);
	popStack( stack - data -> stack  );

	while (exit_loops>1)
	{
		if (dropProgStackToType( cmd_loop )) cmdStack--;
		exit_loops--;
	}

	if (dropProgStackToType( cmd_loop ))
	{
		ptr = cmdTmp[cmdStack-1].tokenBuffer;
		token = *((unsigned short *) (ptr - 2)) ;

		switch (token)
		{
			case 0x023C:	// For
			case 0x0250:	// Repeat
			case 0x0268:	// While
			case 0x027E:	// DO

				cmdStack --;
				ptr =  ptr + ( *((unsigned short *) ptr) * 2 )   ;
				return (ptr+2);
				break;

			default:
				dump_prog_stack();
				proc_names_printf("token was %08x\n", token);
				getchar();
		}
	}

	return NULL;
}

char *cmdExit(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdExit, tokenBuffer );

	return tokenBuffer;
}

char *_cmdExitIf(struct glueCommands *data)
{
	int exit_loops = 1;
	bool is_true = false;
	unsigned short token;
	char *ptr;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data -> stack +1;

	switch (args)
	{
		case 1:
			is_true = _stackInt(stack);
			break;
		case 2:
			is_true = _stackInt(stack -1);
			exit_loops = _stackInt(stack);
			break;
	}

	popStack( stack - data -> stack  );

	if (is_true == false) return NULL;

	while (exit_loops>1)
	{
		if (dropProgStackToType( cmd_loop )) cmdStack--;
		exit_loops--;
	}

	if (dropProgStackToType( cmd_loop ))
	{
		ptr = cmdTmp[cmdStack-1].tokenBuffer;
		token = *((unsigned short *) (ptr - 2)) ;

		switch (token)
		{
			case 0x023C:	// For
			case 0x0250:	// Repeat
			case 0x0268:	// While
			case 0x027E:	// DO

				cmdStack --;
				proc_names_printf("exit from loop %04x\n", token);
				ptr =  ptr + ( *((unsigned short *) ptr) * 2 )   ;
				return (ptr+2);
				break;

			default:
				dump_prog_stack();
				proc_names_printf("token was %08x\n", token);
				getchar();
		}
	}

	return NULL;
}

char *cmdExitIf(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdExitIf, tokenBuffer );
	tokenMode = mode_logical;

	return tokenBuffer;
}


char *_cmdEvery( struct glueCommands *data )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	return  data -> tokenBuffer ;
}

char *_cmdWait( struct glueCommands *data )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;

	Delay( _stackInt(data->stack) );

	return  data -> tokenBuffer ;
}

char *cmdEveryOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	every_on = true;
	return tokenBuffer;
}

char *cmdEveryOff(struct nativeCommand *cmd, char *tokenBuffer )
{
	every_on = false;
	return tokenBuffer;
}

char *cmdEvery(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	on_every_proc_location = NULL;
	on_every_gosub_location = NULL;

	if (NEXT_TOKEN(tokenBuffer) == 0x003E )	// next is variable
	{
		every_timer = *((int *) (tokenBuffer + 2));
		tokenBuffer += 6;

		switch (NEXT_TOKEN(tokenBuffer))
		{
			// gosub
			case 0x02B2:
					tokenBuffer += 2;

					if (NEXT_TOKEN(tokenBuffer ) == 0x006)	// label
					{
						char *name;
						struct reference *ref = (struct reference *) (tokenBuffer + 2);
						name = strndup( tokenBuffer + 2 + sizeof(struct reference), ref->length );	

						if (name)
						{
							on_every_gosub_location = findLabel(name);
							every_on = true;
							free(name);
						}

						tokenBuffer += (2 + sizeof(struct reference) + ref -> length) ;					
					}

					break;
			// proc
			case 0x0386:
					tokenBuffer += 2;

					switch (NEXT_TOKEN(tokenBuffer ))
					{
						case 0x0012:
						case 0x0006:

							char *name;
							struct reference *ref = (struct reference *) (tokenBuffer + 2);
							name = strndup( tokenBuffer + 2 + sizeof(struct reference), ref->length );

							if (name)
							{
								int found = findVarPublic(name);
								if (found)
								{
									on_every_proc_location = globalVars[found -1].var.tokenBufferPos;
									every_on = true;
								}

								free(name);
							}

							tokenBuffer += (2 + sizeof(struct reference) + ref -> length) ;	
					}
	
					break;
		}

		printf("every timer: %d\n",every_timer);

	}

	gettimeofday(&every_before, NULL);	// reset diff.

	stackCmdNormal( _cmdEvery, tokenBuffer );

	return tokenBuffer;
}

char *cmdWait(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdWait, tokenBuffer );

	return tokenBuffer;
}


char *_set_timer( struct glueCommands *data )
{
	timer_offset = _stackInt( stack );
	gettimeofday(&timer_before, NULL);	// reset diff.
	_do_set = _setVar;
	return NULL;
}


char *cmdTimer(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned int ms_before;
	unsigned int ms_after;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((last_token == 0x0000) && (NEXT_TOKEN(tokenBuffer) == 0xFFA2 ))
	{
		tokenMode = mode_store;
		_do_set = _set_timer;
	}

	gettimeofday(&timer_after, NULL);	// reset diff.

	ms_before = (timer_before.tv_sec * 1000) + (timer_before.tv_usec/1000);
	ms_after = (timer_after.tv_sec * 1000) + (timer_after.tv_usec/1000);

	_num( ((ms_after - ms_before) / 20) + timer_offset );		// 1/50 sec = every 20 ms

	return tokenBuffer;
}

char *cmdBreakOff(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdBreakOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdCloseWorkbench(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdCloseEditor(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

