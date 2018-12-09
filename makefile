##
## @file Makefile
## VCS - TCP/IP Server
## 
## @author Alexander Feldinger ic17b055@technikum-wien.at
## @author Manuel Seifner icXXXX@technikum-wien.at
## @author Thomas Thiel icXXXX@technikum-wien.at
## @date 2017/03/24
##
## @version 1.0
##
## 
##
## ------------------------------------------------------------- variables --
##

CC=gcc
CFLAGS= -Wall -Werror -Wextra -Wstrict-prototypes -pedantic -fno-common -O3 -g -std=gnu11
CP=cp
CD=cd
MV=mv
GREP=grep
DOXYGEN=doxygen

EXCLUDE_PATTERN=footrulewidth

##
## ----------------------------------------------------------------- rules --
##
##
## --------------------------------------------------------------- targets --
##
# Compile and link all object files
.PHONY: all 

all: sserver sclient

sserver: sserver.c
	$(CC) -o sserver $(CFLAGS) sserver.c 

sclient: sclient.c
	$(CC) -o sclient $(CFLAGS) sclient.c -lsimple_message_client_commandline_handling

# Delete compilation output
.PHONY: clean
clean:
	$(RM) *.o *~ sserver *~ sclient

# Delete compilation output and documentation
.PHONY: distclean
distclean: clean
	$(RM) -r doc
	
# Create HTML documentation	
.PHONY: html pdf
html:
	$(DOXYGEN) doxygen.dcf

# Create HTML documentation and PDF file
pdf: html
	$(CD) doc/pdf && \
	$(MV) refman.tex refman_save.tex && \
	$(GREP) -v $(EXCLUDE_PATTERN) refman_save.tex > refman.tex && \
	$(RM) refman_save.tex && \
	make && \
	$(MV) refman.pdf refman.save && \
	$(RM) *.pdf *.html *.tex *.aux *.sty *.log *.eps *.out *.ind *.idx \
		*.ilg *.toc *.tps Makefile && \
	$(MV) refman.save refman.pdf

##
## ---------------------------------------------------------- dependencies --
##

##
## =================================================================== eof ==
##
