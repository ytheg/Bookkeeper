/***************************************************************************************
	btree.c : This is the redone btree algorthim file
	
	issues: Underflow/overflow is not completed
		the number of file reads/writes could be reduced with pointers
		current key is not present as a function
		there isnt a decrementing or previous function
***************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "btree.h"
#include "logfile.h"

/*---------- STATIC PROTOTYPES ------------*/
static	void btree_underflow(int fd, int insert, off_t pagep, off_t pagen, off_t pageq);
static	void btree_split(int fd, struct header h, union element entry[]);
static	void btree_concate(int fd, off_t page);

static	void btree_seek(int fd, off_t offset, int len);

static	void btree_write(int fd, off_t offset, void *buffer, int len);
static	void btree_read(int fd, off_t offset, void *buffer, int len);

static	int btree_page(struct page p, char *y);

static	void btree_zero(struct page *p);
static	void btree_print(struct page p);

static	void btree_count_num(int *count);
static	int btree_count(struct page p);

static	int btree_separate(char *string, long *num, int *a, int *b);
static	int btree_cmp(char *y, char *x);

static	void btree_insertion_permutations(int fd);


/*-----------	FILE FUNCTIONS -----------*/
/*-----	CREATE BTREE FILE -------*/
void	btree_create(char *filename){
	remove(filename);
	int fd = open(filename, O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if	(fd == -1){
		log_message(FAILURE, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "UNABLE TO BUILD INDEX FILES");
		}

	struct header *hd;

	if	((hd = (struct header *)malloc(sizeof(struct header))) == NULL){
		log_message(FAILURE, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "struct header *hd ALLOCATION");
		exit(2);
		}

	memset(hd, -1, sizeof(struct header));
	hd->current[0] = hd->current[1] = hd->avlist = 0;
	hd->next = 0;
	hd->root = U;

	btree_write(fd, 0, (void *)hd, sizeof(struct header));

	free(hd);
	/*
	log_message(SUCCESS, MEMORY, __FILE__, (char *)__FUNCTION__, __LINE__, "struct header *hd FREE");
	*/

	if	(close(fd) == -1){
		log_message(FAILURE, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "UNABLE TO CLOSE FILE");
		}
	}

/*-----	OPEN BTREE FILE ---------*/
int	btree_open(char *filename){
	int fd = open(filename, O_RDWR);
	if	(fd == -1){
		log_message(FAILURE, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "UNABLE TO OPEN INDEX FILES");
		return ERROR;
		}
	
	return fd;
	}

/*-----	CLOSE BTREE FILE --------*/
int	btree_close(int fd){
	if	(close(fd) == -1){
		log_message(FAILURE, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "UNABLE TO CLOSE FILE");
		}
		else	{
			/*
			log_message(SUCCESS, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "CLOSE FILE");
			*/
			return OK;
			}
	}

/*-----------	CONTROLLING FUNCTIONS -----------*/
/*-----	RETRIVAL ALGORITHM ------*/
int	btree_retrieval(int fd, char *y){
	/*start*/
	struct header h;
	struct page p;
	off_t p_off;

	int cmp, cmpin;

	btree_read(fd, 0, (void *)&h, sizeof(struct header));

	/*p <- r, s <- u*/
	p_off = h.root;
	h.s = U;
	h.i = 1;

	/*loop*/
	while	(TRUE){
		/*p = u?*/
		if	(p_off == U){
			/*stop - y not in index*/
			btree_write(fd, 0, (void *)&h, sizeof(struct header));
			return -1;
			}
		
		/*s <- p*/
		h.s = p_off;

		btree_read(fd, p_off, (void *)&p, sizeof(struct page));

		/*page search loop*/
		for	(int i = 1; i < (p.key_count * 2); i += 2){
			h.i = i;
			cmp = btree_cmp(y, p.elements[i].pair.x);

			/*y < x1?*/
			if	(cmp < 0){
				/*p <- p(p0)*/
				p_off = p.elements[0].page;

				/*break page search loop*/
				break;
				}

			/*Ei(y = xi)?*/
			if	(cmp == 0){
				/*stop - y found*/
				btree_write(fd, 0, (void *)&h, sizeof(struct header));
				return 0;
				}

			if	((i + 2) <= (p.key_count * 2)){
				cmpin = btree_cmp(y, p.elements[i + 2].pair.x);
				}
				else	{
					cmpin = -1;
					}

			/*E|i(xi < y < xi+1)?*/
			if	((cmp > 0) && (cmpin < 0)){
				/*p <- p(pi)*/
				p_off = p.elements[i + 1].page;

				h.i += 2;
	
				/*break page search loop*/
				break;
				}

			/*p <- p(pl)*/
			p_off = p.elements[((p.key_count * 2) + 1)].page;
			}
		}

	}

/*-----	INSERTION ALGORITHM -----*/
void	btree_insert(int fd, char *y, off_t v){
	/*start*/
	struct header h;
	struct page p;

	union element entry[2];
	strncpy(entry[0].pair.x, y, strlen(y) + 1);
	entry[0].pair.v = v;
	entry[1].page = U;

	int found = 0;

	/*apply retrieval algorthim for y*/
	found = btree_retrieval(fd, y);

	/*y = found?*/
	if	(found != -1){
		/*yes - stop*/
		return;
		}

	btree_read(fd, 0, (void *)&h, sizeof(struct header));

	/*no - s = u?*/
	if	(h.s == U){
		/*yes - (tree empty) create root page with entry*/
		h.root = h.s = 0;
		h.next++;
		
		memset(p.elements, U, (sizeof(union element) * ((2 * (2 * K)) + 1)));
		p.left = p.right = p.parent = -1;

		memmove(p.elements + 1, entry, (sizeof(union element) * 2));

		p.key_count = 1;
	
		/*print page - for debugging
		btree_print(p);
		*/

		/*stop*/
		btree_write(fd, 0, (void *)&h, sizeof(struct header));
		btree_write(fd, h.root, (void *)&p, sizeof(struct page));
		return;
		}

	btree_read(fd, h.s, (void *)&p, sizeof(struct page));

	/*P(s) >= 2K (full)*/
	if	(p.key_count >= (2 * K)){
		/*split routine for P(s)*/
		btree_split(fd, h, entry);
		return;
		}

	/*insert entry (y, u) in P(s)*/
	memmove(p.elements + (h.i + 2), p.elements + h.i, (sizeof(union element) * ((2 * p.key_count) - (h.i - 1))));

	memmove(p.elements + h.i, entry, (sizeof(union element) * 2));
	p.key_count++;

	/*print page - for debugging*/		
	/*
	btree_print(p);
	*/
	
	/*stop*/
	btree_write(fd, 0, (void *)&h, sizeof(struct header));
	btree_write(fd, h.s, (void *)&p, sizeof(struct page));
	}

/*-----	DELETION ALGORITHM ------*/
void	btree_delete(int fd, char *y){
	printf("Delete function = %s\n", y);
	/*start*/
	struct header h;
	struct page p, pl;

	union element entry[2];

	int found;
	off_t page, pageleaf;

	/*apply retrieval algorthim*/
	found = btree_retrieval(fd, y);

	/*y found?*/
	if	(found != 0){
		/*no - stop*/
		return;
		}


	btree_read(fd, 0, (void *)&h, sizeof(struct header));

	page = h.s;

	btree_read(fd, page, (void *)&p, sizeof(struct page));

	btree_print(p);

	/*yes - is y on leaf?*/
	if	(p.elements[h.i + 1].page == -1){
		/*yes - delete y from leaf*/
		memmove(p.elements + h.i, p.elements + h.i + 2, (sizeof(union element) * ((( 2 * p.key_count) + 1) - (h.i + 1))));
		p.key_count--;
		}
		else	{
		/*else*/
			pageleaf = p.elements[h.i + 1].page;

			btree_read(fd, pageleaf, (void *)&pl, sizeof(struct page));
			/*retrieve pages down leaf along P(p0)*/
			while	(pl.elements[0].page != -1){
				pageleaf = pl.elements[0].page;
				btree_read(fd, pageleaf, (void *)&pl, sizeof(struct page));
				}

			/*replace y by first key on leaf page*/
			memcpy(entry, pl.elements + 1, sizeof(union element));

			memmove(p.elements + h.i, entry, sizeof(union element));

			/*delete first key on leaf page*/
			memmove(pl.elements, pl.elements + 2, (sizeof(union element) * ((2 * pl.key_count) - 2)));

			btree_write(fd, pageleaf, (void *)&pl, sizeof(struct page));
			}

	btree_write(fd, page, (void *)&p, sizeof(struct page));

	/*if necessary perform concatenations & underflow*/
	btree_concate(fd, page);
	/*stop*/
	}

/*-----	UNDERFLOW (incomplete) ---------*/
static	void btree_underflow(int fd, int insert, off_t pagep, off_t pagen, off_t pageq){
	/*start*/
	struct page p, q, pn;

	union element entry[2];
	union element t[2 * (((2 * (2 * K)) + 1) + 2)];

	int t_key_count;

	/*read the pages*/
	btree_read(fd, pageq, (void *)&q, sizeof(struct page));
	btree_read(fd, pagep, (void *)&p, sizeof(struct page));
	btree_read(fd, pagen, (void *)&pn, sizeof(struct page));

	t_key_count = p.key_count;

	/*t <- P*/
	memmove(t, p.elements, (sizeof(union element) * ((2 * p.key_count) + 1)));

	/*entry <- Q(i + 2)*/
	memmove(entry, q.elements + insert, (sizeof(union element) * 2));
	entry[1].page = -1;
	
	/*move entry into P*/
	memmove(t + ((t_key_count * 2) + 1), entry, (sizeof(union element) * 2));

	t_key_count++;

	/*move P' into P*/
	memmove(t + ((t_key_count * 2) + 1), pn.elements, (sizeof(union element) * ((pn.key_count * 2) + 1)));

	t_key_count += pn.key_count;	

	/*move first part into p*/
	memmove(p.elements, t, (sizeof(union element) * ((2 * t_key_count) + 1)));
	p.key_count = btree_count(p);

	/*move middle into into entry*/
	memmove(entry, t + (((t_key_count) * 2) + 1), (sizeof(union element) * 2));
	entry[1].page = pagen;

	/*add entry into Q*/
	memmove(q.elements + insert, entry, (sizeof(union element) * 2));

	/*move into pn the second part*/
	memmove(pn.elements, t + (((t_key_count) * 2) + 1), (sizeof(union element) * 2));
	pn.key_count = btree_count(pn);

	/*write the pages*/
	btree_write(fd, pageq, (void *)&q, sizeof(struct page));
	btree_write(fd, pagep, (void *)&p, sizeof(struct page));
	btree_write(fd, pagen, (void *)&pn, sizeof(struct page));
	}

/*-----	OVERFLOW	---------*/

/*-----	SPLITING	---------*/
static	void btree_split(int fd, struct header h, union element entry[]){
	/*start*/
	struct page p, q, pn, pson;
	int insert, page;
	
	page = h.s;	


	btree_read(fd, page, (void *)&p, sizeof(struct page));
	/*loop*/
	while	(TRUE){
		/*find insertion point in P*/
		insert = btree_page(p, entry[0].pair.x);

		/*insert into P*/
		memmove(p.elements + (insert + 2), p.elements + insert, (sizeof(union element) * ((2 * p.key_count) - (insert - 1))));

		memmove(p.elements + insert, entry, (sizeof(union element) * 2));
		p.key_count++;

		/*print p for debugging*/
		/*
		btree_print(p);
		*/

		/*is P full?*/
		if	(p.key_count <= (2 * K)){
			/*no - write P*/
			btree_write(fd, page, (void *)&p, sizeof(struct page));
			/*stop*/
			return;
			}

		/*yes - does P have Q?*/
		if	(p.parent == -1){
			/*no - make new Q root*/
			if	(h.next != -1){
				h.root = h.next++;
				}
				else	{
					h.root = ++h.next;
					}

			memset(q.elements, U, (sizeof(union element) * ((2 * (2 * K)) + 1)));
			q.left = q.right = q.parent = -1;
			q.elements[0].page = page;

			q.key_count = 0;

			p.parent = h.root;
			}
			else	{
				/*else - read Q from file*/
				btree_read(fd, p.parent, (void *)&q, sizeof(struct page));
				}

		/*split*/
		memset(pn.elements, U, (sizeof(union element) * ((2 * (2 * K)) + 1)));

		memmove(pn.elements, p.elements + ((2 * K) + 2), (sizeof(union element) *  ((2 * K) + 1)));
		memmove(entry, p.elements + ((2 * K) + 1), sizeof(union element));
	
		memset(p.elements + ((2 * K) + 1), U, (sizeof(union element) * ((2 * K) + 1)));

		/*set entry*/
		entry[1].page = h.next++;

		/*write P & Pn*/
		p.right = entry[1].page;
		pn.parent = p.parent;
		pn.left = page;

		pn.key_count = btree_count(pn);
		p.key_count = btree_count(p);

		/*print for debugging*/
		/*
		printf("%d --->", p.parent);
		btree_print(q);
		printf("%d --->", page);
		btree_print(p);
		printf("%d --->", entry[1].page);
		btree_print(pn);
		*/

		btree_write(fd, page, (void *)&p, sizeof(struct page));
		btree_write(fd, entry[1].page, (void *)&pn, sizeof(struct page));

		/*loop through the sons of pn read them and change their parent value to that of pn position entry[1].page*/
		for	(int i = 0; i <= ((2 * pn.key_count) + 1); i += 2){
			if	(pn.elements[i].page != -1){
				btree_read(fd, pn.elements[i].page, (void *)&pson, sizeof(struct page));
				pson.parent = entry[1].page;
				btree_write(fd, pn.elements[i].page, (void *)&pson, sizeof(struct page));
				}
			}

		page = p.parent;
	
		memset(pn.elements, U, (sizeof(union element) * ((2 * (2 * K)) + 1)));
		pn.left = pn.right = pn.parent = -1;

		/*set P to Q - continue loop*/
		p = q;
		btree_write(fd, 0, (void *)&h, sizeof(struct header));
		}
	}

/*-----	CONCATENATION	---------*/
static	void btree_concate(int fd, off_t page){
	/*start*/
	struct page p, q, pn;
	union element entry[2];

	int insert;	
	off_t pageq, pagen;

	btree_read(fd, page, (void *)&p, sizeof(struct page));
	
	/*loop*/
	while	(TRUE){
		/*P(parent) ?*/
		if	(p.parent == -1){
			/*no - write P*/
			btree_write(fd, page, (void *)&p, sizeof(struct page));
			
			/*stop*/
			return;
			}
		pageq = p.parent;
		/*read Q <- P(parent)*/	
		btree_read(fd, pageq, (void *)&q, sizeof(struct page));

		/*find insert (Q, p.elements[1].pair.x)*/
		insert = btree_page(q, p.elements[1].pair.x);

		/*if Q(i + 1).page != -1*/
		if	(q.elements[insert + 1].page == -1){
			/*no - stop*/
			return;
			}

		/*read P' <- Q(i + 3)*/
		pagen = q.elements[insert + 1].page;
		btree_read(fd, pagen, (void *)&pn, sizeof(struct page));

		printf("Concate results = pagen %d- > ", pagen);
		btree_print(q);
		btree_print(p);
		btree_print(pn);

		/*if p'.key_count + p.key_count <= 2 * K?*/
		if	((pn.key_count + p.key_count) >= (2 * K)){
			/*no - underflow stop*/
			return;
			}

		/*entry <- Q(i + 2)*/
		memmove(entry, q.elements + insert, (sizeof(union element) * 2));
		
		/*move Q removing emptyed space*/
		memmove(q.elements + insert, q.elements + insert + 2, (sizeof(union element) * (q.key_count * 2) - 1));

		q.key_count--;

		/*move entry into P*/
		memmove(p.elements + ((p.key_count * 2) + 1), entry, (sizeof(union element) * 2));

		p.key_count++;

		/*move P' into P*/
		memmove(p.elements + ((p.key_count * 2) + 1), pn.elements, (sizeof(union element) * ((pn.key_count * 2) + 1)));

		/*zero out P'*/
		btree_zero(&pn);

		/*write P & Q & P'*/
		btree_read(fd, page, (void *)&p, sizeof(struct page));
		btree_read(fd, pageq, (void *)&q, sizeof(struct page));
		btree_read(fd, pagen, (void *)&pn, sizeof(struct page));
		
		/*P <- Q*/
		p = q;
		}
	}

/*-----------	TRANSVERSAL FUNCTIONS -----------*/
/*----- INCREMENT KEY/VALUE PAIR -*/
struct	header btree_increment(int fd, void (*function)(struct page *, int)){
	/*start*/
	struct header h;
	struct page p, pt;
	int page;
	union element entry[2];
	
	btree_read(fd, 0, (void *)&h, sizeof(struct header));
	btree_read(fd, h.s, (void *)&p, sizeof(struct page));
	
	/*is the a son in h.i + 1?*/
	page = p.elements[h.i + 1].page;
	if	(page != -1){
		/*while P(0) is != -1*/
		btree_read(fd, page, (void *)&p, sizeof(struct page));
		while	(p.elements[0].page != -1){
			/*read P(0) into P*/
			page = p.elements[0].page;
			btree_read(fd, page, (void *)&p, sizeof(struct page));
			}

		/*visit P(1), h.i = 1, h.s = page number*/
		h.i = 1;
		h.s = page;

		function(&p, h.i);

		/*stop*/
		btree_write(fd, 0, (void *)&h, sizeof(struct header));
		return h;
		}

	/*loop*/
	page = h.s;
	while	(TRUE){
		/*is there a key in h.i + 2?*/
		if	(*(p.elements[h.i + 2].pair.x) != -1 && *(p.elements[h.i + 2].pair.x) != -1 && ((h.i + 2) < (p.key_count * 2))){
			/*yes - visit P(h.i + 2), h.i += 2, h.s = page number*/
			h.s = page;
			h.i += 2;
			function(&p, h.i);
			/*stop*/
			btree_write(fd, 0, (void *)&h, sizeof(struct header));
			return h;	
			}

		/*no - is there a parent page?*/
		if	(p.parent == -1){
			/*no - go to the last key*/
			/*while P(l) != -1*/
			while	(p.elements[(2 * p.key_count) + 1].page != -1){
				/*read P <- P(l)*/
				page = p.elements[(2 * p.key_count) + 1].page;
				btree_read(fd, page, (void *)&p, sizeof(struct page));
				}
			/*visit (P(l - 1)), h.i = l - 1, h.s page number*/
			h.i = (2 * p.key_count);
			h.s = page;
			//function(&p, h.i);
			/*stop*/
			btree_write(fd, 0, (void *)&h, sizeof(struct header));
			return h;	
			}
			else	{
				/*yes - store key*/
				memcpy(entry, p.elements + h.i, sizeof(union element));

				/*read P(parent)*/
				page = p.parent;
				btree_read(fd, page, (void *)&p, sizeof(struct page));


				/*find insert point store as h.i, h.s = P(parent)*/
				h.s = page;
				h.i = (btree_page(p, entry[0].pair.x) - 2);
				/*
				printf("page = %d --> ", page);
				btree_print(p);
				btree_read(fd, p.parent, (void *)&pt, sizeof(struct page));
				btree_print(pt);
				*/
				}
		}
			
			
		
	}

/*-----	TRANSVERSE TREE	----------*/
int	btree_transverse(int fd, void (*function)(struct page *, int)){
	/*start*/

	/*go to first key*/
	struct header h, ret;
	struct page p, pl;

	int pagei;
	off_t pagel;
	
	
	btree_read(fd, 0, (void *)&h, sizeof(struct header));
	
	btree_read(fd, h.root, (void *)&p, sizeof(struct page));

	int pre, page;
	page = p.elements[0].page;
	pre = h.root;
	while	(TRUE){
		if	(page == -1){
			break;
			}
			else	{	
				pre = page;
				btree_read(fd, page, (void *)&p, sizeof(struct page));
				}
		page = p.elements[0].page;
		}

	function(&p, 1);

	h.s = pre;
	h.i = 1;

	btree_write(fd, 0, (void *)&h, sizeof(struct header));

	/*find last key*/
	pagel = h.root;

	btree_read(fd, pagel, (void *)&pl, sizeof(struct page));

	while	(pl.elements[(pl.key_count * 2)].page != -1){
		pagel = pl.elements[(pl.key_count * 2)].page;
		btree_read(fd, pagel, (void *)&pl, sizeof(struct page));
		}

	pagei = (pl.key_count * 2);

	/*print last key for debugging*/
	/*
	printf("LAST KEY == -->");
	btree_printkeyval(&pl, pagei - 1);
	printf("\n");
	*/

	int count = 1;

	/*loop incremement*/
	while	(TRUE){
		/*print key*/
		ret = btree_increment(fd, function);
		count++;
		if	(ret.i == (pagei - 1) && ret.s == pagel){
			break;
			}
		}
	
	return count;
	}

/*-----	FIRST KEY/VALUE PAIR -----*/
int	btree_first(int fd){
	struct header h;
	struct page p;


	btree_read(fd, 0, (void *)&h, sizeof(struct header));

	if	(h.root == -1){
		return -1;
		}

	btree_read(fd, h.root, (void *)&p, sizeof(struct page));

	if	(p.key_count == 0){
		return -1;
		}

	int pre, page;

	page = p.elements[0].page;
	pre = h.root;
	while	(TRUE){
		if	(page == -1){
			break;
			}
			else	{	
				pre = page;
				btree_read(fd, page, (void *)&p, sizeof(struct page));
				}
		page = p.elements[0].page;
		}

	h.s = pre;
	h.i = 1;


	btree_write(fd, 0, (void *)&h, sizeof(struct header));

	/*return the value in the first pair*/
	//if	(flag == 1){
		return p.elements[h.i].pair.v;
	//	}

	return 0;
	}

/*-----	LAST KEY/VALUE PAIR ------*/
int	btree_last(int fd){
	struct header h;
	struct page p;

	int page, pagei;

	btree_read(fd, 0, (void *)&h, sizeof(struct header));

	if	(h.root == -1){
		return -1;
		}

	page = h.root;

	btree_read(fd, page, (void *)&p, sizeof(struct page));

	if	(p.key_count == 0){
		return -1;
		}

	while	(p.elements[(p.key_count * 2)].page != -1){
		page = p.elements[(p.key_count * 2)].page;
		btree_read(fd, page, (void *)&p, sizeof(struct page));
		}

	pagei = (p.key_count * 2);

	h.i = pagei - 1;
	h.s = page;

	btree_write(fd, 0, (void *)&h, sizeof(struct header));
	
	return 0;
	}

/*-----	NEXT KEY/VALUE PAIR ------*/
int	btree_next(int fd){
	struct header h;
	
	btree_read(fd, 0, (void *)&h, sizeof(struct header));
	if	(h.root == -1){
		return -1;
		}

	h = btree_increment(fd, btree_printkeyval);


	/*the return value should be that of the current btree_v*/
	struct page p;
	btree_read(fd, h.s, (void *)&p, sizeof(struct page));

	return p.elements[h.i].pair.v;

	/*
	return 0;
	*/
	}

/*-----******** CURRENT KEY/VALUE PAIR ******---*/

/*-----------	HELPER FUNCTIONS ----------------*/
/*-----	SEEK IN FILE -------------*/
static	void btree_seek(int fd, off_t offset, int len){
	if	((len == sizeof(struct header)) && (offset == 0)){
		if	(lseek(fd, offset, SEEK_SET) == -1){
			log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "NOT FOUND");
			}
		}
		else	{
			if	(lseek(fd, (sizeof(struct header) + ((offset) * sizeof(struct page))), SEEK_SET) == -1){
				log_message(ERR, RUNTIME, __FILE__, (char *)__FUNCTION__, __LINE__, "NOT FOUND");
				}
			}
	}

