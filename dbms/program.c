/*
*	program.c: implements the TUI aspects of the DBMS
*		   more specific for the double book keeping system
*
*		issues: not generalised and not made into library form
*			very specific to the problem at hand
*			schema migrations
*			does not have left right key capabilities when inputting data
*
*
*		possible layout:
*			src/
*			|------>input_handlers/
*			|	|------>dashboard.c
*			|	|------>accounts.c
*			|	|------>transactions.c
*			|
*			|------>ui/
*			|	|------>renders.c
*			|	|------>forms.c
*			|
*			|------>main.c
*
*
*		... some of the tasks that should be completed starting from rendering the accounts page
*		1) finish rendering the accounts page using database information
*		2) process information saved from form
*		3) finish the transactions page
*		4) add autocompletion when entring in forms/different data entry types
*		5) make it easier to use by adding more key handling
*		6) error checking
*		7) complete the remaining pages/or remove them
*		8) add a wallet page
*		9) status bar shit
*		10) split into files?
*		11) document program logic
* 
*
*/
#include <panel.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dirent.h>
#include "dml.h"
#include "ddl.h"

/*----	definitions ------*/
#ifndef	SUCCESS
#define	SUCCESS 0
#define FAILURE 1
#endif


/*----	should the windows be global variables? they are used everywhere*/
WINDOW	*windows_array[4];
PANEL	*panels_array[4];


/*-----	WINDOWS enum --------*/
enum	windows_enum{
	MENU_WINDOW = 0,
	MAIN_WINDOW,
	STATUS_WINDOW,
	FORM_WINDOW
	};


/*-----	color enum codes ----*/
enum	color_codes{
	WARNING_COLOR = 1,
	SUCCESS_COLOR,
	ACTIVE_COLOR,
	YELLOW_TEXT,
	BLUE_TEXT,
	RED_TEXT
	};

/*----	page enum codes 
	this can be used to generalise the program
	but the number of menu items are not acocunted for
	in the size of the terminal window. etc etc  ---*/
enum	page_codes{
	DASHBOARD_PAGE = 0,
	ACCOUNTS_PAGE,
	TRANSACTIONS_PAGE,
	REPORTS_PAGE,
	DB_PAGE,
	TOOLS_PAGE,
	HELP_PAGE,
	EXIT_PAGE
	};

enum	focus_code{
	FOCUS_MENU,
	FOCUS_WORKSPACE,
	};

enum	form_type{
	ACCOUNT_FORM,
	TRANSACTION_FORM,
	CURRENCY_FORM
	};


enum	field_type{
	TEXT_FIELD,
	DROPDOWN_FIELD,
	NUMBER_FIELD,
	ACTION_FIELD
	};


/*-----	Example of possible file formats ----*/
/*----- still need to add the entry structs
	and the exchange rate structs -------*/
struct	account_struct{
	char account_id[50];
	char parent_id[50];
	char currency_id[50];
	char account_number[50];
	char account_name[50];
	char account_type[50];
	char description[50];
	char created_at[50];
	char updated_at[50];
	};

struct	transaction_struct{
	char transaction_id[50];
	char base_currency_id[50];
	char transaction_date[50];
	char description[50];
	char reference_number[50];
	char created_at[50];
	char updated_at[50];
	};

struct	journal_struct{
	char entry_id[50];
	char transaction_id[50];
	char account_id[50];
	char currency_id[50];
	char amount[50];
	char exchange_rate[50];
	char entry_type[50];
	char created_at[50];
	};

struct	currency_struct{
	char CURRENCY_ID[50];
	char CURRENCY_CODE[50];
	char CURRENCY_SYMBOL[50];
	char CURRENCY_NAME[50];
	char DECIMAL_PLACES[50];
	};

struct	exchangerate_struct{
	char rate_id[50];
	char currency_id[50];
	char base_currency_id[50];
	char exchange_rate[50];
	char effective_date[50];
	};

/*--------------------------------------*/

struct	cco{
	int count;
	int capacity;
	int offset;
	};

struct	data_cache{
	int update;
	struct account_struct *accounts;
	struct transaction_struct *transactions;
	struct currency_struct *currencies;
	struct cco account_cco;
	struct cco transaction_cco;
	}data_cache;

struct	program_state{
	int running;	/*is the program running*/
	enum page_codes current_page;	/*what page are we currently in*/
	int n_refresh;	/*does the page need refresh*/
	int f_panel;
	int focus;
	int a_context;	/*what did I mean by this?? why is it necessary*/
	int s_account;	/*what is the selected accounts ID*/
	/*add a cache of the account list*/
	}page_state;

struct	form_field{
	int bi;
	char label[100];
	char buffer[200];	/*input buffer*/
	int y, x;		/*screen position*/
	int err;
	enum field_type type;
	};

struct	form_state{
	int visible;
	enum form_type type;
	int focus;
	int n_entry;
	int save;
	struct form_field fields[10];
	char options[100];
	}form_state;

