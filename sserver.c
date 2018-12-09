
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

#define BACKLOG 10 // how many pending connections queue will hold
#define MAX_BUFFER (1500)

// to-do:
// -Kommentare
// -sigchild_handler ändern (direkt aus beej)
// -löschen von nicht benötigten printfs



void getPort(int argc, char *argv[], char **server_port);
void sigchld_handler(int s);
void printUsage(void);

const char *program_name = NULL;

int main(int argc, char *argv[])
{
	char *server_port;
	char in_buff[MAX_BUFFER];
	program_name = argv[0];
	int sfd, new_fd; // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *result, *rp;
	struct sockaddr_storage remote_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	int s;

	getPort(argc, argv, &server_port);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	s = getaddrinfo(NULL, server_port, &hints, &result);
	if (s != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	// loop through all the results and bind to the first we can
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
		{
			perror("server: socket");
			continue;
		}
		//was macht das genau?
		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == -1)
		{
			close(sfd);
			perror("server: bind");
			continue;
		}
		break;
	}
	freeaddrinfo(result); // all done with this structure

	if (rp == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		exit(EXIT_FAILURE);
	}
	if (listen(sfd, BACKLOG) == -1)
	{
		close(sfd);
		perror("listen");
		exit(EXIT_FAILURE);
	}
	sa.sa_handler = sigchld_handler; // reap all dead processes

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		close(sfd);
		exit(EXIT_FAILURE);
	}

	while (1)
	{ // main accept() loop

		sin_size = sizeof remote_addr;
		new_fd = 0;
		new_fd = accept(sfd, (struct sockaddr *)&remote_addr, &sin_size);
		if (new_fd == -1)
		{
			perror("accept");
			close(sfd);
			exit(EXIT_FAILURE);
			//continue;
		}
		
		//forken & umleiten
		int child_id;
		child_id = fork();
		switch (child_id)
		{
		case -1: //Fehlerfall
			close(sfd);
			close(new_fd);
			fprintf(stderr, "Child erzeugen nicht möglich\n");
			exit(EXIT_FAILURE);
			break;
		case 0: //child

			close(sfd);
			memset(in_buff, 0, MAX_BUFFER * sizeof(in_buff[0]));
			dup2(new_fd, STDOUT_FILENO);
			dup2(new_fd, STDIN_FILENO);
			close(new_fd);

			execlp("simple_message_server_logic", "simple_message_server_logic", (char *)NULL);
			perror("Couldn't exec");
			exit(EXIT_FAILURE);
			
			break;
		default: //parent
			close(new_fd);
			//exit(EXIT_SUCCESS);
			break;
		}
	}
	return 0;
}

void getPort(int argc, char *argv[], char **server_port)
{
	int argument;
	int server_port_num;

	/* retrieve port argument from commandline */
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

	/* if no port, we exit */
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
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;
	if (s)
	{
	} //da sonst compiler errror
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