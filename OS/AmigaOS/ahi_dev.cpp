

#include <stdafx.h>

#ifdef _MSC_VER
#include <stdint.h>
typedef uint32_t LONG;
#endif

#ifdef __amigaos__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ahi.h>
#include <dos/dostags.h>
#include <dos/dos.h>
#endif


#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ahi_dev.h"

#ifdef __amoskittens__
#include "spawn.h"
#include "debug.h"
#endif

#include "../../AmosKittens.h"


int current_audio_channel = 0;

extern bool running;


struct audioIO
{
	struct AHIRequest *io;
	struct audioChunk *data;
};

struct contextChannel {
	int device ;
	int channel ;
	bool audio_stopped;
 	struct MsgPort *AHIMsgPort;     // The msg port we will use for the ahi.device
 	struct audioIO *AHIio; 		// First AHIRequest structure
 	struct audioIO *AHIio2;   		// second one. Double buffering !
 	struct audioIO *AHIio_orig;   	// First AHIRequest structure
 	struct audioIO *AHIio2_orig;    	// second one. Double buffering !
 	struct audioIO *link;
};


struct audioIO *new_audio( struct AHIRequest *io)
{
	struct audioIO *_new = (struct audioIO *) malloc( sizeof( struct audioIO *) );
	if (_new)
	{
		_new -> io = io;
		_new -> data = NULL;
	}
	return _new;
}

std::vector<struct audioChunk *> audioBuffer[4];


LONG volume=0x10000;

static struct Process *main_task = NULL;
static struct Process *audioTask[4] = { NULL, NULL, NULL, NULL };

#define audioTask_NAME       "Amos Kittens Audio Thread"
#define audioTask_PRIORITY   2

int ahi_dev_init(int rate,int channels,int format,int flags);
void ahi_dev_uninit(int immed);


#ifdef __amoskittens__

static void cleanup_task(struct contextChannel *c, struct AHIIFace	*IAHI)
{
	while ( audioBuffer[ c -> channel ].size() > 0 )
	{
		Printf("clearing buffer\n");
		free( audioBuffer[c -> channel ][0] );
		audioBuffer[ c -> channel ].erase( audioBuffer[c -> channel ].begin() );
	}

	if ( c -> device == 0 )
	{
		if (c->link) if ( c-> link -> io)
		{
			AbortIO( (struct IORequest *) c->link -> io);
			WaitIO( (struct IORequest *) c->link -> io);
		}

		if (c->AHIio_orig) if (c->AHIio_orig -> io)
		{
			CloseDevice ( (struct IORequest *) c->AHIio_orig -> io);
		}
		if ( IAHI ) DropInterface((struct Interface*) IAHI); IAHI = 0;
	}

   	if (c->AHIio_orig)
	{
		Printf("Free AHIio\n");

		if (c->AHIio_orig -> io) FreeSysObject ( ASOT_IOREQUEST, (struct IORequest *) c->AHIio_orig -> io);
		free(c->AHIio_orig);
		c->AHIio_orig = NULL;
	}

	if (c->AHIio2_orig)
	{
		Printf("Free AHIio2\n");

		if (c->AHIio2_orig -> io) FreeVec(c->AHIio2_orig -> io);
		free(c->AHIio2_orig);
		c->AHIio2_orig = NULL;
	}

	if (c->AHIMsgPort) 
	{
		Printf("Free AHIMsgPort\n");

		FreeSysObject( ASOT_PORT, c->AHIMsgPort);
		c->AHIMsgPort = NULL;
	}

	c->link = NULL;
	c->AHIio = NULL;
	c->AHIio2 = NULL;
	c -> device = -1;       // -1 -> Not opened !
}