char	*form_title[] =	{
			"ACCOUNT",
			"TRANSACTION",
			"CURRENCY",
			NULL
			};

char	*menu_items[] =	{
			"DASHBOARD",
			"ACCOUNTS",
			"TRANSACTIONS",
			"REPORTS",
			"DB",
			"TOOLS",
			"HELP",
			"EXIT",
			NULL,
			};

/*declare the status string??*/
char	*status_string[] =	{
			"DASHBOARD",
			"ACCOUNTS",
			"TRANSACTIONS",
			"REPORTS",
			"DB",
			"TOOLS",
			"HELP",
			"EXIT",
			};

char	error_string[100] = "_";

/*----	prototypes -----*/


/*------initialisation routines--------------------------------*/
/*----	parse command line (very limited argc control - very basic) ---*/
int	parse_cli(int argc, char *argv[]){
	DIR *directory;
	struct dirent *directory_struct;

	/*check for database file*/
	if	(argc > 1){
		directory = opendir(argv[1]);
		if	(directory == NULL){
			return -1;
			}

		while	((directory_struct = readdir(directory)) != NULL){
			if	(strcmp(directory_struct->d_name, ".") == 0 ||
				strcmp(directory_struct->d_name, "..") == 0){
				
				continue;
				}

			/*close the directory and return*/
			/*if path contains files*/
			/*return success*/
			closedir(directory);
			return SUCCESS;
			}
		/*else*/
		return FAILURE;
		/*return non exiting path*/
		}
		/*else : print usage message on stdout*/
		else	{
			printf("Usage: program DB_DIRECTORY\n");
			/*exit gracefully*/
			exit(1);
			}
	}

/*----	init ncurses ---*/
int	init_ncurses(void){
	/*init stdscr*/
	initscr();
	/*enable (raw) pass characters directly*/
	//raw();
	cbreak();
	/*enable keypad and colors*/
	keypad(stdscr, TRUE);
	noecho();

	/*make cursor not visible*/
	curs_set(0);
	
	if	(has_colors()){
		start_color();

		/*use default colors when using ncurses on terminal*/
		use_default_colors();

		/*set up the main color pairs etc*/
		init_pair(WARNING_COLOR, COLOR_BLACK, COLOR_RED);
		init_pair(SUCCESS_COLOR, COLOR_BLACK, COLOR_GREEN);
		init_pair(ACTIVE_COLOR, COLOR_BLACK, COLOR_YELLOW);
		init_pair(YELLOW_TEXT, COLOR_YELLOW, COLOR_BLACK);
		init_pair(BLUE_TEXT, COLOR_BLUE, COLOR_BLACK);
		init_pair(RED_TEXT, COLOR_RED, COLOR_BLACK);
		}	

	/*create main window and panels  - the program does not account
	for a larger number of windows*/
	/*windows and panels are (menu, main window, status bar*/
	/*newwin function parameters: nlines, ncolumns, begin y, being x*/
	windows_array[MENU_WINDOW] = newwin(3, COLS, 0, 0);
	windows_array[MAIN_WINDOW] = newwin(LINES - 5, COLS, 3, 0);
	windows_array[STATUS_WINDOW] = newwin(3, COLS, LINES - 3, 0);
	windows_array[FORM_WINDOW] = newwin((LINES - 10), 80, (LINES - (LINES - 10))/2, (COLS - 80)/2);

	/*put the windows into panels for easy handling*/ 
	panels_array[MENU_WINDOW] = new_panel(windows_array[MENU_WINDOW]);
	panels_array[MAIN_WINDOW] = new_panel(windows_array[MAIN_WINDOW]);
	panels_array[STATUS_WINDOW] = new_panel(windows_array[STATUS_WINDOW]);
	panels_array[FORM_WINDOW] = new_panel(windows_array[FORM_WINDOW]);

	}

/*----	init the page state ---*/
void	init_page_state(void){
	page_state.current_page = DASHBOARD_PAGE;	
	page_state.n_refresh = 1;
	page_state.f_panel = 0;
	page_state.focus = FOCUS_MENU;
	page_state.running = 1;

	}

/*-----	init the data cache ---*/
void	init_data_cache(void){
	/*clear out the cache struct*/
	data_cache = (struct data_cache){0};

	data_cache.update = 0;

	/*initialise the struct capacity, count, offset*/
	data_cache.account_cco.capacity = 50;
	data_cache.account_cco.count = 0;
	data_cache.account_cco.offset = 0;
	data_cache.transaction_cco = data_cache.account_cco;

	/*allocate storage for the arrays*/
	data_cache.accounts = (struct account_struct *) malloc((data_cache.account_cco.capacity) * sizeof(struct account_struct));
	data_cache.transactions = (struct transaction_struct*)malloc(data_cache.transaction_cco.capacity * sizeof(struct transaction_struct));
	}

