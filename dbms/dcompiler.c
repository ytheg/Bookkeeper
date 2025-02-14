/*
 *	Dcompiler: Used to compile the Schema
 *		<input>: filename.sch
 *		<output>: 
 *			1) filename.c : 
 *					Contains the arrays to enums and arrays to arrays
 *			2) filename.h :
 *					Contains the enums & char * arrays
 * 
 *	Issues: Memory leaks, unclean code. The logic of the program, too many functions
 *		doesnt account for variability in the size of the data
 *		poorly written code, must be reviewed and rewritten in parts or all
 *		review the issues seriously over every detail
 *		
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define	FIRST	1
#define	LAST	0
#define	MAXELEMENTS	100
#define	MAXFILES	100
#define	MAXKEYS	100
#define	MAXCATS 3
#define	FOUND	1
#define	NOTFOUND	-1
#define	SUCCESS	0
#define	FAILURE	-1

/* -- error codes enum */
enum	error_code{
	ED_DIC = 1,
	ED_DIC_IN,
	ED_FULL,
	ED_FILE_IO,
	ED_FILE_EX,
	ED_WORD_SV,
	ED_KEY_NOT,
	ED_KEY_SV,
	ED_USAGE,
	ED_ALREADY,
	ED_SCHEMA,
	ED_NULL,
	ED_TERMINAL
	};

/* -- error codes messages struct */
struct	error_struct{
	enum error_code erc;
	char *error_message;
	} erc[] =	{
			ED_DIC, "Not in Dictionary",
			ED_DIC_IN, "Already in Dictionary",
			ED_FULL, "Data structure full",
			ED_FILE_IO, "File I/O issue",
			ED_FILE_EX, "File already stored",
			ED_WORD_SV, "Word already stored",
			ED_KEY_NOT, "Key not element",
			ED_KEY_SV, "Key already stored",
			ED_USAGE, "Incorrect usage of program",
			ED_ALREADY, "Already in data structure",
			ED_SCHEMA, "Not in Schema",
			ED_NULL, "NULL",
			ED_TERMINAL, NULL
			};


void	dp_dictionary(char *buffer, off_t *position);
void	dp_file(char *buffer, char *line, off_t *position);
void	dp_key(char *buffer);
void	dw_hfile(void);
void	dw_cfile(void);
void	dsae_print(int fd, char *title, char **array, int end_flag, char *delim);
void	mdsae_print(int fd, char *title, int array[]);

void	derror(enum error_code code);
char	*read_line(char *buffer, off_t *position);
char	*read_word(char *line, int flag);
void	array_clear(char *array[], int value);
int	array_search(char *array[], int s, char *string);
int	array_insert(char *array[], int s, char *string);
void	array_print(char *array[], int s);
void	array_zero(int array[], int s);
int	array_search_int(int array[], int s, int value);
int	array_insert_int(int array[], int s, int value);

/* -- global variables */
char	*dict_array[MAXELEMENTS];
char	*fname_array[MAXFILES];
int	ftable_array[MAXFILES][MAXELEMENTS];
int	ktable_array[MAXFILES][MAXKEYS][MAXCATS];


