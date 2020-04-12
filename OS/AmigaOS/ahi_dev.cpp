
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
#include <proto/retroMode.h>
#endif

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "kittyaudio.h"

#ifdef __amoskittens__
#include "spawn.h"
#include "debug.h"
#endif

#include <AmosKittens.h>

int current_audio_channel = 0;

extern bool running;

struct audioIO
{
	bool sendt;
	struct AHIRequest *io;
	struct audioChunk *chunk;
};

enum 
{
	audio_not_ready,
	audio_ready,
	audio_closed,
	audio_failed
};

struct contextChannel {
	int device ;
	int channel ;
	bool audio_abort;
	bool audio_status;
 	struct MsgPort *AHIMsgPort;     // The msg port we will use for the ahi.device
 	struct audioIO *AHIio; 		// First AHIRequest structure
 	struct audioIO *AHIio2;   		// second one. Double buffering !
 	struct audioIO *AHIio_orig;   	// First AHIRequest structure
 	struct audioIO *AHIio2_orig;    	// second one. Double buffering !
 	struct audioIO *link;
};


struct contextChannel contexts[4];

struct audioIO *new_audio( struct AHIRequest *io)
{
	struct audioIO *_new = (struct audioIO *) malloc( sizeof( struct audioIO ) );
	if (_new)
	{
		_new -> sendt = false;
		_new -> io = io;
		_new -> chunk = NULL;
	}
	return _new;
}

std::vector<struct audioChunk *> audioBuffer[4];

#define AHI_CHUNKSIZE 1024
#define AHI_DEFAULTUNIT 0

static struct Process *main_task = NULL;
static struct Process *audioTask[4] = { NULL, NULL, NULL, NULL };

#define audioTask_NAME       "Amos Kittens Audio Thread"
#define audioTask_PRIORITY   2

int ahi_dev_init(int rate,int channels,int format,int flags);
void ahi_dev_uninit(int immed);


#ifdef __amoskittens__

void abort_audio_channel(struct contextChannel *context, struct AHIIFace	*IAHI)
{
	if (context -> link)
	{
		if (context -> link -> sendt)
		{
			 if ( context-> link -> io)
			{
				AbortIO ( (struct IORequest *) context -> link -> io );
				WaitIO( (struct IORequest *) context->link -> io);
			}

			context -> link -> sendt = false;
			context -> link = NULL;
		}
	}

	if (context -> AHIio) 
	{
		if (context -> AHIio -> sendt)
		{
			if (context -> AHIio -> io)
			{
				AbortIO( (struct IORequest *) context -> AHIio -> io );
				WaitIO( (struct IORequest *) context -> AHIio -> io);
			}
			context -> AHIio -> sendt = false;
		}
	}
}

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


void setup_lowpass( double samplerate, double cutFreq,  struct audioChunk *chunk )
{
	double dt = 1.0f / samplerate;
	double rc = 1.0f / (2 * M_PI * cutFreq );
	chunk -> lowpass_alpha = dt / (rc + dt );
	chunk -> lowpass_n_alpha = 1.0f - chunk -> lowpass_alpha;
}

#define lowpass( value ) _new_value_ = lowpass_alpha * (value) + ( lowpass_n_alpha * last_amplitude ); last_amplitude = value; value = _new_value_;	
	