void audio_engine (void) {

	static struct AHIRequest *io;
	static struct audioIO *tempRequest;
	struct AHIIFace	*IAHI ;		// instence.
	struct contextChannel context;

	context.device = -999;
	context.channel = current_audio_channel;
	context.audio_stopped = false;

	context.AHIMsgPort  = NULL;     // The msg port we will use for the ahi.device
	context.AHIio       = NULL;     // First AHIRequest structure
 	context.AHIio2      = NULL;     // second one. Double buffering !
 	context.AHIio_orig       = NULL;     // First AHIRequest structure
	context.AHIio2_orig      = NULL;     // second one. Double buffering !
	context.link=NULL;

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	Printf("AHIDevice is %ld\n", context.device);

	if( context.AHIMsgPort = (struct MsgPort*) AllocSysObjectTags(ASOT_PORT, TAG_END) )
	{
		io = (struct AHIRequest *) AllocSysObjectTags ( 
					ASOT_IOREQUEST, 
					ASOIOR_ReplyPort, context.AHIMsgPort, 
					ASOIOR_Size, sizeof(struct AHIRequest), 
					TAG_END );

		if (io)
		{
			 if ( context.AHIio = new_audio( io ) ) 
			{
				context.AHIio->io->ahir_Version = 4;
				context.device = OpenDevice(AHINAME, AHI_DEFAULTUNIT,(struct IORequest *) context.AHIio -> io, 0);

				if (context.device == 0)
				{
					AHIBase = (struct Device *) context.AHIio->io->ahir_Std.io_Device;
					IAHI = (struct AHIIFace*) GetInterface( (struct Library *) AHIBase,"main",1L,NULL) ;
				}
				else
				{
					Printf("Open device error code %ld\n", context.device);
					goto end_audioTask;
				}
			}
			else 	goto end_audioTask;
		}
		else
		{
			FreeSysObject( ASOT_IOREQUEST, io );
			goto end_audioTask;
		}
	}
	else
	{
		Printf("Failed to create port %ld\n", context.device);
		goto end_audioTask;
	}

	context.AHIio2 = new_audio((struct AHIRequest*) AllocVecTags(sizeof(struct AHIRequest), AVT_Type, MEMF_SHARED, TAG_END )) ;

	if(! context.AHIio2)
	{
		goto end_audioTask;
	}
	else
	{
		Printf("Cool we got a copy..\n");
		memcpy(context.AHIio2 -> io, context.AHIio -> io, sizeof(struct AHIRequest));
	}

	context.AHIio_orig = context.AHIio;
	context.AHIio2_orig= context.AHIio2;

	Signal( (struct Task *) main_task, SIGF_CHILD );

	for(;;)
	{
		if ( running == false ) break;

		audio_lock();

		if ( audioBuffer[context.channel].size() == 0 )
		{
			audio_unlock(); 
			Delay(1);
			continue; 
		}    
		else if ( audioBuffer[context.channel][0])
		{
			if (context.AHIio-> data)	free( context.AHIio-> data );	// free old data.
			context.AHIio-> data = audioBuffer[context.channel][0];
			audioBuffer[context.channel].erase( audioBuffer[context.channel].begin());
			audio_unlock();

			Printf("AHIio %08lx, (io: %08lx,  frequency: %ld)\n", context.AHIio, context.AHIio -> io, context.AHIio->data -> frequency);

			if (io = context.AHIio -> io)
			{
				io->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
				io->ahir_Std.io_Command  = CMD_WRITE;
				io->ahir_Std.io_Offset   = 0;
				io->ahir_Std.io_Data     =  context.AHIio-> data -> ptr; 
				io->ahir_Std.io_Length   = (ULONG) context.AHIio-> data -> size; 
				io->ahir_Frequency       = (ULONG) context.AHIio-> data -> frequency;
				io->ahir_Volume          = volume; 
				io->ahir_Position        = (ULONG) context.AHIio-> data -> position;
				io->ahir_Type = AHIST_M8S;
				io->ahir_Link = context.link ? context.link -> io : NULL;
			}
		}
		else 	audio_unlock();

		Printf("SendIO( AHIio %08lx (io: %08lx) )\n", context.AHIio, context.AHIio -> io );

		SendIO( (struct IORequest *) context.AHIio -> io );

		if (context.link)
		{
			Printf("wait for link %08lx (io: %08lx, len %ld) \n", 
					context.link, 
					context.link -> io, 
					context.link -> io ? context.link -> io -> ahir_Std.io_Length : 0 );

			WaitIO ( (struct IORequest *) context.link -> io );

			Printf("check if main is done... \n");

			if (CheckIO( (struct IORequest *) context.AHIio -> io ))
			{
				// Playback caught up with us, rebuffer... 
				WaitIO( (struct IORequest *) context.AHIio -> io );
				context.link = NULL;
				continue;
			}
		}

		// Swap requests
		tempRequest = context.AHIio;
		context.AHIio  = context.AHIio2;
		context.AHIio2 = tempRequest;
		context.link = context.AHIio2;
	} 

end_audioTask:

	audio_lock();
	cleanup_task( &context, IAHI );
	Signal( (struct Task *) main_task, SIGF_CHILD );
	audio_unlock();
}

