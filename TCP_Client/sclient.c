#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <simple_message_client_commandline_handling.h>

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
int main(int argc, const char *argv[])
{

    const char *server, *port, *user, *message, *img_url;
    int sfd, s, verbose;

    struct addrinfo hints;
    struct addrinfo *result, *rp;

    smc_parsecommandline(argc, argv, cli_error, &server, &port, &user, &message, &img_url, &verbose);
    printf("server:%s port: %s user:%s message:%s img_url:%s verbose:%i\n", server, port, user, message, img_url, verbose);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    s = getaddrinfo(server, port, &hints, &result);
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
    printf("[%s]---[%i]\n", msg, err_code);
    fprintf(f_out, "Fehlermeldung::%s::%i\n", msg, err_code);
    exit(err_code);
}