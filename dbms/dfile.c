/*******************************************************************
	dfile.c : datafile management software functions.
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "dfile.h"
#include "logfile.h"

/*----- File Creation function --------*/
void	dfile_create(char *filename){
	remove(filename);

	int cont_fd, fd;
	
	char *cont_fn = (char *)malloc(strlen(filename) + strlen(".cont") + 1);
	strncpy(cont_fn, filename, strlen(filename) + 1);
	strncat(cont_fn, ".cont", strlen(".cont")+1);

	cont_fd = open(cont_fn, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	free(cont_fn);
	if	(cont_fd == -1 || fd == -1){
		log_message(FAILURE, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "UNABLE TO OPEN FILES");
		return;
		}

	struct cont_head hd;
	hd.nextrn = 1;
	hd.avlist = -1;
	write(cont_fd, (void *)&hd, sizeof(struct cont_head));
	
	close(cont_fd);
	close(fd);
	}


/*----- Open Files return descriptor ------*/
void	dfile_open(char *filename, int fd[]){
	char *cont_fn = (char *)malloc((strlen(filename) + strlen(".cont"))+1);
	sprintf(cont_fn, "%s.cont", filename);	

	fd[0] = open(cont_fn, O_RDWR);
	fd[1] = open(filename, O_RDWR);
	free(cont_fn);
	if	(fd[0] == -1 || fd[1] == -1){
		log_message(FAILURE, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "UNABLE TO OPEN FILES");
		return;
		}
		
	struct cont_head hd;
	read(fd[0], (void *)&hd, sizeof(struct cont_head));
	}

/*----- Close Files Using Descriptors -----*/
void	dfile_close(int fd[]){
	if	(close(fd[0]) == -1 || close(fd[1]) == -1){
		log_message(FAILURE, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "UNABLE TO CLOSE FILE");
		exit(2);
		}
	log_message(SUCCESS, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "FILES CLOSED");
	}

/*----- Record Manipulation Functions ------*/

/*----- New Record --------------------------------*/
off_t	new_record(int fd[], void *buffer, int len){
	off_t record_number;
	struct cont_head hd;

	if	(lseek(fd[0], 0, SEEK_SET) == -1){
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "UNABLE TP SEEK IN FILE");
		}
	read(fd[0], (void *)&hd, sizeof(struct cont_head));

	struct cont_record cont_rec;

	if	(hd.avlist != -1){
		struct cont_record ptr;
		grab_record(fd[0], (sizeof(struct cont_head) + (hd.avlist * (sizeof(struct cont_record)))), (void *)&ptr, sizeof(struct cont_record));
		if	(ptr.len == len){
			cont_rec = ptr;
			hd.avlist = cont_rec.avlist;
			}
			else	{
				int sub = 0;
				struct cont_record preptr = ptr;
				while	(ptr.avlist != -1){
					preptr = ptr;
					grab_record(fd[0], (sizeof(struct cont_head) + (ptr.avlist * (sizeof(struct cont_record)))), (void *)&ptr, sizeof(struct cont_record));
					if	(ptr.len == len){
						sub = 1;
						}
					}
				if	(sub){
					preptr.avlist = ptr.avlist;
					place_record(fd[0], (sizeof(struct cont_head) + (preptr.rnumber * (sizeof(struct cont_record)))), (void *)&preptr, sizeof(struct cont_record));
					cont_rec = ptr;
					}
					else	{
						record_number = hd.nextrn++;
						cont_rec.rnumber = record_number;
						cont_rec.rfile_offset = lseek(fd[1], 1, SEEK_END);
						cont_rec.len = len;
						}
				}
		
		}
		else	{
			record_number = hd.nextrn++;
			cont_rec.rnumber = record_number;
			cont_rec.rfile_offset = lseek(fd[1], 1, SEEK_END);
			cont_rec.len = len;
			}

	cont_rec.avlist = 0;

	place_record(fd[0], (sizeof(struct cont_head) + ((cont_rec.rnumber) * (sizeof(struct cont_record)))) ,(void *)&cont_rec, sizeof(struct cont_record));
	place_record(fd[0], 0, (void *)&hd, sizeof(struct cont_head));

	place_record(fd[1], cont_rec.rfile_offset, buffer, cont_rec.len);

	//t_records(fd[0]);
	return cont_rec.rnumber;
	}

