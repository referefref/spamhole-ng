#include "spamhole.h"

int init( char *myname ) {

	/* Open our syslog connection */
	openlog( basename(myname), LOG_PID, LOG_MAIL );

	/* Version Info */
	printf( "spamhole reference implementation %s by I)ruid [CAU]\n", VERSION );
	syslog( LOG_INFO, "spamhole reference implementation %s by I)ruid [CAU] started...\n", VERSION );

	return 0;
}
