/************************************************************************************
	dml.c : database manipulation functions used by the DBMS
************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include "btree.h"
#include "dfile.h"
#include "ddl.h"
#include "dml.h"
#include "logfile.h"

/*maybe add the functionn prototypes here*/
struct fd fd_table[MAXFILES];

/*----- Data Manipulation Functions --------*/

/*----- Clear the Current Array of fd Structs ----*/
static	void clear_struct(void){
	for	(int z = 0; z < MAXFILES; z++){	
		fd_table[z].index_list = NULL;
		}
	memset(fd_table, '\0', sizeof(struct fd) * MAXFILES);
	}

/*----- Initialise the Index files ---------*/
static	void init_index(char *path, int file_number){
	int i = 0;
	for	(;index_table[file_number][i] != NULL; i++);
	//printf(": number of files = %d\n", i);
	log_message(DEFAULT_STATUS, DEFAULT_TEXT, __FILE__, (char *)__FUNCTION__, __LINE__, NULL);

	if	((fd_table[file_number].index_list = (int *)malloc(sizeof(i) + 1)) == NULL){
		log_message(FAILURE, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "index_list ALLOCATION");
		exit(2);
		}

	log_message(SUCCESS, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "index_list ALLOCATION");

	char *full_name;
	for	(int j = 0; j < i; j++){
		if	((full_name = (char *)malloc(strlen(path) + strlen(file_names[file_number]) + strlen(".00")+1)) == NULL){
			log_message(FAILURE, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "full_name ALLOCATION");
			}
		log_message(SUCCESS,MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "full_name ALLOCATION");

		sprintf(full_name, "%s%s.0%d", path, file_names[file_number], j);
		
		/*test in order to build change to btree create*/
		/*
			btree_build(full_name);
			btree_create(full_name);
		*/
		
		/*change to btree_open*/
		/*
		fd_table[file_number].index_list[j] = btree_init(full_name);
		*/
	
		fd_table[file_number].index_list[j] = btree_open(full_name);

		free(full_name);
		log_message(SUCCESS, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "full_name FREE");
		}
	fd_table[file_number].index_list[i] = -1;
	}

/*----- Close File Index to Specific -------------*/
static	void close_index(int file_number){
	if	(fd_table[file_number].index_list != NULL){
		int i = 0;
		while	(fd_table[file_number].index_list[i] != -1){
			btree_close(fd_table[file_number].index_list[i++]);
			}
		free(fd_table[file_number].index_list);
		log_message(SUCCESS, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "index_list FREE");

		fd_table[file_number].index_list = NULL;
		}
	}

/*----- Open the Database ------------------*/
void	database_open(char *path, int file_list[]){
	clear_struct();

	char *full_name;
	while	(*file_list != -1){
		if	((full_name = (char *)malloc(strlen(path) + strlen(file_names[*file_list]) + 1)) == NULL){
			log_message(FAILURE, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "full_name ALLOCATION");
			}
		log_message(SUCCESS, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "full_name ALLOCATION");

		sprintf(full_name, "%s%s", path, file_names[*file_list]);
	
		/*test functions by creating the necessary files remove this call to file_create*/	
		/*
			file_create(full_name);
		*/

		/*change this call to dfile open*/
		/*
			file_open(full_name, fd_table[*file_list].files);
		*/
		dfile_open(full_name, fd_table[*file_list].files);

		init_index(path, *file_list);	

		free(full_name);
		log_message(SUCCESS, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "full_name FREE");
	
		file_list++;
		}
	}

/*----- Close the Database (why does the function only close the index files? )---------*/
void	database_close(void){
	for	(int i = 0; i < MAXFILES; i++){
		if	(fd_table[i].files[0] != -1 && fd_table[i].files[1] != -1){
			close(fd_table[i].files[0]);
			close(fd_table[i].files[1]);
			close_index(i);
			}
		}
	memset(fd_table, '\0', sizeof(struct fd) * MAXFILES);
	}