void audio_engine (void) 
{
	static struct AHIRequest *io;
	static struct audioIO *tempRequest;
	struct AHIIFace	*IAHI = NULL;		// instence.
	struct contextChannel *context;
	struct Process *thisProcess;
	int this_channel = -1;
	int c;
	double last_amplitude = 0.0f;

	thisProcess = (struct Process *) FindTask(NULL);

	do
	{
		for (c=0;c<4;c++)
		{
			Printf("channel %ld %08lx == %08lx\n", c, audioTask[c], thisProcess );

			if (audioTask[c]==thisProcess)
			{
				this_channel = c;
				break;
			}
		}
		Delay(1);
	} while ( this_channel == -1 );


	context = &contexts[this_channel];

	context -> audio_status = audio_not_ready;
	context -> device = -999;
	context -> channel = this_channel;
	context -> audio_abort = false;
	context -> AHIMsgPort  = NULL;     // The msg port we will use for the ahi.device
	context -> AHIio       = NULL;     // First AHIRequest structure
 	context -> AHIio2      = NULL;     // second one. Double buffering !
 	context -> AHIio_orig       = NULL;     // First AHIRequest structure
	context -> AHIio2_orig      = NULL;     // second one. Double buffering !
	context -> link=NULL;

	SetTaskPri( FindTask(0),5);

	if( context -> AHIMsgPort = (struct MsgPort*) AllocSysObjectTags(ASOT_PORT, TAG_END) )
	{
		io = (struct AHIRequest *) AllocSysObjectTags ( 
					ASOT_IOREQUEST, 
					ASOIOR_ReplyPort, context -> AHIMsgPort, 
					ASOIOR_Size, sizeof(struct AHIRequest), 
					TAG_END );

		if (io)
		{
			 if ( context -> AHIio = new_audio( io ) ) 
			{
				context -> AHIio->io->ahir_Version = 4;

				context -> device = OpenDevice( 
					AHINAME, 
					AHI_DEFAULTUNIT, 
					(struct IORequest *) context -> AHIio -> io, 
					0 );

				if (context -> device == 0)
				{
					AHIBase = (struct Device *) context -> AHIio->io->ahir_Std.io_Device;
					IAHI = (struct AHIIFace*) GetInterface( (struct Library *) AHIBase,"main",1L,NULL) ;
				}
				else
				{
					Printf("Open device error code %ld\n", context -> device);
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
		Printf("Failed to create port %ld\n", context -> device);
		goto end_audioTask;
	}

	context -> AHIio2 = new_audio((struct AHIRequest*) AllocVecTags( sizeof(struct AHIRequest), AVT_Type, MEMF_SHARED, TAG_END )) ;

	if(! context -> AHIio2)
	{
		goto end_audioTask;
	}
	else
	{
		Printf("Cool we got a copy..\n");
		memcpy(context -> AHIio2 -> io, context -> AHIio -> io, sizeof(struct AHIRequest));
	}

	context -> AHIio_orig = context -> AHIio;
	context -> AHIio2_orig= context -> AHIio2;

	Printf("send SIGF_CHILD here\n");
	Signal( (struct Task *) main_task, SIGF_CHILD );

	context -> audio_status = audio_ready;

	for(;;)
	{
		Printf("for(;;)\n");

		if ( running == false ) break;

		Printf("channel_lock(%ld)\n",context -> channel);
		channel_lock(context -> channel);

		Printf("context -> audio_abort %ld\n",context -> audio_abort);

		if ( context -> audio_abort == true )
		{
			Printf("Audio aborted\n");
			abort_audio_channel( context, IAHI );
			Printf("channel_unlock(%ld)\n",context -> channel);
			channel_unlock(context -> channel); 
			Printf("abort done\n");
			context -> audio_abort = false;
			continue; 
		}

		if ( audioBuffer[context -> channel].size() == 0 )
		{
			channel_unlock(context -> channel); 
			Printf("wait for buffer %ld\n",context -> channel);
			Delay(1);
			continue; 
		}    
		else if ( audioBuffer[context -> channel].front() )
		{
			if (sample_loop == false)
			{
				if (context -> AHIio-> chunk)
				{
					Printf("Free data..\n");
					free( context -> AHIio-> chunk );	// free old data.
					context -> AHIio-> chunk = NULL;
				}
			}
			else
			{
				Printf("sample loop push data back..\n");
				audioBuffer[context -> channel].push_back( context -> AHIio-> chunk);
			}			

			if (io = context -> AHIio -> io)
			{
				context -> AHIio-> chunk = audioBuffer[context -> channel].front();

				audioBuffer[context -> channel].erase( audioBuffer[context -> channel].begin());

				channel_unlock(context -> channel);

				if (context -> AHIio-> chunk)
				{
					struct audioChunk *chunk = context -> AHIio-> chunk;

					io->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
					io->ahir_Std.io_Command	= CMD_WRITE;
					io->ahir_Std.io_Offset	= 0;
					io->ahir_Std.io_Data		=  &(chunk -> ptr); 
					io->ahir_Std.io_Length	= (ULONG) chunk -> size; 
					io->ahir_Frequency		= (ULONG) chunk -> bytesPerSecond;
					io->ahir_Volume		=  instance.volume; 
					io->ahir_Position		= (ULONG) chunk -> position;
					io->ahir_Type			= AHIST_M8S;		// mono 8 bit signed
					io->ahir_Link			= context -> link ? context -> link -> io : NULL;

					if (instance.audio_3k3_lowpass)
					{
						char *ptr = (char *)  io->ahir_Std.io_Data;
						char *e_ptr = ptr + io->ahir_Std.io_Length;

						double _new_value_;
						double lowpass_alpha = chunk -> lowpass_alpha;
						double lowpass_n_alpha = chunk -> lowpass_n_alpha;

						for (;ptr<e_ptr;ptr++)
						{
							lowpass( *ptr );
						}
					}

				}
				else
				{
					Printf("has no data, wtf\n");
					running = false;
					continue; 
				}
			}
			else
			{
				channel_unlock(context -> channel);
				Printf("has no IO, wtf\n");
				running = false;
				continue; 
			}
		}
		else 	
		{
			channel_unlock(context -> channel);
			Printf("has buffer, but there is no data, WTF\n");
			running = false;
			continue; 
		}

		Printf("SendIO( AHIio %08lx (io: %08lx) ) -- buffer size: %ld\n", 
				context -> AHIio, 
				context -> AHIio -> io, 
				audioBuffer[context -> channel].size() );

		SendIO( (struct IORequest *) context->AHIio -> io );

		Printf("context -> AHIio -> sendt = true\n" );

		context -> AHIio -> sendt = true;

		if (context -> link)
		{
			if (context -> link -> sendt)
			{
			 	if (context -> link -> io)
				{
					WaitIO ( (struct IORequest *) context -> link -> io );
				}
			}
			context -> link -> sendt = false;
		}
	
		// Swap requests
		tempRequest = context -> AHIio;
		context -> AHIio  = context -> AHIio2;
		context -> AHIio2 = tempRequest;
		context -> link = context -> AHIio2;
	} 

end_audioTask:

	switch (context -> audio_status)
	{
		case audio_not_ready:
				context -> audio_status = audio_failed;
				break;

		case audio_ready:
				context -> audio_status = audio_closed;
				break;
	}

	audioLock();
	cleanup_task( context, IAHI );
	Signal( (struct Task *) main_task, SIGF_CHILD );
	audioUnlock();
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

	SetSignal(0L,SIGF_CHILD);	 // clear SIGF_CHILD 
	audioTask[channel] = spawn( audio_engine, "Amos audio engine",audio_debug_output);
	if (audioTask[channel])
	{
		Wait(SIGF_CHILD);
		return audioTask[channel] ? true: false;
	}

	return false;
}


bool audioStart()
{
	init_channel(0);
	init_channel(1);
	init_channel(2);
	init_channel(3);
}

#endif

void makeChunk(uint8_t * data,int offset, int size, int totsize, int channel, int bytesPerSecond, struct audioChunk **chunk )
{
	chunk[0] = (struct audioChunk *) malloc(sizeof(struct audioChunk) + size);

	if (chunk)
	{
		int8_t *s8bit;
		uint8_t *u8bit;
		uint8_t *u8bit_e;

		chunk[0] -> size = size;
		chunk[0] -> bytesPerSecond = bytesPerSecond;

		// Amiga lowpass fliter use 3,3 Khz, precalc mixing to speed up fliter.
		setup_lowpass(  bytesPerSecond, 3300.0f,  chunk[0] );

		switch (channel)
		{
			case 0: 		(*chunk) -> position = 0x00000;	break;
			case 1: 		(*chunk) -> position = 0x10000;	break;
			case 2: 		(*chunk) -> position = 0x10000;	break;
			case 3: 		(*chunk) -> position = 0x00000;	break;
		}

		memcpy( &(chunk[0] -> ptr), (void *) (data + offset),  chunk[0] -> size );
	}
}

uint32_t color =0;

struct phase 
{
	int phase;
	int endDuration;
	int deltaAmplitude;
	struct audioChunk *chunk;
};


void makeChunk_wave(struct wave *wave, struct phase *phaseContext, int offset, int size, int totsize, int channel)
{
	int n;
	signed char *data;
	signed char *chunk_data;
	int d, w, reln, amplitude;

	phaseContext -> endDuration = wave->envels[phaseContext -> phase].startDuration + wave->envels[phaseContext -> phase].duration;
	phaseContext -> deltaAmplitude = wave->envels[phaseContext -> phase + 1].volume - wave->envels[phaseContext -> phase].volume;

	if (offset>phaseContext -> endDuration*wave->bytesPerSecond)
	{
		if (phaseContext -> phase == 5) 
		{
			Printf("can't make chunk\n");
			phaseContext -> chunk = NULL;
			return;
		}
	}

	phaseContext -> chunk = (struct audioChunk *) malloc(sizeof(struct audioChunk) + size );
	if (phaseContext -> chunk)
	{
		phaseContext -> chunk -> size = size;
		phaseContext -> chunk -> bytesPerSecond = wave -> bytesPerSecond;

		// Amiga lowpass fliter use 2,2 Khz, precalc mixing to speed up fliter.
		setup_lowpass( wave -> bytesPerSecond, 2200.0f,  phaseContext -> chunk );

		switch (channel)
		{
			case 0: 		phaseContext -> chunk -> position = 0x00000;	break;
			case 1: 		phaseContext -> chunk -> position = 0x10000;	break;
			case 2: 		phaseContext -> chunk -> position = 0x10000;	break;
			case 3: 		phaseContext -> chunk -> position = 0x00000;	break;
		}

		data = (signed char*) &(wave -> sample.ptr);
		chunk_data = (signed char*) &( phaseContext -> chunk -> ptr);
		
		for (n = offset; n < offset+size; n++)
		{
			while (n>phaseContext -> endDuration*wave->bytesPerSecond)
			{
				if (phaseContext -> phase == 5) 
				{
					int remain = (offset+size)-n;
					if (remain>0) memset( chunk_data, 0, remain );
					break;
				}

				phaseContext -> phase++;
				phaseContext -> endDuration = wave->envels[phaseContext -> phase].startDuration + wave->envels[phaseContext -> phase].duration;
				phaseContext -> deltaAmplitude = wave->envels[phaseContext -> phase + 1].volume - wave->envels[phaseContext -> phase].volume;
			}

			d = n % wave->sample.bytes;
			w = (int) data[d] ;

			reln = n - wave->envels[phaseContext -> phase].startDuration*wave->bytesPerSecond;
			amplitude = (phaseContext -> deltaAmplitude * reln / (wave->envels[phaseContext -> phase].duration*wave->bytesPerSecond)) + wave->envels[phaseContext -> phase].volume;
			w = w* amplitude / 64;

			chunk_data[n-offset] = w;
		}
	}
}

bool audioPlayWave(struct wave *wave, int len, int channels)
{
	int blocks = len / AHI_CHUNKSIZE;
	int lastLen = len % AHI_CHUNKSIZE;
	struct phase phaseContexts[4];
	int offset = 0;
	int c;

	printf("-- blocks %d lastLen %d\n",blocks,lastLen);

	for (c=0;c<4;c++) phaseContexts[c].phase = 0;

	while (blocks--)
	{
		for (c = 0; c<4; c++)
		{
			if (channels & (1 << c))
			{
				makeChunk_wave(
					wave, 
					&phaseContexts[c], 
					offset, 
					AHI_CHUNKSIZE, 
					len, 
					channels );
			}
			else phaseContexts[c].chunk = NULL;
		}

#ifdef __amoskittens__
		audioLock();
		for ( c=0;c<4; c++)	if (phaseContexts[c].chunk) audioBuffer[c].push_back( phaseContexts[c].chunk );
		audioUnlock();
#endif
		offset += AHI_CHUNKSIZE;
	}

	if (lastLen)
	{
		for (c = 0; c<4; c++)
		{
			if (channels & (1 << c))
			{
				makeChunk_wave(
					wave, 
					&phaseContexts[c], 
					offset, 
					lastLen, 
					len, 
					channels);
			}
			else phaseContexts[c].chunk = NULL;
		}

#ifdef __amoskittens__
		audioLock();
		for ( c=0;c<4; c++)	if (phaseContexts[c].chunk) audioBuffer[c].push_back( phaseContexts[c].chunk );
		audioUnlock();
#endif
	}

	return true;
}

#ifdef __amoskittens__

void audio_channel_flush( int c )
{
	struct audioChunk *chunk;

	while (audioBuffer[c].size())
	{
		chunk = audioBuffer[c].back();
		if (chunk) free(chunk);
		audioBuffer[c].pop_back();
	}
}

void abort_channel( int c )
{
	// check if task is running.
	if (audioTask[c] == NULL)	return;

	// wait for status 
	while ( contexts[c].audio_status == audio_not_ready ) Delay(1);

	// check if audio is ready.
	if (contexts[c].audio_status != audio_ready) return;

	contexts[c].audio_abort = true;

	Printf("waiting for abort on channel %d\n",c);

	while (contexts[c].audio_abort == true)
	{
		printf("contexts[%d].audio_abort is %d\n",
			contexts[c].channel,
			contexts[c].audio_abort);	
		Delay(1);
	}
}

void audioDeviceFlush(int voices)
{
	int c;

	audioLock();
	for ( c=0;c<4; c++)	if (voices & (1<<c) ) audio_channel_flush( c );
	audioUnlock();

	for ( c=0;c<4; c++)	if (voices & (1<<c) ) abort_channel( c );
}

bool audioPlay(uint8_t * data,int len, int channel, int frequency)
{
	int blocks = len / AHI_CHUNKSIZE;
	int lastLen = len % AHI_CHUNKSIZE;
	struct audioChunk *chunk[4];
	int offset = 0;
	int c;

/*
	if ((frequency <= 0) || (frequency > (44800*4)))
	{
		printf("bad frequency\n"); getchar();
		return false;	// avoid getting stuck...
	}
*/
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


		for ( c=0;c<4; c++)	if (chunk[c])
		{
			channel_lock(c);
			audioBuffer[c].push_back( chunk[c] );
			channel_unlock(c);
		}

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

		for ( c=0;c<4; c++)	if (chunk[c]) 
		{
			channel_lock(c);
			audioBuffer[c].push_back( chunk[c] );
			channel_unlock(c);
		}
	}

	return true;
}


void channel_lock(int n)
{
	MutexObtain(channel_mx[n]);
}

void channel_unlock(int n)
{
	MutexRelease(channel_mx[n]);
}

void audioLock()
{
	channel_lock(0);
	channel_lock(1);
	channel_lock(2);
	channel_lock(3);
}

void audioUnlock()
{
	channel_unlock(0);
	channel_unlock(1);
	channel_unlock(2);
	channel_unlock(3);
}

#endif

