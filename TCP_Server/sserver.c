#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

void getPort(int argc, char *argv[], char** server_port);
void printUsage (void);
char *program_name = NULL;


int main(int argc, char *argv[]) {

	char *server_port;

	program_name = argv[0];

   	getPort(argc, argv, &server_port);
	
   	//setUpListeningSocket(server_port);

	//doServerLoop();

	return 0;
}

void getPort(int argc, char *argv[], char** server_port)
{
	int argument;
	int server_port_num;
	const char* p, h = NULL;

	while ((argument = getopt(argc, argv, p | h)) != -1)
		switch (argument) {
		case 'p':
			*server_port = optarg;
			break;
		case 'h':
			printUsage();
			exit(EXIT_SUCCESS);
		case '?':
			printUsage();
			exit(EXIT_SUCCESS);
		default:
			printUsage();
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
		printUsage();
		exit(EXIT_FAILURE);
	}

	server_port_num = strtol(*server_port, NULL, 0);
	printf("serverport: %d\n", server_port_num);

	if (server_port_num < 1024 || server_port_num > 65535) {
		printUsage();
		exit(EXIT_FAILURE);
	}
}

void printUsage(void)
{
	fprintf(stderr, "usage: sserver options\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "\t-p, --port <port>\n");
	fprintf(stderr, "\t-h, --help\n");				
}