
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "AmalCompiler.h"
#include "channel.h"
#include "AmalCommands.h"
#include "amal_object.h"
#include "amoskittens.h"

extern void pushBackAmalCmd( amal::Flags flags, struct kittyChannel *channel, void *cmd ) ;
extern int amreg[26];
extern void dumpAmalRegs();

void *amalFlushParaCmds( struct kittyChannel *self );
void *amalFlushAllCmds( struct kittyChannel *self );

#ifdef test_app
	#define amal_mouse_x 1000
	#define amal_mouse_y 2000
	#define AmalPrintf Printf
#else
	extern int engine_mouse_x;
	extern int engine_mouse_y;

	#define amal_mouse_x engine_mouse_x
	#define amal_mouse_y engine_mouse_y
	#define AmalPrintf Printf
	//define AmalPrintf(fmt,...)
#endif

void *amal_call_pause API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> status = channel_status::paused;
	self -> loopCount = 0;
	return NULL;
}

void *amal_call_j0 API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount ] = amiga_joystick_dir[0] | (amiga_joystick_button[0] << 4);
	amalFlushParaCmds( self );
	return NULL;
}

void *amal_call_j1 API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount ] = amiga_joystick_dir[1] | (amiga_joystick_button[1] << 4);
	amalFlushParaCmds( self );
	return NULL;
}

void *amal_call_screen_x API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_screen_y API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_set_num API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount ] = (int) code[1];

	amalFlushParaCmds( self );

	return code+1;
}

void *amal_call_x API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> last_reg = 5;
	self -> argStack [ self -> argStackCount ] = self -> objectAPI -> getX( self -> number );
	return NULL;
}

void *amal_call_y API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> last_reg = 6;
	self -> argStack [ self -> argStackCount ] = self -> objectAPI -> getY( self -> number );
	return NULL;
}

void *amal_call_reg API_AMAL_CALL_ARGS
{
	unsigned char c;
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	c = (unsigned char) (int) code[1];
	self -> last_reg = c;

	if ((c>='0')&&(c<='9'))
	{
		self -> argStack [ self -> argStackCount ] = self -> reg[c-'0'];
	} else if ((c>='A')&&(c<='Z'))
	{
		self -> argStack [ self -> argStackCount ] = amreg[ c - 'A'];
	}

	amalFlushParaCmds( self );

	return code+1;
}

void *amal_call_on API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_direct API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_wait API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *callback_amal_if  (struct kittyChannel *self, struct amalCallBack *cb)
{
	unsigned char c = self -> last_reg;
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	return NULL;
}	

void *amal_call_if API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	pushBackAmalCmd( amal::flag_para ,code, self, callback_amal_if ); 
	return code+1;
}

void *amal_call_jump API_AMAL_CALL_ARGS
{
	void **ret;

	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ret = (void **) code[1];
	if (ret)
	{
		if (self -> loopCount>9)
		{
			self -> status = channel_status::paused;
			self -> loopCount = 0;
			return ret-1;
		}
		else
		{
			self -> loopCount++;
			return ret-1;
		}
	}

	return code+1;
}

void *amal_call_exit API_AMAL_CALL_ARGS
{
	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_let API_AMAL_CALL_ARGS
{
	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_autotest API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *callback_move  (struct kittyChannel *self, struct amalCallBack *cb)
{
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 3)
	{
		if (( self -> move_count == 0)&&( self -> move_count_to == 0))
		{
			self -> move_from_x = self -> objectAPI -> getX( self -> number );
			self -> move_from_y = self -> objectAPI -> getY( self -> number );

			self -> move_delta_x = self -> argStack [ self -> argStackCount - 2 ];
			self -> move_delta_y = self -> argStack [ self -> argStackCount - 1 ];
			self -> move_count = 0;
			self -> move_count_to = self -> argStack [self -> argStackCount ];

			Printf("after read: self -> move_count = %ld, self -> move_count_to = %ld\n",self -> move_count , self -> move_count_to);

			// reset stack
			self -> argStackCount = cb -> argStackCount;
			return cb -> code - 1;
		}
		else	if (self -> move_count < self -> move_count_to)
		{
			int dxp,dyp;
			self -> move_count ++;

			dxp = self -> move_delta_x * self -> move_count / self -> move_count_to;
			dyp = self -> move_delta_y * self -> move_count / self -> move_count_to;
			self -> objectAPI -> setX( self -> number, self -> move_from_x + dxp );
			self -> objectAPI -> setY( self -> number, self -> move_from_y + dyp );

			self -> status = channel_status::paused;

			// reset stack
			self -> argStackCount = cb -> argStackCount;
			return cb -> code - 1;
		}
	}

	self -> move_count = 0;
	self -> move_count_to = 0;

	// reset stack
	self -> argStackCount = cb -> argStackCount;

	return NULL;
}	

void *amal_call_move API_AMAL_CALL_ARGS
{
	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	pushBackAmalCmd( amal::flag_cmd, code, self, callback_move ); 
	self -> argStack [ self -> argStackCount ] = 0;	// 
	return NULL;
}

static void *add (struct kittyChannel *self, struct amalCallBack *cb)
{
	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] + self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}
	return NULL;
}