/*-----	WRITE IN FILE ------------*/
static	void btree_write(int fd, off_t offset, void *buffer, int len){
	/*go to the desired position in the file*/
	btree_seek(fd, offset, len);

	/*write to file*/
	write(fd, buffer, len);
	}

/*-----	READ FROM FILE -----------*/
static	void btree_read(int fd, off_t offset, void *buffer, int len){
	/*go to the desired position in the file*/
	btree_seek(fd, offset, len);

	/*read from file*/
	read(fd, buffer, len);
	}


/*-----	SCAN PAGE ----------------*/
static	int btree_page(struct page p, char *y){
	int insert = 1, cmp, cmpin;
	for	(int i = 1; i < (p.key_count * 2); i += 2){
		insert = i;
		if	((*(p.elements[i].pair.x) != -1)){
			cmp = btree_cmp(y, p.elements[i].pair.x);
			}
			else	{
				cmp = -1;
				}

	
		/*y < x1?*/
		if	(cmp < 0){
			/*less than the current key*/
			/*break page search loop*/
			break;
			}

		/*Ei(y = xi)?*/
		if	(cmp == 0){
			/*stop - y found*/
			break;
			}

		if	((i + 2) <= (p.key_count * 2)){
			cmpin = btree_cmp(y, p.elements[i + 2].pair.x);
			}
			else	{
				cmpin = -1;
				}

		/*E|i(xi < y < xi+1)?*/
		if	((cmp > 0) && (cmpin < 0)){
			/*p <- p(pi)*/
			insert += 2;

			/*break page search loop*/
			break;
			}
		}

	return insert;
	}
	