bool init_channel(int channel)
{
#ifdef enable_audio_debug_output_yes
	int x= channel % 2;
	int y = channel / 2;
	char temp[100];
	sprintf( temp, "CON:%d/%d/600/480/Kitty audio output %d",x*620+600,y*500+20, channel );	
	BPTR audio_debug_output = Open(temp,MODE_NEWFILE);
#else
	BPTR audio_debug_output = NULL;
#endif

	current_audio_channel = channel;
	main_task = (struct Process *) FindTask(NULL);
	audioTask[channel] = spawn( audio_engine, "Amos audio engine",audio_debug_output);

	if (audioTask[channel])
	{
		Wait(SIGF_CHILD);
		return audioTask[channel] ? true: false;
	}

	return false;
}


bool audio_start()
{
	init_channel(0);
	init_channel(1);
	init_channel(2);
	init_channel(3);
}

#endif

void makeChunk(uint8_t * data,int offset, int size, int totsize, int channel, int frequency, struct audioChunk **chunk )
{
	chunk[0] = (struct audioChunk *) malloc(sizeof(struct audioChunk));

	if (chunk)
	{
		chunk[0] -> size = size;
		chunk[0] -> frequency = frequency;

		switch (channel)
		{
			case 0: 		(*chunk) -> position = 0x00000;	break;
			case 1: 		(*chunk) -> position = 0x10000;	break;
			case 2: 		(*chunk) -> position = 0x10000;	break;
			case 3: 		(*chunk) -> position = 0x00000;	break;
		}


		memcpy( chunk[0] -> ptr, (void *) (data + offset),  chunk[0] -> size );
	}
}

void makeChunk_wave(struct wave *wave, int offset, int size, int totsize, int channel, struct audioChunk **chunk)
{
	//	int	totDuration = wave->envels[6].startDuration;

	int n;
	uint8_t *waveData = &wave->sample.ptr;
	int phase;
	int endDuration, deltaAmplitude;
	int d, w, reln, amplitude;

	*chunk = (struct audioChunk *) malloc(sizeof(struct audioChunk));

	if (*chunk)
	{
		(*chunk)->size = size;
		(*chunk)->frequency = wave->sample.frequency;

		switch (channel)
		{
			case 0: 		(*chunk)->position = 0x00000;	break;
			case 1: 		(*chunk)->position = 0x10000;	break;
			case 2: 		(*chunk)->position = 0x10000;	break;
			case 3: 		(*chunk)->position = 0x00000;	break;
		}

		phase = 0;
		endDuration = wave->envels[phase].startDuration + wave->envels[phase].duration;
		deltaAmplitude = wave->envels[phase + 1].volume - wave->envels[phase].volume;

		printf("(*chunk) =%08x\n", *chunk);
		printf("(*chunk)->size = %d\n", (*chunk)->size);
		printf("(*chunk)->frequency = %d\n", (*chunk)->frequency);
		
		for (n = offset; n < offset+size; n++)
		{
			while (n>endDuration*wave->sample.frequency)
			{
				phase++;
				if (phase == 6) break;
				endDuration = wave->envels[phase].startDuration + wave->envels[phase].duration;
				deltaAmplitude = wave->envels[phase + 1].volume - wave->envels[phase].volume;
			}

			d = n % wave->sample.bytes;
			w = ((int)waveData[d] - 127);
			reln = n - wave->envels[phase].startDuration*wave->sample.frequency;
			amplitude = (deltaAmplitude * reln / (wave->envels[phase].duration*wave->sample.frequency)) + wave->envels[phase].volume;
			w = w* amplitude / 64;

			(*chunk)->ptr[n - offset] = w;
		}
	}
}


