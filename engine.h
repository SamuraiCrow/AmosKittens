
extern bool start_engine();
extern void wait_engine();

extern void engine_lock();
extern void engine_unlock();
extern void set_default_colors( struct retroScreen *screen );
extern void clear_cursor( struct retroScreen *screen );
extern void draw_cursor( struct retroScreen *screen );
extern void atomic_add_to_keyboard_queue( ULONG Code, ULONG Qualifier, char Char );

extern bool engine_wait_key;
extern int engine_mouse_key;
extern int engine_mouse_x;
extern int engine_mouse_y;
extern bool engine_stopped;

extern struct retroVideo *video;
extern struct retroScreen *screens[8] ;

#ifdef __amigaos4__
extern Process *EngineTask;
#endif

struct keyboard_buffer
{
	ULONG Code;
	ULONG Qualifier;
	char	Char;
};

struct amos_selected
{
	ULONG menu;
	ULONG item;
	ULONG sub;
};

struct amosMenuItem
{
	int levels;
	int index[3];
	char *str;
};