/*-----	ZERO OUT THE PAGE --------*/
static	void btree_zero(struct page *p){
	memset(p->elements, U, (sizeof(union element) * ((2 * (2 * K)) + 1)));
	p->left = p->right = p->parent = -1;
	p->key_count = 0;
	}

/*-----	PRINT ENTIRE PAGE --------*/
static	void btree_print(struct page p){
	printf("[%d]", p.elements[0].page);
	for	(int i = 1; i < (p.key_count * 2); i += 2){
		printf("[%s,%d,%d]", p.elements[i].pair.x, p.elements[i].pair.v, p.elements[(i + 1)].page);
		}
	printf("\n");
	}

/*-----	EMPTY FUNCTION CALL -----*/
void	btree_empty(struct page *p, int i){
	}

/*-----	PRINT KEY/VALUE PAIR -----*/
void	btree_printkeyval(struct page *p, int i){
	printf("[%d][%s,%d,%d]\n", p->elements[i - 1].page, p->elements[i].pair.x, p->elements[i].pair.v, p->elements[i + 1].page);
	}

/*-----	COUNT KEY/VALUE PAIR -----*/
static	void btree_count_num(int *count){
	(*count)++;
	}

/*-----	COUNT KEY/VALUE IN PAGE --*/
static	int btree_count(struct page p){
	int count = 0;
	for	(int i = 1; (*(p.elements[i].pair.x) != -1) && (p.elements[i].pair.v != -1); i += 2){
		count++;
		}

	return count;
	}