/*----	init form defaults ----*/
void	init_form_defaults(struct form_state *f, int c_page){
	f->visible = 0;
	
	switch	(c_page){
		case	DASHBOARD_PAGE:
		case	ACCOUNTS_PAGE:
			f->fields[0] = (struct form_field){0,"ACCOUNT NAME", "", 5, 5, TEXT_FIELD};
			f->fields[1] = (struct form_field){0,"ACCOUNT TYPE", "", 7, 5, TEXT_FIELD};
			f->fields[2] = (struct form_field){0,"DESCRIPTION", "", 9, 5, TEXT_FIELD};
			f->fields[3] = (struct form_field){0,"CURRENCY", "", 11, 5, DROPDOWN_FIELD};
			f->n_entry = 4;
			break;

		case	TRANSACTIONS_PAGE:
			/*Add a time aspect to the transaction*/
			f->fields[0] = (struct form_field){0,"DATE", "", 5, 5, TEXT_FIELD};
			f->fields[1] = (struct form_field){0,"TXID", "", 7, 5, TEXT_FIELD};
			f->fields[2] = (struct form_field){0,"CR:DR", "", 9, 5, TEXT_FIELD};
			f->fields[3] = (struct form_field){0,"DESCRIPTION", "", 11, 5, TEXT_FIELD};
			f->fields[4] = (struct form_field){0,"AMOUNT", "", 13, 5, TEXT_FIELD};
			f->fields[5] = (struct form_field){0,"ACCOUNT", "", 15, 5, TEXT_FIELD};
			f->n_entry = 6;
			break;
		}

	for	(int i = 0; i < f->n_entry; i++){
		f->fields[i].err = 0;
		}

	f->save = 0;
	strcpy(f->options, "Move : [Arrow UP/DOWN]  |  Save : [F3]  |  Cancel : [F1]");
	};

/*----	DBMS connection ---*/
int	connect_dbms(char *path, int flag){
	/*to open all the files a the database can use the file_names array*/
	int j, i;
	for	(j = 0; file_names[j] != NULL; j++);
	int ftopen[j + 1];
	for	(i = 0; i < j; i++){
		ftopen[i] = i;
		}
	ftopen[i + 1] = -1;

	if	(flag == SUCCESS){
		/*open existing database*/
		database_open(path, ftopen);
		}
		else	{
			/*or create a new one*/
			char *dbinit_command;
			dbinit_command = (char *)malloc(strlen(path) + strlen("./dbinit") + 2);
			sprintf(dbinit_command, "./dbinit %s", path);
			if	(system(dbinit_command) == -1){
				/*error message, fix these error messages*/
				printf("Failed to initialize the empty database directory\n");
				exit(1);
				}

			free(dbinit_command);

			/*open the newly made directory*/	
			database_open(path, ftopen);
			}

	return 1;

	/*add any extra steps here*/

	/*schema migrations (later development step)*/
	}


/*------main application loop routines----------------------*/
/*------rendering function --------*/
/*	these functions should take
	into account the current state -*/
int	render_menu(void){
	int y, x;
	/*draw a box in the menu window*/
	box(windows_array[MENU_WINDOW], 0, 0);
	
	/*move cursor to a position that makes sense*/
	wmove(windows_array[MENU_WINDOW], 1, 3);

	/*print the menu title*/
	wattron(windows_array[MENU_WINDOW], A_BOLD);
	wprintw(windows_array[MENU_WINDOW], "MENU BAR");
	wattroff(windows_array[MENU_WINDOW], A_BOLD);

	getyx(windows_array[MENU_WINDOW], y, x);
	wmove(windows_array[MENU_WINDOW], y, x + 4);
	
	int style = (page_state.focus == FOCUS_MENU) ? A_REVERSE : COLOR_PAIR(ACTIVE_COLOR);

	/*--- draw the menu items according to the state*/
	for	(int i = 0; menu_items[i] != NULL; i++){
		if	(i == page_state.current_page){
			wattron(windows_array[MENU_WINDOW], style);
			wprintw(windows_array[MENU_WINDOW], "%s", menu_items[i]);
			wattroff(windows_array[MENU_WINDOW], style);
			}
			else	{
				wprintw(windows_array[MENU_WINDOW], "%s", menu_items[i]);
				}
		/*move the cursor 3 positions on the x*/
		getyx(windows_array[MENU_WINDOW], y, x);
		wmove(windows_array[MENU_WINDOW], y, x + 3);
		}

	wnoutrefresh(windows_array[MENU_WINDOW]);
	//show_panel(panels_array[MENU_WINDOW]);
	}


