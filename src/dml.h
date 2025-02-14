/************************************************************************************
	dml.h : database manipulation functions used by the DBMS
************************************************************************************/
#define MAXFILES 6

extern char *file_names[];
extern char *element_names[];
extern int *file_table[];
extern int **index_table[];

struct	fd{
	int files[2];
	int *index_list;
	off_t cur;
	};
extern	struct fd fd_table[MAXFILES];
/*
	};
*/

void	database_open(char *path, int file_list[]);
void	database_close(void);
int	add_indexes(int file_number, off_t *value, void *buffer);
int	remove_indexes(int file_number, off_t *rnumber, void *buffer);
int	add_record(int file_number, void *buffer, int length);
int	remove_record(int file_number, off_t rnumber);
int	get_rec(int file_number, off_t *rnumber, void *buffer);
int	find_record(int file_number, int knumber, char *key, void *buffer);
int	verify_record(int file_number, int knumber, char *key);
int	first_record(int file_number, int knumber, void *buffer);
int	last_record(int file_number, int knumber, void *buffer);
int	next_record(int file_number, int knumber, void *buffer);
int	previous_record(int file_number, int knumber, void *buffer);
int	return_record(int file_number, void *buffer);
int	current_record(int file_number, int knumber, void *buffer);
int	sequential_record(int file_number, void *buffer);
int	fill_record(void *s_buffer, void *d_buffer, int s_list[], int d_list[]);
int	return_element_position(void *buffer, int element, int element_list[]);
int	return_record_length(int file_number, off_t rnumber);
void	clear_record_buffer(void *buffer, int element_list[]);
void	init_record(int file_number, void *buffer);