/* -- main : open the file for reading and read output the results */
int	main(int argc, char *argv[]){
	/*start*/
	/*grab the file name*/
	if	(argc > 1){
		char *filename, *suffix, *filenamep, *buffer;
		off_t position, pl;

		filename = (char *)malloc(strlen(argv[1])+1);
		strncpy(filename, argv[1], strlen(argv[1])+1);

		/*open the file*/
		int fd_sch;
		fd_sch = open(filename, O_RDONLY);
		if	(fd_sch == -1){
			derror(ED_FILE_IO);
			perror("open");
			}

		/*parse the filename up to the delimiter .sch and save its name*/
		suffix = strchr(filename, '.');
		/*error check filenamp*/
		int length = suffix - filename;
		filenamep = (char *)malloc(length + 1);
		strncpy(filenamep, filename, length);

			/*
			printf("filename: %s\nParsed string : %s\n", filename, filenamep);
			*/
		
		/*read in buffer*/
		pl = position = lseek(fd_sch, 0, SEEK_END);
		position = lseek(fd_sch, 0, SEEK_SET);

		buffer = (char *)malloc(pl + 1);

		size_t count;
		count = read(fd_sch,(void *)buffer, (size_t)pl);

		buffer[pl + 1] = '\0';

		printf("Count: %d, Buffer: \n%s\nCurrent Position: %d\n", count, buffer, position);

	
		/*clear the file array*/
		array_clear(fname_array, MAXFILES);

		/*zero the keys arrays*/
		for	(int j = 0; j < MAXFILES; j++){
			for	(int m = 0; m < MAXKEYS; m++){
				for	(int r = 0; r < MAXCATS; r++){
					ktable_array[j][m][r] = -1;
					}
				}
			}


		char *line;
		line = read_line(buffer, &position);


		/*while not 'END'*/
		while	(line != NULL && (strncmp(line, "#schema END", 11) != 0)){
			/*read line*/
			if	(line != NULL){
				printf("Line: %s\nPosition: %d\n", line, position);

				/*if the file contains '#dictionary' - call dp_dictionary*/
				if	(strncmp(line, "#dictionary", 12) == 0){
					dp_dictionary(buffer, &position);
					}
					/*else if the file contains '#file' - call dp_file*/
					else	if	(strncmp(line, "#file", 5) == 0){
							dp_file(buffer, line, &position);
							}
					/*else if the file contains '#key' - call dp_key*/
					else	if	(strncmp(line, "#key", 4) == 0){
							dp_key(line);
							}
				}

			/*nake sure to clean up the char * line variable
			 * its being allocated storage when read_line is called*/

			free(line);
			line = read_line(buffer, &position);
			}


		/*write .h file*/
		dw_hfile();
		/*write .c file*/
		dw_cfile();

		/*clean up*/
		free(filename);
		free(filenamep);
		free(buffer);

		/*clear arrays*/
		array_clear(dict_array, MAXELEMENTS);

		/*close files*/
		if	(fd_sch != -1){
			close(fd_sch);
			}
		}
		else	{
			derror(ED_USAGE);
			}
	}

/* -- dictionary */
void	dp_dictionary(char *buffer, off_t *position){
	char *line;
	line = read_line(buffer, position);

	char *word;
	int f;
	
	/*NULL out the dict array*/
	array_clear(dict_array, MAXELEMENTS);

	while	(line != NULL && (strncmp(line, "#dictionary end", 16) != 0)){
		/*grab word*/
		word = read_word(line, LAST);

		//printf("\tLINE : %s, word: %s\n", line, word);
		
		/*loop through the dictionary*/
		f = array_search(dict_array, MAXELEMENTS, word);
			/*if the word already in the dictionary*/
			if	(f > -1){
				/*error check*/
				derror(ED_DIC_IN);
				}
				/*else save it*/
				else	{
					array_insert(dict_array, MAXELEMENTS, word);
					}

		/*clean up*/
		free(line);
		free(word);

		/*read the next line*/
		line = read_line(buffer, position);
		}
	array_print(dict_array, MAXELEMENTS);
	}

/* -- file definitions */
void	dp_file(char *buffer, char *line, off_t *position){
	/*grab filename*/
	char *filename, *fline, *word;
	filename = read_word(line, LAST);

	fline = read_line(buffer, position);
	int f, ai;

	if	(filename != NULL){
		printf("File name: %s\n", filename);

		
		f = array_search(fname_array, MAXFILES, filename);

		/*if the filename is already saved(vaildate the filename)*/
		if	(f > -1){
			/*error check*/
			derror(ED_FILE_EX);
			}
			/*else - save the filename*/
			else	{
				ai = array_insert(fname_array, MAXFILES, filename);

				/*zero out the ai array*/
				array_zero(ftable_array[ai], MAXELEMENTS);

				/*while - the line does not contain '#file END'*/
				while	(fline != NULL && (strncmp(fline, "#file end", 16) != 0)){
					/*grab word from line*/
					word = read_word(fline, LAST);
					/*if the word is in the dictionary*/
					f = array_search(dict_array, MAXELEMENTS, word);

					if	(f > -1){
						/*if the word is already saved to the file*/
						if	(array_search_int(ftable_array[ai], MAXELEMENTS, f) > -1){
							/*error check - continue*/
							derror(ED_ALREADY);
							}
							/*else - save the word to the file*/
							else	{
								array_insert_int(ftable_array[ai], MAXELEMENTS, f);					
								}
						}
						else	{
							/*error check and continue*/
							derror(ED_DIC);
							}


					/*grab line*/
					free(fline);
					free(word);

					fline = read_line(buffer, position);
					}
				}
		}

	/*clean up*/
	free(filename);

	/*print array*/
	for	(int i = 0; ftable_array[ai][i] != -1; i++){
		printf("%s ", dict_array[ftable_array[ai][i]]);
		}
	printf("\n");

	array_print(fname_array, MAXFILES);
	}