/*------workspace rendering functions ----*/
/*----	render the dash board page ----*/
void	render_dashboard_page(void){
	wattron(windows_array[MAIN_WINDOW], A_BOLD);
	mvwprintw(windows_array[MAIN_WINDOW], 2, 5, "Welcome to Double-Entry Book-Keeping");
	wattroff(windows_array[MAIN_WINDOW], A_BOLD);

	mvwprintw(windows_array[MAIN_WINDOW], (LINES - 20)/2, 5, "Your Database is currently %s. Start by:", "empty");
	mvwprintw(windows_array[MAIN_WINDOW], ((LINES - 20)/2)+2, 5, "1: Creating an Account: Press F1");
	mvwprintw(windows_array[MAIN_WINDOW], ((LINES - 20)/2)+3, 5, "2: Recording a Transaction");

	wattron(windows_array[MAIN_WINDOW], A_DIM);
	mvwprintw(windows_array[MAIN_WINDOW], LINES - 10, 5, "Recent transactions: %s","(No Recent Transactions)");
	wattroff(windows_array[MAIN_WINDOW], A_DIM);
	}


/*----	define the column positions for accounts*/
#define	X_ID_LABEL	4
#define	X_NAME_LABEL	12
#define	X_TYPE_LABEL	32
#define	X_CURRENCY_LABEL	42
#define	X_BALANCE_LABEL	52


/*----- table rendering function --------*/
void	print_accounts_table(void){
	/*ID - NAME - TYPE - CURRENCY - BALANCE (how should the balance be calculated*/
	/*the balance requires quering the database in a loop and adding up all the values 
	... that make up the balance using the account base currency and exchange rate at the time of said transaction*/
	/*... algorithm for printing the data in the list*/
	/*print the titles and the line*/
	/*ID - NAME - TYPE - CURRENCY - BALANCE*/
	mvwprintw(windows_array[MAIN_WINDOW], 4, X_ID_LABEL, "ID");
	mvwprintw(windows_array[MAIN_WINDOW], 4, X_NAME_LABEL, "NAME");
	mvwprintw(windows_array[MAIN_WINDOW], 4, X_TYPE_LABEL, "TYPE");
	mvwprintw(windows_array[MAIN_WINDOW], 4, X_CURRENCY_LABEL, "CURRENCY");
	mvwprintw(windows_array[MAIN_WINDOW], 4, X_BALANCE_LABEL, "BALANCE");
	
	/*-- horizontal line ---*/
	wattron(windows_array[MAIN_WINDOW], A_DIM);
	mvwhline(windows_array[MAIN_WINDOW], 5, 3, ACS_HLINE, (COLS - 3)/2);
	wattroff(windows_array[MAIN_WINDOW], A_DIM);
	}

/*----	render the accounts page ----*/
void	render_accounts_page(void){
	int y, x;
	y = 2;
	x = 5;
	
	page_state.a_context = 1;

	/*Title of the accounts page*/
	wattron(windows_array[MAIN_WINDOW], A_BOLD);
	mvwprintw(windows_array[MAIN_WINDOW], 2, 3, "ACCOUNTS LIST: %s", page_state.a_context == 1? "ALL": "SPECIFIC");
	wattroff(windows_array[MAIN_WINDOW], A_BOLD);
	mvwhline(windows_array[MAIN_WINDOW], 3, 3, ACS_HLINE, (COLS - 3)/2);


	/*change this to the program state alist variable contents*/
	if	(data_cache.account_cco.count == 0){
		wattron(windows_array[MAIN_WINDOW], A_DIM);
		mvwprintw(windows_array[MAIN_WINDOW], 5, 5, "NO ACCOUNTS CREATED - PRESS F1 TO CREATE NEW ACCOUNT");
		wattroff(windows_array[MAIN_WINDOW], A_DIM);
		}
		else	{
			print_accounts_table();
			}

	
	}
/*----	render the transactions page ----*/
/*----	render the reports page ----*/
/*----	render the db page ----*/
/*----	render the tools page ----*/
/*----	render the help page ----*/
/*----	render the exit page ---*/
void	render_exit(void){
	wattron(windows_array[MAIN_WINDOW], A_BOLD);
	mvwprintw(windows_array[MAIN_WINDOW], 2, 2, "EXIT");
	wattroff(windows_array[MAIN_WINDOW], A_BOLD);
	}

