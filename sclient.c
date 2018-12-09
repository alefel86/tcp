#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simple_message_client_commandline_handling.h"

#define MAX_NAME_L (255)
#define MAX_BUFFER (1500)
typedef struct
{
    char key[10];
    char value[MAX_NAME_L];
} keyValue;
void cli_error(FILE *, const char *, int);
void printUsage(void);
void get_kv(char *, keyValue *);

const char *program_name = NULL;
/*
ToDo: 
-fprintf(stderr) mit perror ersetzen 
-get_kv
-makefile kontrollieren ob wie im letzen Semester
-strcpy durch strncpy ersetzen
-check wenn nicht status=0 als erstes gesendet wird, muss noch 
        close(sfd);
        fclose(send_socket);
    eingefügt werden
-Kommentare schreiben bzw kennzeichen wenn Code kompliziert ist
-TESTCASE 5 macht Probleme, in get_kv eine kontelle machen ob in key status||len||file steht wenn nicht error
*/
int main(int argc, const char *argv[])
{
    const char *server, *port, *user, *message, *img_url;
    int verbose;
    //rsp-Response
    int rsp_status;
    char rsp_name[MAX_NAME_L];
    int rsp_len;
    //Buffer in dem eingelesen wird
    char in_buff[MAX_BUFFER];

    int sfd, s;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    program_name = argv[0];

    //den Speicherplatz auf 0 setzen um Fehler zu vermeiden
    memset(rsp_name, 0, MAX_NAME_L * sizeof(rsp_name[0]));
    memset(in_buff, 0, MAX_BUFFER * sizeof(in_buff[0]));
    memset(&hints, 0, sizeof(struct addrinfo));
//Wenn Status im ersten Durchgang nicht gesetzt wird kommt es zu einem Fehler
    rsp_status = -1;

    smc_parsecommandline(argc, argv, cli_error, &server, &port, &user, &message, &img_url, &verbose);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    s = getaddrinfo(NULL, port, &hints, &result);
    if (s!=0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
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

    FILE *send_socket = fdopen(sfd, "w");
    if (send_socket == NULL)
    {  
        close(sfd);
    }
    if (img_url != NULL)
        fprintf(send_socket, "user=%s\nimg=%s\n%s", user, img_url, message);
    else
        fprintf(send_socket, "user=%s\n%s", user, message);
    fflush(send_socket);

    FILE *rcv_socket = fdopen(sfd, "r");
    if (rcv_socket == NULL)
    {
        close(sfd);
        fclose(send_socket);
        fprintf(stderr, "RCV-Socketd problems\n");
    }
    shutdown(sfd, SHUT_WR);

    keyValue kv;
    char **ptr = 0;
    int is_len = 0;

    do
    {

        if (is_len == 0)
        {

            if (fgets(in_buff, MAX_BUFFER, rcv_socket) == NULL)
            {
                if (ferror(rcv_socket))
                    exit(EXIT_FAILURE);
                else
                {
                    close(sfd);
                    fclose(rcv_socket);
                    fclose(send_socket);
                    exit(EXIT_SUCCESS);
                }
            }

            get_kv(in_buff, &kv);

            if (strcmp(kv.key, "status") == 0)
            {
                rsp_status = (int)strtol(kv.value, ptr, 10);
            }
            if(rsp_status!=0)
            {
                printf("STATUS!=0\n");
                exit(EXIT_FAILURE);
            }
            else if (strcmp(kv.key, "len") == 0)
            {
                is_len = 1;
                rsp_len = (int)strtol(kv.value, ptr, 10);
            }
            else if (strcmp(kv.key, "file") == 0)
            {
                strcpy(rsp_name, kv.value);
            }
        }

        else if (is_len == 1)
        {
            int file_in_stat;
            file_in_stat = 0;
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
                if (rsp_len - MAX_BUFFER > 0)
                {
                    wrt_cnt = MAX_BUFFER;
                }
                else
                {
                    wrt_cnt = rsp_len;
                }

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

            memset(rsp_name, 0, sizeof(rsp_name));
            rsp_len = 0;
            fclose(fp);
            is_len = 0;
        }

        memset(in_buff, 0, MAX_BUFFER * sizeof(in_buff[0]));
    } while (1);

    // return 0;
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

/*
Wir übergeben der Fkt. die Ziele die per fgets eingelesen wird(diese enthält auch ein newline)
und einen keyValue Strucktpointer

zuerst schneiden wir das newline zeichen weg

strstr zeigt dann auch das Zeichen das in delimiter stetht(=), wir setzen es um einzeichen weiter
um zum Value zu gelangen
Key         Kev variable    Input Zeile     länge Zeile(bis \n)  -  länge Value         -   1 Zeichen für =
strncat(    kv->key,        line,            strlen(line)        -  strlen(kv->value)   -   1);
Bsp:    status=0\n
        line(8)
        value(1)
        =(1)
        8-1-1=6 
        status(6)
*/
void get_kv(char *line, keyValue *kv)
{
    memset(kv, 0, sizeof(keyValue));
    char delimiter = '=';
    char *tmp;

    int ll;
    ll = strlen(line);
    if (line[ll - 1] == '\n')
        line[ll - 1] = 0;
    tmp = strstr(line, &delimiter);
    tmp++;
    strncpy(kv->value, tmp,strlen(tmp));
    strncat(kv->key, line, strlen(line) - strlen(kv->value) - 1);
//ToDo schauen was der Server im Fehlerfall zurück gibt und einbauen -1 ist Standartwert
    if (strcmp(kv->key, "status") == 0 && strcmp(kv->value, "-1") == 0)
    {        
        fprintf(stderr, "status=0 expected\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "k<%s> v<%s>\n,", kv->key, kv->value);
}