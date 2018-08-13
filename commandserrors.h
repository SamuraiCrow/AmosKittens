#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

extern char *(*onError)(char *ptr);

extern char *onErrorBreak(char *ptr);
extern char *onErrorGoto(char *ptr);
extern char *onErrorProc(char *ptr);

extern char *cmdError(nativeCommand *cmd, char *ptr);
extern char *cmdOnError(nativeCommand *cmd, char *ptr);
extern char *cmdResumeLabel(nativeCommand *cmd, char *ptr);
extern char *cmdResumeNext(nativeCommand *cmd, char *ptr);

