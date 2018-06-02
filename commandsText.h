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
extern char *textBorderStr(nativeCommand *cmd, char *ptr);
extern char *textPenStr(nativeCommand *cmd, char *ptr);
extern char *textPaperStr(nativeCommand *cmd, char *ptr);
extern char *textWriting(nativeCommand *cmd, char *ptr);
extern char *textShadeOff(nativeCommand *cmd, char *ptr);
extern char *textShadeOn(nativeCommand *cmd, char *ptr);
extern char *textUnderOff(nativeCommand *cmd, char *ptr);
extern char *textUnderOn(nativeCommand *cmd, char *ptr);
extern char *textAt(nativeCommand *cmd, char *ptr);
extern char *textXText(nativeCommand *cmd, char *ptr);
extern char *textYText(nativeCommand *cmd, char *ptr);
extern char *textCMove(nativeCommand *cmd, char *ptr);
extern char *textCUp(nativeCommand *cmd, char *ptr);
extern char *textCDown(nativeCommand *cmd, char *ptr);
extern char *textCLeft(nativeCommand *cmd, char *ptr);
extern char *textCRight(nativeCommand *cmd, char *ptr);
extern char *textSetTab(nativeCommand *cmd, char *ptr);
extern char *textSetCurs(nativeCommand *cmd, char *ptr);
extern char *textMemorizeX(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textMemorizeY(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textRememberX(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textRememberY(struct nativeCommand *cmd, char *tokenBuffer);

