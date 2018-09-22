
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "AmalCompiler.h"
#include "channel.h"
#include "AmalCommands.h"

int nest = 0;
char last_reg[1000] ;

struct AmalLabelRef
{
	unsigned int pos;
	char *name;
};

std::vector<void **> amalloops;
static std::vector<struct AmalLabelRef> looking_for_labels;
static std::vector<struct AmalLabelRef> found_labels;

#ifdef test_app
int amreg[26];

void print_code( void **adr );

void dumpAmalRegs()
{
	int i;
	for (i=0;i<26;i++) printf("R%c is %d\n", 'A'+i,amreg[i]);
}

#endif

void *(*amal_cmd_equal) API_AMAL_CALL_ARGS = NULL;

unsigned int (*amal_to_writer) ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num) = NULL;


void dumpAmalStack( struct kittyChannel *channel )
{
	int s;
	printf("Amal Stack\n");
	for (s=0;s<=channel -> argStackCount;s++) printf("stack %d: value %d\n",s, channel -> argStack[s] );
}

void pushBackAmalCmd( void **code, struct kittyChannel *channel, void *(*cmd)  (struct kittyChannel *self, struct amalCallBack *cb)  ) 
{
	if (channel -> progStack)
	{
		struct amalCallBack *CallBack = &channel -> progStack[ channel -> progStackCount ];
		if (CallBack)
		{
			CallBack -> cmd = cmd;
			CallBack -> argStackCount = channel -> argStackCount;
			CallBack -> progStackCount = channel -> progStackCount;
			CallBack -> last_reg = channel -> last_reg;
			CallBack -> code = code;
			channel -> progStackCount ++;
			return;
		}
	}
}

unsigned int numAmalWriter (	struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	call_array[0] = self -> call;

	printf("saves num %d\n",num);

	*((int *) &call_array[1]) = num;
	return 2;
}


unsigned int stdAmalWriterJump (	struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	char labelname[20];
	int le;
	const char *s;
	char *d;

	printf("jump\n");
	printf("writing %08x to %08x\n", self -> call, &call_array[0]);

	call_array[0] = self -> call;

	s = data -> at_script;
	while ((*s != 0)&&(*s != ' ')) s++;
	while (*s == ' ') s++;

	le = 0;
	d = labelname;
	while ((*s != 0)&&( *s != ';')&&(le<18)) { *d++=*s++; le++; }
	*d = 0;

	data -> arg_len = le ? le+1 : 0;

	printf("label: %s\n",labelname);

	{
		struct AmalLabelRef label;
		char *current_location;
		char *start_location;

		current_location = (char *) &call_array[0];
		start_location = (char *) channel -> amalProg.call_array;

		label.pos 	= (int) (current_location - start_location) / sizeof(void *);
		label.name = strdup( labelname );
		looking_for_labels.push_back( label );
	}

	*((int *) &call_array[1]) = 0;
	return 2;
}


unsigned int stdAmalWriterIgnore ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	printf("data ignored\n");
	return 0;
}

unsigned int stdAmalWriter ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	printf("writing %08x to %08x\n", self -> call, &call_array[0]);
	call_array[0] = self -> call;
	return 1;
}

unsigned int stdAmalWriterEqual ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	if (amal_cmd_equal)
	{
		printf("writing [code block] to %08x\n", &call_array[0]);
		call_array[0] = amal_cmd_equal;
		amal_cmd_equal = NULL;
	}
	else
	{
		printf("writing %08x to %08x\n", self -> call, &call_array[0]);
		call_array[0] = self -> call;
	}

	return 1;
}

unsigned int amal_for_to ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
		printf("writing [code block] to %08x\n", &call_array[0]);
		char *current_location = (char *) (&call_array[1]);
		char *start_location = (char *) channel -> amalProg.call_array;

		call_array[0] = amal_call_next_cmd;

		call_array[1] = amal_call_while;
		call_array[2] = NULL;
		amalloops.push_back( (void **) (current_location - start_location) );

		printf("start location:    %08x\n",start_location);
		printf("current location: %08x\n",current_location);

		call_array[3] = amal_call_reg;
		*((int *) &call_array[4]) = last_reg[nest];

		call_array[5] = amal_call_less_or_equal;
		amal_to_writer = NULL;
		nest++;
		return 6;
}

