/***************************************************************************************
	btree.c : This is the redone btree algorthim file
***************************************************************************************/

/*--------	DEFINITIONS ----------------*/
#ifndef	OK
#define OK 0
#define ERROR -1
#define TRUE 1
#define FALSE 0
#endif

/*MAX == K*/
#define	K 5
#define	X_LEN 20
#define U -1

/*-----------	STRUCTURES USED ----------*/
struct	pair{
	/*key is x*/
	char x[X_LEN];
	off_t v;
	};

union	element{
	struct pair pair;
	off_t page;
	};

struct	header{
	off_t current[2];
	int i;
	off_t s;
	off_t root;
	off_t next;
	off_t avlist;	
	};

struct	page{
	/*index 2K pairs and 2K + 1 sons*/
	union element elements[((2 * (2 * K)) + 1) + 2];
	off_t parent;
	off_t left;
	off_t right;
	int key_count;
	};


/*-----------	FUNCTION PROTOTYPES ------*/
/*---------- GLOBAL PROTOTYPES ------------*/
void	btree_create(char *filename);
int	btree_open(char *filename);
int	btree_close(int fd);

int	btree_retrieval(int fd, char *y);
void	btree_insert(int fd, char *y, off_t v);
void	btree_delete(int fd, char *y);

struct	header btree_increment(int fd, void (*function)(struct page *, int));
int	btree_transverse(int fd, void (*function)(struct page *, int));

int	btree_first(int fd);
int	btree_last(int fd);
int	btree_next(int fd);

void	btree_empty(struct page *p, int i);
void	btree_printkeyval(struct page *p, int i);

void	btree_edit(int fd, char *y, off_t v);
off_t	btree_v(int fd, char *y);
