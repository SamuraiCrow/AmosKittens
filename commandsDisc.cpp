
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commandsDisc.h"
#include "debug.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;

bool pattern_match( char *name , const char *pattern )
{
	char *n;
	const char *p;

	if (pattern == NULL) return true;
	n = name;
	p = pattern;

	do
	{
		while (*p == '*') p++;
		while ((*n != *p)&&(*n)) n++;	// seek 

		while ((*n == *p)&&(*n)&&(*p))
		{
			n++; p++;
		}

		if ((*p != '*') && ( *p != 0)) p = pattern;	// reset pattern if not wild case.
	}
	while (*p == '*');

	return (*p == 0);	// if we are at end of patten then it match.
}

char *_cmdDir( struct glueCommands *data )
{
	char *_path = NULL;
	const char *_pattern = NULL;
	int i;
	int _len;
	char c;
	char *str;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	str = _stackString( stack );

	if (str == NULL) return NULL;

	_len = strlen( str );
	
	printf("%s:%d\n",__FUNCTION__,__LINE__);


	if (_len>0) 
	{
		c = str[_len-1];

		if ((c == '/') || ( c == ':'))
		{
			_path = strdup(str);
		}
		else
		{
			for (i=_len-1; i>=0;i--)
			{
				c = str[i];

				if ((c == '/') || ( c == ':'))
				{
					_path = strndup( str, i+1 );
					_pattern = str + i +1;
				}
			}
		}		
	}


	if (_path == NULL) return NULL;

	APTR context = ObtainDirContextTags(EX_StringNameInput, _path,
	                   EX_DoCurrentDir,TRUE, /* for ExamineObjectTags() */
	                   EX_DataFields,(EXF_NAME|EXF_LINK|EXF_TYPE), TAG_END);

	if( context )
	{
	    struct ExamineData *dat, *target;

	    while((dat = ExamineDir(context)))  /* until no more data.*/
	    {
	        if( EXD_IS_LINK(dat) ) /* all links, must check these first ! */
	        {
	            if( EXD_IS_SOFTLINK(dat) )        /* a FFS style softlink */
	            {
	                CONST_STRPTR target_type = "unavailable"; /* default  */
	                APTR oldwin = SetProcWindow((APTR)-1); 
	                target = ExamineObjectTags(EX_StringNameInput,
	                                                   dat->Name,TAG_END);
	                SetProcWindow(oldwin);

	                if( target )
	                {
	                    if( EXD_IS_FILE(target) )
	                    {
	                        target_type = "file";
	                    }
	                    if( EXD_IS_DIRECTORY(target) )
	                    {
	                        target_type = "dir";
	                    }
	                    FreeDosObject(DOS_EXAMINEDATA,target);
	                    /* Free target data when done */
	                }
	                Printf("softlink=%s points to %s and it's a %s\n",
	                                 dat->Name,dat->Link,target_type);
	            }
	            else if( EXD_IS_FILE(dat) )       /* hardlink file */
	            {
	                Printf("file hardlink=%s points to %s\n",
	                                 dat->Name, dat->Link);
	            }
	            else if( EXD_IS_DIRECTORY(dat) )  /* hardlink dir */
	            {
	                Printf("dir hardlink=%s points to %s\n",
	                                 dat->Name, dat->Link);
	            }
	        }
	        else if( EXD_IS_FILE(dat) )           /* a plain file */
	        {
			if (pattern_match( dat->Name, _pattern )) Printf("filename=%s\n", dat->Name);			
	        }
	        else if ( EXD_IS_DIRECTORY(dat) )     /* a plain directory */
	        {
	            Printf("dirname=%s\n",  dat->Name);
	        }
	    }
       
	    if( ERROR_NO_MORE_ENTRIES != IoErr() )
	    {
	        PrintFault(IoErr(),NULL); /* failure - why ? */
	    }
	}
	else
	{
	    PrintFault(IoErr(),NULL);     /* failure - why ? */
	}
	
	ReleaseDirContext(context);             /* NULL safe */

	popStack( stack - cmdTmp[cmdStack-1].stack  );

	return  data -> tokenBuffer ;
}

char *cmdDir(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdDir, tokenBuffer );

	return tokenBuffer;
}

char *dir_str(struct nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

