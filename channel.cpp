#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <proto/dos.h>


#include "amosKittens.h"
#include "amalCompiler.h"
#include "channel.h"

struct kittyChannel * ChannelTableClass::newChannel(  int channel )
{
	struct kittyChannel *item = (struct kittyChannel *) malloc(sizeof(struct kittyChannel));

	if (item)
	{
		item -> id = channel;
		item -> token = 0;
		item -> number = 0;
		item -> amal_script = NULL;
		item -> amal_at = NULL;
		item -> anim_script = NULL;
		item -> anim_at = NULL;
		item -> movex_script = NULL;
		item -> movex_at = NULL;
		item -> movey_script = NULL;
		item -> movey_at = NULL;

		item -> count = 0;
		item -> count_to = 0;
		item -> progStack = NULL;
		item -> progStackCount = 0;
		item -> argStack = NULL;
		item -> argStackCount = 0;
		item -> parenthses = 0;
		item -> objectAPI = NULL;
	}

	if (used < allocated )
	{
		tab[used] = item;
		used ++;

		printf("used %d allocated %d\n",used,allocated);
		return item;
	}
	else
	{
		struct kittyChannel **tmp;
		int old_allocated = allocated;
		allocated += 20;
		tmp = (struct kittyChannel **) malloc(sizeof(struct kittyChannel *) * allocated );

		if (tmp)
		{
			memcpy(tmp,tab,sizeof(struct kittyChannel *) * old_allocated );
			free(tab);

			tab = tmp;
			tab[used] = item;
			used ++;
			return item;
		}
		else
		{
			// resvert 
			if (item) free( (char *) item );
			allocated = old_allocated;
		}

		printf("used %d allocated %d\n",used,allocated);
	}

	return NULL;
}

struct kittyChannel *ChannelTableClass::getChannel(int id)
{
	for (int n=0;n<used;n++) if (tab[n] -> id == id) return tab[n];
	return NULL;
}

struct kittyChannel *ChannelTableClass::item(int index)
{
	return tab[index];
}

int ChannelTableClass::_size()
{
	return used;
}

void setChannelAnim( struct kittyChannel *item, char *str)
{
	if (item -> anim_script) free(item -> anim_script);
	item -> anim_script = str;
	item -> anim_at = str;
}

void setChannelAmal( struct kittyChannel *item, char *str)
{
	if (item -> amal_script) free(item -> amal_script);
	item -> amal_script = str;
	item -> amal_at = str;
}

void setChannelMoveX( struct kittyChannel *item, char *str)
{
	if (item -> movex_script) free(item -> movex_script);
	item -> movex_script = str;
	item -> movex_at = str;
	item -> deltax = 0;
}

void setChannelMoveY( struct kittyChannel *item, char *str)
{
	if (item -> movey_script) free(item -> movey_script);
	item -> movey_script = str;
	item -> movey_at = str;
	item -> deltay = 0;
}


