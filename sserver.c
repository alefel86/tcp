/**
 * @file sserver.c
 *
 * sserver - recieves port from commandline and receives messages and/or images from sclient
 *
 * @author Alexander Feldinger  <ic17b055@technikum-wien.at>
 * @author Manuel Seifner	    <ic17b022@technikum-wien.at>
 * @author Thomas Thiel		    <Ic18b049@technikum-wien.at>
 * @date 2018/12/09
 *
 * @version 1.0
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10 
#define MAX_BUFFER (1500)

// to-do:
// -Kommentare

void getPort(int argc, char *argv[], char **server_port);
void sigchld_handler(int s);
void printUsage(void);

const char *program_name = NULL;

int main(int argc, char *argv[])
{
	char *server_port;
	char in_buff[MAX_BUFFER];
	program_name = argv[0];
	int sfd, new_fd; 
	struct addrinfo hints, *result, *rp;
	struct sockaddr_storage client_addr; 
	socklen_t client_size;
	struct sigaction sa;
	int yes = 1;
	int s;

	getPort(argc, argv, &server_port);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 

	s = getaddrinfo(NULL, server_port, &hints, &result);
	if (s != 0)
	{
		fprintf(stderr, "%s::getaddrinfo: %s\n", program_name, gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
		{
			fprintf(stderr, "%s::Socket: %s\n", program_name, strerror(errno));
			continue;
		}
		
		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			fprintf(stderr, "%s::setSocket: %s\n", program_name, strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == -1)
		{
			close(sfd);

			fprintf(stderr, "%s::bind: %s\n",program_name, strerror(errno));
			continue;
		}
		break;
	}
	freeaddrinfo(result); 

	if (rp == NULL)
	{

		fprintf(stderr, "%s::Bind: %s\n", program_name, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (listen(sfd, BACKLOG) == -1)
	{
		close(sfd);
		fprintf(stderr, "%s::Listen: %s\n", program_name, strerror(errno));
		exit(EXIT_FAILURE);
	}
	sa.sa_handler = sigchld_handler; 

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		fprintf(stderr, "%s::Signal: %s\n", program_name, strerror(errno));
		close(sfd);
		exit(EXIT_FAILURE);
	}

	while (1)
	{ 
		client_size = sizeof client_addr;
		new_fd = 0;
		new_fd = accept(sfd, (struct sockaddr *)&client_addr, &client_size);
		if (new_fd == -1)
		{
			fprintf(stderr, "%s::Accept: %s\n", program_name, strerror(errno));
			close(sfd);
			exit(EXIT_FAILURE);
		}
		
		int child_id;
		child_id = fork();
		switch (child_id)
		{
		case -1: 
			close(sfd);
			close(new_fd);
			fprintf(stderr, "%s::Child create: %s\n", program_name, strerror(errno));
			exit(EXIT_FAILURE);
			break;
		case 0: //child

			close(sfd);
			memset(in_buff, 0, MAX_BUFFER * sizeof(in_buff[0]));
			dup2(new_fd, STDOUT_FILENO);
			dup2(new_fd, STDIN_FILENO);
			close(new_fd);

			execlp("simple_message_server_logic", "simple_message_server_logic", (char *)NULL);
			fprintf(stderr, "%s::exec: %s\n", program_name, strerror(errno));
			exit(EXIT_FAILURE);

			break;
		default: //parent
			close(new_fd);
			break;
		}
	}
	return 0;
}

void getPort(int argc, char *argv[], char **server_port)
{
	int argument;
	int server_port_num;

	while ((argument = getopt(argc, argv, "p:")) != -1)
		switch (argument)
		{
		case 'p':
			*server_port = optarg;
			break;
		case '?':
			printUsage();
			exit(EXIT_SUCCESS);
		default:
			printUsage();
			exit(EXIT_FAILURE);
		}

	if (*server_port == NULL)
	{
		printUsage();
		exit(EXIT_FAILURE);
	}

	server_port_num = strtol(*server_port, NULL, 0);

	if (server_port_num < 1024 || server_port_num > 65535)
	{
		printUsage();
		exit(EXIT_FAILURE);
	}
}

void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it
	int saved_errno = errno;
	if (s)
	{
	} 
	while (waitpid(-1, NULL, WNOHANG) > 0)
	{
	}
	errno = saved_errno;
}

void printUsage()
{
	fprintf(stderr, "%s:\n", program_name);
	fprintf(stderr, "usage: sserver option\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "\t-p, --port <port>\n");
	fprintf(stderr, "\t-h, --help\n");
}