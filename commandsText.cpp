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

extern int pen0;
extern int pen1;
extern int pen2;
extern int last_var;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern int current_screen;

bool curs_on = true;
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

char *_textHome( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	bool success = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		if (screens[current_screen])
		{
			screens[current_screen] -> locateX = 0;
			screens[current_screen] -> locateY = 0;
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

void __print_text(const char *txt, int maxchars)
{
	if (engine_started)
	{
		if (screens[current_screen])
		{
			_my_print_text(  screens[current_screen], (char *) txt, maxchars);
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
	__print_text(tmp,0);
}

void __print_double( double d )
{
	char tmp[40];
	sprintf(tmp,"%lf",d);
	__print_text(tmp,0);
}

char *_print( struct glueCommands *data )
{
	int n;

	if (screens[current_screen]) clear_cursor(screens[current_screen]);

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
				if (kittyStack[n].str) __print_text(kittyStack[n].str,0);
				break;
		}

		if (n<stack) __print_text("    ",0);
	}
	__print_text("\n",0);
	if (screens[current_screen]) draw_cursor(screens[current_screen]);

	popStack( stack - data->stack );
	do_breakdata = NULL;	// done doing that.

	return NULL;
}

char *_textCentre( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int n;
	int charsPerLine = 100;
	const char *txt = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	dump_stack();

	printf("args: %d, stack %d, data -> stack %d \n",args,stack,data->stack);

	if (args!=1) setError(22);

	if (engine_started)
	{
		if (screens[current_screen])
		{
			txt = _stackString(stack);
			charsPerLine = screens[current_screen] -> realWidth / 8;

			clear_cursor(screens[current_screen]);

			if (txt)
			{
				screens[current_screen] -> locateX = (charsPerLine/2) - (strlen( txt ) / 2);

				if (screens[current_screen] -> locateX<0)
				{
					txt -= screens[current_screen] -> locateX;	// its read only.
					screens[current_screen] -> locateX = 0;
				}
			}
		}
	}

	if (txt)
	{
		__print_text(txt,0);
	}

	if (screens[current_screen]) draw_cursor(screens[current_screen]);

	popStack( stack - data->stack );
	do_breakdata = NULL;	// done doing that.

	return NULL;
}

char *textPaper(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textPaper, tokenBuffer );
	return tokenBuffer;
}

char *textCentre(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textCentre, tokenBuffer );
	return tokenBuffer;
}

char *_addDataToText( struct glueCommands *data );

void _print_break( struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _addDataToText, tokenBuffer );
	stack++;
}

char *textPrint(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _print, ptr );
	do_breakdata = _print_break;

	return ptr;
}

char *textCursOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	clear_cursor(screens[current_screen]);
	curs_on = false;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	return tokenBuffer;
}

char *textHome(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _textHome, tokenBuffer );
	return tokenBuffer;
}

char *textInverseOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	int t;

	t = pen0 ;
	pen0 = pen1;
	pen1 = t;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *textInverseOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	int t;

	t = pen0 ;
	pen0 = pen1;
	pen1 = t;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_textBorderStr( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	char *newstr = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 2)
	{
		char *txt = _stackString( stack-1 );
		int border = _stackInt( stack );

		if ((txt)&&(border>0)&&(border<16))
		{
			newstr = (char *) malloc( strlen(txt) + 6 + 1 ); 
			if (newstr)
			{
				sprintf(newstr,"%cE0%s%cE%c",27,txt,27,48+ border );
			}
		}

		if (newstr == NULL) setError(60);
	}
	else setError(22);

	popStack( stack - data->stack );
	if (newstr) setStackStr( newstr );

	return NULL;
}

char *textBorderStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _textBorderStr, tokenBuffer );
	return tokenBuffer;
}