void *amal_call_add API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, add ); 
	return NULL;
}

static void *sub (struct kittyChannel *self, struct amalCallBack *cb)
{
	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] - self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}
	return NULL;
}

void *amal_call_sub API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, sub ); 
	return NULL;
}

static void *mul (struct kittyChannel *self, struct amalCallBack *cb)
{
//	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] * self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}
	return NULL;
}

void *amal_call_mul API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, mul ); 
	return NULL;
}

static void *div (struct kittyChannel *self, struct amalCallBack *cb)
{
	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] / self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}
	return NULL;
}

void *amal_call_div API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, div ); 
	return NULL;
}

void *amal_call_and API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *not_equal (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] > self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}

	return NULL;
}

void *amal_call_not_equal API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, not_equal ); 
	return NULL;
}

void *less (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] > self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}

	return NULL;
}

void *amal_call_less API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, less ); 
	return NULL;
}

void *more (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] > self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}

	return NULL;
}

void *amal_call_more API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_cmd, code, self, more ); 
	return NULL;
}

void *less_or_equal  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] <= self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}

	return NULL;
}

void *amal_call_less_or_equal API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStackCount  ++;
	self -> argStack [ self -> argStackCount ] = 0;	// 
	pushBackAmalCmd( amal::flag_cmd, code, self, less_or_equal ); 
	return NULL;
}

void *more_or_equal  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] >= self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}

	return NULL;
}

void *amal_call_more_or_equal API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStackCount  ++;
	self -> argStack [ self -> argStackCount ] = 0;
	pushBackAmalCmd( amal::flag_cmd, code, self, more_or_equal ); 
	return NULL;
}

void *amal_call_equal API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount ] = 0;	
	return NULL;
}

void *amal_call_play API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_end API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_xm API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount  ] = amal_mouse_x;
	return NULL;
}

void *amal_call_ym API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount  ] = amal_mouse_y;	
	return NULL;
}

void *amal_call_k1 API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_k2 API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *callback_z  (struct kittyChannel *self, struct amalCallBack *cb)
{
	unsigned char c = self -> last_reg;
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	Printf("callback %ld\n",self -> argStack [ self -> argStackCount ]);

	self -> argStack [ self -> argStackCount ] = rand() % (self -> argStack [ self -> argStackCount ]+1);

	return NULL;
}

void *amal_call_z API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	pushBackAmalCmd( amal::flag_para ,code, self, callback_z ); 
	return NULL;
}

void *amal_call_xh API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_yh API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_sx API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_sy API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_bobCol API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_spriteCol API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_col API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_vumeter API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amalFlushParaCmds( struct kittyChannel *self )
{
	void *ret;

	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	while (self -> progStackCount)
	{
		struct amalCallBack *cb;
		cb = &self -> progStack[ self -> progStackCount -1 ];

		Printf("cmd flag: %lx\n",cb -> Flags);

		if (cb -> Flags & amal::flag_para)
		{
			Printf("found a command to flush\n");
			self -> progStackCount --;
			ret =cb -> cmd( self, cb );
			if (ret) return ret;
		}
		else break;
	}
	return NULL;
}