/* -- key bindings */
void	dp_key(char *line){
	/*grab filename*/
	char *filename;
	filename = read_word(line, FIRST);

	int f;

	/*if not in schema?*/
	if	((f = array_search(fname_array, MAXFILES, filename)) == -1){
		/*error check - return*/
		derror(ED_SCHEMA);
		return;
		}

	/*grab word*/
	char *keys;
	keys = read_word(line, LAST);

	int keysarray[MAXCATS];
	array_zero(keysarray, MAXCATS);

	char *key1, *key2;

	/*if comma*/
	if	((key2 = strchr(keys, ',')) != NULL){
		/*split word at ',' delimiter ?? */
		int i;
		for	(i = 0; keys[i] != ','; i++);
		key1 = (char *)malloc(i + 1);
		key2 = key2 + 1;
		strncpy(key1, keys, i);
		/*check if key is in the dictionary*/
		keysarray[0] = array_search(dict_array, MAXELEMENTS, key1);
		keysarray[1] = array_search(dict_array, MAXELEMENTS, key2);
		free(key1);
		}
		else	{
			/*check if key is in the dictionary*/
			keysarray[0] = array_search(dict_array, MAXELEMENTS, keys);
			}

	free(keys);

	/*if already saved to file keys*/
		/*error check - return*/
	/*else - save word to file*/

	int inpoint;
	for	(inpoint = 0; ktable_array[f][inpoint][0] != -1; inpoint++);

	for	(int j = 0; ktable_array[f][inpoint][j] < MAXCATS && keysarray[j] != -1; j++){
		ktable_array[f][inpoint][j] = keysarray[j];
		}
	}