/*-----	SEPARATE STRING/NUM FLAG -*/
static	int btree_separate(char *string, long *num, int *a, int *b){
	/*start*/
	char *str = string;
	char *ptr;
	long number;

	/*check for number start*/
	if	(isdigit(*str)){
		/*set a*/
		*a = 1;
		/*separate*/
		number = strtol(str, &ptr, 10);
		/*check for text end*/
		if	(*ptr != '\0'){
			/*set b*/
			*b = 0;
			}
			else	{
				*b = 1;
				}
		*num = number;

		/*stop*/
		return 0;
		}

	/*check for text start*/
	/*find the number part or end of string*/
	while	(isdigit(*str) == 0 && *str != '\0'){
		str++;
		}

	*a = 0;

	/*if number is in the string*/
	if	(*str != '\0'){
		number = strtol(str, &ptr, 10);

		/*a is text, b is nothing*/
		*b = 1;
		}
		else	{
			*b = 0;
			}

	*num = number;
	return 1;
	}

/*-----	COMPAIR KEY VS KEY -------*/
static	int btree_cmp(char *y, char *x){
	/*
	return ((atoi(y) - atoi(x)));
	*/

	int y_len, x_len;
	y_len = strlen(y);
	x_len = strlen(x);

	int a, b, c, d;
	long left, right;
	
	btree_separate(y, &left, &a, &b);
	btree_separate(x, &right, &c, &d);

	/*category 'A' 0000,0101,0100,0001*/
	if	((a == 0) && ((b == 0) || (b == 1)) && (c == 0) && ((d == 0) || (d == 1))){
		return strncmp(y, x, x_len > y_len? x_len : y_len);
		}

	/*category 'B' 0010,0011,0110,0111*/
	if	((a == 0) && ((b == 0) || (b == 1)) && (c == 1) && ((d == 0) || (d == 1))){
		return 2;
		}

	/*category 'C' 1000,1001,1100,1101*/
	if	((a == 1) && ((b == 0) || (b == 1)) && (c == 0) && ((d == 0) || (d == 1))){
		return -2;
		}

	/*category 'D' 1010,1011,1110,1111*/
	if	((a == 1) && ((b == 0) || (b == 1)) && (c == 1) && ((d == 0) || (d == 1))){
		return (left - right);
		}
	
	}

