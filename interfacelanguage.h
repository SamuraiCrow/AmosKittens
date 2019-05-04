
struct ivar
{
	int type;
	int num;
	char *str;
};

struct dialog
{
	int x;
	int y;
	int width;
	int height;
};

struct ibutton
{
	int x;
	int y;
	int w;
	int h;
};

struct cmdcontext
{
	int id;
	char *tokenBuffer;
	bool tested;
	int stackp;
	int lstackp;
	struct ivar stack[20];
	struct ivar *vars;
	char *labels[512];
	int programStackCount;
	char *programStack[10];
	int selected_dialog;
	struct dialog dialog[2];
	void (*cmd_done)( struct cmdcontext *context, struct cmdinterface *self );
	int args;
	int error;
	char *script;
	char *at;
	int l;
	int ink0;
	int ink1;
	int ink3;
	int image_offset;
	int block_level;
	void (**block_fn)( struct cmdcontext *context, struct cmdinterface *self );
	int max_vars;
	int button_active;
	int xgcl;
	int ygcl;
	int xgc;
	int ygc;
	bool has_return_value;
	int return_value;
	bool mouse_key;
	bool button_action;
};

struct cmdinterface
{
	const char *name;
	int type;
	void (*pass)( struct cmdcontext *context, struct cmdinterface *self );
	void (*cmd)( struct cmdcontext *context, struct cmdinterface *self );
};

enum
{
	i_normal,
	i_parm
};


extern void isetvarstr( struct cmdcontext *context,int index, char *str);
extern void isetvarnum( struct cmdcontext *context,int index,int num);
extern void init_interface_context( struct cmdcontext *context, int id, char *script, int x, int y, int varSize, int bufferSize  );
extern void cleanup_inerface_context( struct cmdcontext *context );
extern void execute_interface_script( struct cmdcontext *context, int32_t label);