unsigned int stdAmalWriterFor (  struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	amal_to_writer = amal_for_to;
	return 0;
}

unsigned int stdAmalWriterTo (  struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	unsigned int ret = 0;

	if (amal_to_writer) 
	{
		ret = amal_to_writer( channel, self, call_array, data, num );
		amal_to_writer = NULL;
	}

	return ret;
}

unsigned int stdAmalWriterWend (  struct kittyChannel *channel,struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	nest--;

	call_array[0] = amal_call_inc;
	call_array[1] = amal_call_reg;
	*((int *) &call_array[2]) = last_reg[nest];
	call_array[3] = amal_call_next_cmd;
	call_array[4] = amal_call_wend;

	if (!amalloops.empty())
	{
		int i;
		int elements;
		void **code;
		int rel_location = (int) amalloops.back() ;
		char *current_location;
		char *start_location;

		current_location = (char *) &call_array[4];
		start_location = (char *) channel -> amalProg.call_array;
		code = (void **)  (start_location + rel_location);
		elements = (int) (current_location - ((char*) code)) / sizeof(void *);

		code[1] = (void *) (current_location - start_location) ;

		*((void **) &call_array[5]) = (void **) rel_location;
		amalloops.pop_back();
	}

	return 6;
}

unsigned int stdAmalWriterReg (  struct kittyChannel *channel,struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int )
{
	int num = *(data -> at_script + 1);
	printf("writing %08x to %08x\n", amal_call_reg, &call_array[0]);
	call_array[0] = amal_call_reg;
	*((int *) &call_array[1]) = num;

	last_reg[nest] =num;

	return 2;
}

struct amalTab amalCmds[] =
{
	{"@{never used}",stdAmalWriter,NULL},
	{"@{number}",numAmalWriter,amal_set_num},	//number token, reserved.
	{"O",stdAmalWriter,amal_call_on},	// On
	{"D",stdAmalWriter,NULL},	// Direct
	{"W",stdAmalWriter,NULL},	// Wait
	{"I",stdAmalWriter,NULL},	 // If
	{"X",stdAmalWriter,NULL},	// eXit
	{"L",stdAmalWriterIgnore,NULL},	// Let
	{"AU",stdAmalWriter,NULL},	// AUtotest
	{"A",stdAmalWriter,amal_call_anim},	// Anim
	{"M",stdAmalWriter,NULL},	// Move
	{"P",stdAmalWriter,amal_call_pause},	// Pause
	{"R0",stdAmalWriterReg,NULL },	// R0
	{"R1",stdAmalWriterReg,NULL },	// R0
	{"R2",stdAmalWriterReg,NULL },	// R0
	{"R3",stdAmalWriterReg,NULL },	// R0
	{"R4",stdAmalWriterReg,NULL },	// R0
	{"R5",stdAmalWriterReg,NULL },	// R0
	{"R6",stdAmalWriterReg,NULL },	// R0
	{"R7",stdAmalWriterReg,NULL },	// R0
	{"R8",stdAmalWriterReg,NULL },	// R0
	{"R9",stdAmalWriterReg,NULL },	// R0
	{"RA",stdAmalWriterReg,NULL },	// R0
	{"RB",stdAmalWriterReg,NULL },	// R0
	{"RC",stdAmalWriterReg,NULL },	// R0
	{"RD",stdAmalWriterReg,NULL },	// R0
	{"RE",stdAmalWriterReg,NULL },	// R0
	{"RF",stdAmalWriterReg,NULL },	// R0
	{"RG",stdAmalWriterReg,NULL },	// R0
	{"RH",stdAmalWriterReg,NULL },	// R0
	{"RI",stdAmalWriterReg,NULL },	// R0
	{"RJ",stdAmalWriterReg,NULL },	// R0
	{"RK",stdAmalWriterReg,NULL },	// R0
	{"RL",stdAmalWriterReg,NULL },	// R0
	{"RM",stdAmalWriterReg,NULL },	// R0
	{"RN",stdAmalWriterReg,NULL },	// R0
	{"RO",stdAmalWriterReg,NULL },	// R0
	{"RP",stdAmalWriterReg,NULL },	// R0
	{"RQ",stdAmalWriterReg,NULL },	// R0
	{"RR",stdAmalWriterReg,NULL },	// R0
	{"RS",stdAmalWriterReg,NULL },	// R0
	{"RT",stdAmalWriterReg,NULL },	// R0
	{"RU",stdAmalWriterReg,NULL },	// R0
	{"RV",stdAmalWriterReg,NULL },	// R0
	{"RW",stdAmalWriterReg,NULL },	// R0
	{"RX",stdAmalWriterReg,NULL },	// R0
	{"RY",stdAmalWriterReg,NULL },	// R0
	{"RZ",stdAmalWriterReg,NULL },	// R0
	{"+",stdAmalWriter,amal_call_add},		// +
	{"-",stdAmalWriter,amal_call_sub},			// -
	{"*",stdAmalWriter,amal_call_mul},		// *
	{"/",stdAmalWriter,amal_call_div},			// /
	{"&",stdAmalWriter,amal_call_and},		// &
	{"<>",stdAmalWriter,amal_call_not_equal},	// <>
	{"<",stdAmalWriter,amal_call_less},		// <
	{">",stdAmalWriter,amal_call_more},		// >
	{"=",stdAmalWriterEqual,amal_call_set},	// =
	{"F",stdAmalWriterFor,NULL},				// For
	{"T",stdAmalWriterTo,amal_call_nextArg},	// To
	{"N",stdAmalWriterWend,amal_call_wend},	// Next
	{"PL",stdAmalWriter,NULL},	// Play
	{"E",stdAmalWriter,NULL},	// End
	{"XM",stdAmalWriter,NULL},	// XM
	{"YM",stdAmalWriter,NULL},	// YM

