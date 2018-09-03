#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <proto/dos.h>
#include "amosKittens.h"

#include "channel.h"

struct kittyChannel * ChannelTableClass::newChannel(  int channel )
{
	struct kittyChannel *item = (struct kittyChannel *) malloc(sizeof(struct kittyChannel));

	if (item)
	{
		item -> id = channel;
		item -> token = 0;
		item -> number = 0;
		item -> cmd = NULL;
		item -> script = NULL;
		item -> at = NULL;
		item -> count = 0;
		item -> count_to = 0;
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