/*----	render workspaec unfer current context --*/
void	render_workspace(void){
	/*clear the window*/
	werase(windows_array[MAIN_WINDOW]);

	/*this function should be a switch statement that
	... controls the rendering by calling different screen rendering functions ---*/

	/*first draw a box*/
	box(windows_array[MAIN_WINDOW], 0, 0);

	/*call the switch statement to check the current page*/
	switch	(page_state.current_page){
		case	DASHBOARD_PAGE:
			/*call the dashboard rendering function*/
			render_dashboard_page();
			break;
		case	ACCOUNTS_PAGE:
			/*call the accounts rendering function*/
			render_accounts_page();
			break;
		case	TRANSACTIONS_PAGE:
			/*call the transactions rendering function*/
			break;
		case	REPORTS_PAGE:
			/*call the reports rendering function*/
			break;
		case	DB_PAGE:
			/*call the database rendering function*/
			break;
		case	TOOLS_PAGE:
			/*call the tools rendering function*/
			break;
		case	HELP_PAGE:
			/*call the help rendering function*/
			break;
		case	EXIT_PAGE:
			/*call the exit function*/
			render_exit();
			break;
		default:
			/*the default should maybe be the dashboard*/
			/*call the dash rendering function*/
		}


	/*place holder string for debugging*/
	mvwprintw(windows_array[MAIN_WINDOW], 0, 2,"%s", menu_items[page_state.current_page]);

	if	(page_state.n_refresh == 1){
		//wnoutrefresh(windows_array[MAIN_WINDOW]);
		}
	}

/*-----	render the status bar -----*/
/*		the status bar is not dynamic
		and does not show messages unique to the
		current situation yet (improve this) ----------*/
void	render_statusbar(void){
	int y, x;

	/*draw box around the status bar*/
	
	wattron(windows_array[STATUS_WINDOW], COLOR_PAIR(YELLOW_TEXT));
	box(windows_array[STATUS_WINDOW], 0, 0);
	wattroff(windows_array[STATUS_WINDOW], COLOR_PAIR(YELLOW_TEXT));
	
	/*move to a better position*/
	wmove(windows_array[STATUS_WINDOW], 0, 5);

	/*print the status message created*/
	wattron(windows_array[STATUS_WINDOW], COLOR_PAIR(WARNING_COLOR) | A_BOLD);
	wprintw(windows_array[STATUS_WINDOW], " STATUS_BAR ");
	wattroff(windows_array[STATUS_WINDOW], COLOR_PAIR(WARNING_COLOR) | A_BOLD);

	/*move forward abit*/
	//getyx(windows_array[STATUS_WINDOW], y, x);
	//wmove(windows_array[STATUS_WINDOW], y, x + 3);
	//wprintw(windows_array[STATUS_WINDOW], "  %s ", status_string[page_state.current_page]);

	if	(page_state.focus == FOCUS_WORKSPACE){
		wprintw(windows_array[STATUS_WINDOW], "  Context : %s ", status_string[page_state.current_page]);
		//wnoutrefresh(windows_array[STATUS_WINDOW]);
		//mvwprintw(windows_array[STATUS_WINDOW], 2, 2, "Context Changed");
		wprintw(windows_array[STATUS_WINDOW], "  Error : %s ", error_string);
		}
		else	{
			wprintw(windows_array[STATUS_WINDOW], "  Context : MENUBAR ");
			}
	}

/*------render form window --------------*/
/*----	render create form entries -----*/
/*
void	render_account_form(void){
*/
void	draw_form_entries(void){
	int y, x, c;
	x = getmaxx(windows_array[FORM_WINDOW]);

	/*loop through the form fields*/
	for	(int i = 0; i < form_state.n_entry; i++){
		/*if the field currently focused is the one being printed -> change the attributes*/
		if	(form_state.focus == i){
			wattron(windows_array[FORM_WINDOW], A_REVERSE | A_BOLD);
			c = '>';
			}
			else	{
				c = ' '; 
				}

		if	(form_state.fields[i].err == 1){
			wattron(windows_array[FORM_WINDOW], COLOR_PAIR(RED_TEXT));
			mvwaddch(windows_array[FORM_WINDOW], form_state.fields[i].y, form_state.fields[i].x + 23, '*');
			wattroff(windows_array[FORM_WINDOW], COLOR_PAIR(RED_TEXT));
			}

		/*print the label of form state*/
		mvwprintw(windows_array[FORM_WINDOW], form_state.fields[i].y, form_state.fields[i].x - 2, " %c %-20s", c,
			form_state.fields[i].label);
		wattroff(windows_array[FORM_WINDOW], A_REVERSE | A_BOLD);

		/*loop through and print the buffer*/
		for	(int j = 0; form_state.fields[i].buffer[j] != '\0'; j++){
			mvwaddch(windows_array[FORM_WINDOW], form_state.fields[i].y, form_state.fields[i].x + 23 + j,
				form_state.fields[i].buffer[j]);

			/*move the custom cursor*/
			if	(form_state.fields[i].buffer[j + 1] == '\0' && form_state.focus == i){
				wattron(windows_array[FORM_WINDOW], A_REVERSE | A_BLINK);
				mvwaddch(windows_array[FORM_WINDOW], form_state.fields[i].y, form_state.fields[i].x + 24 + j,' ');
				wattroff(windows_array[FORM_WINDOW], A_REVERSE | A_BLINK);
				}
		
			/*autocomplete options*/
			}

		/*set the custom cursor*/
		if	(form_state.fields[i].buffer[0] == '\0' && form_state.focus == i){
			wattron(windows_array[FORM_WINDOW], A_REVERSE | A_BLINK);
			mvwaddch(windows_array[FORM_WINDOW], form_state.fields[i].y, form_state.fields[i].x + 23,' ');
			wattroff(windows_array[FORM_WINDOW], A_REVERSE | A_BLINK);
			}

		wattron(windows_array[FORM_WINDOW], A_DIM);
		mvwhline(windows_array[FORM_WINDOW], form_state.fields[i].y + 1, ((x - (x - 6))/2), ACS_HLINE, x - 6);
		wattroff(windows_array[FORM_WINDOW], A_DIM);
		}

	mvwvline(windows_array[FORM_WINDOW], form_state.fields[0].y, form_state.fields[0].x + 21, ACS_VLINE, form_state.fields[form_state.n_entry - 1].y - 2);

	/*display control options*/
	mvwprintw(windows_array[FORM_WINDOW], getmaxy(windows_array[FORM_WINDOW]) - 3, 
		(getmaxx(windows_array[FORM_WINDOW]) - strlen(form_state.options))/2, "%s", form_state.options);
	}

