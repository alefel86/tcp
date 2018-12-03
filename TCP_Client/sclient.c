#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simple_message_client_commandline_handling.h"

#define MAX_NAME_L (50)
#define MAX_BUFFER (1500)
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
int verbose;
//char status[9];
int rsp_status;
char rsp_name[MAX_NAME_L];
//char rsp_img_name[MAX_NAME_L];
int rsp_len;
char in_buff[MAX_BUFFER];

int main(int argc, const char *argv[])
{

    int sfd, s;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    program_name = argv[0];

    //den Speicherplatz auf 0 setzen um Fehler zu vermeiden
    // memset(status, 0, 9 * sizeof(status[0]));
    memset(rsp_name, 0, MAX_NAME_L * sizeof(rsp_name[0]));
    // memset(img_name, 0, MAX_NAME_L * sizeof(img_name[0]));
    memset(in_buff, 0, MAX_BUFFER * sizeof(in_buff[0]));
    memset(&hints, 0, sizeof(struct addrinfo));
    rsp_status = -1;

    smc_parsecommandline(argc, argv, cli_error, &server, &port, &user, &message, &img_url, &verbose);

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
    int temp_sfd = dup(sfd);
    int sh;
    sh = shutdown(temp_sfd, SHUT_WR);
    if (sh < 0)
        printf("shutdown Error\n");
    FILE *rcv_socket = fdopen(sfd, "r");
    if (rcv_socket == NULL)
    {
        fprintf(stderr, "RCV-Socketd problems\n");
    }
    //int eof = FALSE;
    int durchgang;
    durchgang = 0;
    //int anz_in = 1500;
    // char a[MAX_BUFFER];
    //  char name[MAX_NAME_L];
    //int len;

    // memset(a, 0, MAX_BUFFER * sizeof(a[0]));
    // memset(rsp_name, 0, MAX_NAME_L * sizeof(a[0]));

    kv kv;
    // memset(kv, 0,sizeof(kv));
    // memset(&kv,0,sizeof(kv));
    // strcpy(kv.key,"");
    //strcpy(kv.key,'');
    char **ptr = 0;
    int is_len = 0;
    // int i;

    do
    {

        if (is_len == 0)
        {
            if (fgets(in_buff, MAX_BUFFER, rcv_socket) == NULL)
            {
                fprintf(stderr, "RCV-Error fgets\n");
                exit(EXIT_FAILURE);
            }
            /*
            for (i = 0; in_buff[i] != '\n'; i++)
            {
                a[i] = in_buff[i];
            }
            */
            get_kv(in_buff, &kv);

            if (strcmp(kv.key, "status") == 0)
            {
                rsp_status = (int)strtol(kv.value, ptr, 10);
                printf("::status::\n");
            }
            else if (strcmp(kv.key, "len") == 0)
            {
                is_len = 1;
                rsp_len = (int)strtol(kv.value, ptr, 10);
                // rsp_len++;
                printf("::len::\n");
                //printf("__LEN0__\n");
                //printf("__%i__\n", (int)strtol(kv[durchgang].value, ptr, 10));
            }
            else if (strcmp(kv.key, "file") == 0)
            {
                strcpy(rsp_name, kv.value);
                printf("::file::\n");
            }
        }

        else if (is_len == 1)
        {
            printf("is_len=1\n");
            int file_in_stat;
            file_in_stat = 0;
            //  fprintf(stdout, "strol:%i\n", (int)(strtol(kv[durchgang - 1].value, ptr, 10) + 1));
            int wrt_cnt;
            int wrt_check;
            wrt_cnt = 0;
            FILE *fp;
            fp = fopen(rsp_name, "w");
            if (fp == NULL)
            {
                fprintf(stderr, "fp expected\n");
            }
            do
            {
                printf("rsp_len:%d\n", rsp_len);
                printf("file_in_stat:%d\n", file_in_stat);
                if (rsp_len - MAX_BUFFER > 0)
                {
                    wrt_cnt = MAX_BUFFER;
                }
                else
                {
                    wrt_cnt = rsp_len;
                }

                printf("cnt:%d\n", wrt_cnt);
                file_in_stat = fread(in_buff, 1, wrt_cnt, rcv_socket);
                if (file_in_stat < 0)
                {
                    fprintf(stderr, "RCV-Error fread\n");
                }

                wrt_check = fwrite(in_buff, 1, wrt_cnt, fp);
                rsp_len -= wrt_cnt;
                if (wrt_check < 0)
                {
                    fprintf(stderr, "WriteProblems\n");
                }

            } while (wrt_cnt > 0);

            fclose(fp);
            is_len = 0;
        }

        // fprintf(stdout, "Durchgang:%i\n\tk<%s> v<%s>\n", durchgang, kv[durchgang].key, kv[durchgang].value);
        durchgang++;
        //  memset(a, 0, MAX_BUFFER * sizeof(a[0]));
        memset(in_buff, 0, MAX_BUFFER * sizeof(in_buff[0]));

    } while (1);

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

    int ll;
    ll = strlen(line);
    if (line[ll - 1] == '\n')
        line[ll - 1] = 0;
    //tmp_l=strncpy(line,line,strlen(line)-1);
    //line = strtok(line, "\n");
    //printf("ln:%s\n", line);
    // printf(" ");
    tmp = strstr(line, &delimiter);
    tmp++;
    // printf("ntmp:%s\n", tmp);

    strcpy(kv->value, tmp);
    strncat(kv->key, line, strlen(line) - (strlen(kv->value)) - 1);
    printf("k<%s> v<%s>\n", kv->key, kv->value);
}