#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "config.h"

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <retromode.h>
#include <retromode_lib.h>
#include <unistd.h>
#endif

#include "debug.h"
#include <string>
#include <vector>
#include <iostream>
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsbanks.h"
#include "errors.h"
#include "engine.h"

#include "interfacelanguage.h"

extern int last_var;
extern struct globalVar globalVars[];
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern int current_screen;
extern std::vector<struct kittyBank> kittyBankList;

extern char *(*_do_set) ( struct glueCommands *data, int nextToken ) ;

std::vector<struct cmdcontext *> icmdcontexts;

int current_resource_bank = 16;

void _my_print_text(struct retroScreen *screen, char *text, int maxchars);

uint8_t getByte( char *adr, int &pos )
{
	uint8_t ret = *((uint8_t *) (adr + pos));
	pos+=1;
	return ret;
}

uint16_t getWord( char *adr, int &pos )
{
	short ret = *((uint16_t *) (adr + pos));
	pos+=2;
	return ret;
}

uint32_t getLong( char *adr, int &pos )
{
	short ret = *((uint32_t *) (adr + pos));
	pos+=4;
	return ret;
}

bool __resource_bank_has_pictures( struct kittyBank *bank1 )
{
	struct resourcebank_header *header; 
	int pos,pupics;

	if (bank1 == NULL) return false;

	header = (resourcebank_header*) bank1->start;
	if (header -> img_offset == 0) return false;

	pos = header -> img_offset;
	pupics = getWord( bank1->start, pos );

	if (pupics==0) return false;
	return true;
}

void init_amos_kittens_screen_resource_colors(struct retroScreen *screen)
{
	struct kittyBank *bank1;

	bank1 = findBank(current_resource_bank);

	 if (__resource_bank_has_pictures( bank1 ) == false )
	{
		bank1 = findBank(-2);
	}

	if (bank1)
	{
		struct resourcebank_header *header = (resourcebank_header*) bank1->start;
		int hunk,pos,adr_gfx,pupics,color,colors,mode,n;
		int videomode = 0;

		hunk = header -> img_offset ;
   		pos=hunk; 
		adr_gfx = hunk;

		pupics = getWord( bank1->start, pos );

   		pos = hunk =adr_gfx+2+pupics*4 ;

		colors = getWord( bank1->start, pos );
		mode = getWord( bank1->start, pos );

		if (mode & 0x8000)
		{
			 videomode |= retroHires; 
		}
		else
		{
			 videomode |= retroLowres_pixeld; 
		}

		if (mode & 0x2000) videomode |= retroHam6;

		screen -> videomode = videomode;


  		for ( n= 0; n<32; n++)
		{
			color = getWord( bank1->start, pos );
			retroScreenColor( screen, n, ((color & 0xF00) >> 8) * 0x11, ((color & 0xF0) >> 4) * 0x11,  ((color & 0xF)) * +0x11 );
		}
	}
}

struct cmdcontext *find_interface_context(int id)
{
	unsigned int n;

	for (n=0;n<icmdcontexts.size();n++)
	{
		if (icmdcontexts[n] -> id == id) return icmdcontexts[n];
	}

	return NULL;
}

void 	erase_interface_context( struct cmdcontext *context )
{
	unsigned int n;

	for (n=0;n<icmdcontexts.size();n++)
	{
		if (icmdcontexts[n] == context) 
		{
			icmdcontexts.begin()+n;
			return;
		}
	}
}

char *_guiDialogRun( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	int guiChannel = 0;
	int label = 0;
	int x = 0,y = 0;
	struct cmdcontext *context = NULL;

	switch (args)
	{
		case 1:	guiChannel = getStackNum(stack);

				if (context = find_interface_context( guiChannel ))
				{
					execute_interface_script( context, -1 );
				}
				break;

		case 2:	guiChannel = getStackNum(stack-1);
				label = getStackNum(stack);

				if (context = find_interface_context( guiChannel ))
				{
					execute_interface_script( context, label );
				}
				break;

		case 4:	guiChannel = getStackNum(stack-3);
				label = getStackNum(stack-2);
				x = getStackNum(stack-1);
				y = getStackNum(stack);

				if (context = find_interface_context( guiChannel ))
				{
					execute_interface_script( context, label );
				}
				break;

		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( context ? (context -> has_return_value ? context -> return_value : 0) : 0 );

	return NULL;
}

char *guiDialogRun(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _guiDialogRun, tokenBuffer );
	return tokenBuffer;
}

