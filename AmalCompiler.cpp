
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#ifdef _MSC_VER
#include <string.h>
#include "vs_missing_string_functions.h"
#define strdup _strdup
//#define Printf printf
#endif

#ifdef __amoskittens_amal_test__
#include "debug_amal_test_app.h"
#else
#include "debug.h"
#endif 

#if defined(__amigaos4__) || defined(__amigaos)
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/retroMode.h>
#include <string.h>
#endif

#ifdef __linux__
#include <string.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#define Printf printf

#ifdef __amoskittens_amal_test__
#define engine_fd stdout
#else
extern FILE *engine_fd;
#endif
#endif

#include "AmosKittens.h"
#include "AmalCompiler.h"
#include "channel.h"
#include "AmalCommands.h"
#include "pass1.h"

#include "amosString.h"

enum
{
	id_void = 0,
	id_autotest =1,
	id_volume
};


int nest = 0;
char last_reg[1000] ;
int amal_error_pos = 0;


struct AmalLabelRef
{
	unsigned int pos;
	char *name;
};

std::vector<void **> amalloops;
static std::vector<struct AmalLabelRef> looking_for_labels;
static std::vector<struct AmalLabelRef> found_labels;

void reAllocAmalBuf( struct amalBuf *i, int e );
void dump_amal_labels();

void *amalAllocBuffer( int size )
{
	return sys_public_alloc_clear(size);
}

#define amalFreeBuffer( ptr ) { sys_free( ptr ); ptr = NULL; }

struct amalNested amal_nested_command[ max_nested_commands ];

#ifdef __amoskittens_amal_test__
	int nested_count = 0;
	int parenthesis_count;
	int amreg[26];
	void print_code( void **adr );
	int parenthesis[40];

	#define dump_channels()

#else
	extern int amreg[26];
	extern int parenthesis[];
#endif

void dumpAmalRegs(struct kittyChannel *channel)
{
	char a[2]={0,0};
	char b[2]={0,0};

	int i;
	for (i=0;i<26;i++)
	{
		a[0]='A'+i;
		b[0]='0'+i;

		if (i<10)
		{
			Printf_iso("R%s is %3d    R%s is %3d\n",a, amreg[i], b, channel -> reg[i] );
		}
		else	Printf_iso("R%s is %3d\n",a, amreg[i]);
	}
}


void *(*amal_cmd_equal) API_AMAL_CALL_ARGS = NULL;

unsigned int (*amal_to_writer) ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num) = NULL;

#ifdef show_debug_amal_yes
void dumpAmalStack( struct kittyChannel *channel )
{
	unsigned int s;
	Printf("Amal Stack\n");
	for (s=0;s<=channel -> argStackCount;s++)
	{
		Printf_iso("stack %d: value %d\n",s, channel -> argStack[s] );
	}

	if (channel -> argStackCount>50)
	{
		Printf("Amal sick puppy :-(, stack is growing too quickly... \n");

		if (channel -> amal_script)
		{
			Printf_iso("script '%s'\n", channel -> amal_script);
		}
	}
}

extern struct amalDebugitem amalDebugList[];

const char *getAmalProgStackName( void *(*fn) (kittyChannel*, amalCallBack*) )
{
	struct amalDebugitem *itm;

	for (itm  = amalDebugList; itm -> fn ; itm++ )
	{
		if ( itm -> fn == fn )	return itm -> name;		
	}

	return NULL;
}

void dumpAmalProgStack( struct kittyChannel *channel )
{
	unsigned int s;
	const char *name;

	Printf("Amal Prog Stack\n");
	for (s=0;s<=channel -> progStackCount;s++)
	{
		struct amalCallBack *CallBack = &channel -> progStack[ s ];

		name = getAmalProgStackName(  CallBack -> cmd  );

		Printf_iso("stack[%d]: cmd %08x (%s), arg stack %d, flags %x\n",
			s, 
			CallBack -> cmd, 
			name ? name : "???", 
			CallBack -> argStackCount, 
			CallBack -> Flags );
	}
}

#endif

