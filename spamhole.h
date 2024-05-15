/* Current Version */
#define VERSION "0.6"

/* Configuration Parameters */
#include "config.h"

/* Includes */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <syslog.h>
#include <libgen.h>

/* Function Prototypes */
extern int init( char *myname );
extern int spamhole();
