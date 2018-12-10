/**
 * @file sclient.c
 *
 * sclient - receives username and message from commandline and sends them to the server
 *
 * @author Alexander Feldinger  <ic17b055@technikum-wien.at>
 * @author Manuel Seifner	    <ic17b022@technikum-wien.at>
 * @author Thomas Thiel		    <Ic18b049@technikum-wien.at>
 * @date 2018/12/09
 *
 * @version 1.0
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "simple_message_client_commandline_handling.h"

#define MAX_NAME_L (255)
#define MAX_BUFFER (1500)
typedef struct
{
    char key[10];
    char value[MAX_NAME_L];
} keyValue;

bool get_connection(const char *port, int *socket_fd);

bool send_data(const char *user, const char *message, const char *img_url, const int *socket_fd, FILE *send_socket);

bool receive_data(FILE *rcv_socket, const int *sfd, int verbose);

void cleanup(FILE *send_socket, FILE *rcv_socket);

void cli_error(FILE *, const char *, int);

void printUsage(void);

bool get_kv(char *line, keyValue *kv, int verbose);

const char *program_name = NULL;

/*
ToDo:
-Kommentare schreiben bzw kennzeichen wenn Code kompliziert ist

*/

/**
 * @brief Client sends data to Server and receives Files
 * 
 * @param argc  the number of arguments
 * @param argv the arguments themselves (including the program name in argv[0])
 * @return int EXIT_FAILURE returns only in case of error
 */
int main(int argc, const char *argv[])
{
    bool rc = true;
    const char *server, *port, *user, *message, *img_url;
    int verbose;
    int sfd;
    FILE *rcv_socket = NULL;
    FILE *send_socket = NULL;

    program_name = argv[0];

    smc_parsecommandline(argc, argv, cli_error, &server, &port, &user, &message, &img_url, &verbose);

    rc = rc && get_connection(port, &sfd);

    rc = rc && send_data(user, message, img_url, &sfd, send_socket);

    rc = rc && receive_data(rcv_socket, &sfd, verbose);

    cleanup(send_socket, rcv_socket);

    if (rc)
        exit(EXIT_SUCCESS);
    else
        exit(EXIT_FAILURE);
}

/**
 * @brief connects to the server
 * 
 * @param port remote Port
 * @param socket_fd output parameter for the socket
 * \return	rc
 * \retval  true        if function performed correctly
 * \retval  false       if an error occured
 */

bool get_connection(const char *port, int *socket_fd)
{
    bool rc = true;
    int sfd;
    int s;
    struct addrinfo hints;
    struct addrinfo *result, *address;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    s = getaddrinfo(server, port, &hints, &result);
    if (s != 0)
    {
        fprintf(stderr, "%s::getaddrinfo: %s\n", program_name, gai_strerror(s)); //getaddrinfo doesn't set errno
        rc = false;
    }

    if (rc)
    {
        for (address = result; address != NULL; address = address->ai_next)
        {
            sfd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
            if (sfd == -1)
                continue;

            if (connect(sfd, address->ai_addr, address->ai_addrlen) != -1)
            {
                *socket_fd = sfd;
                break; /* Success */
            }

            close(sfd);
        }
        freeaddrinfo(result);

        if (address == NULL)
        { /* No address succeeded */
            fprintf(stderr, "%s::connect: %s\n", program_name, strerror(errno));

            rc = false;
        }
    }

    return rc;
}
/**
 * @brief sends data to the server
 * 
 * @param user  username from argv
 * @param message message from argv
 * @param img_url image url from argv
 * @param socket_fd filedescriptor from the socket 
 * @param send_socket   output parameter file we wrote to
 * \return	rc
 * \retval  true        if function performed correctly
 * \retval  false       if an error occured
 */
bool send_data(const char *user, const char *message, const char *img_url, const int *socket_fd, FILE *send_socket)
{
    bool rc = true;

    int sfd = dup(*socket_fd); //duplicate sfd so we can close the send and receive stream independently

    send_socket = fdopen(sfd, "w");
    if (send_socket == NULL)
        rc = false;

    if (rc)
    {
        if (img_url != NULL)
            fprintf(send_socket, "user=%s\nimg=%s\n%s", user, img_url, message);
        else
            fprintf(send_socket, "user=%s\n%s", user, message);
    }

    fflush(send_socket);

    shutdown(sfd, SHUT_WR);

    return rc;
}
/**
 * @brief receives data from the Server and writes it to a file
 * 
 * @param rcv_socket  output parameter file we wrote to
 * @param socket_fd     filedescripter we open to read
 * @param verbose   if set generates output on stdout
 * \return	rc
 * \retval  true        if function performed correctly
 * \retval  false       if an error occured
 */
