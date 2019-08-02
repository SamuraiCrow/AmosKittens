
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include <string>
#include <iostream>
#include <math.h>

#include "debug.h"
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

bool correct_order( int last_token, int next_token )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (last_token)
	{
		case token_semi:
			if ((next_token == token_add)
				|| (next_token == token_sub)
				|| (next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)
				|| (next_token == token_more_or_equal )
				|| (next_token == token_less_or_equal	)
				|| (next_token == token_less_or_equal2 )
				|| (next_token == token_more_or_equal2 )
				|| (next_token == token_not_equal )
				|| (next_token == token_equal )
				|| (next_token == token_more )
				|| (next_token == token_less ) 
				|| (next_token == token_or)
				|| (next_token == token_xor)		// don't know the token number yet.
				|| (next_token == token_and)) return false;
			break;

		case token_or:
		case token_xor:
		case token_and:
			if ((next_token == token_add)
				|| (next_token == token_sub)
				|| (next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)
				|| (next_token == token_more_or_equal )
				|| (next_token == token_less_or_equal	)
				|| (next_token == token_less_or_equal2 )
				|| (next_token == token_more_or_equal2 )
				|| (next_token == token_not_equal )
				|| (next_token == token_equal )
				|| (next_token == token_more )
				|| (next_token == token_less )) return false;
			break;

		case token_more_or_equal:
		case token_less_or_equal:
		case token_less_or_equal2:
		case token_more_or_equal2:
		case token_not_equal:
		case token_equal:
		case token_more:
		case token_less:

			if ((next_token == token_add)
				|| (next_token == token_sub)
				|| (next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)) return false;
			break;

		case token_add:
			if ((next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)) return false;
			break;

		case token_sub:
			if ((next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)) return false;
			break;

		case token_mul:
			if (next_token == token_power) return false;
			break;

		case token_div	:
			if (next_token == token_power) return false;
			break;
	}
	
	return true;
}

