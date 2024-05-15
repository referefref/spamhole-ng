#include "spamhole.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>

int spamhole() {
    int lsock, csock;
    FILE *cfile;
    int c;
    char mesg[4096];
    char rply[4096];
    char command[5];
    char dataterm[4];
    char *saddr;
    struct sockaddr_in laddr, caddr;
    socklen_t caddrlen = sizeof(caddr); // Correct type
    fd_set fdsr, fdse;
    int x, nbyt;
    int data = 0, mail = 0;

    /* Convert localport to unsigned short int, then to network byte order */
    laddr.sin_port = htons((unsigned short)(atol(LOCAL_PORT)));

    /* Open a TCP socket and assign it to lsock */
    if ((lsock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("socket");
        return 20;
    }

    /* Convert AF_INET into an unsigned short int */
    laddr.sin_family = AF_INET;
    if (!inet_aton(LOCAL_IP, &laddr.sin_addr))
        laddr.sin_addr.s_addr = INADDR_ANY;

    /* Bind our local address to our socket (lsock) */
    if (bind(lsock, (struct sockaddr*)&laddr, sizeof(laddr))) {
        perror("bind");
        if (DEBUG) syslog(LOG_ERR, "Error: could not bind to port, exiting.\n");
        return 20;
    }

    /* Set our socket to listen for new connections */
    if (listen(lsock, 1)) {
        perror("listen");
        if (DEBUG) syslog(LOG_ERR, "Error: could not listen on socket, exiting.\n");
        return 20;
    }

    /* Drop privileges to our UID */
    if (setuid(UID) != 0) {
        perror("setuid");
        if (DEBUG) syslog(LOG_ERR, "Error: could not switch to uid %d. exiting.\n", UID);
        return 20;
    }

    /* Reset the session group */
    setsid();

    /* Accept incoming connections in the queue and assign to csock */
    while ((csock = accept(lsock, (struct sockaddr*)&caddr, &caddrlen)) != -1) {
        /* Open a socket file descriptor for reading and writing */
        cfile = fdopen(csock, "r+");
        /* Fork connection to child, if fails, print error to socket and close it */
        if ((nbyt = fork()) == -1) {
            fprintf(cfile, "500 fork: %s\n", strerror(errno));
            shutdown(csock, 2);
            fclose(cfile);
            continue;
        }
        /* Child process from fork() */
        if (nbyt == 0) break;
        /* Parent process from fork() */
        fclose(cfile);
        /* Wait for any children to exit */
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    /* Make source addr human readable */
    saddr = inet_ntoa(caddr.sin_addr);

    /* Log incoming connection */
    if (DEBUG) syslog(LOG_INFO, "Received connection from %s\n", saddr);
    if (DEBUG > 2) syslog(LOG_DEBUG, "Session transcript for %s:\n", saddr);

    /* Send session intro */
    snprintf(rply, sizeof(rply), "220 ESMTP\n");
    write(csock, rply, strlen(rply));
    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);

    /* Interactive Session */
    while (1) {
        FD_ZERO(&fdsr);
        FD_ZERO(&fdse);
        FD_SET(csock, &fdsr);
        FD_SET(csock, &fdse);
        mesg[0] = '\0';

        /* Check for data to be read from the connection */
        if (select(20, &fdsr, NULL, &fdse, NULL) == -1) {
            fprintf(cfile, "500 select: %s\n", strerror(errno));
            fflush(cfile);
            shutdown(csock, 2);
            fclose(cfile);
            return 0;
        }

        /* Inbound Data */
        if (FD_ISSET(csock, &fdsr) || FD_ISSET(csock, &fdse)) {
            for (x = 0;; x++) {
                if (read(csock, &c, 1) <= 0) {
                    fflush(cfile);
                    shutdown(csock, 2);
                    fclose(cfile);
                    if (DEBUG) syslog(LOG_DEBUG, "Lost Connection for %s", saddr);
                    return 0;
                }
                if (x < sizeof(mesg) - 1) {
                    mesg[x] = c;
                    mesg[x] = tolower(mesg[x]);
                    if (mesg[x] == '\n') {
                        mesg[x + 1] = '\0';
                        break;
                    }
                } else {
                    if (c == '\n') {
                        mesg[sizeof(mesg) - 2] = '\r';
                        mesg[sizeof(mesg) - 1] = '\n';
                        mesg[sizeof(mesg)] = '\0';
                        break;
                    }
                }
            }
            if (DEBUG > 2) syslog(LOG_DEBUG, "rcvd: %s", mesg);

            /* Check to see if the read process read too many bytes */
            if (x > sizeof(mesg) - 1) {
                fprintf(cfile, "500 read: Command too long.\n");
                if (DEBUG > 2) syslog(LOG_DEBUG, "sent: 500 read: Command too long.\n");
                break;
            }

            /* SMTP Processing on message */
            if (data) {
                strncpy(dataterm, mesg, sizeof(dataterm) - 1);
                dataterm[sizeof(dataterm) - 1] = '\0';
                if (strcmp(dataterm, ".\r\n") == 0) {
                    data = 0;
                    snprintf(rply, sizeof(rply), "250 ok\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                }
            } else {
                strncpy(command, mesg, sizeof(command) - 1);
                command[sizeof(command) - 1] = '\0';
                if (DEBUG > 3) syslog(LOG_DEBUG, "command: [%s]", command);

                if (strcmp(command, "helo") == 0) {
                    snprintf(rply, sizeof(rply), "250\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                } else if (strcmp(command, "ehlo") == 0) {
                    snprintf(rply, sizeof(rply), "250-\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    snprintf(rply, sizeof(rply), "250-PIPELINING\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    snprintf(rply, sizeof(rply), "250 8BITMIME\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                } else if (strcmp(command, "mail") == 0) {
                    snprintf(rply, sizeof(rply), "250 ok\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    mail = 1;
                } else if (strcmp(command, "rcpt") == 0) {
                    if (mail) {
                        snprintf(rply, sizeof(rply), "250 ok\n");
                        write(csock, rply, strlen(rply));
                        if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    } else {
                        snprintf(rply, sizeof(rply), "503 MAIL first (#5.5.1)\n");
                        write(csock, rply, strlen(rply));
                        if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    }
                } else if (strcmp(command, "data") == 0) {
                    if (mail) {
                        snprintf(rply, sizeof(rply), "354 go ahead\n");
                        write(csock, rply, strlen(rply));
                        if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                        data = 1;
                    } else {
                        snprintf(rply, sizeof(rply), "503 MAIL first (#5.5.1)\n");
                        write(csock, rply, strlen(rply));
                        if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    }
                } else if (strcmp(command, "rset") == 0) {
                    snprintf(rply, sizeof(rply), "250 flushed\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    mail = 0;
                } else if (strcmp(command, "send") == 0) {
                    snprintf(rply, sizeof(rply), "250 ok\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    mail = 0;
                } else if (strcmp(command, "soml") == 0) {
                    snprintf(rply, sizeof(rply), "250 ok\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    mail = 0;
                } else if (strcmp(command, "saml") == 0) {
                    snprintf(rply, sizeof(rply), "250 ok\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    mail = 0;
                } else if (strcmp(command, "quit") == 0) {
                    snprintf(rply, sizeof(rply), "221\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                    if (DEBUG > 2) syslog(LOG_DEBUG, "Connection Closed for %s\n", saddr);
                    fflush(cfile);
                    shutdown(csock, 2);
                    fclose(cfile);
                    return 0;
                } else {
                    snprintf(rply, sizeof(rply), "502 unimplemented (#5.5.1)\n");
                    write(csock, rply, strlen(rply));
                    if (DEBUG > 2) syslog(LOG_DEBUG, "sent: %s", rply);
                }

            } /* End SMTP Processing */
        } /* End pending data if() */
    } /* End Interactive Session */
    return 0;
} /* End spamhole() */

