#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/retroMode.h>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsBanks.h"
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

void _my_print_text(struct retroScreen *screen, char *text, int maxchars);

const char *amos_file_ids[] =
	{
		"AmBk",		// work
		"",			// chip work 
		"AmIc",
		"AmSp",
		NULL
	};


const char *bankTypes[] = {
	"Chip work",		// 0
	"Fast work",		// 1
	"Icons",			// 2
	"Sprites",			// 3
	"Music",			// 4
	"Amal",			// 5
	"Samples",		// 6
	"Menu",			// 7
	"Chip Data",		// 8
	"Fast Data",		// 9
	"Code"
};



extern void clean_up_bank(int n);

char *_cmdErase( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==1)
	{
		n = getStackNum(data->stack);
		if ((n>0)&&(n<16))	clean_up_bank(n-1);		
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdStart( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = getStackNum(stack);

		if ((n>0)&&(n<16))
		{
			ret = (int) kittyBanks[n-1].start;
			success = true;
		} 
	}

	if (success == false ) ret = 0;

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_cmdLength( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = getStackNum(stack);
		if ((n>0)&&(n<16))
		{
			ret = (int)  kittyBanks[n-1].length;
			success = true;
		} 
	}

	if (success == false ) ret = 0;

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}


char *_cmdBload( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	int n;
	int args = stack - data->stack +1 ;
	FILE *fd;
	int size;
	char *adr = NULL;

	dump_stack();

	if (args==2)
	{
		fd = fopen( getStackString( stack - 1 ) , "r");
		if (fd)
		{
			n = getStackNum(stack);

			fseek(fd , 0, SEEK_END );			
			size = ftell(fd);
			fseek(fd, 0, SEEK_SET );

			if (size)
			{
				if ((n>0)&&(n<16))
				{
					kittyBanks[n-1].length = size;
					if (kittyBanks[n-1].start) free( kittyBanks[n-1].start );
					kittyBanks[n-1].start = malloc( size );
					kittyBanks[n-1].type = 9;	
					adr = (char *)  kittyBanks[n-1].start;
				}
				else
				{
					char *adr = (char *) n;
				}
				
				if (adr) fread( adr ,size,1, fd);
			}

			fclose(fd);
		}

	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdBsave( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int n;
	int args = stack - data->stack +1 ;
	FILE *fd;
	char *start, *to;

	dump_stack();

	if (args==3)
	{
		fd = fopen( getStackString( stack - 2 ) , "w");

		start = (char *) getStackNum(stack -1 );
		to = (char *) getStackNum( stack );

		if (fd)
		{
			if ((to-start)>0)
			{
				fwrite( start, to-start,1, fd );
			}
			fclose(fd);
		}
	}

	getchar();

	popStack( stack - data->stack );
	return NULL;
}

bool __ReserveAs( int type, int bank, int length, char *name, char *mem )
{
	if ((bank>0)&&(bank<16))
	{
		kittyBanks[bank-1].length = length;
		if (kittyBanks[bank-1].start) free( kittyBanks[bank-1].start );

		if (mem)
		{
			kittyBanks[bank-1].start = mem;
		}
		else
		{
			kittyBanks[bank-1].start = malloc( kittyBanks[bank-1].length );
		}

		kittyBanks[bank-1].type = type;
		return true;
	}
	return false;
}


char *_cmdReserveAsWork( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	bool success = false;

	if (args==2)
	{
		success = __ReserveAs( 1, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsChipWork( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	bool success = false;

	if (args==2)
	{
		success = __ReserveAs( 0, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsData( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	bool success = false;

	if (args==2)
	{
		success = __ReserveAs( 8 | 1, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsChipData( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	bool success = false;

	if (args==2)
	{
		success = __ReserveAs( 8 | 0, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *cmdReserveAsWork(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsWork, tokenBuffer );
	return tokenBuffer;
}

char *cmdReserveAsChipWork(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsChipWork, tokenBuffer );
	return tokenBuffer;
}

char *cmdReserveAsData(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsData, tokenBuffer );
	return tokenBuffer;
}

char *cmdReserveAsChipData(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsChipData, tokenBuffer );
	return tokenBuffer;
}


char *cmdListBank(nativeCommand *cmd, char *tokenBuffer)
{
	int n = 0;
	char txt[1000];
	struct retroScreen *screen;

	screen = screens[current_screen];

	if (screen)
	{
		clear_cursor( screen );
		_my_print_text( screen, (char *) "Nr   Type       Start       Length\n\n", 0);

		for (n=0;n<15;n++)
		{
			if (kittyBanks[n].start)
			{
				sprintf(txt,"%2d - %-10s S:$%08X L:%d\n", 
					n+1,
					bankTypes[kittyBanks[n].type],
					kittyBanks[n].start, 
					kittyBanks[n].length);

				_my_print_text( screen, txt, 0 );
			}
		}
		_my_print_text( screen, (char *) "\n", 0 );
	}

	return tokenBuffer;
}

char *cmdErase(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdErase, tokenBuffer );
	return tokenBuffer;
}

char *cmdStart(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _cmdStart, tokenBuffer );
	return tokenBuffer;
}

char *cmdLength(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _cmdLength, tokenBuffer );
	return tokenBuffer;
}

char *cmdBload(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdBload, tokenBuffer );
	return tokenBuffer;
}

char *cmdBsave(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdBsave, tokenBuffer );
	return tokenBuffer;
}

struct bankItemDisk
{
	unsigned short bank;
	unsigned short type;
	unsigned int length;
	char name[8];
} __attribute__((packed)) ;



void __load_work_data__(FILE *fd,int bank)
{
	struct bankItemDisk item;
	char *mem;

	if (fread( &item, sizeof(struct bankItemDisk), 1, fd )==1)
	{
		if (bank>0) item.bank = bank;

		if (item.length & 0x80000000) item.type += 8;
		item.length = (item.length & 0x7FFFFFF) - 8;
		if (item.length >0 )
		{
			mem = (char *) malloc( item.length );
			if (mem)
			{
				fread( mem, item.length, 1, fd );
				if (__ReserveAs( item.type, item.bank, item.length,NULL, mem ) == false) free(mem);
			}
		}
	}
}

// callback.

int cust_fread (void *ptr, int size,int elements, FILE *fd)
{
//	printf("ptr %08x, size %d elements %d, FILE %08x\n",ptr,size,elements,fd);
//	Delay(100);
	if (ptr)
	{
		return fread(ptr,size,elements,fd);
	}
	else return 0;
}

extern void clean_up_banks();

void __load_bank__(const char *name, int bank )
{
	FILE *fd;
	int size;
	char id[5];
	unsigned short banks = 0;
	id[4]=0;
	int type = -1;
	int n;

			fd = fopen( name , "r");
			if (fd)
			{
				if (fread( &id, 4, 1, fd )==1)
				{	
					if (strcmp(id,"AmBs")==0)
					{
						fread( &banks, 2, 1, fd);
						clean_up_banks();		
					}
				}

				if (banks == 0) 
				{
					fseek( fd, 0, SEEK_SET );	// set set, to start no header found.
					banks = 1;
				}

				for (n=0;n<banks;n++)
				{
					type = -1;

					if (fread( &id, 4, 1, fd )==1)
					{	
						int cnt = 0;
						const char **idp;

						for (idp = amos_file_ids; *idp ; idp++)
						{
							if (strcmp(id,*idp)==0) { type = cnt; break; }
							cnt++;
						}
					}
					
					switch (type)
					{
						case bank_type_sprite:

							// should append to end of sprite bank.

							engine_lock();
							if (sprite) retroFreeSprite(sprite);
							sprite = retroLoadSprite(fd, cust_fread );
							engine_unlock();

							// 99 Bottles of beer. 
							if (__ReserveAs( bank_type_sprite, 3, 99,NULL, (char *) sprite ) == false)
							{
								if (sprite) retroFreeSprite(sprite);
								sprite = NULL;
							}
							break;
	
						case bank_type_icons:

							if (icons) retroFreeSprite(icons);
							icons = retroLoadSprite(fd, cust_fread );

							// 99 Bottles of beer. 
							if (__ReserveAs( bank_type_sprite, 2, 99,NULL, (char *) icons ) == false)
							{
								if (icons) retroFreeSprite(icons);
								icons = NULL;
							}
							break;

						case bank_type_work_or_data:
							__load_work_data__(fd,bank);
							break;

						default:
							printf("oh no!!... unexpected id: %s\n", id);
							Delay(120);

					}
				}

				fclose(fd);
			}
}

char *_cmdLoad( struct glueCommands *data, int nextToken )
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int n;
	int args = stack - data->stack +1 ;


	switch (args)
	{
		case 1:
			__load_bank__(getStackString( stack  ), -1 );
			break;

		case 2:
			__load_bank__( getStackString(stack-1), getStackNum(stack) );
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}


char *cmdLoad(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdLoad, tokenBuffer );
	return tokenBuffer;
}

char *_cmdSave( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int n;
	int args = stack - data->stack +1 ;
	FILE *fd;
	char *start, *to;

	dump_stack();

	if (args==3)
	{
		fd = fopen( getStackString( stack - 2 ) , "w");

		if (fd)
		{
			fclose(fd);
		}
	}

	getchar();

	popStack( stack - data->stack );
	return NULL;
}

char *cmdSave(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdSave, tokenBuffer );
	return tokenBuffer;
}

