#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "stack.h"
#include "amosKittens.h"
#include <vector>
#include <proto/retroMode.h>
#include "commandsbanks.h"
#include "amalcompiler.h"
#include "channel.h"

extern struct retroScreen *screens[8] ;
extern struct retroSpriteObject bobs[64];
extern struct globalVar globalVars[1000];
extern std::vector<struct label> labels;
extern int global_var_count;
extern char *dir_first_pattern ;
extern struct retroSprite *sprite ;
extern struct retroSprite *icons ;
extern ChannelTableClass *channels;

void clean_up_vars()
{
	int n;
	int i;

	for (n=0;n<global_var_count;n++)
	{
//		printf("Free var %d -- type: %x is shared %d\n",n, globalVars[n].var.type, globalVars[n].pass1_shared_to);
//		printf("--->name %s\n",globalVars[n].varName);

		if (globalVars[n].varName) 
		{
			free(globalVars[n].varName);
			globalVars[n].varName = NULL;
		}

		switch (globalVars[n].var.type)
		{
			case type_string:
			case type_int | type_array:
			case type_float | type_array:
			case type_string | type_array:

//				printf("--->Free array?");

				// its a union, so any array or string will be freed.

 				if (globalVars[n].var.str) free (globalVars[n].var.str);
				globalVars[n].var.str = NULL;
				break;
		}
//		printf("--->Done :-)\n");
	}

	global_var_count = 0;
}

void clean_up_stack()
{
	int n;

	for (n=0; n<=stack;n++)
	{
		switch( kittyStack[n].type )
		{		
			case type_string:
				if (kittyStack[n].str) free (kittyStack[n].str);
				kittyStack[n].str = NULL;
				kittyStack[n].type = 0;			
				break;
		}
	}
	stack = 0;
}

void clean_up_files()
{
	int n;
	for (n=0;n<10;n++)
	{
		if (kittyFiles[n].fd) fclose(kittyFiles[n].fd);
		if (kittyFiles[n].fields) free(kittyFiles[n].fields);

		kittyFiles[n].fd = NULL;
		kittyFiles[n].fields = NULL;
	}
}

void clean_up_bank(int n)
{
	if (kittyBanks[n].start)
	{
		switch ( kittyBanks[n].type )
		{
			case bank_type_icons:

					printf("try free icons\n");

					retroFreeSprite( (struct retroSprite *) kittyBanks[n].object_ptr );
					icons = NULL;
					break;

			case bank_type_sprite:

					printf("try free sprite\n");

					retroFreeSprite( (struct retroSprite *) kittyBanks[n].object_ptr );
					sprite = NULL;
					break;

			default:
					free( kittyBanks[n].start - 8 );
					break;
		}

		kittyBanks[n].start = NULL;
		kittyBanks[n].length = 0;
		kittyBanks[n].type = 0;
	}
}

void clean_up_banks()
{
	int n;
	for (n=0;n<15;n++)
	{
		clean_up_bank(n);
	}
}

void clean_up_special()
{
	int n;

	printf("should clean up menus here, don't forget me\n");

	printf("clean up channels!!\n");

	if (channels) 
	{
		delete channels;
		channels = NULL;
	}

	printf("clean up bobs!!\n");

	for (n=0;n<64;n++)
	{
		retroFreeSpriteObject( &bobs[n],TRUE);		// TRUE = only data
	}

	printf("clean up banks!!\n");

	clean_up_banks();

	printf("clean up contextDir\n");

	if (contextDir)
	{
		ReleaseDirContext(contextDir);
		contextDir = NULL;
	}

	printf("clean up dir first pattern");

	if (dir_first_pattern)
	{
		free(dir_first_pattern);
		dir_first_pattern = NULL;
	}

	printf("clean up zones\n");

	if (zones)
	{
		free(zones);
		zones = NULL;
	}
}