/*----- Add Indexes ----------------------*/
int	add_indexes(int file_number, off_t *value, void *buffer){
	char key[200] = {0};

	for	(int i = 0; index_table[file_number][i] != NULL; i++){
		for	(int j = 0; index_table[file_number][i][j] != -1; j++){
			/*why is the key being written this way? - fix*/
			/*
			strncat(key, (char *)( (void *)(buffer) + ((int) *((char *)(buffer) + index_table[file_number][i][j])) ) , 
				strlen( (char *)((void *)(buffer) + ((int) *((char *)buffer) + index_table[file_number][i][j])) ));
			*/
			}
		sprintf(key, "%d", *value);
		//printf("Key == %s ----> file = %d - value == %d\n", key, i, *value);
		log_message(DEFAULT_STATUS, DEFAULT_TEXT, __FILE__, (char *)__FUNCTION__, __LINE__, key);

		/*adjust the btree_insert function to call properly*/
		/*
		btree_insert(fd_table[file_number].index_list[i], key, *value, !i);
		*/

		btree_insert(fd_table[file_number].index_list[i], key, *value);
		memset(key, '\0', sizeof(char) * 200);
		}
	return 1;
	}

/*----- Remove Indexes ---------------------------*/
int	remove_indexes(int file_number, off_t *rnumber, void *buffer){
	char key[200] = {0};

	for	(int i = 0; index_table[file_number][i] != NULL; i++){
		for	(int j = 0; index_table[file_number][i][j] != -1; j++){
			strncat(key, (char *)( (void *)(buffer) + ((int) *((char *)(buffer) + index_table[file_number][i][j])) ) , 
				strlen( (char *)((void *)(buffer) + ((int) *((char *)buffer) + index_table[file_number][i][j])) ));
			}
		//printf("Key to Delete == %s ----> file = %d\n", key, i);
		log_message(DEFAULT_STATUS, DEFAULT_TEXT, __FILE__, (char *)__FUNCTION__, __LINE__, key);

		/*adjusr the btree_delete function to call properly*/
		/*
		btree_delete(fd_table[file_number].index_list[i], key, *rnumber);
		*/
		btree_delete(fd_table[file_number].index_list[i], key);
		
		memset(key, '\0', sizeof(char) * 200);
		}
	free(buffer);
	log_message(SUCCESS, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "buffer FREE");

	return 1;
	}

/*----- Add a Record to the File ---------*/
int	add_record(int file_number, void *buffer, int length){
	off_t value;

	/*add the relate function*/
	value = new_record(fd_table[file_number].files, buffer, length);

	/*error check the add_indexes call*/
	/*
	add_indexes(file_number, &value, buffer);
	*/
	if	(add_indexes(file_number, &value, buffer) == 1){
		return 1;
		}
	return -1;
	}

/*----- Remove record (not working on the current record) ----------------*/
int	remove_record(int file_number, off_t rnumber){
	void *buffer;
	if	((buffer = malloc(get_length(fd_table[file_number].files, rnumber))) == NULL){
		log_message(FAILURE, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "buffer ALLOCATION");
		}
	log_message(SUCCESS, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "buffer ALLOCATION");

	/*adjust the get record call after fixing it in dfile.h/dfile.c */
	/*
	get_record(fd_table[file_number].files, rnumber, buffer, 0);
	*/
	get_record(fd_table[file_number].files, rnumber, buffer);

	delete_record(fd_table[file_number].files, rnumber);
	remove_indexes(file_number, &rnumber, buffer);
	}

/*----	Remove the current record, test this function ---------*/
int	remove_cur_record(int file_number, off_t rnumber){
	void *buffer;
	
	if	(fd_table[file_number].cur != -1){
		if	((buffer = malloc(get_length(fd_table[file_number].files, fd_table[file_number].cur))) == NULL){
			return -1;
			}
		
		get_record(fd_table[file_number].files, fd_table[file_number].cur, buffer);
		delete_record(fd_table[file_number].files, fd_table[file_number].cur);
		remove_indexes(file_number, &(fd_table[file_number].cur), buffer);
		}
	}

/*----- Get Record -------------------------*/
int	get_rec(int file_number, off_t *rnumber, void *buffer){
	/*adjust the call to get record to not include the length parameter*/
	/*
	if	(get_record(fd_table[file_number].files, *rnumber, buffer, 0) == -1){
	*/
	if	(get_record(fd_table[file_number].files, *rnumber, buffer) == -1){
		log_message(DEFAULT_STATUS, DEFAULT_TEXT, __FILE__, (char *)__FUNCTION__, __LINE__, "NO RECORD FOUND");
		return -1;
		}
	log_message(DEFAULT_STATUS, DEFAULT_TEXT, __FILE__, (char *)__FUNCTION__, __LINE__, "RECORD FOUND");

	fd_table[file_number].cur = *rnumber;
	return 1;
	}