void pushBackAmalCmd( amal::Flags flags, void **code, struct kittyChannel *channel, void *(*cmd)  (struct kittyChannel *self, struct amalCallBack *cb)  )
{
#ifdef __amoskittens_amal_test__
	const char *name;
	name = getAmalProgStackName( cmd  );
	printf("push_back %08x (%s)\n", cmd, name ? name : "<NULL>");
#endif

	if (channel -> progStack)
	{
		struct amalCallBack *CallBack = &channel -> progStack[ channel -> progStackCount ];
		if (CallBack)
		{
			CallBack -> Flags = flags;
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

unsigned int AmalWriterIf (	struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	call_array[0] = self -> call;

	AmalPrintf("Writing %-8d to %010d/%010d - If\n",
			num,
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
			(unsigned int) &call_array[1] - (unsigned int) channel -> amalProg.call_array);

	*((int *) &call_array[1]) = 0;
	addNestAmal( if, &call_array[1] );
	amal_cmd_equal = amal_call_equal;

	channel -> next_arg =true;

	return 2;
}

// DO NOT ADD TO TABLE.
unsigned int AmalWriterCondition (	struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	call_array[0] = self -> call;

	AmalPrintf("Writing %-8d to %010d/%010d - Condition\n",
			0,
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
			(unsigned int) &call_array[1] - (unsigned int) channel -> amalProg.call_array);

	*((int *) &call_array[1]) = 0;

	// modify the nest.
	amal_nested_command[ nested_count -1 ].cmd = num;
	amal_nested_command[ nested_count -1 ].offset = (unsigned int) &call_array[1] - (unsigned int) channel -> amalProg.call_array;

	channel -> let =false;

	return 2;
}

unsigned int AmalWriterNum (	struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	call_array[0] = self -> call;

	AmalPrintf("Writing %-8d to %010d/%010d - num\n",
			num, (unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
			(unsigned int) &call_array[1] - (unsigned int) channel -> amalProg.call_array);

	*((int *) &call_array[1]) = num;
	channel -> let = false;
	channel -> next_arg = false;
	return 2;
}

int animScriptLength( const char *str, const char *valid_chars )
{
	const char *c;
	const char *vc;
	bool valid;
	int l = 0;

	AmalPrintf("%s:%d\n",__FUNCTION__,__LINE__);

	AmalPrintf("valid_char: %s\n",valid_chars);

	for (c = str ; (*c!=0) && (*c!=';') ; c++)
	{
		valid = false;
		for (vc=valid_chars; *vc ; vc++) if (*vc == *c) { valid = true; break; }
		if (valid == false) break;
		l ++;
	}

	printf("exited on '%c' - len %d\n",*c,l);

#ifdef __amoskittens_amal_test__
	printf("%s\n",str);
	printf("%s: ",__FUNCTION__);
	for (c = str; c<(str+l);c++) printf("%c",*c);
	printf("\n");
#endif


	return l;
}


int amalStringLength( const char *str )
{
	const char *c;
	int l = 0;
	for (c = str ; (*c==0) | (*c==';') ; c++) l ++;
	return l;
}

const char *AmalAtStringArg( const char  *at_script )
{
	const char *s = at_script;
	while ((*s != 0)&&(*s != ' ')) s++;
	while (*s == ' ') s++;
	return s;
}

int writeAmalStringToBuffer( const char *s, char *d, int _max_len )
{
	int le = 0;
	while ((*s != 0)&&(*s != ' ')&&( *s != ';')&&(le < _max_len)) { *d++=*s++; le++; }
	*d = 0;
	return le;
}

unsigned int stdAmalWriterScript (	struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	const char *s;
	int anim_script_len;
	unsigned int size;
	int le;
	unsigned int offset;

	call_array[0] = self -> call;

	struct amalBuf *amalProg = &channel -> amalProg;

	AmalPrintf("Writing %-8d to %010d/%010d - script\n",
		num,
		(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
		(unsigned int) &call_array[1] - (unsigned int) channel -> amalProg.call_array);

// find script find the size

	s = AmalAtStringArg( data -> at_script );
	anim_script_len =animScriptLength( s, "R,0123456789() " );
	size = 2 + ((anim_script_len + sizeof(void *)) / sizeof(void *) );

// check if size is ok, or need a new buffer.

	if (amalProg -> used > amalProg -> elements - size )	// larger writer, takes max 6 elements.
	{
		reAllocAmalBuf(amalProg,size + 20);	// add another 20 elements if buffer is low.

		// now that call array is new, need to reset it.
		call_array = &amalProg -> call_array[amalProg -> used];
	}

// text arg length, 			(do not confuse with token arg length)

	data -> arg_len = anim_script_len;

// write the script into buffer.

	le = writeAmalStringToBuffer( s, (char *) (&call_array[2]) , anim_script_len );

	offset = ((le + sizeof(void *)) / sizeof(void *) );
	*((int *) &call_array[1]) = offset ;
	return 2 + offset;
}

void amal_clean_up_labels( )
{
	struct AmalLabelRef label;

	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	while ( ! found_labels.empty() )
	{
		label = found_labels.back();

		if (label.name)
		{
			free(label.name);
			label.name = NULL;
		}

		found_labels.pop_back();
	}

	while ( ! looking_for_labels.empty() )
	{
		label = looking_for_labels.back();

		if (label.name)
		{
			free(label.name);
			label.name = NULL;
		}

		looking_for_labels.pop_back();
	}
}


unsigned int stdAmalWriterJump (	struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	char labelname[20];
	int le = 0;
	const char *s;
	char *d;
	bool is_autotest = false;

	if (instance.parenthesis_count) if ( parenthesis[instance.parenthesis_count-1]  == id_autotest ) is_autotest = true;

	if (is_autotest)
	{
		AmalPrintf("writing %08x to %010d/%010d - jump\n",
			(unsigned int) amal_call_jump_autotest,
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
			(unsigned int) &call_array[1] - (unsigned int) channel -> amalProg.call_array );

		call_array[0] = amal_call_jump_autotest;
	}
	else
	{
		AmalPrintf("writing %08x to %010d/%010d - jump\n",
			(unsigned int) self -> call,
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
			(unsigned int) &call_array[1] - (unsigned int) channel -> amalProg.call_array );

		call_array[0] = self -> call;
	}

	s = data -> at_script + data -> command_len;

	while (*s == ' ')
	{
		s++; le++;
	}

	d = labelname;
	if ((*s != 0) && (*s != ' ') && (*s != ';') && (*s != ':') && (le < 18)) { *d++ = *s++; le++; }
	*d = 0;

	if ((*s == ';') || (*s ==':')) le++;	// if next command is ; we can skip it.

	data -> arg_len = le ;
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

unsigned int stdAmalWriterLet ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	call_array[0] = amal_flush_prog;
	channel -> let = true;
	amal_cmd_equal = NULL;
	return 1;
}

unsigned int stdAmalWriterX ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	AmalPrintf("writing %08x to %010d  - X\n",
			(unsigned int) amal_call_x,
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array );

	call_array[0] = amal_call_x;
	return 1;
}

unsigned int stdAmalWriterImage ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	AmalPrintf("writing %08x to %010d  - Anim image\n",
			(unsigned int) amal_call_image,
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array );

	call_array[0] = amal_call_image;
	return 1;
}


unsigned int stdAmalWriterExit( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	AmalPrintf("writing %08x to %010d  - exit\n",
			(unsigned int) NULL,
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array );

	call_array[0] = NULL;		// terminate AMAL program in Autotest
	return 1;
}


void fix_condition_branch( struct kittyChannel *channel,  unsigned int relative_adr )
{
	unsigned int *ptr;
	unsigned int offset = amal_nested_command[ nested_count-1 ].offset;

	ptr = (unsigned int *) ((char *) channel -> amalProg.call_array + offset);
	*ptr = relative_adr;
}


unsigned int stdAmalWriterNextCmd ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	AmalPrintf("writing %08x to %010d  - ;   \n",
			(unsigned int) self -> call,
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array );

	// this command doubles as "end if"

	switch (GET_LAST_AMAL_NEST)
	{
		case nested_if:
		case nested_then:
		case nested_else:
			fix_condition_branch( channel,  (unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array );
			nested_count --;
			break;
	}

	call_array[0] = self -> call;
	amal_cmd_equal = NULL;
	channel -> let = false;
	channel -> next_arg = false;
	return 1;
}

unsigned int stdAmalWriterValue ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	AmalPrintf("writing %08x to %010d  - %s\n", 
			(unsigned int) self -> call, 
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array, 
			self->name );
	call_array[0] = self -> call;
	channel -> next_arg = false;
	return 1;
}



unsigned int stdAmalWriterWithArg ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	AmalPrintf("writing %08x to %010d  - %s\n", 
			(unsigned int) self -> call, 
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array, 
			self->name );
	call_array[0] = self -> call;
	channel -> next_arg = true;
	return 1;
}
unsigned int stdAmalWriter ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	AmalPrintf("writing %08x to %010d  - %s\n",
			(unsigned int) self -> call,
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
			self->name );

	if ((data  -> lastClass == amal::class_cmd_arg) && ( amal_call_next_cmd != self -> call ))
	{
		call_array[0] = amal_call_next_cmd;
		call_array[1] = self -> call;
		return 2;
	}
	else
	{
		call_array[0] = self -> call;
		return 1;
	}
}