	{"K1",stdAmalWriter,NULL},	// k1		mouse key 1
	{"K2",stdAmalWriter,NULL},	// k2		mouse key 2
	{"J0",stdAmalWriter,NULL},	// j0		joy0
	{"J1",stdAmalWriter,NULL},	// J1		Joy1
	{"J",stdAmalWriterJump,amal_call_jump},	// Jump
	{"Z",stdAmalWriter,NULL},	// Z(n)	random number

	{"XH",stdAmalWriter,NULL},	// x hardware
	{"YH",stdAmalWriter,NULL},	// y hardware
	{"XS",stdAmalWriter,NULL},	// screen x
	{"YS",stdAmalWriter,NULL},	// screen y
	{"BC",stdAmalWriter,NULL},	// Bob Col(n,s,e)	// only with Synchro
	{"SC",stdAmalWriter,NULL},	// Sprite Col(m,s,e)	// only with Synchro
	{"C",stdAmalWriter,NULL},	// Col
	{"V",stdAmalWriter,NULL},	// Vumeter
	{";",stdAmalWriter,amal_call_next_cmd },
	{"(",stdAmalWriter,amal_call_parenthses_start },
	{")",stdAmalWriter,amal_call_parenthses_end },
	{",",stdAmalWriter,amal_call_nextArg },
	{"@while",stdAmalWriter,amal_call_while },
	{"@set",stdAmalWriter,amal_call_set },
	{"@reg",stdAmalWriter,amal_call_reg },
	{NULL, NULL,NULL }
};

void print_code( void **adr )
{
	if (*adr == NULL)
	{
		printf("%08X - %08X - %d\n",adr,0,0);
	}

	for (struct amalTab *tab = amalCmds; tab -> name ; tab++ )
	{
		if ( tab->call == *adr ) 
		{
			printf("%08X - %08X - name %s\n",adr,*adr, tab -> name);
			return;
		}
	}
	
	{
		unsigned int c = (unsigned int) *adr;

		if (((c>='0')&&(c<='9')) || ((c>='A')&&(c<='Z')))
		{
			printf("%08X - %08X - R%c\n",adr,*adr,*adr);
		}
		else
		{
			printf("%08X - %08X - %d\n",adr,*adr,*adr);
		}

	}}

