Readme
======

Verteilte Systeme - TCPIP Message Bulletin Board 
  @author Alexander Feldinger  <ic17b055@technikum-wien.at>
  @author Manuel Seifner	    <ic17b022@technikum-wien.at>
  @author Thomas Thiel		    <Ic18b049@technikum-wien.at>
Datum: 10.12.2018

Inhalt:
	sclient.c
	sserver.c
	makefile
	readme
	
	
sserver:

SYNOPSIS:

   The VCS TCP/IP message bulletin board server

USAGE:
   
   sserver -p port

DESCRIPTION:
   
   parameters:

    -p <port> : 
	<port> has a range between 1024 and 65535	

	example:
		<port> = 5000 + <uid>
        ./sserver -p 7521

			
sclient:

SYNOPSIS:
   
   The VCS TCP/IP message bulletin board client
       
USAGE:
       
   sclient  -s server -p port -u user - i image_url -m message 

DESCRIPTION:

   parameters:
		-s, --server <server>           full qualified domain name or IP address of the server
        -p, --port <port>               well-known port of the server [0..65535]
        -u, --user <name>               name of the posting user
        -i, --image <URL>               URL pointing to an image of the posting user
        -m, --message <message>         message to be added to the bulletin board
        -v, --verbose                   verbose output
        -h, --help

	 example:
		./sclient -s localhost -p 6676 -u USER -m Message