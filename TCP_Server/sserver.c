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

int main(int argc, char *argv[])
{
	char *server_port;

	int sfd, new_fd; // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *result, *rp;
	struct sockaddr_storage remote_addr; // connector's address information
	socklen_t sin_size;
	//struct sigaction sa;
	int yes = 1;
	//char ip6[INET6_ADDRSTRLEN];
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
		perror("listen");
		exit(EXIT_FAILURE);
	}
	//sa.sa_handler = sigchld_handler; // reap all dead processes
	/*
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
	*/
	printf("server: waiting for connections...\n");

	while (1)
	{ // main accept() loop
		sin_size = sizeof remote_addr;
		new_fd = accept(sfd, (struct sockaddr *)&remote_addr, &sin_size);
		if (new_fd == -1)
		{
			perror("accept");
			continue;
		}
		inet_ntop(remote_addr.ss_family, get_in_addr((struct sockaddr *)&remote_addr), ip, sizeof(ip));
		printf("server: got connection from %s\n", ip);

		//forken & umleiten
		if (!fork())
		{			
			//DUP
				// this is the child process
			close(sfd); // child doesn't need the listener
			if (send(new_fd, "Hello, world!", 13, 0) == -1)
				perror("send");
			close(new_fd);
			exit(EXIT_SUCCESS);
		}
		close(new_fd); // parent doesn't need this
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

	/*optstring is a string containing the legitimate option characters.
       If such a character is followed by a colon, the option requires an
       argument, so getopt() places a pointer to the following text in the
       same argv-element, or the text of the following argv-element, in
       optarg.  Two colons mean an option takes an optional arg; if there is
       text in the current argv-element (i.e., in the same word as the
       option name itself, for example, "-oarg"), then it is returned in
       optarg, otherwise optarg is set to zero.  This is a GNU extension.
       If optstring contains W followed by a semicolon, then -W foo is
       treated as the long option --foo.  (The -W option is reserved by
       POSIX.2 for implementation extensions.)  This behavior is a GNU
       extension, not available with libraries before glibc 2.*/

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
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}