struct amalTab *find_amal_command(const char *str)
{
	for (struct amalTab *tab = amalCmds; tab -> name ; tab++ )
	{
		if (strncasecmp(str, tab -> name, strlen(tab->name)) == 0) return tab;
	}
	return NULL;
}

void remove_lower_case(char *txt)
{
	char *c;
	char *d;
	bool space_repeat;

	d=txt;
	for (c=txt;*c;c++)
	{
		// remove noice.
		while ((*c>='a')&&(*c<='z'))	c++;
		
		space_repeat = false;
		if (d!=txt) 
		{
			char ld = *(d-1);
			if (	((ld==' ')||(ld==',')||(ld==';'))	&&	(*c==' ')	)	space_repeat = true;
		}

		if (space_repeat == false)
		{
			*d++=*c;
		}
	}
	*d = 0;
}

void allocAmalBuf( struct amalBuf *i, int e )
{
	i -> elements = e;
	i -> size = sizeof(void *) * i -> elements;
	i -> call_array = (void *(**) API_AMAL_CALL_ARGS) malloc(i -> size);
}

void reAllocAmalBuf( struct amalBuf *i, int e )
{
	void *(**new_array) API_AMAL_CALL_ARGS;
	int new_elements = i -> elements + e;
	int new_size = sizeof(void *) * new_elements;

	new_array = (void *(**) API_AMAL_CALL_ARGS) malloc( new_size );

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (i -> call_array)
	{
		if (new_array)
		{
			memcpy( new_array, i -> call_array, i->size );
			i->elements = new_elements;
			i->size = new_size;
		}
		else
		{
			new_elements = 0;
			new_size = 0;
		}

		free((void *) i -> call_array);
		i -> call_array = new_array;
	}
	else
	{
		new_elements = 0;
		new_size = 0;
	}

	i -> elements = new_elements;
	i -> size = new_size;
}

bool asc_to_amal_tokens( struct kittyChannel  *channel )
{
	int pos = 0;
	const char *s;
	struct amalTab *found;
	char txt[30];
	const char *script = channel -> script;
	struct amalBuf *amalProg = &channel -> amalProg;
	struct amalWriterData data;

	allocAmalBuf( amalProg, 60 );

	s=script;
	while (*s)
	{
		found = find_amal_command(s);
		
		if (found)
		{
			printf("Command found\n");

			data.at_script = s;
			data.command_len = strlen(found -> name);
			data.arg_len = 0;

			pos += found -> write( channel, found, &amalProg -> call_array[pos], &data, 0 );
			s += data.command_len + data.arg_len;
		}
		else 	if (*s == ' ') 
		{
			printf("skip a space...\n");

			s++;	// skip space..
		}
		else if ((*s >= '0')&&(*s<='9'))
		{
			printf("found number\n");

			unsigned int num = 0;
			while ((*s >= '0')&&(*s<='9'))
			{
				num = (num *10) + (*s-'0');
				s++;
			}

			found = &amalCmds[1];
			data.at_script = s;
			data.command_len =0;
			data.arg_len = 0;

			pos += found -> write( channel, found, &amalProg -> call_array[pos], &data, num );
		}
		else if ((*s >= 'A')&&(*s<='Z'))
		{
			char *t = txt;
			const char *l = s;

			while ((*l >= 'A')&&(*l<='Z')&&((int) (l-s)<25))
			{
				*t++ = *l++;
			}
			*t = 0;

			if (*l==':')
			{
				struct AmalLabelRef label;
				label.pos = pos;
				label.name = strdup( txt );
				found_labels.push_back( label );
				s = l+1;

				printf("found label at pos %d\n",pos);

				getchar();
			}
			else
			{
				printf("unkown data at %s\n",s);
				amalProg -> call_array[pos] = 0;
				return false;
			}
		}
		else
		{
			printf("unkown data at %s\n",s);
			amalProg -> call_array[pos] = 0;
			return false;
		}

		if (pos > amalProg -> elements - 6 )	// larger writer, takes max 6 elements.
		{
			printf("%d > %d\n", pos , amalProg -> elements - 6);

			printf("overflow\n");

			reAllocAmalBuf(amalProg,20);	// add another 20 elements if buffer is low.
		}

	}
	amalProg -> call_array[pos++] = amal_call_next_cmd;
	amalProg -> call_array[pos] = 0;

	// setup default stack of 500.

	channel -> argStack = (int *) malloc( sizeof(int) * 500 );
	channel -> argStackCount = 0;

	// setup callback buffer.

	channel -> progStack = (struct amalCallBack *) malloc(sizeof(struct amalCallBack *)*500);
	channel -> progStackCount = 0;

	return true;
}

