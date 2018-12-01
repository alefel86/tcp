#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simple_message_client_commandline_handling.h"

/* struct addrinfo {
               int              ai_flags;
               int              ai_family;
               int              ai_socktype;
               int              ai_protocol;
               socklen_t        ai_addrlen;
               struct sockaddr *ai_addr;
               char            *ai_canonname;
               struct addrinfo *ai_next;
           };
*/
void cli_error(FILE *, const char *, int);
void printUsage(void);

const char *program_name = NULL;


int main(int argc, const char *argv[])
{
	
    const char *server, *port, *user, *message, *img_url;
    int sfd, s, verbose;

    struct addrinfo hints;
    struct addrinfo *result, *rp;
	program_name = argv[0];

    smc_parsecommandline(argc, argv, cli_error, &server, &port, &user, &message, &img_url, &verbose);
	if (verbose)
		printf("Using the following options: server=%s, port=%s, user=%s, message=%s, image=%s\n", server, port, user, message, img_url);
	
	if (argc < 4) 
	{
        printUsage();
        exit(EXIT_FAILURE);
    }
	
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    s = getaddrinfo(NULL, port, &hints, &result);
    if (s)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; /* Success */

        close(sfd);
    }
    freeaddrinfo(result);

    if (rp == NULL)
    { /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Verbunden \n");
    }

    FILE *send_socket = fdopen(sfd, "w");
    if (img_url != NULL)
        fprintf(send_socket, "user=%s\nimg=%s\n%s", user, img_url, message);
    else
        fprintf(send_socket, "user=%s\n%s", user, message);
    fflush(send_socket);
    int temp_sfd=dup(sfd);

    shutdown(temp_sfd, SHUT_WR);
    FILE *rcv_socket = fdopen(sfd, "r");

    char rcv[50];
    if (fgets(rcv, 50, rcv_socket) != NULL)
    {
        printf("TEXT:%s\n", rcv);
    }

    return 0;
}

void cli_error(FILE *f_out, const char *msg, int err_code)
{
    //printf("[%s]---[%i]\n", msg, err_code);
    fprintf(f_out, "Fehlermeldung::%s::%i\n", msg, err_code);
	printUsage();
    exit(err_code);
}

void printUsage() 
{
	fprintf(stderr, "%s:\n", program_name);
	fprintf(stderr, "usage: sclient options\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "\t-s, --server <server>		full qualified domain name or IP address of the server\n");
	fprintf(stderr, "\t-p, --port <port>		well-known port of the server [0..65535]\n");
	fprintf(stderr, "\t-u, --user <name>		name of the posting user\n");
	fprintf(stderr, "\t-i, --image <URL> 		URL pointing to an image of the posting user\n");
	fprintf(stderr, "\t-m, --message <message>		message to be added to the bulletin board\n");
	fprintf(stderr, "\t-v, --verbose			verbose output\n");
	fprintf(stderr, "\t-h, --help\n");				
}