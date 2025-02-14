/*
*	dbinit.c : file to initialise dbms files according to the schema
*		   call this whenever making a new database
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dml.h"
#include "dfile.h"
#include "btree.h"

/*----	open the necessary files according to the ddl found in the dml using dfile functions */
int	main(int argc, char *argv[]){
	
	if	(argc > 1){
		/*the directory name for the database is in argv[]*/
		/*for the file names*/
		char *filename_c;
		char *indexname_c;

		for	(int i = 0; file_names[i] != NULL; i++){
			printf("Creating files for: %s\n", file_names[i]);
			filename_c = (char *)malloc(strlen(argv[1]) + strlen(file_names[i]) + 2);
			sprintf(filename_c, "%s%s", argv[1], file_names[i]);

			/*this function creates the 2 files name & name.cont*/
			dfile_create(filename_c);
			//dfile_create(file_names[i]);

			/*make this a function in the dml file*/
			for	(int j = 0; index_table[i][j] != NULL; j++){
				indexname_c = (char *)malloc(strlen(filename_c) + 3);
				sprintf(indexname_c,"%s.0%d", filename_c, j);
	
				btree_create(indexname_c);
					
				//printf("\t%s", indexname_c);
				free(indexname_c);
				}

			free(filename_c);
			//printf("\n");
			}
		}
		else	{
			printf("Usage: dbinit path\n");
			}

	return 0;
	}
