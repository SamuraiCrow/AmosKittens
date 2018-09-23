
struct kittyChannel;

#define API_AMAL_CALL_ARGS ( struct kittyChannel *self, void **code, unsigned int opt )

struct amalBuf
{
	unsigned int elements;
	unsigned int size;
	void *(**call_array) API_AMAL_CALL_ARGS;
};

struct amalWriterData
{
	const char *at_script;
	unsigned int command_len;
	unsigned int arg_len;
};

struct amalTab
{
	const char *name;

	unsigned int (*write) (
				struct kittyChannel *channel,
				struct amalTab *self, 
				void *(**call_array) API_AMAL_CALL_ARGS, 
				struct amalWriterData *data,
				unsigned int
				);

	void *(*call) API_AMAL_CALL_ARGS;
};

extern void pushBackAmalCmd( void **code, struct kittyChannel *channel, void *(*cmd)  (struct kittyChannel *self, struct amalCallBack *cb)  ) ;
extern void dumpAmalStack( struct kittyChannel *channel );
extern bool asc_to_amal_tokens( struct kittyChannel  *channel );
extern void amal_run_one_cycle( struct kittyChannel  *channel );

