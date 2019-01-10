
#ifdef _MSC_VER
#include "stdafx.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#if defined(__amigaos4__) || defined(__amigaos__)
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif
#include "commandsScreens.h"

extern struct retroScreen *screens[8] ;

#ifndef test_app

int XScreen_formula( struct retroScreen *screen, int x )
{
	x = x - screen -> scanline_x 	- screen -> offset_x;
	if ( (screen -> videomode & retroHires) == 0 ) x /= 2;
	return x;
}

int YScreen_formula( struct retroScreen *screen, int y )
{
	y = y - screen -> scanline_y - screen -> offset_y;
	if ( (screen -> videomode & retroInterlaced) == 0 ) y /= 2;
	return y;
}

int XHard_formula( struct retroScreen *screen, int x )
{
	if ( (screen -> videomode & retroHires) == 0 ) x *= 2;
	x = x + screen -> scanline_x 	+ screen -> offset_x;
	return x;
}

int YHard_formula( struct retroScreen *screen, int y )
{
	if ( (screen -> videomode & retroInterlaced) == 0 ) y *= 2;
	y = y + screen -> scanline_y + screen -> offset_y;
	return y;
}

#else

struct retroScreen
{
	void *ptr;
};


int XScreen_formula(struct retroScreen *screen, int x)
{
	return x;
}

int YScreen_formula(struct retroScreen *screen, int y)
{
	return y;
}

int XHard_formula(struct retroScreen *screen, int x)
{
	return x;
}

int YHard_formula(struct retroScreen *screen, int y)
{
	return y;
}


#endif