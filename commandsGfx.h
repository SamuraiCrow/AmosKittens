#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"


char *gfxScreenOpen(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxLowres(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxHires(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenDisplay(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxColour(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxMouseKey(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxWaitVbl(struct nativeCommand *cmd, char *tokenBuffer);

char *gfxBox(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxBar(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxDraw(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxCircle(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxEllipse(struct nativeCommand *cmd, char *tokenBuffer);

char *gfxInk(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxXMouse(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxYMouse(struct nativeCommand *cmd, char *tokenBuffer);