bool receive_data(FILE *rcv_socket, const int *socket_fd, const int verbose)
{
    bool rc = true;
    bool done = false;
    //Buffer in dem eingelesen wird
    char in_buff[MAX_BUFFER];
    //rsp-Response
    //Wenn Status im ersten Durchgang nicht gesetzt wird kommt es zu einem Fehler
    int rsp_status = -1;
    int rsp_len;
    char rsp_name[MAX_NAME_L];
    keyValue kv;
    char **ptr = 0;
    int is_len = 0;
    int sfd = *socket_fd;

    //den Speicherplatz auf 0 setzen um Fehler zu vermeiden
    memset(rsp_name, 0, MAX_NAME_L * sizeof(rsp_name[0]));
    memset(in_buff, 0, MAX_BUFFER * sizeof(in_buff[0]));

    rcv_socket = fdopen(sfd, "r");
    if (rcv_socket == NULL)
    {
        fprintf(stderr, "RCV-Socketd problems\n");
        fprintf(stderr, "%s::fopen rcv socket: %s\n", program_name, strerror(errno));
        rc = false;
    }

    while (rc && !done)
    {
        if (is_len == 0)
        {
            if (fgets(in_buff, MAX_BUFFER, rcv_socket) == NULL)
            {
                if (ferror(rcv_socket))
                    rc = false;
                done = true;
            }
            else
            {
                rc = rc && get_kv(in_buff, &kv, verbose);

                if (strcmp(kv.key, "status") == 0)
                {
                    rsp_status = (int)strtol(kv.value, ptr, 10);
                    if (rsp_status != 0)
                    {
                        fprintf(stderr, "%s::status=0 expected %s\n", program_name, strerror(errno));
                        rc = false;
                    }
                }
                else if (strcmp(kv.key, "len") == 0)
                {
                    is_len = 1;
                    rsp_len = (int)strtol(kv.value, ptr, 10);
                }
                else if (strcmp(kv.key, "file") == 0)
                {
                    strncpy(rsp_name, kv.value, sizeof(rsp_name));
                }
            }
        }
        else
        { //is_len == 1
            int wrt_cnt;
            int wrt_check = 0;
            int file_in_stat = 0;
            FILE *fp;

            fp = fopen(rsp_name, "w");
            if (fp == NULL)
            {
                fprintf(stderr, "%s::file write open: %s\n", program_name, strerror(errno));

                rc = false;
            }
            do
            {
                if (rsp_len - MAX_BUFFER > 0)
                    wrt_cnt = MAX_BUFFER;
                else
                    wrt_cnt = rsp_len;

                file_in_stat = fread(in_buff, 1, wrt_cnt, rcv_socket);
                if (file_in_stat < wrt_cnt)
                {
                    fprintf(stderr, "%s::Error fread socket in\n", program_name); //fread doesn't set errno -> no perror
                    rc = false;
                }

                wrt_check = fwrite(in_buff, 1, wrt_cnt, fp);
                rsp_len -= wrt_cnt;
                if (wrt_check < wrt_cnt)
                {
                    fprintf(stderr, "%s::Error file write\n", program_name); //fwrite doesn't set errno -> no perror
                    rc = false;
                }

            } while (rc && wrt_cnt > 0);

            memset(rsp_name, 0, sizeof(rsp_name));
            rsp_len = 0;
            fclose(fp);
            is_len = 0;
        }

        memset(in_buff, 0, MAX_BUFFER * sizeof(in_buff[0]));
    }

    return rc;
}
/**
 * @brief closes the open files
 * 
 * @param send_socket open file will be closed
 * @param rcv_socket open file will be closed
 * @return void
 */
void cleanup(FILE *send_socket, FILE *rcv_socket)
{
    if (send_socket != NULL)
        fclose(send_socket);
    if (rcv_socket != NULL)
        fclose(rcv_socket);
}
/**
 * @brief prints Errormessages if the commandlinearguments are wrong 
 * 
 * @param f_out file we write to
 * @param msg message we write to the file
 * @param err_code the errorcode the smc_parsecommandline returns
 */
void cli_error(FILE *f_out, const char *msg, int err_code)
{
    fprintf(f_out, "Fehlermeldung::%s::%i\n", msg, err_code);
    printUsage();
    exit(err_code);
}
/**
 * @brief prints usage
 * @return void
 */
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


/**
 * @brief Get key and value from line
 * 
 * @param line where key and value will be extracted
 * @param kv  output parameter struct stores key and value
 * @param verbose boolean   if set generates output on stdout
 * \return	rc
 * \retval  true        if function performed correctly
 * \retval  false       if an error occured
 */
bool get_kv(char *line, keyValue *kv, const int verbose)
{
    bool rc = true;
    memset(kv, 0, sizeof(keyValue));
    char *delimiter = "=\n";

    strncpy(kv->key, strtok(line, delimiter), sizeof(kv->key));
    strncpy(kv->value, strtok(NULL, delimiter), sizeof(kv->value));

    if (verbose)
        fprintf(stderr, "k<%s> v<%s>\n", kv->key, kv->value);

    if (strcmp(kv->key, "status") != 0 && strcmp(kv->key, "len") != 0 && strcmp(kv->key, "file") != 0)
    {
        fprintf(stderr, "%s::Error wrong response from server: %s\n", program_name, strerror(errno));
        rc = false;
    }
    return rc;
}