char *_equalData( struct glueCommands *data, int nextToken )
{
	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	item0 = kittyStack + stack-1;
	item1 = kittyStack + stack;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		stack --;
		
		if (type1 == type_int)
		{
			setStackNum( item0->decimal.value == (double) item1->integer.value ? ~0 : 0);
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->decimal.value == item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		stack --;

		if (type1 == type_int)
		{
			setStackNum( item0->integer.value == item1->integer.value  ? ~0 : 0);
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->integer.value == item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackEqualStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}


char *_lessData( struct glueCommands *data, int nextToken )
{
	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	item0 = kittyStack + stack-1;
	item1 = kittyStack + stack;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		stack --;
		
		if (type1 == type_int)
		{
			setStackNum( item0->decimal.value < (double) item1->integer.value  ? ~0 : 0 );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			proc_names_printf ("( %d > %d ) = %d \n", item0->decimal.value , item1->decimal.value , item0->decimal.value < item1->decimal.value);
			setStackNum( item0->decimal.value < item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		stack --;

		if (type1 == type_int)
		{
			dprintf ("( %d < %d ) = %d \n", item0->integer.value , item1->integer.value , item0->integer.value > item1->integer.value);
			setStackNum( item0->integer.value < item1->integer.value  ? ~0 : 0);
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->integer.value < item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackLessStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *_lessOrEqualData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ((stack - data -> stack + 1)!=2)
	{
		printf("stack - data -> stack + 1 is %d\n",stack - data -> stack + 1);

		setError(22,data->tokenBuffer);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackNum( item0->decimal.value <= (double) item1->integer.value  ? ~0 : 0);
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->decimal.value <= item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			proc_names_printf ("( %d <= %d ) = %d \n", item0->integer.value , item1->integer.value , item0->integer.value >= item1->integer.value);
			setStackNum( item0->integer.value <= item1->integer.value  ? ~0 : 0);
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->integer.value <= item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackLessOrEqualStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}


char *_moreData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackNum( item0->decimal.value > (double) item1->integer.value  ? ~0 : 0);
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->decimal.value > item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			dprintf ("( %d > %d ) = %d \n", item0->integer.value , item1->integer.value , item0->integer.value > item1->integer.value);
			setStackNum( item0->integer.value > item1->integer.value  ? ~0 : 0);
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->integer.value > item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackMoreStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *_moreOrEqualData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	item0 = kittyStack + stack - 1;
	item1 = kittyStack + stack;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		stack --;

		if (type1 == type_int)
		{
			setStackNum( item0->decimal.value >= (double) item1->integer.value  ? ~0 : 0);
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->decimal.value >= item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		stack --;

		if (type1 == type_int)
		{
			dprintf ("( %d >= %d ) = %d \n", item0->integer.value , item1->integer.value , item0->integer.value >= item1->integer.value);
			setStackNum( item0->integer.value >= item1->integer.value  ? ~0 : 0);
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->integer.value >= item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackMoreOrEqualStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *_orData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackNum( (item0->decimal.value != 0) || (item1->integer.value != 0)  ? ~0 : 0);
			success = true;
		}
		else if (type1 == type_float)
		{
			setStackNum( (item0->decimal.value != 0) || (item1->decimal.value != 0)  ? ~0 : 0);
			success = true;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			setStackNum( (item0->integer.value != 0) ||  (item1->integer.value != 0)  ? ~0 : 0);
			success = true;
		}
		else if (type1 == type_float)
		{
			setStackNum(  (item0->integer.value != 0 ) || (item1->decimal.value != 0)  ? ~0 : 0);
			success = true;
		}
	}

	if (success)
	{
		correct_for_hidden_sub_data();
	}
	else
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *_andData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackNum( (int) item0->decimal.value & item1->integer.value );
			success = true;
		}
		else if (type1 == type_float)
		{
			setStackNum( (int) item0->decimal.value & (int) item1->decimal.value );
			success = true;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			setStackNum( item0->integer.value & item1->integer.value );
			success = true;
		}
		else if (type1 == type_float)
		{
			setStackNum(  item0->integer.value & (int) item1->decimal.value );
			success = true;
		}
	}

	if (success)
	{
		correct_for_hidden_sub_data();
	}
	else
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *_xorData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackNum( ((int) item0->decimal.value) ^ item1->integer.value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( ((int) item0->decimal.value) ^ ((int) (item1->decimal.value)) );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			setStackNum( item0->integer.value ^ item1->integer.value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->integer.value ^ ((int) item1->decimal.value) );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}


char *_addData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackDecimal( item0->decimal.value + (double) item1->integer.value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( item0->decimal.value + item1->decimal.value );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			dprintf(" = %d + %d\n", item0->integer.value , item1->integer.value );

			setStackNum( item0->integer.value + item1->integer.value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( (double) item0->integer.value + item1->decimal.value );
			success = TRUE;
		}
	}
	else if ( type0 == type_string) 
	{
		switch (type1)
		{
			case type_int:		success = stackStrAddValue( item0, item1 ); break;
			case type_float:	success = stackStrAddDecimal( item0, item1 ); break;
			case type_string:	success = stackStrAddStr( item0, item1 ); break;
		}
	}

	if (success )
	{
		correct_for_hidden_sub_data();
	}
	else
	{
		dprintf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *_addDataToText( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d stack is %d cmd stack is %d state %d\n",__FILE__,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	int args;
	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;
	struct stringData *buffer = (struct stringData *) alloca( sizeof(struct stringData) + 100);

	args = stack - data -> stack + 1;

	if (args<2)
	{
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	switch( type0 )
	{
		case type_int:
			sprintf(&(buffer->ptr),(item0->integer.value>-1) ? " %d" : "%d", item0->integer.value );
			buffer -> size = strlen(&(buffer->ptr));
			setStackStrDup(buffer);
			break;

		case type_float:
			sprintf(&(buffer->ptr),(item0->decimal.value>=0.0f) ? " %f" : "%f", item0->decimal.value );
			buffer -> size = strlen(&(buffer->ptr));
			setStackStrDup(buffer);
			break;
	}

	switch (type1)
	{
		case type_int:		success = stackStrAddValue( item0, item1 ); break;
		case type_float:	success = stackStrAddDecimal( item0, item1 ); break;
		case type_string:	success = stackStrAddStr( item0, item1 ); break;
		case type_none:	success = true; stack++; break;	// nothing to add, will be added some where else.
	}
	
	if (success )
	{
		correct_for_hidden_sub_data();
	}
	else
	{
		dprintf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

bool _subStr( struct kittyData *item0, struct kittyData *item1 )
{
	proc_names_printf("%s%s:%d stack is %d cmd stack is %d state %d\n",__FILE__,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct stringData *string;
	struct stringData *remove;
	int spos;
	char *d,*s;

	string = item0 -> str;
	remove = item1 -> str;

	if ((string)&&(remove))
	{
		int new_len = string->size;
		s=d=&string -> ptr;
		for(spos=0;spos < (new_len - remove->size) ;spos++)
		{
			if (memcmp(s,&remove -> ptr,remove -> size)==0) 
			{		
				s+=remove -> size;
				new_len -= remove -> size;
				printf("removed %d\n",remove -> size);
			}

			*d++=*s++;
		}
		*d = 0;

		string -> size = new_len;
		return true;
	}

	return false;
}

char *_subData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d stack is %d cmd stack is %d state %d\n",__FILE__,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	switch (type0)
	{
		case type_none:
				if (type1 == type_int)
				{
					setStackNum( - item1->integer.value );
					dprintf(" 0 - %d = %d\n",  item1->integer.value, - item1->integer.value );
					success = TRUE;
				}
				else if (type1 == type_float)
				{
					setStackDecimal( - item1->decimal.value );
					success = TRUE;
				}
				break;

		case type_float:
				if (type1 == type_int)
				{
					setStackDecimal( item0->decimal.value - (double) item1->integer.value );
					success = TRUE;
				}
				else if (type1 == type_float)
				{
					setStackDecimal( item0->decimal.value - item1->decimal.value );
					success = TRUE;
				}
				break;
		case type_int:
				if (type1 == type_int)
				{
					dprintf(" = %d - %d\n", item0->integer.value , item1->integer.value );
					setStackNum( item0->integer.value - item1->integer.value );
					success = TRUE;
				}
				else if (type1 == type_float)
				{
					setStackDecimal( (double) item0->integer.value - item1->decimal.value );
					success = TRUE;
				}
				break;
		case type_string: 
				if ( type1 == type_string ) 
				{
					success = _subStr( item0 , item1 ); 
					stack++;
					popStack( 1 );
				}
				break;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		dprintf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *_modData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			dprintf("%f %% %d\n",  item0->decimal.value , (double) item1->integer.value );
			setStackNum( (int) item0->decimal.value % item1->integer.value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			dprintf("%f %% %f\n",  item0->decimal.value , item1->decimal.value );
			setStackNum( (int) item0->decimal.value % (int) item1->decimal.value );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			dprintf(" %d %% %d\n", item0->integer.value , item1->integer.value );
			setStackNum( item0->integer.value % item1->integer.value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			printf("%f %% %f\n",  (double) item0->integer.value , item1->decimal.value );
			setStackNum( item0->integer.value % (int) item1->decimal.value );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *_mulData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d stack is %d cmd stack is %d state %d\n",__FILE__,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			dprintf("%lf * %d\n",  item0->decimal.value , item1->integer.value );
			setStackDecimal( item0->decimal.value * (double) item1->integer.value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			dprintf("%lf * %lf\n",  item0->decimal.value , item1->decimal.value );
			setStackDecimal( item0->decimal.value * item1->decimal.value );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			dprintf("%d * %d\n", item0->integer.value , item1->integer.value );
			setStackNum( item0->integer.value * item1->integer.value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			dprintf("%d * %lf\n", item0->integer.value , item1->decimal.value );
			setStackDecimal( (double) item0->integer.value * item1->decimal.value );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *_divData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		proc_names_printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			dprintf(" %f / %d\n", item0->decimal.value , item1->integer.value );
			setStackDecimal( item0->decimal.value / (double) item1->integer.value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			dprintf(" %f / %f\n", item0->decimal.value , item1->decimal.value );
			setStackDecimal( item0->decimal.value / item1->decimal.value );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			dprintf(" %d / %d\n", item0->integer.value , item1->integer.value );
			setStackNum( item0->integer.value / item1->integer.value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			dprintf(" %d / %f\n", item0->integer.value , item1->decimal.value );
			setStackDecimal( (double) item0->integer.value / item1->decimal.value );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *_powerData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		proc_names_printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackDecimal( pow( item0->decimal.value , (double) item1->integer.value ) );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( pow( item0->decimal.value , item1->decimal.value ) );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			proc_names_printf(" = %d ^ %d\n", item0->integer.value , item1->integer.value );
			setStackNum( (int) pow( item0->integer.value , item1->integer.value ) );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( pow( (double) item0->integer.value , item1->decimal.value ) );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}
	return NULL;
}

char *addData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdMathOperator( _addData, tokenBuffer, token_add );
	incStack;
	return tokenBuffer;
}

char *subData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdMathOperator(_subData,tokenBuffer, token_sub );
	incStack;
	return tokenBuffer;
}

char *modData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdMathOperator( _modData, tokenBuffer, token_mod );
	incStack;
	return tokenBuffer;
}

char *mulData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdMathOperator( _mulData, tokenBuffer, token_mul );
	incStack;
	return tokenBuffer;
}

char *divData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdMathOperator( _divData, tokenBuffer, token_div );
	incStack;
	return tokenBuffer;
}

char *powerData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdMathOperator( _powerData, tokenBuffer, token_power );
	incStack;
	return tokenBuffer;
}

char *orData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdMathOperator( _orData, tokenBuffer, token_or );
	incStack;
	return tokenBuffer;
}

char *andData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdMathOperator( _andData, tokenBuffer, token_and );
	incStack;
	return tokenBuffer;
}

char *xorData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdMathOperator( _xorData, tokenBuffer, token_xor );
	incStack;
	return tokenBuffer;
}

char *lessData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdMathOperator(_lessData, tokenBuffer, token_less );
	incStack;
	return tokenBuffer;
}

char *moreData(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdMathOperator(_moreData, tokenBuffer, token_more );
	incStack;
	return tokenBuffer;
}

char *lessOrEqualData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdMathOperator(_lessOrEqualData, tokenBuffer, token_less_or_equal );
	incStack;
	return tokenBuffer;
}

char *moreOrEqualData(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdMathOperator(_moreOrEqualData,tokenBuffer, token_more_or_equal);
	incStack;
	return tokenBuffer;
}

char *_not_equal( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22,data->tokenBuffer);
		return NULL;
	}

	item0 = kittyStack + stack-1;
	item1 = kittyStack + stack;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	if (type0 == type_float) 
	{
		stack --;	

		if (type1 == type_int)
		{
			setStackNum( item0->decimal.value != (double) item1->integer.value ? ~0 : 0 );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->decimal.value != item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		stack--;

		if (type1 == type_int)
		{
			dprintf ("( %d != %d ) = %d \n", item0->integer.value , item1->integer.value , item0->integer.value != item1->integer.value);

			setStackNum( item0->integer.value != item1->integer.value  ? ~0 : 0);
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->integer.value != item1->decimal.value  ? ~0 : 0);
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackNotEqualStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch,data->tokenBuffer);
		return NULL;
	}

	return NULL;
}

char *cmdNotEqual(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdMathOperator(_not_equal, tokenBuffer, token_not_equal);
	incStack;

	return tokenBuffer;
}