void amal_run_one_cycle(struct kittyChannel  *channel)
{
	void *ret;
	void *(**call) ( struct kittyChannel *self, void **code, unsigned int opt );

	printf("%s\n", channel -> amalProg.call_array ? "has amal program code" : "do not have amal program code");

	for (call = channel -> amalProgCounter ;  *call ; call ++ )
	{
		printf("%08x\n",call);
		ret = (*call) ( channel, (void **) call, 0 );
		if (ret) 
		{
			printf("code offset set %08x\n",ret);
			call = (void* (**)(kittyChannel*, void**, unsigned int)) ret;
		}

		if (channel -> status == channel_status::paused) 	// if amal program gets paused, we break loop
		{
			channel -> status = channel_status::active;
			call++;
			break;
		}
	}
	channel -> amalProgCounter = call;	// save counter.
	if (*call == NULL) channel -> status = channel_status::done;

}


#ifdef test_app

void test_run(struct kittyChannel  *channel)
{
	printf("%s\n", channel -> amalProg.call_array ? "has amal program code" : "do not have amal program code");

	// init amal Prog Counter.
	channel -> status = channel_status::active;
	channel -> amalProgCounter = channel -> amalProg.call_array;

	while ( ( channel -> status == channel_status::active ) && ( *channel -> amalProgCounter ) )
	{
		amal_run_one_cycle(channel);
		dumpAmalRegs();
		getchar();
	}
}

void dump_labels()
{
	int i;

	printf("looking for labels\n");

	for (i=0;i<looking_for_labels.size();i++)
	{
		printf("pos 0x%08x, name %s\n",looking_for_labels[i].pos,looking_for_labels[i].name);
	}

	printf("found labels\n");

	for (i=0;i<found_labels.size();i++)
	{
		printf("pos 0x%08x, name %s\n",found_labels[i].pos,found_labels[i].name);
	}
}

bool find_label(char *name, unsigned int &pos)
{
	int i;

	for (i=0;i<found_labels.size();i++)
	{
		if (strcmp(found_labels[i].name, name)==0)
		{
			pos = found_labels[i].pos;
			return true;
		}
	}
	return false;
}

void fix_labels( void **code )
{
	int i;
	unsigned int pos;

	for (i=0;i<looking_for_labels.size();i++)
	{
		 

		if (find_label(looking_for_labels[i].name,pos))
		{
			printf("fix pos %d\n",pos);
			getchar();

			code[ looking_for_labels[i].pos + 1] = &code[pos];
		}
	}
}


int main(int args, char **arg)
{
	struct kittyChannel  channel;

	amalBuf *amalProg = &channel.amalProg;

	amalProg->call_array = NULL;
	amalProg->size = 0;
	amalProg->elements = 0;

	if (args==2)
	{
		channel.script = strdup( (char *) arg[1]);

		if (channel.script)
		{
			remove_lower_case(channel.script);
			printf("%s\n",channel.script);

			if (asc_to_amal_tokens( &channel ))
			{
				fix_labels( (void **) amalProg -> call_array );

				dump_labels();
				getchar();

				test_run( &channel );
			}

			free(channel.script);

			dumpAmalRegs();
		}
	}

	return 0;
}
#endif