/*----	form rendering window ---*/
void	render_form(void){
	/*clear the form window*/
	werase(windows_array[FORM_WINDOW]);

	/*draw the box of the from window*/
	box(windows_array[FORM_WINDOW], 0, 0);

	/*the title according to the form_type*/
	wattron(windows_array[FORM_WINDOW], A_BOLD);
	mvwprintw(windows_array[FORM_WINDOW], 2, (getmaxx(windows_array[FORM_WINDOW]) - 
				(strlen(form_title[form_state.type])+ 12))/2, "CREATE %s FORM", form_title[form_state.type]);
	wattroff(windows_array[FORM_WINDOW], A_BOLD);

	/*draw a horizontal line here */
	wattron(windows_array[FORM_WINDOW], COLOR_PAIR(YELLOW_TEXT));
	mvwhline(windows_array[FORM_WINDOW], 3, 3, ACS_HLINE, (getmaxx(windows_array[FORM_WINDOW]) - 6));
	wattroff(windows_array[FORM_WINDOW], COLOR_PAIR(YELLOW_TEXT));
	

	/*... rather than switching between different form layouts, use the same layout different
		entries - just call it draw entries*/
		/*switch between the different screen states
			switch	(form_state.type){
				case	ACCOUNT_FORM:
					render_account_form();
					break;
				}
		*/
	draw_form_entries();
	}

/*----	render user interface ----*/
int	render_ui(void){
	/*clear screen - maybe not necessary*/
	/*
	clear();
	*/

	/*refresh - here??*/
	refresh();

	/*draw menu bar on current context*/
	render_menu();
	
	/*draw current workspace*/
	render_workspace();

	/*draw current status bar*/
	render_statusbar();

	/*render form*/
	render_form();
	if	(form_state.visible == 1){
		show_panel(panels_array[FORM_WINDOW]);
		}
		else	{
			hide_panel(panels_array[FORM_WINDOW]);
			}
	
	if	(page_state.n_refresh == 1){
		update_panels();
		}

	doupdate();
	//refresh();

	page_state.n_refresh = 0;
	page_state.f_panel = 0;

	/*adjust this so that it will return 1 on successful render*/
	return 1;
	}

/*----	handle menu input ----*/
int	handle_menu_input(int c){
	switch	(c){
		case	KEY_LEFT:
			/*move the menu item to the left*/
			if	(page_state.current_page == DASHBOARD_PAGE){
				page_state.current_page = EXIT_PAGE;
				}
				else	{
					page_state.current_page--;
					}
			break;
		case	KEY_RIGHT:
		case	'\t':
			/*move the menu item to the left*/
			if	(page_state.current_page == EXIT_PAGE){
				page_state.current_page = DASHBOARD_PAGE;
				}
				else	{
					page_state.current_page++;
					}
			break;

		case	'\n':
			/*set the need to refresh*/
			page_state.n_refresh = 1;
			page_state.focus = FOCUS_WORKSPACE;
			return 1;
			break;

		case	KEY_F(4):
			return -1;
			break;

		case	'f':
			//show_panel(panels_array[FORM_WINDOW]);	
			page_state.f_panel = 1;
			//page_state.n_refresh = 1;
			break;

		/*after running the program test this and add more*/
		}
	}

/*-------workspace input handling functions---------------------*/
/*----	handle accounts page input -----*/
int	handle_dashboard_page(int c){
	if	(c == KEY_F(1)){
		if	(form_state.visible == 0){
			form_state.visible = 1;
			}
			else	{
				form_state.visible = 0;
				}
		form_state.type = ACCOUNT_FORM;

		page_state.n_refresh = 1;
		}
	}

