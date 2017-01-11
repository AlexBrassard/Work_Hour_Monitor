/*
 *
 *  Work Hours Monitor main header file.
 *
 */


#ifndef WHM_MAIN_HEADER_FILE
# define WHM_MAIN_HEADER_FILE


/** Constants **/

/* Do not input a trailing slash. */
static const char WHM_WORK_DIR[]           = "/home/lappop/.whm.d"; /* The directory to find config files and work sheets. */
static const char WHM_MAIN_CONF_NAME[]     = "/home/lappop/.whm.d/.mainconf.whmconfig"; /* Main configuration file. */
static const char WHM_CONFIG_SUFX[]        = ".whmconfig";          /* The program's configuration file suffix. */
static const char WHM_HOUR_SHEET_NAME[]    = "hour_sheet";          /* Prefixed by the month name and current year. */

static const char WHM_FR_WEEK_DAYS[][4]    = {
  "Dim", "Lun",
  "Mar", "Mer",
  "Jeu", "Ven",
  "Sam"
};

static const char WHM_FR_LG_WEEK_DAYS[][9] = {
  "Dimanche", "Lundi",
  "Mardi",    "Mercredi",
  "Jeudi",    "Vendredi",
  "Samedi"
};

static const char WHM_FR_MONTH_NAMES[][10] = {
  "Janvier",   "Fevrier",
  "Mars",      "Avril",
  "Mai",       "Juin",
  "Juillet",   "Aout",
  "Septembre", "Octobre",
  "Novembre",  "Decembre"
};

static const char WHM_EN_WEEK_DAYS[][4]    = {
  "Sun", "Mon",
  "Tue", "Wed",
  "Thu", "Fri",
  "Sat"
};

static const char WHM_MAIN_CONF_HEADER[]   =
  "/*\n *\n * Work Hour Monitor's main configuration file.\n * Careful when editing, this file is very sensitive to changes.\n *\n */";
/* Represents non-filled entries. */
static const char WHM_NO_DATE[]   = "--";
static const char WHM_NO_HOUR[]   = "--.--";
static const char WHM_NO_CASH[]   = "---.--";
static const char WHM_NO_T_HOUR[] = "---.--";
static const char WHM_NO_T_CASH[] = "-----.--";

/* Represents the width of some doubles. */
# define WHM_DATE_WIDTH 2
# define WHM_HOUR_WIDTH 4
# define WHM_CASH_WIDTH 5
# define WHM_T_HOUR_WIDTH 5
# define WHM_T_CASH_WIDTH 7

# define SPACE   ' '                                     /* Represents a space character.    */
# define NEWLINE '\n'                                    /* Represents a newline character.  */
# define DASH    '-'                                     /* Represents the dash character.   */
# define COLON   ':'                                     /* Represents a colon character.    */
# define SLASH   '/'                                     /* Represents a slash character.    */
# define STAR    '*'                                     /* Represents the star character.   */
# define NUMBER  '#'                                     /* Represents the number character. */
# define DOT     '.'                                     /* Represents the dot character.    */

# define S_FTOA_MIN_STR_SIZE 32                          /* Minimum size of a string passed to s_ftoa().             */

# define WHM_NEWDIR_PERM 448                             /* New directory permissions, in decimal (0700 octal).      */
# define WHM_NEWFILE_PERM 384                            /* New file permissions, in decimal (0600 octal).           */