/* -- write the .h file */
void	dw_hfile(void){
	/*create the .h file using the filename // or should it just be ddl?*/
	int fd;

	fd = open("ddl.h", O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if	(fd == -1){
		derror(ED_FILE_IO);
		}
	
	/*print the header of the file*/
	//char *header = "/*\n *\tddl.h: This is the header file to the DBMS contains the enums\n */\n\n\n";
	//write(fd, header, strlen(header));

	/*print the enum element index title using SAE function using the dictionary*/
	dsae_print(fd, "enum\telement_index{\n", dict_array, 0, "");

	/*print the file name array using SAE function using the dictionary*/
	dsae_print(fd, "\n\nenum\tfile_index{\n", fname_array, 0, "");

	char *includes = "#include \"dml.h\"\n\n\n";
	write(fd, includes, strlen(includes));

	close(fd);
	}

/* -- write the .c file */
void	dw_cfile(void){
	/*create the files*/
	int fd;

	fd = open("ddl.c", O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if	(fd == -1){
		derror(ED_FILE_IO);
		}

	/*print the includes*/
	char *includes = "#include <stddef.h>\n#include <stdio.h>\n#include \"ddl.h\"\n\n\n";
	write(fd, includes, strlen(includes));

	/*print the filesnames array using the SAE function*/
	dsae_print(fd, "char\t*file_names[] = {\n", fname_array, 1, "\"");

	/*print the element names array using the SAE function*/
	dsae_print(fd, "char\t*element_names[] = {\n", dict_array, 1, "\"");

	char *lfilename;
	int length, j;
	
	char *suf_name[MAXFILES];
	char *title;

	for	(int i = 0; fname_array[i] != NULL; i++){
		/*make the filename[i] to lower case and suffix it with '_f'*/
		length = strlen(fname_array[i]);
		lfilename = (char *)malloc(length + 3);
		for	(j = 0; j < length; j++){
			lfilename[j] = tolower(fname_array[i][j]);
			}

		lfilename[j++] = '_';
		lfilename[j++] = 'f';
		lfilename[j] = '\0';

		/*print to the file the elements in the file as an array using SAE*/
		title = (char *)malloc(j + 13);
		sprintf(title, "int\t%s[] = {\n", lfilename);
		
		mdsae_print(fd, title, ftable_array[i]);
		
		free(title);

		/*store the lower cased filename into an array*/
		suf_name[i] = lfilename;
		suf_name[i + 1] = NULL;
		}

	/*print the suffexed filenames using the SAE function*/
	dsae_print(fd, "int\t*file_table[] = {\n", suf_name, 1, ""); 

	char *lfilenamex, *titlex, *suf_namex[MAXFILES];
	char *suf_namei[MAXFILES];
	/*while the files are not the end of the files*/
	for	(int i = 0; fname_array[i] != NULL; i++){
		/*make the filename[i] to lower case*/
		length = strlen(fname_array[i]);
		lfilenamex = (char *)malloc(length + 3);
		for	(j = 0; j < length; j++){
			lfilenamex[j] = tolower(fname_array[i][j]);
			}

		/*
		lfilenamex[j++] = '_';
		lfilenamex[j++] = 'f';
		lfilenamex[j] = '\0';
		*/
	
		char *suf_namex2[MAXKEYS], *titlei;
		for	(int m = 0; ktable_array[i][m][0] != -1; m++){
			titlex = (char *)malloc(j + 15);
			sprintf(titlex, "int\t%s_x%d[] = {\n", lfilenamex, m);
			
			mdsae_print(fd, titlex, ktable_array[i][m]);

			printf("%s\n", titlex);

			free(titlex);

			/*store array titles*/
			suf_namex2[m] = (char *)malloc(j + 4);
			sprintf(suf_namex2[m], "%s_x%d", lfilenamex, m);
			suf_namex2[m + 1] = NULL;
			}

		/*store the filename with the i suffex*/
		/*store arrays into titles*/
		suf_namei[i] = (char *)malloc(length + 3);
		sprintf(suf_namei[i], "%s_i", lfilenamex);

		suf_namei[i + 1] = NULL;

		/*write the array of the index arrays of that file*/
		titlei = (char *)malloc(strlen(suf_namei[i]) + 13);
		sprintf(titlei,"int\t*%s[] = {\n", suf_namei[i]); 
		dsae_print(fd, titlei, suf_namex2, 1, ""); 

		free(titlei);
		}

	/*print the ** index array using the SAE*/
	dsae_print(fd, "int\t**index_table[] = {\n", suf_namei, 1, ""); 
	

	close(fd);
	}

/* -- write structure, array, enum */
void	dsae_print(int fd, char *title, char **array, int end_flag, char *delim){
	/*check that the file is open*/
	/*print the title*/
	write(fd, title, strlen(title));

	for	(int i = 0; array[i] != NULL; i++){
		printf("title: %s = String = %s\n", title, array[i]);
		}
	printf("Breakpoint\n");

	char *string;
	int length;
	/*loop the array - while is not end*/
	for	(int i = 0; array[i] != NULL; i++){
		/*sprint element with delimiter*/
		length = strlen(array[i]) + 1;
		if	(strlen(delim) > 0){
			length = length + 2;
			}
		string = (char *)malloc(length + 1);
		sprintf(string, "\t%s%s%s", delim, array[i], delim);
		write(fd, string, length);
		/*if not last*/
		if	(array[i + 1] != NULL){
			/*print ',\n'*/
			write(fd, ",\n", 2);
			}
			/*else print '\n};'*/
			else	{
				if	(end_flag == 1){
					write(fd, ",\n\tNULL\n", 8);
					}
					else	{
						write(fd, "\n", 1);
						}
				}
		free(string);
		}
	write(fd, "\t};\n\n\n", 6);
	}

/* -- write array using 2 dimensional array to file*/
void	mdsae_print(int fd, char *title, int array[]){
	printf("MDSAE Function\n");
	write(fd, title, strlen(title));

	char *string;
	int length;

	/*loop the array - while is not end*/
	for	(int i = 0; array[i] != -1; i++){
		/*sprint element with delimiter*/
		length = strlen(dict_array[array[i]]) + 1;

		string = (char *)malloc(length + 3);
		sprintf(string, "\t%s", dict_array[array[i]]);
		write(fd, string, length);

		/*if not last*/
		if	(array[i + 1] != -1){
			/*print ',\n'*/
			write(fd, ",\n", 2);
			}
			/*else print '\n};'*/
			else	{
				write(fd, ",\n\t-1\n", 6);
				}
		free(string);
		}
	write(fd, "\t};\n\n\n", 6);
	}

/* ++ read line **/
char	*read_line(char *buffer, off_t *position){
	/*find position of newline*/
	char *newline;
	newline = strchr(buffer + (int) (*position), '\n');
	/*error check here*/
	if	(newline == NULL){
		derror(ED_NULL);
		return NULL;
		}
	
	int length = newline - (buffer + (int) (*position));

	char *line_proper = (char *)malloc(length + 1);
	strncpy(line_proper, (buffer + (int) (*position)), length);

	*position = *position + length + 1;

	line_proper[length] = '\0';

	return line_proper;
	}

/* ++ read word **/
char	*read_word(char *line, int flag){
	char *word;
	/*switch*/
	switch	(flag){
		/*first*/
		case	FIRST:
			/*return the first occurence of the space or tab*/
			word = strchr(line, '\t');
			break;
		/*last*/
		case	LAST:
			word = strrchr(line, '\t');
			break;
		/*return the last occurence of the space or tab*/
		}

	if	(word == NULL){
		return NULL;
		}

	word = word + 1;
	int i;
	for	(i = 0; (isalpha(word[i])) || (word[i] == '_') || (word[i] == ','); i++);

	char *cword;
	cword = (char *)malloc(i + 1);
	strncpy(cword, word, i);

	cword[i] = '\0';

	return cword;
	}

/* -- array clearing */
void	array_clear(char *array[], int s){
	for	(int i = 0; i < s; i++){
		if	(array[i] != NULL){
			free(array[i]);
			}
		array[i] = NULL;
		}
	}

/* -- array zero */
void	array_zero(int array[], int s){
	for	(int i = 0; i < s; i++){
		if	(array[i] != -1){
			array[i] = -1;
			}
		}
	}

/* -- search array */
int	array_search(char *array[], int s, char *string){
	for	(int i = 0; i < s; i++){
		if	(array[i] != NULL){
			if	(strncmp(array[i], string, strlen(string)) == 0){
				return i;
				}
			}
		}
	return NOTFOUND;
	}

/* -- search int array */
int	array_search_int(int array[], int s, int value){
	for	(int i = 0; i < s; i++){
		if	(array[i] != -1){
			if	(array[i] == value){
				return i;
				}
			}
		}
	return NOTFOUND;
	}

/* -- insert into array */
int	array_insert(char *array[], int s, char *string){
	for	(int i = 0; i < s; i++){
		if	(array[i] == NULL){
			/*allocate copy and return*/
			int length = strlen(string);
			array[i] = (char *)malloc(length + 1);
			strncpy(array[i], string, length);
			array[i][length] = '\0';
			return i;
			}
		}
	/*error check - return status*/
	return FAILURE;
	}

int	array_insert_int(int array[], int s, int value){
	for	(int i = 0; i < s; i++){
		if	(array[i] == -1){
			array[i] = value;
			return i;
			}
		}
	return FAILURE;
	}

/* -- print array */
void	array_print(char *array[], int s){
 	for	(int i = 0; i < s; i++){
		if	(array[i] != NULL){
			printf("%d: %s\n", i, array[i]);
			}
		}
	}

/* -- error checking */
void	derror(enum error_code code){
	/*loop through the error code struct, while its not TERMINAL ERROR*/
	int i;
	for	(i = 0; erc[i].erc != ED_TERMINAL; i++){
		/*if code test for the code*/
		if	(erc[i].erc == code){
			/*print the error*/
			printf("Error: %s\n", erc[i].error_message);
			/*return*/
			return;
			}
		}
	/*error check and return*/
	printf("Error: TERMINAL\n");
	}