char *_guiDialog( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	int guiChannel = 0;
//	int label = 0;
//	int x = 0,y = 0;
//	const char *ret = NULL;

	switch (args)
	{
		case 1:	guiChannel = getStackNum(stack);
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

char *guiDialog(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _guiDialog, tokenBuffer );
	return tokenBuffer;
}

char *_guiDialogStr( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	int guiChannel = 0;
//	const char *ret = NULL;

	switch (args)
	{
		case 2:	guiChannel = getStackNum(stack);
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

char *guiDialogStr(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _guiDialogStr, tokenBuffer );
	return tokenBuffer;
}


char *_guiDialogBox( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	char *script = NULL;
	struct cmdcontext context ;

	context.return_value = 0;
	context.tokenBuffer = data -> tokenBuffer;

	switch (args)
	{
		case 1:
				{
					script = getStackString(stack);
					init_interface_context( &context, 0, script, 0, 0, 16, 0 );
					execute_interface_script( &context, -1 );
					cleanup_inerface_context( &context );
				}
				break;

		case 3:	
				{
					script = getStackString(stack-2);
					int var1 = getStackNum(stack-1);
					char *var2s = getStackString(stack);

					init_interface_context( &context, 0, script, 0, 0, 16, 0 );

					isetvarnum( &context,0,var1); 
					if (var2s) isetvarstr( &context,1,var2s);

					execute_interface_script( &context, -1 );
					cleanup_inerface_context( &context );
				}
				break;

		case 5:
				{
					script = getStackString(stack-4);
					int var1 = getStackNum(stack-3);
					char *var2s = getStackString(stack-2);
					int x = getStackNum(stack-1);
					int y = getStackNum(stack);

					init_interface_context( &context, 0, script, x, y, 16, 0 );

					isetvarnum( &context,0,var1); 
					if (var2s) isetvarstr( &context,1,var2s);

					execute_interface_script( &context, -1 );
					cleanup_inerface_context( &context );
				}
				break;

		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( context.has_return_value ? context.return_value : 0 );

	return NULL;
}

char *guiDialogBox(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _guiDialogBox, tokenBuffer );
	return tokenBuffer;
}

char *guiDialogFreeze(nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *guiDialogUnfreeze(nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

int current_dialog = -1;

void retroPutBlock(struct retroScreen *screen, struct retroBlock *block,  int x, int y, unsigned char bitmask);

char *_guiDialogClose( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	int id=-1;

	switch (args)
	{
		case 1:
				switch ( kittyStack[stack].type )
				{
					case type_none:
						id = current_dialog;
						break;

					case type_int:
						id = kittyStack[stack].value;
						break;
				}

				break;

		default:
				setError(22,data->tokenBuffer);
	}

	if (id != -1)
	{
		struct cmdcontext *context = find_interface_context(id);
		
		if (context)
		{
			if (context -> saved_block)
			{
				retroPutBlock( screens[current_screen], context -> saved_block, context -> dialog[0].x, context -> dialog[0].y, 0xFF );
			}

			cleanup_inerface_context( context );
			erase_interface_context( context );
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *guiDialogClose(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _guiDialogClose, tokenBuffer );
	setStackNone();

	return tokenBuffer;
}

char *_guiDialogOpen( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	char *script;
	int id = -1;
	int ret = 0;
	int varSize=17,bufferSize=0;

	switch (args)
	{
		case 2:	id = getStackNum(stack-1);
				script = getStackString(stack);
				break;

		case 4:
				id = getStackNum(stack-3);
				script = getStackString(stack-2);
				varSize = getStackNum(stack-1);
				bufferSize = getStackNum(stack);
				break;

		default:
				setError(22,data->tokenBuffer);
	}

	if ((id != -1) && (script))
	{
		struct cmdcontext *item = find_interface_context(id);
	
		if (item == NULL)
		{
			item = new cmdcontext();
			if (item) 
			{
				current_dialog = id;
				init_interface_context( item, id, script, 0, 0 , varSize, bufferSize );
				icmdcontexts.push_back(item);
				ret = ~1;
			}
			else
			{
				printf("Oh no, context found?\n");
			}
		}
	}

	popStack( stack - data->stack );
	setStackNum( ret );

	return NULL;
}

char *guiDialogOpen(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _guiDialogOpen, tokenBuffer );
	return tokenBuffer;
}

extern char *_setVar( struct glueCommands *data, int nextToken );

static int _set_interface_ = 0;
static int _set_var_ = 0;

char *_set_interface_command ( struct glueCommands *data, int nextToken ) 
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (_set_interface_!=-1)
	{
		struct cmdcontext *item = find_interface_context(_set_interface_);

		printf("set interface %d var %d\n",_set_interface_,_set_var_);

		if (item)
		{
			int num = getStackNum(stack);
			isetvarnum( item,_set_var_, num); 
		}
		else setError(22,data->tokenBuffer);
	}

	_do_set = _setVar;
	return NULL;
}

char *_set_interface_str_command ( struct glueCommands *data, int nextToken ) 
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (_set_interface_!=-1)
	{
		struct cmdcontext *item = find_interface_context(_set_interface_);

		printf("set interface %d var %d\n",_set_interface_,_set_var_);

		if (item)
		{
			char *str = getStackString(stack);
			if (str)
			{
				isetvarstr( item,_set_var_, str); 
			}
		}
		else setError(22,data->tokenBuffer);
	}

	_do_set = _setVar;
	return NULL;
}

char *_guiVdialog( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;

	_set_interface_ = -1;

	switch (args)
	{
		case 2:	_set_interface_ = getStackNum(stack-1);
				_set_var_ = getStackNum(stack);
				_do_set = _set_interface_command;
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

char *guiVdialog(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _guiVdialog, tokenBuffer );
	return tokenBuffer;
}

char *_guiVdialogStr( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;

	_set_interface_ = -1;

	switch (args)
	{
		case 2:	_set_interface_ = getStackNum(stack-1);
				_set_var_ = getStackNum(stack);
				_do_set = _set_interface_str_command;
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

char *guiVdialogStr(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _guiVdialogStr, tokenBuffer );
	return tokenBuffer;
}

char *_guiDialogClr( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	int a;

	switch (args)
	{
		case 1:	a = getStackNum(stack);
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

 char *guiDialogClr(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _guiDialogClr, tokenBuffer );
	return tokenBuffer;
}

extern void init_amos_kittens_screen_default_text_window( struct retroScreen *screen, int colors );
extern void init_amos_kittens_screen_default_colors(struct retroScreen *screen);

char *_guiResourceScreenOpen( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;

	switch (args)
	{
		// ResourceScreenOpen n,w,h,n

		case 4:
				{
					struct retroScreen *screen;
					int screen_num = getStackNum(stack-3);
					int w = getStackNum(stack-2);
					int h = getStackNum(stack-1);
					int f = getStackNum(stack);

					engine_lock();
					if (screens[screen_num]) retroCloseScreen(&screens[screen_num]);

					screens[screen_num] = retroOpenScreen(w,h,retroLowres);
					if (screen = screens[screen_num])
					{
						init_amos_kittens_screen_default_text_window(screen, 64);
						init_amos_kittens_screen_resource_colors(screen);

						screen -> paper = 2;
						retroBAR( screen, 0,0, screen -> realWidth,screen->realHeight, screen -> paper );

						retroApplyScreen( screen, video, 0, 0, screen -> realWidth,screen->realHeight );

						current_screen = screen_num;
					}
					engine_unlock();
				}

				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

char *guiResourceScreenOpen(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _guiResourceScreenOpen, tokenBuffer );
	return tokenBuffer;
}

char *guiEDialog(nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum( 0 );
	return tokenBuffer;
}

