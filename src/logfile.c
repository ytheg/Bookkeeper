/****************************************************************************
	logfile.c : message logging functions
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "logfile.h"

char	*message_color[] =	{
				"\033[0m",
				"\033[1;34m",
				//"\033[1;31m",
				//"\033[30;5m\e[1;91m",
				"\e[1;91m",
				"\033[1;37m",
				"\033[1;33m",
				"\033[1;32m",
				"\033[1;35m"
				};

char	*message_text[]	=	{
				"LOGGING PROCEDURE",
				"MEMORY ALLOCATION",
				"RUNTIME PROCEDURE"
				};

char	*message_status[] =	{
				"LOGMESS",
				"SUCCESS",
				"FAILURE",
				"ERROR"
				};


#ifndef	LOG_FILE_MESSAGES
void	log_message(int status, int type, char *file, char *function, off_t line, char *message){
	int color, shade;
	switch	(status){
		case	DEFAULT_STATUS:
			color = GREY;
			break;
		case	SUCCESS:
			color = GREEN;
			break;
		case	FAILURE:
			color = RED;
			break;
		case	ERR:
			color = YELLOW;
			break;
		default:
			color = DEFAULT_COLOR;
			break;
		}

	switch	(type){
		case	MEMORY:
			shade = BLUE;
			break;
		case	RUNTIME:
			shade = GREY;
			break;
		default:
			shade = PURPLE;
			break;
		}

	printf("[%s%7s%s] - %s%s%s :: FILE : %-20s FUNCTION : %-20s \tLINE : [%-5d] %s%s\n", message_color[color], message_status[status], message_color[DEFAULT_COLOR], 
		message_color[shade], message_text[type], message_color[DEFAULT_COLOR], file, function, line, 
		message == NULL? "" : "MESSAGE: ", message == NULL? "" : message);
	}
#else
void	log_message(int status, int type, char *file, char *function, off_t line, char *message){
	}
#endif