void *amalFlushAllCmds( struct kittyChannel *self )
{
	void *ret;

	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	while (self -> progStackCount)
	{
		struct amalCallBack *cb;
		self -> progStackCount --;
		cb = &self -> progStack[ self -> progStackCount ];
		ret =cb -> cmd( self, cb );
		if (ret) return ret;
	}
	return NULL;
}

void *amal_call_next_cmd API_AMAL_CALL_ARGS
{
	void *ret;
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ret = amalFlushAllCmds( self );

	Printf("<next cmd>\n");
	getchar();

	return ret ? ret : NULL;
}

void *amal_call_parenthses_start API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_parenthses_end API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_end_label API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_nextArg API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	amalFlushParaCmds( self );

	self -> argStackCount  ++;
	self -> argStack [ self -> argStackCount ] = 0;	// 

	return NULL;
}

void *amal_call_anim API_AMAL_CALL_ARGS
{
	int le;
	char *animCode ;
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	le = (int) code[1];
	animCode = (char *) &code[2];

#ifdef test_app

	Printf("le %ld\n",le);
	Printf("str: %s\n", animCode);

#else 
	setChannelAnim( self, strdup(  animCode ) );
#endif

	return code+1+le;	// 
}

void *while_status  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	printf("args: %d\n", self -> argStackCount - cb -> argStackCount + 1 );
	dumpAmalStack( self );

	if ( ! self -> argStack [ self -> argStackCount ] )
	{
		char *start_location = (char *) self -> amalProg.call_array;
		int offset_location ;
		void **code = cb -> code;

		offset_location = (int) code[1];

		printf("code at %08x\n",code);
		printf("%08x\n",code[0]);
		printf("%08x\n",code[1]);

		return start_location + offset_location + 4;
	}

	return NULL;
}

void *amal_call_while API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount ] = 0;	// set default value. 
	pushBackAmalCmd( amal::flag_cmd ,code, self, while_status ); 
	printf("ptr %08x\n",code[1]);
	return code +1;
}

void *amal_call_wend API_AMAL_CALL_ARGS
{
	char *location;
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	printf("ptr %08x ------ \n", ((void **) code[1]) );
	
	location = (char *) self -> amalProg.call_array;
	location += (int) code[1];

	self -> status = channel_status::paused;

	return location - sizeof(void *) ;
}

void *set_reg (struct kittyChannel *self, struct amalCallBack *cb)
{
	unsigned int c;
	char chr[2] = {0,0};

	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	c = cb-> last_reg;

	if (c==5)
	{
		self -> objectAPI -> setX( self -> number, self -> argStack [ self -> argStackCount ] );
	}
	else if (c==6)
	{
		self -> objectAPI -> setY( self -> number, self -> argStack [ self -> argStackCount ] );
	}
	if ((c>='0')&&(c<='9'))
	{
		self -> reg[ c - '0' ] = self -> argStack [ self -> argStackCount ];
		chr[0] = c;
		Printf("R%s=%ld\n", chr, self -> reg[ c - '0' ]);
	}
	else if ((c>='A')&&(c<='Z'))
	{
		amreg[ c - 'A' ] = self -> argStack [ self -> argStackCount ];
		chr[0]=c;
		Printf("R%s=%ld\n",chr, amreg[ c - 'A' ]);
	}

	return NULL;
}

void *amal_call_set API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	self -> argStack [ self -> argStackCount ] = 0;	// set default value. 
	pushBackAmalCmd( amal::flag_para ,code, self, set_reg ); 
	return NULL;
}


void *callback_inc  (struct kittyChannel *self, struct amalCallBack *cb)
{
	unsigned char c = self -> last_reg;
	char chr[2] = {c,0};
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if ((c>='0')&&(c<='9'))
	{
		self -> reg[ c - '0' ] ++;
		Printf("R%c=%ld\n",c, self -> reg[ c - '0' ]);
	}
	else 	if ((c>='A')&&(c<='Z'))
	{
		amreg[ c - 'A' ] ++;
		chr[0]=c;
		Printf("R%c=%ld\n",c, amreg[ c - 'A' ]);
	}

	return NULL;
}	

void *amal_call_inc API_AMAL_CALL_ARGS
{
	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.
	AmalPrintf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	pushBackAmalCmd( amal::flag_para ,code, self, callback_inc ); 
	return NULL;
}


