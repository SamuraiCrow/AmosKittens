
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *cmdDir(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdDirStr(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdParent(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdSetDir(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdDfree(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdKill(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdRename(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdFselStr(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdExist(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdDirFirstStr(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdDirNextStr(struct nativeCommand *cmd, char *tokenBuffer);


