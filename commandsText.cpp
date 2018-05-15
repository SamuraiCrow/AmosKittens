#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/retroMode.h>

#include "stack.h"
#include "amosKittens.h"
#include "commandsData.h"
#include "commandsText.h"
#include "commandsScreens.h"
#include "errors.h"
#include "engine.h"
#include "bitmap_font.h"

extern int last_var;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern int current_screen;

int pen = 2;
int paper = 1;

void _print_break( struct nativeCommand *cmd, char *tokenBuffer );

char *_textLocate( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	bool success = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		if (screens[current_screen])
		{
			screens[current_screen] -> locateX = _stackInt( stack-1 );
			screens[current_screen] -> locateY = _stackInt( stack );
		}
		success = true;
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *textLocate(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textLocate, tokenBuffer );
	return tokenBuffer;
}

char *_textPen( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	bool success = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		pen  = _stackInt( stack );
		success = true;
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *textPen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textPen, tokenBuffer );
	return tokenBuffer;
}

char *_textPaper( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	bool success = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		paper = _stackInt( stack );
		success = true;
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

void __print_text(const char *txt)
{
	if (engine_started)
	{
		if (screens[current_screen])
		{
			_my_print_text(  screens[current_screen], (char *) txt);
		}
	}
	else
	{
		printf("%s", txt);
	}
}

void __print_num( int num )
{
	char tmp[40];
	sprintf(tmp,"%d",num);
	__print_text(tmp);
}

void __print_double( double d )
{
	char tmp[40];
	sprintf(tmp,"%lf",d);
	__print_text(tmp);
}

char *_print( struct glueCommands *data )
{
	int n;

	for (n=data->stack;n<=stack;n++)
	{
		switch (kittyStack[n].type)
		{
			case type_int:
				__print_num( kittyStack[n].value);
				break;
			case type_float:
				__print_double( kittyStack[n].decimal);
				break;
			case type_string:
				if (kittyStack[n].str) __print_text(kittyStack[n].str);
				break;
		}

		if (n<stack) __print_text("    ");
	}
	__print_text("\n");

	popStack( stack - data->stack );
	do_breakdata = NULL;	// done doing that.

	return NULL;
}
char *textPaper(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textPaper, tokenBuffer );
	return tokenBuffer;
}


void _print_break( struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _addData, tokenBuffer );
	stack++;
}

char *cmdPrint(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _print, ptr );
	do_breakdata = _print_break;
	return ptr;
}