/*-----	PERMUTATION TEST --------*/
static	void btree_insertion_permutations(int fd){
	btree_insert(fd, "3", 2);
	btree_insert(fd, "2", 2);
	btree_insert(fd, "1", 2);
	btree_insert(fd, "0", 2);
	}


/*-----	EDIT ENTRY VALUE --------*/
void	btree_edit(int fd, char *y, off_t v){
	/*start*/
	struct header h;
	struct page p;

	int found = 0;

	/*apply retrieval algorthim for y*/
	found = btree_retrieval(fd, y);

	/*y = found?*/
	if	(found == -1){
		/*yes - stop*/
		return;
		}

	btree_read(fd, 0, (void *)&h, sizeof(struct header));

	btree_read(fd, h.s, (void *)&p, sizeof(struct page));

	p.elements[h.i].pair.v = v;
	
	btree_write(fd, h.s, (void *)&p, sizeof(struct page));
	btree_write(fd, 0, (void *)&h, sizeof(struct header));
	}

/*-----	RECORD NUMBER ----------*/
off_t	btree_v(int fd, char *y){
	/*start*/
	struct header h;
	struct page p;

	int found = 0;

	if	(y != NULL){
		/*apply retrieval algorthim for y*/
		found = btree_retrieval(fd, y);

		/*y = found?*/
		if	(found == -1){
			/*yes - stop*/
			return -1;
			}
		}

	btree_read(fd, 0, (void *)&h, sizeof(struct header));

	btree_read(fd, h.s, (void *)&p, sizeof(struct page));

	return p.elements[h.i].pair.v;
	}

/*----- TEST MAIN FUNCTION ---------------------*/
int	btree_test(void){
	
	/*create btree file*/
	btree_create("testfile");

	/*open btree file*/
	int fd = btree_open("testfile");

	/*retrive a key*/
	printf("Retrieval Algorthim = %d\n", btree_retrieval(fd, "btc"));

	/*insert 1 key*/
	/*
	btree_insert(fd, "1", 2);
	*/

	/*TEST - insertion permutations*/
	/*
	btree_insertion_permutations(fd);
	*/

	/*insert 1000 keys*/
	char y[10];
	for	(int i = 1; i <= 2900; i++){
		sprintf(y, "%d", i);
		btree_insert(fd, y, i);
		}

	/*delete 1 key*/
	/*
	btree_delete(fd, "1");
	btree_delete(fd, "2");
	btree_delete(fd, "3");
	btree_delete(fd, "4");
	*/
	
	/*count keys in tree*/
	printf("Count = %d\n", btree_transverse(fd, btree_printkeyval));

	/*delete x number of keys*/
	/*count keys in tree*/

	/*close btree file*/
	btree_close(fd);

	return 0;
	}
