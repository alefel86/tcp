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

#define MAX_NAME_L (50)
typedef struct
{
    char key[10];
    char value[MAX_NAME_L];
} kv;
void cli_error(FILE *, const char *, int);
void printUsage(void);
void get_kv(char *, kv *);

const char *program_name = NULL;
const char *server, *port, *user, *message, *img_url;
char status[9];
char html_name[MAX_NAME_L];
char img_name[MAX_NAME_L];
char in_buff[1500];

int main(int argc, const char *argv[])
{

    int sfd, s, verbose;

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    program_name = argv[0];
    memset(status, 0, 9 * sizeof(status[0]));
    memset(html_name, 0, MAX_NAME_L * sizeof(html_name[0]));
    memset(img_name, 0, MAX_NAME_L * sizeof(img_name[0]));
    memset(in_buff, 0, 1500 * sizeof(in_buff[0]));

    smc_parsecommandline(argc, argv, cli_error, &server, &port, &user, &message, &img_url, &verbose);

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
    int temp_sfd = dup(sfd);
    int sh;
    sh = shutdown(temp_sfd, SHUT_WR);
    if (sh == 0)
        printf("shutdown ist 0");
    FILE *rcv_socket = fdopen(sfd, "r");
    if (rcv_socket == NULL)
    {
        fprintf(stderr, "RCV-Socketd problems\n");
    }
    //int eof = FALSE;
    int durchgang = 0;
    int anz_in = 1500;
    char a[1500];
    char name[MAX_NAME_L];
    int len;

    memset(a, 0, 1500 * sizeof(a[0]));
    nemset(name, 0, MAX_NAME_L * sizeof(a[0]));

    kv kv;
    char **ptr = 0;
    int is_len = 0;
    int i;

    do
    {

        if (is_len == 0)
        {
            if (fgets(in_buff, anz_in, rcv_socket) == NULL)
            {
                fprintf(stderr, "RCV-Error expected\n");
            }

            for (i = 0; in_buff[i] != '\n'; i++)
            {
                a[i] = in_buff[i];
            }
            get_kv(a, &kv[durchgang]);
            if (strcmp(kv[durchgang].key, "len") == 0)
            {
                is_len = 1;
                printf("__LEN0__\n");
                printf("__%i__\n", (int)strtol(kv[durchgang].value, ptr, 10 + 1));
            }
        }

        else if (is_len == 1)
        {

            int read_status;
            fprintf(stdout, "strol:%i\n", (int)(strtol(kv[durchgang - 1].value, ptr, 10) + 1));
            read_status = fread(in_buff, 1, (int)(strtol(kv[durchgang - 1].value, ptr, 10)), rcv_socket);
            fprintf(stdout, "stauts:%i\n", read_status);

            if (read_status == (strtol(kv[durchgang - 1].value, ptr, 10) + 1))
                fprintf(stdout, "laenge ok\n");
            fprintf(stderr, "bufferin\n");
            if (read_status < 0)
            {
                fprintf(stderr, "RCV-Error expected\n");
            }

            FILE *fp;
            fp = fopen(kv[durchgang - 2].value, "w");
            if (fp == NULL)
            {
                fprintf(stderr, "fp expected\n");
            }

            fwrite(in_buff, 1, read_status, fp);
            fclose(fp);

            fprintf(stdout, "++ESLSE++\n");

            is_len = 0;
        }

        fprintf(stdout, "Durchgang:%i\n\tk<%s> v<%s>\n", durchgang, kv[durchgang].key, kv[durchgang].value);
        durchgang++;
        memset(a, 0, 1500 * sizeof(a[0]));
        memset(in_buff, 0, 1500 * sizeof(a[0]));

    } while (durchgang < 8); //!feof(rcv_socket));

    printf("--ENDE--\n");
    return 0;
}

void cli_error(FILE *f_out, const char *msg, int err_code)
{
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
void get_kv(char *line, kv *kv)
{
    memset(kv, 0, 60 * sizeof(char));
    char delimiter = '=';
    char *tmp;
    tmp = strstr(line, &delimiter);
    tmp++;
    printf("tmp:%s\n", tmp);
    strcpy(kv->value, tmp);
    strncat(kv->key, line, strlen(line) - (strlen(kv->value) + 1));
}