# define WHM_MAX_ENTRIES 256                             /* Maximum number of company entries/directories.           */
# define WHM_MAX_STACK_SIZE WHM_MAX_ENTRIES * 10         /* Maximum size of a whm_stack_T->string field.             */
# define WHM_DEF_STACK_SIZE 32                           /* Default size of a whm_stack_T->string field.             */
# define WHM_MAX_NAME_LENGHT 256                         /* Maximum lenght of a company name.                        */
# define WHM_MAX_PATHNAME_LENGHT 1024                    /* Maximum lenght of a company's absolute working directory pathname. */
# define WHM_MAX_CONF_NAME_LENGHT 17                     /* Maximum of 17 characters for a single job position name. */
# define WHM_MAX_NUMOF_POSITIONS 10                      /* Maximum number of positions and wages per jobs.          */
# define WHM_WAGE_STRING_LENGHT 10                       /* Should be enough for now.                                */
# define WHM_TIME_STRING_LENGHT 196                      /* Should be enough to display time using strftime()        */
# define WHM_TIME_O_STRING_LENGHT 16                     /* Maximum lenght of all strings in a whm_time_T object.    */
# define WHM_MAX_HR_SHEET_LENGHT 6000                    /* An hour sheet is aprox. 4200 bytes.                      */
# define WHM_HEADER_MAX_LEN 256                          /* Maximum lenght of any file's header message.             */
# define WHM_MAX_LINE_LENGHT 120                         /* Maximum number of characters per line in a file.         */
/* 
 * Maximum number of bytes to read from the main configuration file. 
 * (number of lines per entries by the lenght of each line by the library's max number of entries plus the len of the header)
 */
# define WHM_MAX_MAINCONF_LENGHT (((4+WHM_MAX_NUMOF_POSITIONS) * WHM_MAX_LINE_LENGHT * WHM_MAX_ENTRIES) \
				  + WHM_HEADER_MAX_LEN)

# define WHM_MAINCONF_POSNAMES_LINE_NUM 4                /* Line number of the position names within main configuration file. */
# define WHM_MAINCONF_WAGES_LINE_NUM 5                   /* Line number of the position names' wages within main config file. */
# define WHM_HR_SHEET_POSITIONS_LINE_NUM 2               /* Line number of the begining of positions hours in an hour sheet. */

# define WHM_USELESS_POP_OP 254                          /* To help _read_hour_sheet pop a useless word out the stack. */

/** Data types **/

/* 
 * Changing the order in which the struct elements appear in the following
 * declarations might affect what gets pushed/poped where, in main().
 */

/* To hold parsed information regarding current date. */
typedef struct {
  char                     *day;            /* The day name as per the current locale.  */
  char                     *month;          /* The month name. */
  char                     *date;           /* From 01 - 31 */
  char                     *year;           /* The year including centuries. */
  char                     *week;           /* The ISO 8601 week number from 1 to 53. */

} whm_time_T;

/* Main stack data type used to parse jobs and configuration informations. */
typedef struct whm_stack_type {
  char                     **string;
  int                      tos;             /* When tos is -1, the stack is emtpy. */
  size_t                   size;

} whm_stack_T;

/* To hold information on each jobs found in the main configuration file. */
typedef struct whm_job_info_type {
  size_t                   status;          /* 0: Inactive,  1: Active. */
  char                     *name;           /* The company's name. */
  char                     *work_dir;       /* The company's absolute WHM_WORK_DIR directory path. */
  size_t                   numof_positions; /* The number of different positions occupied in the company. */
  char                     **position_name;
  char                     **wage;          /* The company's hourly wage for each different occupied position. */
  size_t                   vacation_pay;    /* 0: pay == hrs * wage ; 1: pay = (hrs * wage) + 4% */
  double                   night_bonus;     /* -1 indicates no night shifts, 0 no bonus, > 0 is the amount given per hours. */

} whm_job_info_T;

/* To hold all the informations contained within one single day of a given hour sheet. */
typedef struct whm_day_type {
  int                      daynum;          /* Date from 0 to 30 inclusively. */
  char                     *dayname;        /* The weekday's name, sunday to monday. */
  double                   *pos_hours;      /* Number of hours worked for each positions. */
  double                   total_hours;     /* Total hours made this day. */
  double                   total_cash;      /* Total amount of money made this day. */
  int                      is_night;        /* <= 0: Day shift; > 0: Night shift, add bonus if applicable. */
 
} whm_day_T;

/* 
 * To hold all informations concerning a single week from a given hour sheet.
 * Exactly 6 are needed to represent a complete given hour sheet.
 */
typedef struct whm_week_type {
  struct whm_day_type      **daily_info;    /* Informations for each days of the current struct's week. */
  int                      week_number;     /* This week's number. */
  double                   total_hours;     /* This week's total number of hours worked. */
  double                   total_cash;      /* This week's total amount of money made. */
  double                   *weekly_hours;   /* Total of hours for each combined days of this week for each positions. */
  double                   *weekly_cash;    /* Total of money for each combined days of this week for each positions. */
   
} whm_week_T; 

