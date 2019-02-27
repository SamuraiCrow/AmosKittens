
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <string.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#endif

#include "debug.h"
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsBanks.h"
#include "commandsBlitterObject.h"
#include "errors.h"
#include "engine.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern int current_screen;
extern struct retroSprite *sprite ;
extern struct retroSprite *icons ;

extern void _my_print_text(struct retroScreen *screen, char *text, int maxchars);

extern struct retroTextWindow *newTextWindow( struct retroScreen *screen, int id );
extern void freeAllTextWindows(struct retroScreen *screen);

// palette data for RLE
static int r[32]={0},g[32]={0},b[32]={0};

// data to image 
// static unsigned char *data = NULL;

#define get2( pos ) (int(data[pos])*256 + int(data[pos+1]))
#define get4(  pos ) ( ( ( int(data[pos])*256 + int(data[pos+1]) )*256 + int(data[pos+2]) ) * 256 + int(data[pos+3]) )

void getRGB( unsigned char *data, int pos, int &r, int &g, int &b ) { // get RGB converted to 0..255
	r = (data[pos] & 0x0F) * 0x11;
	g = ((data[pos+1] & 0xF0)>>4) * 0x11;
	b = (data[pos+1] & 0x0F) * 0x11;
}

void openUnpackedScreen(int screen_num, int bytesPerRow, int height, int depth, int *r, int *g, int *b, unsigned char *raw, unsigned short mode)
{
	int n;
	int row;
	int byte;
	int bit;
	int color;
	int d;
	int planeOffset;
	int bytesPerPlan;
	int colors = 1 << depth;
	unsigned int videomode = retroLowres_pixeld;
	struct retroScreen *screen = NULL;
	struct retroTextWindow *textWindow = NULL;

	if (mode & 0x8000) videomode = retroHires; 

	engine_lock();

	if (screens[screen_num]) 
	{
		videomode = screens[screen_num] -> videomode;

		freeScreenBobs(screen_num);
		freeAllTextWindows( screens[screen_num] );
		retroCloseScreen(&screens[screen_num]);
	}

	screens[screen_num] = retroOpenScreen( bytesPerRow * 8, height, videomode );

	if (screen = screens[screen_num])
	{
		current_screen = screen_num;

		retroApplyScreen( screen, video, 0, 0,	screen -> realWidth,screen->realHeight );

		if (textWindow = newTextWindow( screen, 0 ))
		{
			textWindow -> charsPerRow = screen -> realWidth / 8;
			textWindow -> rows = screen -> realHeight / 8;
			screen -> pen = 2;
			screen -> paper = 1;
			screen -> autoback = 2;

			screen -> currentTextWindow = textWindow;
		}

		for (n=0;n<colors;n++)	
		{
			retroScreenColor( screen, n,r[n],g[n],b[n]);
		}

		retroBAR( screen, 0,0, screen -> realWidth,screen->realHeight, screen -> paper );

		bytesPerPlan = bytesPerRow * height;

		for (int y=0; y < screen -> realHeight; y++)
		{
			row = bytesPerRow * y;

			for (int x=0; x < screen -> realWidth; x++)
			{
				byte = x / 8;
				bit = 1<<(7-(x & 7));
				planeOffset = 0;

				color = 0;
				for (d=0;d<depth;d++)
				{
					color += raw[ row + byte + planeOffset ] & bit ? 1<<d: 0 ;
					planeOffset += bytesPerPlan;
				}

				retroPixel( screen, x,y, color );					
			}
		}
	}

	video -> refreshAllScanlines = TRUE;
	engine_unlock();
}


// pac.pic. RLE decompressor
int convertPacPic( int screen, unsigned char *data, const char *name  )
{
	//  int o = 20;
	int o=0;
	unsigned short mode = 0;


	if( get4(o) == 0x12031990 )
	{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		mode = get2(o+20);

		// fetch palette
		for( int i=0; i<32; ++i )
		{ 
			getRGB( data, o+26+i*2, r[i],g[i],b[i] ); 
		}
		o+=90;
	}

	if( get4(o) != 0x06071963 )
	{
		printf("could not find picture header!\n");
		printf("%08x\n",get4(o));
		printf("%08x\n",get4(o+4));
		printf("%08x\n",get4(o+8));
		return 1;
	}

  int w  = get2(o+8),
      h  = get2(o+10),
      ll = get2(o+12),
      d  = get2(o+14);

	// reserve bitplane memory
	unsigned char* raw = (unsigned char*) malloc(w*h*ll*d);
	unsigned char *picdata = &data[o+24];
	unsigned char *rledata = &data[o+get4(o+16)];
	unsigned char *points  = &data[o+get4(o+20)];

	if (raw)
	{
		int rrbit = 6, rbit = 7;
		int picbyte = *picdata++;
		int rlebyte = *rledata++;
		if (*points & 0x80) rlebyte = *rledata++;

		for( int i = 0; i < d; i++)
		{
			unsigned char *lump_start = &raw[i*w*h*ll];

			for( int j = 0; j < h; j++ )
			{
				unsigned char *lump_offset = lump_start;

				for( int k = 0; k < w; k++ )
				{
					unsigned char *dd = lump_offset;
					for( int l = 0; l < ll; l++ )
					{
						/* if the current RLE bit is set to 1, read in a new picture byte */
						if (rlebyte & (1 << rbit--)) picbyte = *picdata++;

						/* write picture byte and move down by one line in the picture */
						*dd = picbyte;
         					dd += w;

						/* if we've run out of RLE bits, check the POINTS bits to see if a new RLE byte is needed */
						if (rbit < 0)
						{
							rbit = 7;
							if (*points & (1 << rrbit--)) rlebyte = *rledata++;
							if (rrbit < 0)  rrbit = 7, points++;
						}
					}
					lump_offset++;
				}
				lump_start += w * ll;
			}
		}

		printf ("%d,%d,%d\n",w*8,h*ll,1<<d);
		openUnpackedScreen( screen, w, h*ll, d, r,g ,b,raw, mode );
		free(raw);
	}

	return 0;
}

char *_ext_cmd_unpack( struct glueCommands *data, int nextToken )
{
	int n;
	int s;
	struct kittyBank *bank;
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = stack - data -> stack  +1;

	if (args==2)
	{
		n = getStackNum(stack-1);
		s = getStackNum(stack);

		printf("unpack %d to %d\n",n,s);

		dump_banks();

		bank = findBank(n);
		if (bank)
		{
			if (bank -> start)
			{
				convertPacPic( s, (unsigned char *) bank -> start, "dump" );

				if (screens[s] == NULL) setError(47,data->tokenBuffer );
			}
			else setError(36,data->tokenBuffer);	// Bank not reserved
		}
		else setError(25, data->tokenBuffer);
	}
	else setError(22, data->tokenBuffer);	// wrong number of args.

	popStack( stack - data->stack );
	return NULL;
}

char *ext_cmd_unpack(nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_unpack, tokenBuffer );
	return tokenBuffer;
}