/*----	handle accounts page input -----*/
int	handle_accounts_page(int c){
	if	(c == KEY_F(1)){
		if	(form_state.visible == 0){
			form_state.visible = 1;
			}
			else	{
				form_state.visible = 0;
				}
		form_state.type = ACCOUNT_FORM;

		page_state.n_refresh = 1;
		}
	}

/*----	handle transactions page input -----*/
int	handle_transactions_page(int c){
	if	(c == KEY_F(1)){
		if	(form_state.visible == 0){
			form_state.visible = 1;
			}
			else	{
				form_state.visible = 0;
				}
		form_state.type = TRANSACTION_FORM;

		page_state.n_refresh = 1;
		}
	}

/*----	handle reports page input -----*/
int	handle_reports_page(void){
	}

/*----	handle db page input -----*/
int	handle_db_page(void){
	}

/*----	handle tools page input -----*/
int	handle_tools_page(void){
	}

/*----	handle help page input -----*/
int	handle_help_page(void){
	}

/*----	handle exit page input -----*/
int	handle_exit_page(void){
	}

/*----	handle workpace input --*/
int	handle_workspace_input(int c){

	if	(c == 47){	/*escape sequence*/
		page_state.focus = FOCUS_MENU;
		form_state.visible = 0;
		page_state.n_refresh = 1;
		return 1;
		}

	init_form_defaults(&form_state, page_state.current_page);

	/*input router for screen specific keys*/
	switch	(page_state.current_page){
		case	DASHBOARD_PAGE:
			handle_dashboard_page(c);
			break;
		case	ACCOUNTS_PAGE:
			handle_accounts_page(c);
			break;
		case	TRANSACTIONS_PAGE:
			handle_transactions_page(c);
			break;
		case	REPORTS_PAGE:
			handle_reports_page();
			break;
		case	DB_PAGE:
			handle_db_page();
			break;
		case	TOOLS_PAGE:
			handle_tools_page();
			break;
		case	HELP_PAGE:
			handle_help_page();
			break;
		case	EXIT_PAGE:
			handle_exit_page();
			break;
		}
	}

/*----	handle form input -----*/
int	handle_form_input(int c){
	switch	(c){
		case	'\t':
		case	'\n':
		case	KEY_DOWN:
			if	(form_state.focus < (form_state.n_entry - 1)){
				form_state.focus++;
				}
				else	{
					form_state.focus = 0;
					}
			break;
		case	('`'):
		case	KEY_UP:
			if	(form_state.focus > 0){
				form_state.focus--;
				}
				else	{
					form_state.focus = form_state.n_entry - 1;
					}
			break;
		case	KEY_F(2):
			beep();
			break;
		case	KEY_F(1):
		case	KEY_HOME:
			form_state.visible = 0;
			break;
		case	KEY_BACKSPACE:
			if	(form_state.fields[form_state.focus].bi > 0){
				form_state.fields[form_state.focus].buffer[--(form_state.fields[form_state.focus].bi)] = '\0';
				}
				else	{
					beep();
					}
			break;
		case	KEY_F(3):
			form_state.save = 1;
			data_cache.update = 1;
			break;
		default:
			if	(form_state.fields[form_state.focus].bi < 30){
				form_state.fields[form_state.focus].buffer[form_state.fields[form_state.focus].bi++] = c;
				}
				else	{
					beep();
					}
		}
	page_state.n_refresh = 1;
	return 1;
	}

/*----	handle input ---------*/
int	handle_input(void){
	/*get user input using getch()*/
	int c;
	c = getch();

	
	/*switch between global input options for all screens*/
	if	(form_state.visible == 1){
		return handle_form_input(c);
		}

	/*switch between menu and workspace*/
	if	(page_state.focus == FOCUS_MENU && c == '\n'){
		page_state.focus = FOCUS_WORKSPACE;
		page_state.n_refresh = 1;
		}

	switch	(page_state.focus){
		case	FOCUS_WORKSPACE:
			return handle_workspace_input(c);
		case	FOCUS_MENU:
			return handle_menu_input(c);
		}


	}

