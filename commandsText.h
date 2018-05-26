#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

extern char *textPrint(nativeCommand *cmd, char *ptr);
extern char *textLocate(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textPaper(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textPen(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textCentre(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textPrint(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textCursOn(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textCursOff(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textHome(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textInverseOn(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textInverseOff(struct nativeCommand *cmd, char *tokenBuffer);

