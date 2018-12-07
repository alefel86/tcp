/*
** server.c -- a stream socket server demo
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

#define BACKLOG 10 // how many pending connections queue will hold
#define MAX_BUFFER (1500)

// void sigchld_handler(int s)
// {
//waitpid() might overwrite errno, so we save and restore it:
// int saved_errno = errno;
// while(waitpid(-1, NULL, WNOHANG) > 0);
// errno = saved_errno;
// }
// get sockaddr, IPv4 or IPv6:

/*
ToDo
-Server printUsage
-ipv6 muss der Server nicht können, entweder verstehen löschen

*/

void getPort(int argc, char *argv[], char **server_port);
void *get_in_addr(struct sockaddr *sa);
void sigchld_handler(int s);
//int s);

int main(int argc, char *argv[])
{
	char *server_port;
	char in_buff[MAX_BUFFER];

	int sfd, new_fd; // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *result, *rp;
	struct sockaddr_storage remote_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char ip[INET_ADDRSTRLEN];
	int s;

	getPort(argc, argv, &server_port);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
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

	printf("server: waiting for connections...\n");

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
		inet_ntop(remote_addr.ss_family, get_in_addr((struct sockaddr *)&remote_addr), ip, sizeof(ip));

		printf("server: got connection from %s\n", ip);

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
			//printUsage();
			exit(EXIT_SUCCESS);
		default:
			//printUsage();
			exit(EXIT_FAILURE);
		}

	/* if no port, we exit */
	if (*server_port == NULL)
	{
		//printUsage();
		exit(EXIT_FAILURE);
	}

	server_port_num = strtol(*server_port, NULL, 0);
	printf("serverport: %d\n", server_port_num);

	if (server_port_num < 1024 || server_port_num > 65535)
	{
		//printUsage();
		exit(EXIT_FAILURE);
	}
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr); //brauchen wir eigentlich nicht da nur ipv4
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