/* This structure holds most information required to fill an hour sheet. */
typedef struct whm_month_type{
  char                     *sheet_path;     /* Absolute pathname of this current sheet. */
  whm_week_T               **week;          /* 1 struct for each six weeks of an hour sheet. */
  /* Cumulated hours and money for each week days of all combined weeks. */
  double                    *cumul_hours;
  double                    *cumul_cash;
  /* Cumulated hours and money, per positions, for each  week days of all combined weeks. */
  double                    **per_pos_hours;
  double                    **per_pos_cash;

} whm_month_T;


/* To tell the WHM_FTOA MACRO what type of string is requested. */
typedef enum whm_str_types {
  WHM_DATE_T,
  WHM_HOUR_T,
  WHM_CASH_T,
  WHM_T_HOUR_T,
  WHM_T_CASH_T

} whm_str_T;


/** Prototypes **/

/* config.d/config.c */
void*                      s_strcpy(char *dest, const char *src, size_t n); /* Safely copy src to dest[n]. */
char*                      s_ftoa(char *dest, size_t dest_s, double src);   /* Converts a double to a string. */
char*                      s_itoa(char *dest, size_t dest_s, int src);      /* Same as s_ftoa() except it takes an integer. */
char*                      whm_replace_spaces(char *input, size_t in_size); /* Replaces all spaces by underscores. */
int                        whm_check_input_done(char *input);               /* Check if the given input is considered done. */
int                        whm_skip_comments(char *content,                 /* Skip commentaries. */
					     size_t content_s,
					     size_t *c_ind);
int                        whm_pop_in_job_info(whm_job_info_T **job_array,  /* Fill up fields of the given job_array[*ja_ind]. */
					       size_t *ja_ind,
					       whm_stack_T *stack,
					       int *numof_positions);
int                        whm_create_work_dir(const char *dirname);        /* Create the program's working directory. */
int                        whm_create_main_config(const char *filename);    /* Create the main config file. */
whm_job_info_T**           whm_read_main_config(const char *filename,       /* Read the main config file. */
						size_t *first_elem);
int                        whm_write_main_config(const char *filename,      /* Write the possibly modified configuration file. */
						 whm_job_info_T **job_array,
						 size_t *ja_ind);
int                        whm_backup_file(const char *pathname,            /* Create backup of the given pathname. */
					   char *new_pathname);
int                        whm_rm_backup_file(const char *pathname);        /* Remove the given backup filename. */

/* mem.d/mem_utils.c */
whm_stack_T*               whm_init_stack(size_t size);                     /* Allocate memory to a whm_stack_T object. */
void                       whm_free_stack(whm_stack_T *stack);              /* Release memory allocated to a whm_stack_T object. */
whm_job_info_T*            whm_init_job_info(void);                         /* Allocate memory to a whm_job_info_T object. */
void                       whm_free_job_info(whm_job_info_T *object);       /* Free memory allocated to a whm_job_info_T object. */
whm_time_T*                whm_fetch_time(void);                            /* Allocate memory to a whm_time_T*, fill it up. */
void                       whm_free_time(whm_time_T *time_o);               /* Free memory of the given whm_time_T* object. */
whm_day_T*                 whm_init_day_type(void);                         /* Allocate memory to a whm_day_T* object. */
void                       whm_free_day_type(whm_day_T *object);            /* Release memory of a whm_day_T object. */
whm_week_T*                whm_init_week_type(void);                        /* Allocate memory to a whm_week_T* object. */
void                       whm_free_week_type(whm_week_T *object);          /* Release memory allocated to a whm_week_T* object. */
whm_month_T*               whm_init_month_type(void);                       /* Allocate memory to a whm_month_T* object. */
void                       whm_free_month_type(whm_month_T *object);        /* Release memory allocated to a whm_month_T*. */

/* sheets.d/utils.c */
int                        whm_print_sheet_header(whm_job_info_T *job_info, /* Print a formated message to the given stream. */
						  whm_time_T *time_o,
						  FILE *stream);
