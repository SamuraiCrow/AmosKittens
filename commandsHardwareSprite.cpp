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
#include "commandsGfx.h"
#include "errors.h"
#include "engine.h"

extern int sig_main_vbl;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern int current_screen;


extern struct retroScreen *screens[8] ;
extern struct retroSprite *sprite;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];

char *_hsGetSpritePalette( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((sprite)&&(screen))
	{
		for (n=0;n<256;n++)
		{
			retroScreenColor( screen, n, sprite -> palette[n].r, sprite -> palette[n].g, sprite -> palette[n].b );
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *hsGetSpritePalette(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _hsGetSpritePalette, tokenBuffer );
	return tokenBuffer;
}

char *_hsSprite( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *sprite;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	num = getStackNum( stack - 3 );
	sprite = &video -> sprites[num];

	stack_get_if_int( stack - 2 , &sprite->x );
	stack_get_if_int( stack - 1 , &sprite->y );

	sprite->image = getStackNum( stack );


	popStack( stack - data->stack );
	return NULL;
}

char *hsSprite(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSprite, tokenBuffer );
	return tokenBuffer;
}

char *_hsGetSprite( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *hsGetSprite(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSprite, tokenBuffer );
	return tokenBuffer;
}

char *_hsSpriteOff( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *hsSpriteOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSpriteOff, tokenBuffer );
	return tokenBuffer;
}