/*----- Put Record -----------------------------------------------*/
int	put_record(int fd[], off_t rnumber, void *buffer, int len){
	struct cont_head hd;
	grab_record(fd[0], 0, (void *)&hd, sizeof(struct cont_head));

	if	(rnumber > hd.nextrn){
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "RECORD OFFSET LARGER THEN RECORD LIST");
		return -1;
		}

	struct cont_record cont_rec;
	grab_record(fd[0], ((sizeof(struct cont_head) + (rnumber * (sizeof(struct cont_record))) )), (void *)&cont_rec, sizeof(struct cont_record));

	place_record(fd[1], cont_rec.rfile_offset, buffer, cont_rec.len);
	return 1;
	}

/*----- Get Length --------------------------*/
int	get_length(int fd[], off_t rnumber){
	struct cont_head hd;
	grab_record(fd[0], 0, (void *)&hd, sizeof(struct cont_head));

	if	((rnumber >= hd.nextrn) || rnumber == -1){
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "RECORD OFFSET LARGER THEN RECORD LIST/RECORD OFFSET == -1");
		return -1;
		}

	struct cont_record cont_rec;
	grab_record(fd[0], ((sizeof(struct cont_head) + (rnumber * (sizeof(struct cont_record))) )), (void *)&cont_rec, sizeof(struct cont_record));
	
	return cont_rec.len;
	}

/*----- Get Record ------------------------------------------------*/
int	get_record(int fd[], off_t rnumber, void *buffer){
	struct cont_head hd;
	grab_record(fd[0], 0, (void *)&hd, sizeof(struct cont_head));

	if	((rnumber >= hd.nextrn) || rnumber == -1){
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "RECORD OFFSET LARGER THEN RECORD LIST/RECORD OFFSET == -1");
		return -1;
		}

	struct cont_record cont_rec;
	grab_record(fd[0], ((sizeof(struct cont_head) + (rnumber * (sizeof(struct cont_record))) )), (void *)&cont_rec, sizeof(struct cont_record));

	if	(cont_rec.avlist == 0){
		grab_record(fd[1], cont_rec.rfile_offset, buffer, cont_rec.len);
		log_message(SUCCESS, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "RECORD IS NOT DELETED");
		return 1;
		}
		else	{
			log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "RECORD IS DELETED");
			return -1;
			}
	}

/*----- Delete Record ----------------------------*/
void	delete_record(int fd[], off_t rnumber){
	struct cont_head hd;
	grab_record(fd[0], 0, (void *)&hd, sizeof(struct cont_head));
	
	if	(rnumber >= hd.nextrn){
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "RECORD OFFSET LARGER THEN RECORD LIST");
		return;
		}
	
	struct cont_record cont_rec;
	grab_record(fd[0], ((sizeof(struct cont_head) + (rnumber * (sizeof(struct cont_record))) )), (void *)&cont_rec, sizeof(struct cont_record));

	if	(cont_rec.avlist != 0){
		log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "RECORD IS DELETED");
		return;
		}

	cont_rec.avlist = hd.avlist;
	hd.avlist = rnumber;

	place_record(fd[0], 0, (void *)&hd, sizeof(struct cont_head));
	place_record(fd[0], ((sizeof(struct cont_head) + (rnumber * (sizeof(struct cont_record))) )), (void *)&cont_rec, sizeof(struct cont_record));
	
	//t_records(fd[0]);
	}

/*----- Place Record ----------------------------------------------*/
void	place_record(int fd, off_t offset, void *buffer, int len){
	lseek(fd, offset, SEEK_SET);
	write(fd, buffer, len);
	}

/*----- Grab Record -----------------------------------------------*/
void	grab_record(int fd, off_t offset, void *buffer, int len){
	lseek(fd, offset, SEEK_SET);
	read(fd, buffer, len);
	}

/*----- Testing Functions ---------------*/
void	t_records(int fd){
	struct cont_head hd;
	lseek(fd, 0, SEEK_SET);
	read(fd, (void *)&hd, sizeof(struct cont_head));
	
	//printf("RECORD HEAD === NEXTRN = %d, AVLIST = %d\n", hd.nextrn, hd.avlist);

	for	(int i = 1; i < hd.nextrn; i++){
		struct cont_record cont_rec;
		lseek(fd, (sizeof(struct cont_head) + ((i) * (sizeof(struct cont_record)))), SEEK_SET);
		read(fd, (void *)&cont_rec, sizeof(struct cont_record));
		//printf("RECORD NUMBER = %d, RECORD_LEN = %d, RECORD_FILE_OFFSET = %d, AVALIST = %d\n", (int)cont_rec.rnumber, (int) cont_rec.len, (int)cont_rec.rfile_offset, (int)cont_rec.avlist);
		}
	}