/*----- Find Record ------------------------*/
int	find_record(int file_number, int knumber, char *key, void *buffer){
	off_t value;

	if	(knumber == -1){	
		log_message(DEFAULT_STATUS, DEFAULT_TEXT, __FILE__, (char *)__FUNCTION__, __LINE__, "FIND RECORD FROM SPECIFIC INDEX");
		for	(int i = 0; fd_table[file_number].index_list[i] != -1; i++){
			/*adjust the btree function call*/
			/*
			if	((value = btree_search(fd_table[file_number].index_list[i], key)) != -1){
			*/
			if	((value = btree_v(fd_table[file_number].index_list[i], key)) != -1){
				break;
				}
			}
		}
		else	{
			/*adjust the btree function call*/
			log_message(DEFAULT_STATUS, DEFAULT_TEXT, __FILE__, (char *)__FUNCTION__, __LINE__, "FIND RECORD SEARCHED INDEX");
			/*
			value = btree_search(fd_table[file_number].index_list[knumber], key);	
			*/
			value = btree_v(fd_table[file_number].index_list[knumber], key);	
			}

	return get_rec(file_number, &value, buffer);;
	}

/*----- Verify Record ----------------------*/
int	verify_record(int file_number, int knumber, char *key){
	off_t value = -1;
	
	if	(knumber == -1){
		log_message(DEFAULT_STATUS, DEFAULT_TEXT, __FILE__, (char *)__FUNCTION__, __LINE__, "VERIFYING RECORD FROM SPECIFIC INDEX");
		for	(int i = 0; fd_table[file_number].index_list[i] != -1; i++){
			/*adjust the call to the btree function*/
			/*
			if	((value = btree_search(fd_table[file_number].index_list[i], key)) != -1){
			*/
			if	((value = btree_v(fd_table[file_number].index_list[i], key)) != -1){
				break;
				}
			}
		}
		else	{
			log_message(DEFAULT_STATUS, DEFAULT_TEXT, __FILE__, (char *)__FUNCTION__, __LINE__, "VERIFYING RECORD SEARCHED INDEX");
			/*adjust the call to the btree function*/
			/*
			value = btree_search(fd_table[file_number].index_list[knumber], key);
			*/
			value = btree_v(fd_table[file_number].index_list[knumber], key);
			}

	return (value != -1) ? 1 : -1;
	}

/*----- First Record Retrieve -------------*/
int	first_record(int file_number, int knumber, void *buffer){
	off_t value;

	if	((value = btree_first(fd_table[file_number].index_list[knumber])) == -1){
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "NO FIRST RECORD FOUND");
		return -1;
		}

	return get_rec(file_number, &value, buffer);;
	}

/*----- Last Record Retrieve ---------------*/
int	last_record(int file_number, int knumber, void *buffer){
	off_t value;

	if	((value = btree_last(fd_table[file_number].index_list[knumber])) == -1){
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "NO LAST RECORD FOUND");
		return -1;
		}

	return get_rec(file_number, &value, buffer);
	}


/*----- Next Record Retrieve --------------*/
int	next_record(int file_number, int knumber, void *buffer){
	off_t value;

	if	((value = btree_next(fd_table[file_number].index_list[knumber])) <= 0){
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "NO NEXT RECORD FOUND");
		return -1;
		}

	return get_rec(file_number, &value, buffer);
	}

/*----- Previous Record Retrieve ------------*/
int	previous_record(int file_number, int knumber, void *buffer){
	off_t value;

	/*add the btree previous*/
	/*
	if	((value = btree_previous(fd_table[file_number].index_list[knumber])) <= 0){
	*/
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "NO PREVIOUS RECORD FOUND");
		return -1;
	/*
		}
	*/

	return get_rec(file_number, &value, buffer);
	}