int autotest_start_ptr_offset;

unsigned int AmalAutotest ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	parenthesis[instance.parenthesis_count]  = id_autotest ;
	return 0;
}

unsigned int  stdAmalWriterParenthsesStart( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	AmalPrintf("writing %08x to %010d  - %s\n", 
			(unsigned int) self -> call, 
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array, 
			self->name );

	printf("parenthesis[%d] is %d\n", instance.parenthesis_count, parenthesis[instance.parenthesis_count]);

	if (parenthesis[instance.parenthesis_count] == id_autotest)
	{
		instance.parenthesis_count++;
		parenthesis[instance.parenthesis_count] = id_void;


		call_array[0] = autotest_start;
		call_array[1] = 0;

		autotest_start_ptr_offset = (unsigned int) &call_array[1] - (unsigned int) channel -> amalProg.call_array;
		channel -> next_arg = false;
		return 2;
	}

	instance.parenthesis_count++;
	parenthesis[instance.parenthesis_count] = id_void;

	call_array[0] = self -> call;
	channel -> next_arg = true;
	return 1;
}


unsigned int stdAmalWriterParenthsesEnd ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{

	AmalPrintf("writing %08x to %010d  - %s\n", 
			(unsigned int) self -> call, 
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array, 
			self->name );

	if (instance.parenthesis_count) instance.parenthesis_count--;

//	printf("parenthesis[%d] is %d\n", instance.parenthesis_count, parenthesis[instance.parenthesis_count]);

	switch ( parenthesis[instance.parenthesis_count] )
	{
		case id_autotest:

			if (autotest_start_ptr_offset>-1)
			{
				AmalPrintf("writing %08x to %010d  - autotest end\n",
					(unsigned int) self -> call,
					(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
					self->name );

					void **ptr = (void **)  ((char *) channel -> amalProg.call_array + autotest_start_ptr_offset);
					*ptr = (void *) ((unsigned int) &call_array[1] - (unsigned int) channel -> amalProg.call_array);

					call_array[0] = 0;	// end of autotest ;-)

					parenthesis[instance.parenthesis_count] = id_void;

				return 1;
			}
			else
			{
				AmalPrintf("writing %08x to %010d  - %s\n",
					(unsigned int) self -> call,
					(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
					self->name );

				call_array[0] = self -> call;
				channel -> next_arg = false;
				parenthesis[instance.parenthesis_count] = id_void;
				return 1;
			}

		case id_void:

			{
				AmalPrintf("writing %08x to %010d  - %s\n",
					(unsigned int) self -> call,
					(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
					self->name );

				call_array[0] = self -> call;
				channel -> next_arg = false;
				parenthesis[instance.parenthesis_count] = id_void;
				return 1;
			}

	}

	return 0;
}

unsigned int stdAmalWriterSymbol ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	AmalPrintf("writing %08x to %010d  - %s\n",
			(unsigned int) self -> call,
			(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array,
			self->name );

	call_array[0] = self -> call;
	channel -> next_arg = true;
	return 1;
}


unsigned int stdAmalWriterEqual ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	if (amal_cmd_equal)
	{
		AmalPrintf("writing %08x to %010d  ==\n", (unsigned int) amal_cmd_equal, (unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array );
		call_array[0] = amal_cmd_equal;
	}
	else
	{
		AmalPrintf("writing %08x to %010d  =\n", (unsigned int) self -> call,(unsigned int)  &call_array[0] - (unsigned int) channel -> amalProg.call_array );
		call_array[0] = self -> call;
		amal_cmd_equal = amal_call_equal;
	}

	return 1;
}

unsigned int amal_for_to ( struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
		AmalPrintf("writing [code block] to %010d - for \n", (unsigned int) &call_array[0]);
		char *current_location = (char *) (&call_array[1]);
		char *start_location = (char *) channel -> amalProg.call_array;

		amalloops.push_back( (void **) (current_location - start_location) );

		AmalPrintf("start location:    %08x\n",(unsigned int) start_location);
		AmalPrintf("current location: %08x\n",(unsigned int) current_location);

		call_array[0] = amal_call_next_cmd;			// ; while 
		call_array[1] = amal_call_while;
		call_array[2] = NULL;
		call_array[3] = amal_call_reg;				// Reg
		*((int *) &call_array[4]) = last_reg[nest];		// number
		call_array[5] = amal_call_less_or_equal;		// <=

		amal_to_writer = NULL;
		nest++;

		channel -> next_arg = true;		// TO R0, TO <value>
		return 6;
}

unsigned int stdAmalWriterFor (  struct kittyChannel *channel, struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int num)
{
	amal_to_writer = amal_for_to;
	channel -> let = true;
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

	call_array[0] = amal_call_inc;		// inc R0
	call_array[1] = amal_call_reg;
	*((int *) &call_array[2]) = last_reg[nest];
	call_array[3] = amal_call_next_cmd;	// ;
	call_array[4] = amal_call_wend;		// wend

	if (!amalloops.empty())
	{
		int elements;
		void **code;
		int rel_location = (int) amalloops.back() ;
		char *current_location;
		char *start_location;

		current_location = (char *) &call_array[4];
		start_location = (char *) channel -> amalProg.call_array;
		code = (void **)  (start_location + rel_location);
		elements = (int) (current_location - ((char*) code)) / sizeof(void *);

		code[1] = (void *) (current_location - start_location) ;		// insert skip over address into while..

		*((void **) &call_array[5]) = (void **) rel_location;
		amalloops.pop_back();
	}

	channel -> next_arg = true;		// Next R0

	return 6;
}

unsigned int stdAmalWriterReg (  struct kittyChannel *channel,struct amalTab *self,
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ),
				struct amalWriterData *data,
				unsigned int )
{
	int num = *(data -> at_script + 1);
	AmalPrintf("writing %08x to %010d  - %s\n",(unsigned int) amal_call_reg,(unsigned int) &call_array[0] - (unsigned int) channel -> amalProg.call_array, self -> name );
	call_array[0] = amal_call_reg;
	*((int *) &call_array[1]) = num;

	last_reg[nest] =num;

	return 2;
}

struct amalTab amalSymbols[] =
{
	{";",amal::class_cmd_normal,stdAmalWriterNextCmd,amal_call_next_cmd },
	{"(",amal::class_cmd_operator,stdAmalWriterParenthsesStart,amal_call_parenthses_start },
	{")",amal::class_cmd_operator,stdAmalWriterParenthsesEnd,amal_call_parenthses_end },
	{",",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_nextArg },
	{"--",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_add},			// +
	{"+-",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_sub},			// -
	{"+",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_add},			// +
	{"-",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_sub},			// -
	{"*",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_mul},			// *
	{"/",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_div},			// /
	{"&",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_and},			// &
	{"!",amal::class_cmd_operator,	stdAmalWriterSymbol,amal_call_xor},				// xor
	{"<>",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_not_equal},	// <>
	{"<",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_less},			// <
	{">",amal::class_cmd_operator,stdAmalWriterSymbol,amal_call_more},			// >
	{"=",amal::class_cmd_operator,stdAmalWriterEqual,amal_call_set},		// =
	{NULL, amal::class_cmd_arg,NULL,NULL }
};

struct amalTab amalCmds[] =
{
	{"@{never used}",amal::class_cmd_arg,stdAmalWriter,NULL},
	{"@{number}",amal::class_cmd_arg,AmalWriterNum,amal_set_num},	//number token, reserved.
	{"O",amal::class_cmd_normal,stdAmalWriter,amal_call_on},			// On
	{"D",amal::class_cmd_normal,stdAmalWriterJump,amal_call_direct},					// Direct
	{"W",amal::class_cmd_normal,stdAmalWriter,amal_call_wait},					// Wait
	{"I",amal::class_cmd_normal,AmalWriterIf,amal_call_if},				 // If
	{"L",amal::class_cmd_normal,stdAmalWriterLet,NULL},				// Let
	{"AU",amal::class_cmd_normal,AmalAutotest,NULL},					// AUtotest
	{"A",amal::class_cmd_arg,stdAmalWriterImage,amal_call_image},		// Anim image
	{"A",amal::class_cmd_normal,stdAmalWriterScript,amal_call_anim},		// Anim
	{"M",amal::class_cmd_normal,stdAmalWriterWithArg,amal_call_move},	// Move
	{"PL",amal::class_cmd_normal,stdAmalWriter,amal_call_play},			// Play
	{"P",amal::class_cmd_normal,stdAmalWriter,amal_call_pause},			// Pause
	{"R0",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R0
	{"R1",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R1
	{"R2",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R2
	{"R3",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R3
	{"R4",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R4
	{"R5",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R5
	{"R6",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R6
	{"R7",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R7
	{"R8",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R8
	{"R9",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R9
	{"RA",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RA
	{"RB",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RB
	{"RC",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RC
	{"RD",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RD
	{"RE",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RE
	{"RF",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RF
	{"RG",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RG
	{"RH",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RH
	{"RI",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RI
	{"RJ",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RJ
	{"RK",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RK
	{"RL",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RL
	{"RM",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RM
	{"RN",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RN
	{"RO",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RO
	{"RP",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RP
	{"RQ",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RQ
	{"RR",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RR
	{"RS",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RS
	{"RT",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RT
	{"RU",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RU
	{"RV",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RV
	{"RW",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RW
	{"RX",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RX
	{"RY",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RY
	{"RZ",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RZ

	{"F",amal::class_cmd_normal,stdAmalWriterFor,NULL},				// For (should be null)
	{"T",amal::class_cmd_normal,stdAmalWriterTo,amal_call_nextArg},		// To
	{"N",amal::class_cmd_normal,stdAmalWriterWend,amal_call_wend},		// Next (should be null)

	{"E",amal::class_cmd_normal,stdAmalWriter,amal_call_end},			// End
	{"XM",amal::class_cmd_arg,stdAmalWriterValue,amal_call_xm},			// XM
	{"YM",amal::class_cmd_arg,stdAmalWriterValue,amal_call_ym},			// YM
	{"K1",amal::class_cmd_arg,stdAmalWriter,amal_call_k1},				// k1		mouse key 1
	{"K2",amal::class_cmd_arg,stdAmalWriter,amal_call_k2},				// k2		mouse key 2
	{"J0",amal::class_cmd_arg,stdAmalWriter,amal_call_j0},				// j0		joy0
	{"J1",amal::class_cmd_arg,stdAmalWriter,amal_call_j1},				// J1		Joy1
	{"J",amal::class_cmd_normal,stdAmalWriterJump,amal_call_jump},		// Jump
	{"Z",amal::class_cmd_arg,stdAmalWriter,amal_call_z},				// Z(n)	random number
	{"XH",amal::class_cmd_arg,stdAmalWriter,amal_call_xh},				// x hardware
	{"YH",amal::class_cmd_arg,stdAmalWriter,amal_call_yh},				// y hardware
	{"XS",amal::class_cmd_arg,stdAmalWriter,amal_call_xscreen},			// x screen
	{"YS",amal::class_cmd_arg,stdAmalWriter,amal_call_yscreen},			// y screen
	{"BC",amal::class_cmd_arg,stdAmalWriter,amal_call_bobCol},			// Bob Col(n,s,e)	// only with Synchro
	{"SC",amal::class_cmd_arg,stdAmalWriter,amal_call_spriteCol},			// Sprite Col(m,s,e)	// only with Synchro

	{"SX",amal::class_cmd_arg,stdAmalWriter,amal_call_screen_width},		// screen width	(same as interface command)
	{"SY",amal::class_cmd_arg,stdAmalWriter,amal_call_screen_height},		// screen height (same as interface command)

	{"C",amal::class_cmd_arg,stdAmalWriter,amal_call_col},				// Col
	{"V",amal::class_cmd_normal,stdAmalWriter,amal_call_vumeter},		// Vumeter

	{"X",amal::class_cmd_arg,stdAmalWriterX,NULL},					// X
	{"X",amal::class_cmd_normal,stdAmalWriterExit,NULL},				// X

	{"Y",amal::class_cmd_arg,stdAmalWriter,amal_call_y},				// Y
	{"Y",amal::class_cmd_normal,stdAmalWriter,amal_call_y},				// Y

	{"@while",amal::class_cmd_normal,stdAmalWriter,amal_call_while },
	{"@set",amal::class_cmd_arg,stdAmalWriter,amal_call_set },
	{"@reg",amal::class_cmd_arg,stdAmalWriter,amal_call_reg },
	{"@flush_prog",amal::class_cmd_arg,stdAmalWriter,amal_flush_prog },
	{NULL, amal::class_cmd_arg,NULL,NULL }
};

const char *amalAdr2Name( void *adr )
{
	struct amalTab *itm;

	if (adr == NULL) return NULL;

	for ( itm = amalCmds; itm -> name; itm ++ )
	{
		if ( ((void *) itm -> call) == adr) return itm -> name;
	}

	for ( itm = amalSymbols; itm -> name; itm ++ )
	{
		if ( ((void *) itm -> call) == adr) return itm -> name;
	}

	return NULL;
}

void print_code( void **adr )
{
	if (*adr == NULL)
	{
		printf("%010d - %010d - %d\n",(unsigned int) adr,(unsigned int) 0,0);
	}

	for (struct amalTab *tab = amalCmds; tab -> name ; tab++ )
	{
		if ( tab->call == *adr )
		{
			printf("%010d - %010d - name %s\n",(unsigned int) adr,(unsigned int) *adr, tab -> name);
			return;
		}
	}

	{
		unsigned int c = (unsigned int) *adr;

		if (((c>='0')&&(c<='9')) || ((c>='A')&&(c<='Z')))
		{
			printf("%010d - %010d - R%c\n",(unsigned int) adr,(unsigned int) *adr,(int) *adr);
		}
		else
		{
			printf("%010d - %010d - %d\n",(unsigned int) adr,(unsigned int) *adr,(int) *adr);
		}

	}
}

struct amalTab *find_amal_symbol(const char *str)
{
	int l;

	for (struct amalTab *tab = amalSymbols; tab -> name ; tab++ )
	{
		l = strlen(tab->name);

		if (strncasecmp(str, tab -> name, l) == 0) return tab;
	}
	return NULL;
}

bool amal_is_label(const char *str)
{
	if (str[0]==0) return false;
	if (str[1]==':') return true;
	return false;
}

struct amalTab *find_amal_command(const char *str , int class_flags )
{
	char next_c;
	int l;
	struct amalTab *symbol = NULL;

	for (struct amalTab *tab = amalCmds; tab -> name ; tab++ )
	{
		l = strlen(tab->name);

//		printf("tab -> Class: %d, class_flags %d \n", tab -> Class , class_flags);

		if ( tab -> Class & class_flags )
		{
			if (strncasecmp(str, tab -> name, l) == 0)
			{
				next_c = *(str+l);

				if (next_c != ':')	// chack if its a label
				{
					symbol = find_amal_symbol( str + l );

					if ((next_c == ' ') || (symbol) || (next_c == 0))
					{
						 return tab;
					}
					else if (find_amal_command(str+l, (int) amal::class_cmd_all_args | (int) amal::class_cmd_normal))
					{
						return tab;
					}
					else if (*str=='J')	// if command is jump.
					{
						return tab;
					}
					else if (amal_is_label(str+1))
					{
						return tab;
					}
				}
			}
		}
	}
	return NULL;
}

struct amalTab *find_amal_command_ends_with_number(const char *str , int class_flags )
{
	char next_c;
	int l;

	for (struct amalTab *tab = amalCmds; tab -> name ; tab++ )
	{
		l = strlen(tab->name);

		if ( tab -> Class & class_flags )
		{
			if (strncasecmp(str, tab -> name, l) == 0)
			{
				next_c = *(str+l);

				if ((next_c >= '0')&&(next_c <='9'))	// chack if its a number
				{
					return tab;
				}
			}
		}
	}
	return NULL;
}

void remove_lower_case(struct stringData *txt)
{
	char *c;
	char *d;
	bool space_repeat;

	d=&txt -> ptr;
	for (c=&txt -> ptr;*c;c++)
	{
		// remove noice.
		while (((*c>='a')&&(*c<='z'))||(*c=='#'))	{ c++;  }

		space_repeat = false;
		if (d!=&txt->ptr)
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
	txt -> size = strlen( &txt -> ptr );

}

void allocAmalBuf( struct amalBuf *i, int e )
{
	i -> elements = e;
	i -> size = sizeof(void *) * i -> elements;
	i -> used = 0;
	i -> prog_crc = 0;
	i -> call_array = (void *(**) API_AMAL_CALL_ARGS) amalAllocBuffer(i -> size);
}

void freeAmalBuf( struct amalBuf *i)
{
	i -> elements = 0;
	i -> size = 0;
	i -> prog_crc = 0;
	if ( i -> call_array ) amalFreeBuffer( i -> call_array );
	i -> call_array = NULL;
}

void reAllocAmalBuf( struct amalBuf *i, int e )
{
	void *(**new_array) API_AMAL_CALL_ARGS;
	int new_elements = i -> elements + e;
	int new_size = sizeof(void *) * new_elements;

	new_array = (void *(**) API_AMAL_CALL_ARGS) amalAllocBuffer  ( new_size );

	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (i -> call_array) // has old buffer, we need to copy it.
	 {
		if (new_array)
		{
			memcpy( new_array, i -> call_array, i->size );
			amalFreeBuffer(i->call_array);

			i->call_array = new_array;
			i->elements = new_elements;
			i->size = new_size;

			return;	// success
		}
		else
		{
			amalFreeBuffer(i->call_array);	// remove old buffer failed.
		}
	}

	if (new_array == NULL)	// failed...
	{
		new_elements = 0;
		new_size = 0;
	}

	i -> elements = new_elements;
	i -> call_array = new_array;
	i -> size = new_size;
}

int asc_to_amal_tokens( struct kittyChannel  *channel )		// return error code
{
	const char *s;
	struct amalTab *found;
	struct amalTab write_cmd;
	char txt[30];
	struct stringData *script = channel -> amal_script;
	struct amalBuf *amalProg = &channel -> amalProg;
	struct amalWriterData data;

	nested_count = 0;
	amal_cmd_equal = NULL;
	channel -> let = false;
	channel -> next_arg = false;

	// reset R0-R9 registers, must be reset etch time we compile a new amal script.
	memset( (char *) channel -> reg, 0, sizeof(channel -> reg) );

#ifdef __amoskittens_amal_test__
// 1000 to avoid reallocs.
// 20 to test reallocs.
	allocAmalBuf( amalProg, 60 );
#else
	allocAmalBuf( amalProg, 60 );
#endif

	autotest_start_ptr_offset = -1;

	s=&script -> ptr;
	while (*s)
	{
		AmalPrintf("LET = %s, next arg %s\n", channel -> let ? "TRUE" : "FALSE", channel -> next_arg ? "TRUE" : "FALSE");

		if ((channel -> let)||(channel -> next_arg))	// this is most likely a arg or a calculation
		{
			found = find_amal_command(s, amal::class_cmd_all_args);
			if (!found)	found = find_amal_command(s, amal::class_cmd_normal);
			if (!found) found = find_amal_command_ends_with_number(s, amal::class_cmd_normal);
		}
		else	// this is a normal command.
		{
			found = find_amal_command(s, amal::class_cmd_normal);
			if (!found) found = find_amal_command_ends_with_number(s, amal::class_cmd_normal);
		}

		if (!found) found = find_amal_symbol(s);

		if (found)
		{
			data.at_script = s;
			data.command_len = strlen(found -> name);
			data.arg_len = 0;

			if ( found -> Class == amal::class_cmd_normal  )
			{
				switch (GET_LAST_AMAL_NEST)
				{
					case nested_if:
						fix_condition_branch( channel, (unsigned int) &amalProg -> call_array[amalProg -> used] - (unsigned int) channel -> amalProg.call_array );
						write_cmd.call = amal_call_then;
						amalProg -> used += AmalWriterCondition( channel, &write_cmd , &amalProg -> call_array[amalProg -> used], &data, nested_then);
						amal_cmd_equal = NULL;
						break;

					case nested_then:
						fix_condition_branch( channel, (unsigned int) &amalProg -> call_array[amalProg -> used] - (unsigned int) channel -> amalProg.call_array );	// skip over else
						write_cmd.call = amal_call_else;

//						amalProg -> used += AmalWriterCondition( channel, &write_cmd , &amalProg -> call_array[amalProg -> used], &data, nested_else);

						nested_count--;

						amal_cmd_equal = NULL;
						break;
				}
			}

			amalProg -> used += found -> write( channel, found, &amalProg -> call_array[amalProg -> used], &data, 0 );
			data.lastClass = found -> Class;
			s += data.command_len + data.arg_len;
		}
		else 	if (*s == ' ')
		{
			s++;	// skip space..
		}
		else if ((*s >= '0')&&(*s<='9'))
		{
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

			amalProg -> used += found -> write( channel, found, &amalProg -> call_array[amalProg -> used], &data, num );
			data.lastClass = found -> Class;
		}
		else if ((*s >= 'A')&&(*s<='Z'))	 	// have not found command, maybe its a label
		{
			char *t = txt;	// txt is a temp buffer
			const char *l = s;

			while ((*l >= 'A')&&(*l<='Z')&&((int) (l-s)<25))		//Get string, until first symbol or number.
			{
				*t++ = *l++;
			}
			*t = 0;

			if (*l==':')	// check if its vaild label.
			{
				struct AmalLabelRef label;
				label.pos = amalProg -> used;
				label.name = strdup( txt );
				found_labels.push_back( label );
				s = l+1;
			}
			else
			{
				AmalPrintf("code bad at: '%s'\n",s);

				amalProg -> call_array[amalProg -> used] = 0;
				amal_error_pos = (ULONG) (s - &(channel -> amal_script -> ptr));
				printf("%d\n",amal_error_pos);

				return 107;
			}
		}
		else
		{
			AmalPrintf("script: %s\n",&(channel -> amal_script -> ptr));
			AmalPrintf("code bad at: '%s'\n",s);

			amalProg -> call_array[amalProg -> used] = 0;
			amal_error_pos = (ULONG) (s - &(channel -> amal_script -> ptr));

			return 107;
		}

		if (amalProg -> used > amalProg -> elements - 6 )	// larger writer, takes max 6 elements.
		{
			reAllocAmalBuf(amalProg,20);	// add another 20 elements if buffer is low.
		}
	}

	amalProg -> call_array[amalProg -> used++] = amal_call_next_cmd;
	amalProg -> call_array[amalProg -> used] = 0;

	amalProg -> prog_crc = 0;
	for (unsigned int i = 0; i < amalProg -> used; i++)
	{
		amalProg -> prog_crc ^= (unsigned int) amalProg -> call_array[i];
	}

	// setup default stack of 500.

	channel -> argStack = allocType(int,500);
	channel -> argStackCount = 0;

	// setup callback buffer.

	channel -> progStack = allocStruct(amalCallBack,500);
	channel -> progStackCount = 0;

	channel -> amalProg.amalProgCounter = channel -> amalProg.call_array;

	if (autotest_start_ptr_offset>-1)
	{
		channel -> amalProg.amalAutotest = (void* (**)(kittyChannel*, void**, unsigned int)) ((char *) channel -> amalProg.call_array + autotest_start_ptr_offset + sizeof(void *) );

		printf("%08x\n", channel -> amalProg.amalAutotest);
	}
	else channel -> amalProg.amalAutotest =  NULL;


//	amalDiscompile( &channel -> amalProg );
//	dump_channels();

	return 0;
}

void amal_run_one_cycle(struct kittyChannel  *channel, void *(**prog) API_AMAL_CALL_ARGS, bool save )
{
	void *ret;
	void *(**call) API_AMAL_CALL_ARGS;

	for (call = prog ;  *call ; call ++ )
	{
		AmalPrintf("offset %d call %08x status %08x\n", (unsigned int) call - (unsigned int) channel -> amalProg.call_array,  *call, channel -> amalStatus );

		ret = (*call) ( channel, (void **) call, 0 );
		if (ret)
		{
			call = (void* (**)(kittyChannel*, void**, unsigned int)) ret;
		}

		if (channel -> amalStatus & channel_status::paused) 	// if amal program gets paused, we break loop
		{
//			channel -> amalStatus &= ~channel_status::paused;
			call++;
			break;
		}

		if (channel -> amalStatus & channel_status::direct) 
		{
			channel -> amalStatus &= ~channel_status::direct;
			break;
		}
	}

	if (save) // normal amal prog
	{
		channel -> amalProg.amalProgCounter = call;	

		if (*call == NULL)
		{
			printf("pos at  %08x\n",call );
			printf("code at %08x\n",channel -> amalProg.call_array );

			AmalPrintf("%s:%s:%d - amal program ended, offset %d\n",__FILE__,__FUNCTION__,__LINE__, (unsigned int) call - (unsigned int) channel -> amalProg.call_array );
			channel -> amalStatus = channel_status::done;
		}
	}
	else	// autotest prog
	{
		if (*call == NULL ) Printf("autotest end\n");

	}
}

bool amal_find_label(char *name, unsigned int *ref_pos)
{
	unsigned int i;
	*ref_pos = 0xFFFFFFFF;

	for (i=0;i<found_labels.size();i++)
	{
		if (strcmp(found_labels[i].name, name)==0)
		{
			*ref_pos = found_labels[i].pos;
			return true;
		}
	}
	return false;
}

bool amal_fix_labels( void **code )
{
	unsigned int i;
	unsigned int ref_pos = 0xFFFFFFFE;
	unsigned int fixed = 0;

	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (i=0;i<looking_for_labels.size();i++)
	{
		if (amal_find_label(looking_for_labels[i].name,&ref_pos))
		{
			if (ref_pos != 0xFFFFFFFF)
			{
				code[ looking_for_labels[i].pos + 1] = &code[ref_pos];
				fixed ++;
			}
		}
	}

	AmalPrintf("looking for labels %d, Fixed labels %d\n",looking_for_labels.size(), fixed );

	return (fixed == looking_for_labels.size());
}


void dump_amal_labels()
{
	unsigned int i;

	AmalPrintf("looking for labels\n");

	for (i=0;i<looking_for_labels.size();i++)
	{
		AmalPrintf("pos 0x%08x, name %s\n",looking_for_labels[i].pos,looking_for_labels[i].name);
	}

	AmalPrintf("found labels\n");

	for (i=0;i<found_labels.size();i++)
	{
		AmalPrintf("pos 0x%08x, name %s\n",found_labels[i].pos,found_labels[i].name);
	}
}

void amalDiscompile( amalBuf *amalProg )
{
	unsigned int off;
	void **ptr;
	const char *name;

	if (amalProg->call_array == NULL) return;

	for (off = 0; off < amalProg -> used ; off++)
	{
		ptr =  (void **) &amalProg->call_array[ off ];
		name = amalAdr2Name( *ptr );

		if (name)
		{
			printf("%10d - %s\n", (unsigned int) ptr - (unsigned int)  amalProg -> call_array ,name);
		}
		else printf("%10d - %08x (%d)\n", (unsigned int) ptr - (unsigned int)  amalProg -> call_array ,*ptr,*ptr);
	}
	getchar();
}



#ifdef __amoskittens_amal_test__

struct KittyInstance instance;

unsigned int amiga_joystick_dir[4];
unsigned int amiga_joystick_button[4];

int obj_x = 100, obj_y = 50, obj_image = 20;

int getMax ( void )
{
	return 1;
}

int getImage (unsigned int object)
{
	return obj_image;
}

int getX (unsigned int object)
{
	return obj_x;
}

int getY (unsigned int object)
{
	return obj_y;
}

void setImage (unsigned int object,int image)
{
	obj_image = image;
}

void setX (unsigned int object,int x)
{
	obj_x = x;
}

void setY (unsigned int object,int y)
{
	obj_y = y;
}

struct channelAPI test_api =
{
	getMax,
	getImage,
	getX,
	getY,
	setImage,
	setX,
	setY
};

void dump_object();

// Se commandsAmal.cpp / channel_amal( ... ) function for the real amal executer.

void test_run(struct kittyChannel  *channel)
{
	// init amal Prog Counter.
	channel -> amalStatus = channel_status::active;
	channel -> objectAPI = &test_api;

	while ( ( channel -> amalStatus & channel_status::active ) && ( channel -> amalProg.amalProgCounter ) )
	{
		if (channel -> amalProg.amalAutotest != NULL)
		{
			Printf("-- Execute autotest --\n");
			channel -> autotest_loopCount = 0;		// unstuck counter.
			amal_run_one_cycle(channel,channel -> amalProg.amalAutotest,false);
			Printf("-- auto test done --\n");
		}

		if (channel -> amalStatus & channel_status::wait) return;		// if amal program is set to wait..., only autotest can activate it.

		if ( ( channel -> amalStatus & channel_status::active ) && ( channel -> amalProg.amalProgCounter ) )
		{
			AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

			// Check that program has not ended.
			if ( *channel -> amalProg.amalProgCounter )	
			{
				amal_run_one_cycle(channel,channel -> amalProg.amalProgCounter,true);
			}
			else 
			{
				AmalPrintf("%s:%s:%d - channel -> amalProgCounter %d\n",__FILE__,__FUNCTION__,__LINE__, channel -> amalProg.amalProgCounter);
				break;
			}

			dump_object();
			printf("amal program counter at %10d\n",(unsigned int) channel -> amalProg.amalProgCounter - (unsigned int) channel -> amalProg.call_array);		

			if (channel -> amalStatus & channel_status::paused) 
			{
				channel -> amalStatus &= ~channel_status::paused;

				Printf("Amal Status %ld\n",channel -> amalStatus);
				Printf("Paused (1=contune, 0=quit)\n");
				char c=getchar();
				if (c=='0') break;
			}
		}
	}


}

void dump_object()
{
	printf("x: %d, y: %d a: %d\n", obj_x, obj_y, obj_image);
}

const char *arg_names[] = 
	{
		"-reg0",
		"-regA",
		"-script",
		"-bin",
		"-file",
		NULL
	};

enum
{
	arg_reg0 ,
	arg_regA,
	arg_script,
	arg_bin,
	arg_file
};

int _main(int args, const char **arg);
//char *binFile = NULL;

extern struct kittyBank fakeBank;

void readBIn( const char *binfile )
{
	FILE *fd;

	fd = fopen(binfile,"r");
	if (fd)
	{
		fseek(fd,0,SEEK_END);
		fakeBank.length = ftell(fd);
		fseek(fd,0,SEEK_SET);

		if (fakeBank.start)
		{
			free(	fakeBank.start );
			fakeBank.start = NULL;
		}

		fakeBank.start = (char *) malloc(fakeBank.length);

		if (fakeBank.start)
		{
			fread( fakeBank.start, fakeBank.length, 1, fd );
		}

		fclose(fd);
	}
}

int main(int args, const char **arg)
{
	int ret;

	fakeBank.start = NULL;

	ret =  _main(args,arg);

	if (fakeBank.start)
	{
		free(	fakeBank.start );
		fakeBank.start = NULL;
	}

	return ret;
}


int _main(int args, const char **arg)
{
	int n,nn,reg0,regA;
	int arg_type = arg_script;
	int RA;
	bool arg_type_found = false;

	struct kittyChannel  channel( 999 );
	const char *script = NULL;

	reg0=0;
	regA=0;

	for (n=1;n<args;n++)
	{
		arg_type_found = false;

		for (nn = 0; arg_names[nn]; nn++)
		{
			if (strcasecmp(arg[n],arg_names[nn])==0)
			{
				arg_type = nn;
				arg_type_found = true;
				break;
			}
		}

		if (arg_type_found == false)
		{
			printf("%s\n",arg_names[arg_type]);
			printf("%s\n",arg[n]);

			switch (arg_type)
			{
				case arg_script:	script = arg[n]; break;
				case arg_bin:		readBIn( arg[n] ); break;
				case arg_file:		file = arg[n]; break;
				case arg_reg0:		sscanf(arg[n],"%d",&channel.reg[reg0]); reg0++; break;
				case arg_regA:		sscanf(arg[n],"%d",&channel.reg[regA]); regA++; break;			
			}
		}
	}


	if (script == NULL)
	{
		printf("Bad input\n");
		return 0;
	}


	amalBuf *amalProg = &channel.amalProg;

	amalProg->call_array = NULL;
	amalProg->size = 0;
	amalProg->elements = 0;

	amiga_joystick_dir[0] = 7;

	if (script)
	{
		channel.amal_script = toAmosString_char( (char *) script, 0 );

		if (channel.amal_script)
		{
			int err;

			remove_lower_case(channel.amal_script);
			printf("amal script: '%s'\n",&(channel.amal_script -> ptr));

			err = asc_to_amal_tokens( &channel );

			if (err == 0)
			{
//				amalDiscompile( amalProg );
//				dump_object();
//				dump_amal_labels();

				if (amal_fix_labels( (void **) amalProg -> call_array ))
				{
					test_run( &channel );
				}
			}
			else
			{
				printf("basic error: %d\n",err);
			}

			dumpAmalRegs( &channel );
			amal_clean_up_labels( );
		}
	}

	return 0;
}
#endif

