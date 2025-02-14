void	dfile_create(char *filename);
void	dfile_open(char *filename, int fd[]);
void	dfile_close(int fd[]);

off_t	new_record(int fd[], void *buffer, int len);
void	delete_record(int fd[], off_t rnumber);

int	put_record(int fd[], off_t rnumber, void *buffer, int len);
int	get_record(int fd[], off_t rnumber, void *buffer);
int	get_length(int fd[], off_t rnumber);

void	place_record(int fd, off_t offset, void *buffer, int len);
void	grab_record(int fd, off_t offset, void *buffer, int len);

struct	cont_head{
	off_t nextrn;
	off_t avlist;
	};

struct	cont_record{
	off_t rnumber;
	off_t rfile_offset;
	off_t avlist;
	off_t len;
	};