/*----- Return Record ----------------------*/
int	return_record(int file_number, void *buffer){	
	if	(fd_table[file_number].cur == '\0'){
		return -1;
		}

	void *buffer_old;
	if	((buffer_old = malloc(get_length(fd_table[file_number].files, fd_table[file_number].cur))) == NULL){
		log_message(FAILURE, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "buffer_old ALLOCATION");
		}
	log_message(SUCCESS, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "buffer_old ALLOCATION");

	/*adjust the get record call remove the length parameter*/
	/*
	get_record(fd_table[file_number].files, fd_table[file_number].cur, buffer_old, 0);
	*/
	get_record(fd_table[file_number].files, fd_table[file_number].cur, buffer_old);
	remove_indexes(file_number, &(fd_table[file_number].cur), buffer_old);

	free(buffer_old);
	log_message(SUCCESS, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "buffer_old FREE");

	int rtn;
	if	((rtn = add_indexes(file_number,  &(fd_table[file_number].cur), buffer)) == 1){
		put_record(fd_table[file_number].files, fd_table[file_number].cur, buffer, get_length(fd_table[file_number].files, fd_table[file_number].cur));
		log_message(DEFAULT_STATUS, DEFAULT_TEXT, __FILE__, (char *)__FUNCTION__, __LINE__, "Indexes and Records Added");
		}

	return rtn;
	}

/*----- Current Record ------------------*/
int	current_record(int file_number, int knumber, void *buffer){
	off_t value;	

	/*adjust the call for to the btree_keyvalue function*/
	/*
	if	((value = btree_keyvalue(fd_table[file_number].index_list[knumber], NULL)) == -1){
	*/
	if	((value = btree_v(fd_table[file_number].index_list[knumber], NULL)) == -1){
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "NO CURRENT RECORD FOUND");
		return -1;
		}
	
	return get_rec(file_number, &value, buffer);
	}


/*----- Sequential Record Phyiscal ----------------*/
int	sequential_record(int file_number, void *buffer){
	off_t rnumber = fd_table[file_number].cur;

	/*adjust the get record length parameter*/
	/*
	return get_record(fd_table[file_number].files, ++rnumber, buffer, 0);
	*/
	return get_record(fd_table[file_number].files, ++rnumber, buffer);
	}


/*----- Record Fill -----------------------------*/
int	fill_record(void *s_buffer, void *d_buffer, int s_list[], int d_list[]){
	int len = 0;
	for	(; s_list[len] != -1; len++);

	/*maybe make this array[len + 1]*/
	char array[len];
	
	for	(int i = 0; s_list[i] != -1; i++){
		for	(int j = 0; d_list[j] != -1; j++){	
			if	(s_list[i] == d_list[j]){
				if	(j == 0){
					array[j] = len;
					}
					else	{
						array[j] = (array[j - 1]) + (strlen( (char *)((void *)d_buffer + array[j - 1]) ) ) + 1;
						}

				memcpy(d_buffer + array[j], (char *)((void *)s_buffer +  ((int) *((char *)(s_buffer + s_list[i])) )), 
					strlen((char *)((void *)s_buffer +  ((int) *((char *)(s_buffer + s_list[i])) ))) + 1);
				}
			}
		}

	memcpy(d_buffer, array, sizeof(char) * len);
	}

/*----- Return Element Position --------------------*/
int	return_element_position(void *buffer, int element, int element_list[]){
	for	(int i = 0; element_list[i] != -1; i++){
		if	(element_list[i] == element){
			//printf("%d = %d, %s\n", i, (int) *((char *)(buffer + i)), (char *)(buffer + ((int) *((char *)(buffer + i)))));
			log_message(SUCCESS, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, NULL);
			return (int) *((char *)(buffer + i));
			}
		}
	log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, NULL);
	return -1;
	}

/*----- Return Record Length -------------*/
int	return_record_length(int file_number, off_t rnumber){
	return get_length(fd_table[file_number].files, rnumber);
	}


/*----- Clear The Record Buffer ---------*/
void	clear_record_buffer(void *buffer, int element_list[]){
	int len = 0;
	for	(int i = 0; element_list[i] != -1; i++){	
		len = strlen((char *)buffer + (int)*((char *)buffer + i) );
		memset(buffer + (int)*((char *)buffer + i), '\0', len);
		}
	log_message(SUCCESS, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, NULL);
	}

/*----- Initialise Record ---------------*/
void	init_record(int file_number, void *buffer){
	clear_record_buffer(buffer, file_table[file_number]);
	}