bool play_wave(struct wave *wave, int len, int channel, int frequency)
{
	int blocks = len / AHI_CHUNKSIZE;
	int lastLen = len % AHI_CHUNKSIZE;
	struct audioChunk *chunk[4];
	int offset = 0;
	int c;

	if ((frequency <= 0) || (frequency > (44800 * 4)))
	{
		return false;	// avoid getting stuck...
	}


	while (blocks--)
	{
		for (c = 0; c<4; c++)
		{
			if (channel & (1 << c))
			{
				makeChunk_wave(wave, offset, AHI_CHUNKSIZE, len, channel, &chunk[c]);
				if (chunk[c]) draw_chunk(chunk[c]);
			}
			else chunk[c] = NULL;
		}

#ifdef __amoskittens__
		audio_lock();
		for ( c=0;c<4; c++)	if (chunk[c]) audioBuffer[c].push_back( chunk[c] );
		audio_unlock();
#endif
		offset += AHI_CHUNKSIZE;
	}


	if (lastLen)
	{
		for (c = 0; c<4; c++)
		{
			if (channel & (1 << c))
			{
				makeChunk_wave(wave, offset, lastLen, len, channel, &chunk[c]);
				if (chunk[c]) draw_chunk(chunk[c]);
			}
			else chunk[c] = NULL;
		}

#ifdef __amoskittens__
		audio_lock();
		for ( c=0;c<4; c++)	if (chunk[c]) audioBuffer[c].push_back( chunk[c] );
		audio_unlock();
#endif
	}

	return true;
}



#ifdef __amoskittens__

bool play(uint8_t * data,int len, int channel, int frequency)
{
	int blocks = len / AHI_CHUNKSIZE;
	int lastLen = len % AHI_CHUNKSIZE;
	struct audioChunk *chunk[4];
	int offset = 0;
	int c;

	if ((frequency <= 0) || (frequency > (44800*4)))
	{
		printf("bad frequency\n"); getchar();
		return false;	// avoid getting stuck...
	}

	while (blocks--)
	{
		for ( c=0;c<4; c++)
		{
			if (channel & (1<<c))
			{
				makeChunk( data , offset, AHI_CHUNKSIZE,  len, c, frequency, &chunk[c] );
			}
			else chunk[c] = NULL;
		}

		audio_lock();
		for ( c=0;c<4; c++)	if (chunk[c]) audioBuffer[c].push_back( chunk[c] );
		audio_unlock();
		offset += AHI_CHUNKSIZE;
	}

	if (lastLen)
	{
		for ( c=0;c<4; c++)
		{
			if (channel & (1<<c))
			{
				makeChunk( data , offset,  lastLen, len,  c,  frequency, &chunk[c] );
			}
			else chunk[c] = NULL;
		}

		audio_lock();
		for ( c=0;c<4; c++)	if (chunk[c]) audioBuffer[c].push_back( chunk[c] );
		audio_unlock();
	}

	return true;
}

#define debug_lock_no

#ifdef debug_lock
int audio_locked = 0;
#endif

void audio_lock()
{
#ifdef debug_lock
	if (audio_locked != 0)  Printf("***** audio is already locked *****\n");
#endif
	MutexObtain(audio_mx);
#ifdef debug_lock
	audio_locked ++;
#endif
}

void audio_unlock()
{
#ifdef debug_lock
	if (audio_locked == 0)  
	{
		Printf("***** audio is not locked, nothing to unlock *****\n");
	}
	else if (audio_locked > 0)  
	{
		Printf("**** unlocked with success ****\n");
	}
	else
	{
		Printf("**** unlock this is none sense  ****\n");
	}
	audio_locked--;
#endif 
	MutexRelease(audio_mx);
}

#endif

