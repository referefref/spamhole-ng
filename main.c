#include "spamhole.h"

int main( int argc, char *argv[] ) {
	pid_t pid;

	/* Initialize our operations */
	if( init(argv[0]) < 0 ) {
		printf( "Initialization error, exiting...\n" );
		exit(-1);
	}

	/* Daemonize to background */
	pid=fork();
	if (pid<0) {
		perror("fork");
		exit(-1);
	}
	else if (pid==0) spamhole();

	return 0;
}
