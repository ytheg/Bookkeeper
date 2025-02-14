/****************************************************************************
	logfile.h : message logging functions
****************************************************************************/

#define	LOG_FILE_MESSAGES

enum	{
	DEFAULT_COLOR = 0, 
	BLUE,
	RED,
	GREY,
	YELLOW,
	GREEN,
	PURPLE
	};

enum	{
	DEFAULT_TEXT = 0,
	MEMORY,
	RUNTIME
	};

enum	{
	DEFAULT_STATUS = 0,
	SUCCESS,
	FAILURE,
	ERR
	};

void	log_message(int status, int type, char *file, char *function, off_t line, char *message);