int                        whm_print_new_sheet(whm_job_info_T *job_info,    /* Write an empty (new) hour sheet given stream. */
					       whm_time_T *time_o,
					       FILE *stream);
int                        whm_print_hour_sheet(whm_job_info_T *job_info,   /* Write an (updated) hour sheet to given stream. */
						whm_month_T *sheet,
						whm_time_T *time_o,
						FILE *stream);
int                        whm_find_first_dom(whm_time_T *time_o,           /* Fetch the name of the first of the month. */
					      int *week_num);
int                        whm_create_hour_sheet(whm_job_info_T *job_info,  /* Create an new hour sheet. */
						 whm_time_T *time_o,
						 char *new_pathname,
						 size_t np_size);
int                        whm_read_hour_sheet(whm_month_T *sheets,         /* Reads the given company's hour sheet. */
					       whm_job_info_T *job_info,
					       whm_time_T *time_o);
char*                      whm_fetch_hs_content(FILE *stream,               /* Returns the content of an hour sheet. */
						char *content);
size_t                     whm_fetch_hours(char *content,                   /* Fetches the "hours" part of an hour sheet. */
					   whm_month_T *sheet,
					   size_t numof_positions);
int                        whm_fetch_cumulatives(char *content,             /* Fetches the "cumulatives" part of an hour sheet. */
						 whm_month_T *sheet,
						 size_t numof_positions,
						 size_t c_char);
int                        whm_get_user_hours(whm_month_T *sheet,           /* Prompt user for today's worked hours. */
					      whm_job_info_T *job_info,
					      whm_time_T *time_o,
					      size_t week_ind,
					      size_t day_ind);
int                        whm_update_hour_sheet(whm_month_T *hour_sheet,   /* Update a single hour sheet. */
						 whm_job_info_T *job_info,
						 whm_time_T *time_o);
char*                      whm_make_sheet_path(char *new_pathname,          /* Builds pathname for a given company's cur. sheet. */
					       whm_job_info_T *job_info,
					       whm_time_T *time_o);
int                        whm_pop_in_month_type(size_t line_count,         /* Fill in the given whm_month_T object. */
						 size_t week_ind,
						 size_t numof_positions,
						 size_t cur_pos,
						 whm_month_T *month_info,
						 whm_stack_T *sheet_stack);
int                        whm_pop_in_cumulatives(size_t line_count,        /* Fill in the "cumulative" fields of a _month_T*. */
						  size_t cur_pos,
						  int day_ind,
						  whm_stack_T *stack,
						  whm_month_T *sheets);

/* main.c */
int                        whm_push(whm_stack_T *stack, char *value);       /* Push the given value on the given stack. */
char*                      whm_pop(whm_stack_T *stack);                     /* Returns the value of the top of stack. */

/* Gdb hooks. */
void                       print_sub(char *s, size_t n, size_t b, size_t e);          /* Print s of lenght n from index b to e. */


/** MACROS **/
# define WHM_TRIM_TRAILING_NL(string) do {				\
    if (string[strlen(string)-1] == NEWLINE) string[strlen(string)-1] = '\0'; \
  } while (0);

# define WHM_MAKE_PATH(dest, head, tail) do{				\
    if (strlen(head) + strlen(tail) + 2 >= WHM_MAX_PATHNAME_LENGHT) {	\
      errno = WHM_PATHNAMETOOLONG; break;				\
    }									\
    if (s_strcpy(dest, head, WHM_MAX_PATHNAME_LENGHT) == NULL) break;	\
    strcat(dest, "/");							\
    strcat(dest, tail);							\
  } while(0);

/* 
 * Loop over the config_content string untill 2 consecutive newlines are found. 
 * Takes a * to a char, a * to a size_t and a size_t argument. No error checks are made. 
 */
# define WHM_SKIP_CONFIG_ENTRY(config_content, content_ind, content_size) do { \
    while (*content_ind < content_size && config_content[*content_ind] != '\0'){ \
      if (config_content[*content_ind] == NEWLINE && config_content[(*content_ind)+1] == NEWLINE) break; \
      ++(*content_ind);							\
    }									\
  } while (0);


#endif /* WHM_MAIN_HEADER_FILE */