/*----	process actions -----*/
int	process_actions(void){
	/*if form save*/
	if	(form_state.save == 1){
		/*show form window*/
		/*validate input - are the buffers complete? if not which buffers are empty, set buffer error*/
		int error_count = 0;
		for	(int i = 0; i < form_state.n_entry; i++){
			if	(strlen(form_state.fields[i].buffer) < 2){	/*empty buffer*/
				form_state.fields[i].err = 1;
				error_count++;
				}		
			}

		if	(error_count > 0){
			page_state.n_refresh = 1;
			return -1;
			}

		/*add to the database - call database function*/
		/*make a struct and assign it the values of the form*/
		switch	(form_state.type){
			case	ACCOUNT_FORM:
				struct account_struct buffer;

				strncpy(buffer.account_name, form_state.fields[0].buffer, strlen(form_state.fields[0].buffer) + 1);
				strncpy(buffer.account_type, form_state.fields[1].buffer, strlen(form_state.fields[1].buffer) + 1);
				strncpy(buffer.description, form_state.fields[2].buffer, strlen(form_state.fields[2].buffer) + 1);

				/*the currency is determined based on the string value of the buffer against the the ID of the
				available currencies in the list - maybe call a function to do this*/
				strncpy(buffer.currency_id, form_state.fields[3].buffer, strlen(form_state.fields[3].buffer) + 1);

				/* ... remaining values in the accounts struct to add
					account_id[50];
					parent_id[50];
					account_number[50];
					created_at[50];
					updated_at[50];
				*/

				
				if	(add_record(ACCOUNTS, (void *) &buffer, sizeof(struct account_struct)) == -1){
					//strcat(error_string, "Add accounts error");
					sprintf(error_string, "Error Adding Record");
					}
				break;
			case	TRANSACTION_FORM:
				/*the transactions type form should also use the journal entry struct
				... this is because a single journal entry can have multiple transactions ---*/
				//struct transaction_struct buffer;
				//struct journal_struct jbuffer;
				//add_record(TRANSACTIONS, (void *) &buffer, sizeof(struct transaction_struct));
				break;

			case	CURRENCY_FORM:
				//struct currency_struct buffer;
				//add_record(CURRENCIES, (void *) &buffer, sizeof(struct currency_struct));
				break;

			}

	
			/*error check*/
		/*set form to default*/
		form_state.save = 0;
		}

	/*--- if post transaction*/
		/*ensure double entry balances*/
		/*add to the database*/

	/*handle navigation between screens*/

	/*close forms*/
	}


/*----	update state ----*/
int	update_state(void){
	/*track active screen menu*/
	/*maintain selected account/transaction*/
	/*cache frequently used DB data - does the cache need to be updated
	... data_cache (use dml to grab data from the offset)
	... loop through from the current offset upto the limit convert the offset to a string
	... also am I updating the accounts only or the transactions or both?*/
	char key[20];
	if	(data_cache.update == 1){
		/*update the cache from the offset*/
		/*if the offset is currently zero, incremenet to position one*/
		if	(data_cache.account_cco.offset == 0){
			data_cache.account_cco.offset = 1;
			}
		/*convert the offset to a string*/
		sprintf(key, "%d", data_cache.account_cco.offset);


		/*call find record*/
		if	(find_record(ACCOUNTS,0, key,(void *)&data_cache.accounts[0]) == -1){
			/*unable to find the first record in the offset - something is off error and stop*/
			//strcat(error_string, "Cant find record");
			//sprintf(error_string, "Cant find record %d", data_cache.account_cco.offset);
			data_cache.account_cco.count = 0;
			data_cache.update = 0;
			return -1;
			}

		/*while < capacity*/
		int i;
		for	(i = 1; i < data_cache.account_cco.capacity; i++){
			struct account_struct acc;
			/*next record*/
			/*is next record == -1*/
			if	(next_record(ACCOUNTS, 0, (void *) &acc) == -1){
				sprintf(error_string, "No next record");
				/*break*/
				break;
				}
			/*save the next record to cache*/
			data_cache.accounts[i] = acc;
			/*incremenet count*/
			}

		/*set offset*/
		data_cache.account_cco.count = i;
		data_cache.account_cco.offset += i;
		data_cache.update = 0;

		/*save the last position to -1 and save count*/
		//data_cache.accounts[i + 1] = NULL;
		}
	}


/*------end routines---------------------------------*/
/*-----	clean up --------*/
int	cleanup(int dbstatus){
	/*close DB*/
	
	if	(dbstatus == 1){
		database_close();
		}

	/*free ncurses resources*/
	/*end ncurses mode*/
	endwin();
	}



/*----	main function loop through the program -----*/
int	main(int argc, char *argv[]){
	/*controlling declarations*/
	int dbparse, dbstatus;

	/* --- init ---*/
	/*parse command line for the DB*/
	dbparse = parse_cli(argc, argv);

	/*init database connection*/
	dbstatus = connect_dbms(argv[1], dbparse);
	//dbstatus = 0;

	/*init ncurses*/
	init_ncurses();
	
	/*initialise the page state*/
	init_page_state();

	/*initialise the data cache*/
	init_data_cache();

	/*initlise the form*/
	init_form_defaults(&form_state, page_state.current_page);

	/*loop while not exit*/
	while	(page_state.running){
		/*... render the user interface */
		if	(render_ui() == -1){
			/*if unable to render the user interface exit - fix this error checking*/
			exit(1);
			}

		/*... handle input adjust the state according to the input, before rendering again*/
		if	(handle_input() == -1){
			break;
			}

		/*... process actions - i.e talk to the database/make calculations and post values
			before rendering again*/
		process_actions();

		/*update state*/
		update_state();

		/*confirm unsaved changes - break*/
		}

	/*cleanup & end*/
	cleanup(dbstatus);

	return 0;
	}
