/*
 *
 * Work Hour Monitor  -  Main header file.
 *
 * Version: 0.01
 *
 */

#ifndef WHM_MAIN_HEADER_FILE
# define WHM_MAIN_HEADER_FILE

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include "whm_sysdep.h" /* System dependent values. */
# include "whm_error.h"
# include "sutils.h"     /* Safe-utils library. */


/*** Constants ***/
/* Including the terminating NULL byte. */
# define WHM_TIME_STR_S           16             /* Size of strings within whm_time_T objects.     */
# define WHM_STRFTIME_STR_S       32               
# define WHM_NAME_STR_S           18             /* Maximum lenght of names.                       */
# define WHM_CASH_STR_S           16             /* Maximum lenght of money strings.               */
# define WHM_SHORT_OPTION_S       3              /* Default size of a short option string.         */
# define WHM_LONG_OPTION_S        9              /* Default size of a long option string.          */
/* Maximum lenght of a command line option's argument. */
# define WHM_MAX_ARG_STR_S        WHM_NAME_STR_S + WHM_MAX_PATHNAME_S + 2
# define WHM_MAX_NUMOF_OPTIONS    256            /* Maximum number of command line options.        */
# define WHM_PROG_NUMOF_OPTIONS   7              /* Current number of options available to use.    */
# define WHM_MAX_NUMOF_ARG_NAMES  9              /* Number of possible argument names (see struct whm_option_type below). */
# define WHM_MAX_CONFIG_ENTRIES   256            /* No more than 256 companies in the config file. */
# define WHM_MAX_NUMOF_SHEETS     WHM_MAX_CONFIG_ENTRIES /* Maximum number of hour sheets in memory. */
# define WHM_MAX_PATHNAME_S       4096           /* Maximum lenght of pathnames.                   */
# define WHM_LINE_BUFFER_S        WHM_MAX_PATHNAME_S /* Size of buffers used to read lines.        */  
# define WHM_NUMOF_EOI_STRINGS    9              /* Number of "end of input" character (see array below). */
# define WHM_HOUR_SHEET_SIZE      9216           /* By default takes 6261 bytes, reserving 2955(2816) bytes for possible comments. */
# define WHM_MAX_COMMENT_SIZE     2816           /* Maximum lenght of a commentary. */
# define WHM_DEF_COMMENT_SIZE     256
# define WHM_DEF_NUMOF_COMMENTS   8              /* Default number of commentary object in an array. */
# define WHM_VALUE_S              WHM_NAME_STR_S*2+1 /* Size of the value a configuration entry's value. */

# define WHM_DEF_NUMOF_POSITIONS  24             /* Default maximum number of positions and wages. */
# define WHM_DEF_QUEUE_SIZE       32             /* Default maximum number of strings in a whm_queue_T* object. */ 

# define WHM_DIRECTORY_PERMISSION 0777           /* Permissions on a newly created directory (gives 755 when & ~umask & 0777). */
# define WHM_FILE_PERMISSION      0600           /* Permissiosn on a newly created file.           */

# define SPACE                    ' '            /* Represents a space character.                  */
# define NEWLINE                  '\n'           /* Represents a newline character.                */
# define DASH                     '-'            /* Represents the dash character.                 */
# define COLON                    ':'            /* Represents a colon character.                  */
# define SLASH                    '/'            /* Represents a slash character.                  */
# define STAR                     '*'            /* Represents the star character.                 */
# define NUMBER                   '#'            /* Represents the number character.               */
# define DOT                      '.'            /* Represents the dot character.                  */
# define U_SCORE                  '_'            /* Represents the underscore character.           */
/* Represents missing records in an hour sheet. */
# define WHM_NO_DATE              "--"
# define WHM_NO_HOUR              "--.--"
# define WHM_NO_CASH              "---.--"
# define WHM_NO_THOUR             "---.--"
# define WHM_NO_TCASH             "-----.--"

# define WHM_INPUTDONE            -2222          /* Used by whm_ask_user to signify input is done. */

# define WHM_NOMATCH              -2             /* Used mainly by whm_validate_name() to indicate no matches were found. */

# define WHM_SHEET_TYPE           1              /* Used by whm_delete_op() to indicate a sheet file type. */
# define WHM_CONFIG_TYPE          2              /* Used by whm_delete_op() to indicate a configuration file type. */

/* Used when finding an entry with no values in an hour sheet. */
static const char WHM_NO_VALUES[]        = "-1.0";

/* The program's name. */
static const char WHM_PROGRAM_NAME[]     = "whm";

/* Suffix appended to every newly created backup file. */
static const char WHM_BKUP_SUFFIX[]      = ".whmbkup";

/* Suffix appended to every newly created hour sheet. */
static const char WHM_SHEET_SUFFIX[]     = ".sheet";

/* Prefix appended to every "last seen sheet". */
static const char WHM_LAST_SEEN_SUFFIX[] = ".LS_";

/* Every supported short options. */
static const char WHM_SHORT_OPTIONS[][WHM_SHORT_OPTION_S] = {"-h", "-d", "-a", "-m", "-u", "-p", "-l" };

/* Every supported long options. */
static const char WHM_LONG_OPTIONS [][WHM_LONG_OPTION_S] = {
  "--help",   "--delete", 
  "--add",    "--modify", 
  "--update", "--print",
  "--list"
  
};

/* Array of options arguments names (used by whm_get_args()). */
static const char WHM_ARG_NAMES[][WHM_NAME_STR_S] = {
  "name",  "position",
  "type",  "year",
  "month", "date",
  "hours", "field",
  "value"

};

/* The maximum number of expected arguments for each options named above. */
static const int WHM_MAX_ARGS_PER_OPTION[] = { 0, 3, 0, 4, 6, 3, 0 };

/* The number of required arguments for each options. */
static const int WHM_REQ_ARGS_PER_OPTION[] = { 0, 1, 0, 1, 1, 1, 0 };

/* The heading message printed to a new configuration file. */
static const char WHM_CONFIG_HEADER_MSG[] =
  "/*\n *\n * Work Hour Monitor  -  Main configuration file.\n *\n * Please do NOT edit this file by hand.\n\
 * Use the appropriate command line options.\n * (More infos: -h or --help)\n *\n *\n\
 * The fields represent:\n * 0. Status\n * 1. Company name\n * 2. Company's working directory\n \
* 3. The number of positions occupied\n * 4. The positions names\n * 5. Positions' wages\n * 6. The night bonus,\
 if any\n * 7. Whether holiday pay is paid each week\n */\n\n";

/* End of input characters/strings. */
static const char WHM_END_OF_INPUT[WHM_NUMOF_EOI_STRINGS][WHM_NAME_STR_S] = {
  "d", "done", "DONE", "e", "exit", "EXIT", "f", "fin", "FIN"
};

/* Names of each months of the year, in french. */
static const char WHM_FR_MONTHS[][WHM_NAME_STR_S] = {
  "Janvier", "Fevrier",  "Mars",
  "Avril",   "Mai",      "Juin",
  "Juillet", "Aout",     "Septembre",
  "Octobre", "Novembre", "Decembre"
};

/* Names of each months in english. */
static const char WHM_EN_MONTHS[][WHM_NAME_STR_S] = {
  "January", "Febuary",  "March",
  "April",   "May",      "June",
  "July",    "August",   "September",
  "October", "November", "December"
};

/* Names of each days of the week, in french. */
static const char WHM_FR_DAYS[][WHM_NAME_STR_S] = {
  "Dimanche", "Lundi",
  "Mardi",    "Mercredi",
  "Jeudi",    "Vendredi",
  "Samedi"
};

/* Names of each days of the week, in english. */
static const char WHM_EN_DAYS[][WHM_NAME_STR_S] = {
  "Sunday",   "Monday",
  "Tuesday",  "Wednesday",
  "Thursday", "Friday",
  "Saturday"
};


/*** Data types ***/

/* Broke down time information strings. */
typedef struct whm_time_type {
  char                     day[WHM_TIME_STR_S];  /* The abreviated day name (english).      */
  char                     date[WHM_TIME_STR_S]; /* Date of the object. (1-31)              */
  char                     week[WHM_TIME_STR_S]; /* Week number of the object.  (1-53)      */
  char                     month[WHM_TIME_STR_S];/* Month number of the object. (1-12)      */
  char                     year[WHM_TIME_STR_S]; /* Year including centuries of the object. */

} whm_time_T;

/* WHM's main data type, a FIFO queue. */
typedef struct whm_queue_type {
  char                     **string;             /* An array of strings.                    */
  int                      index;                /* First free string of ->queue.           */
  int                      top_index;            /* Maximum numbers of strings in ->queue.  */
  size_t                   string_lenght;        /* Maximum lenght of a string of ->queue.  */
  size_t                   is_empty;             /* 0: NOT empty (at least index 0 is populated), > 0: the queue is empty. */

} whm_queue_T;

/* Holds all information regarding a single entry of the configuration file. */
typedef struct whm_config_type {
  size_t                   status;               /* 0: inactive, kept for records only; > 0: active.            */
  char                     *employer;            /* The name of the employer for this entry.                    */
  char                     *working_directory;   /* This company's hour sheet directory.                        */
  int                      numof_positions;      /* Number of different positions occupied in this company.     */
  char                     **positions;          /* The name of all positions occupied in this company.         */
  double                   *wages;               /* Wages for each positions.                                   */
  char                     *night_prime;         /* > 0: The exact amount; 0: night shift recorded, no prime; < 0: not recorded. */
  size_t                   do_pay_holiday;       /* > 0: The 4% holiday pay is paid every week, not cummulated. */

} whm_config_T;

/* Holds all information regarding a single hour sheet. */
typedef struct whm_sheet_type {
  char                     *path;                /* This sheet's path. */
  int                      year;                 /* This sheet's year. */
  int                      month;                /* This sheet's month. */
  double                   *day_total_hours;     /* Total hours, per week day, all positions combined. */
  double                   *day_total_earnings;  /* Total earnings, per week day, all position combined. */
  double                   **day_pos_hours;      /* Cumulatives, per week day, per positions. (+1 for monthly totals.) */
  double                   **day_pos_earnings;   /* Cumulatives, per week day, per positions. (+1 for monthly totals.) */
  struct whm_week_type     **week;               /* Array of 6 week objects, one for each week of the month. */
  struct whm_comment_type  **comments;           /* Array of commentary objects, 1 per commentary found. */
  size_t                   numof_comments;       /* Number of whm_comment_T* in the array. */
  int                      comment_ind;          /* First free element of **comments. */

} whm_sheet_T;


/* Holds all information regarding a single week of an hour sheet. */
typedef struct whm_week_type {
  size_t                   week_number;          /* From 0 to 53.                                          */
  double                   total_hours;          /* Total hours, all positions combined.                   */
  double                   total_earnings;       /* Total earnings, all positions combined.                */
  double                   *pos_total_hours;     /* Total hours worked this week, per positions.           */
  double                   *pos_total_earnings;  /* Total earnings, per positions.                         */
  struct whm_day_type      **day;                /* Array of 7 day objects, one for each days of the week. */

} whm_week_T;

/* Holds all information regarding a single day of a single week of an hour sheet. */
typedef struct whm_day_type {
  int                      date;                 /* This day's date, from 1 to 31. -1: no informations.     */
  double                   total_hours;          /* Total hours worked, all positions combined.             */
  double                   total_earnings;       /* Total earnings, all position combined.                  */
  double                   *pos_hours;           /* Hours worked for each positions.                        */
  double                   *pos_earnings;        /* Earnings for each positions.                            */

} whm_day_T;

enum whm_config_field_type {
  F_NONE = -1,
  F_STATUS = 0,
  F_EMPLOYER,
  F_POSITION = 4,
  F_WAGE,
  F_NIGHT_PRIME,
  F_HOLIDAY_PAY

};

enum whm_time_field_type {
  T_DAY,
  T_DATE,
  T_WEEK,
  T_MONTH,
  T_YEAR

};

/* Please note: Changing the order might screw things up alot! */
enum whm_option_names {
  NONE = -1,
  HELP = 0,
  DELETE,
  ADD,
  MODIFY,
  UPDATE,
  PRINT,
  LIST
 
};

/* Original Order:
enum whm_option_names {
  NONE = -1,
  PRINT = 0,
  UPDATE,
  ADD,
  DELETE,
  MODIFY,
  LIST,
  HELP
};
*/

typedef struct whm_option_type {
  enum whm_option_names     operation;
  /* Arguments that might be present, depending on the requested operation. */
  char                      *name;
  char                      *position;
  char                      *date;
  char                      *month;
  int                       type;
  int                       year;
  double                    worked_hours;
  enum whm_config_field_type field;
  char                      *value;

} whm_option_T;

/* To help fill the right whm_option_T structure field. */
enum whm_optarg_type {
  OPT_NAME,
  OPT_POSITION,
  OPT_TYPE,
  OPT_YEAR,
  OPT_MONTH,
  OPT_DATE,
  OPT_HOURS,
  OPT_FIELD,
  OPT_VALUE

};

enum whm_question_type {
  /* When creating the configuration file. */
  EMPLOYER = 0,
  POSITION,
  POSITION2,
  WAGE,
  NIGHT_PRIME,
  HOLIDAY_PAY,
  ADD_COMPANY,
  /* When modifing the configuration file. */
  MODIF_COMPANY_NAME,
  MODIF_CONFIG_FIELD,
  MODIF_UNKNOWN_COMPANY,
  MODIF_POSITION,
  FIELD_STATUS,
  FIELD_EMPLOYER,
  FIELD_POSITION,
  FIELD_WAGE,
  FIELD_NIGHT_PRIME,
  FIELD_HOLIDAY_PAY,
  /* When updating an hour sheet. */
  SHEET_WORKED_HOURS

};

/* 
 * List of sheets and their corresponding configuration entry
 * to be written to disk, along with the current size of arrays
 * and the index of the first free element.
 */
typedef struct whm_backup_type {
  whm_sheet_T               **sheets;                      /* Array of pointers to sheets to be written to disk.      */
  whm_config_T              **configs;                     /* The configuration entry of the sheet of the same index. */
  char                      **filename;                    /* Array of backup names.                                  */
  int                       c_ind;                         /* First free element of the arrays.                       */
  int                       size;                          /* 1 more than the highest available index.                */

} whm_backup_T;


/* 
 * Holds the offset from the begining of file of the first character of the commentary,
 * the offset of the last character of the commentary, from the begining of file
 * and a pointer to the text of the commentary.
 * To find the comment's lenght, substract b_offset from e_offset.
 *
 * Note that the size is NOT the lenght!
 */
typedef struct whm_comment_type {
  int                       b_offset;                      /* Begining of comments absolute file offset.(not size_T for consistency) */
  int                       e_offset;                      /* Ending of comments absolute file offset. (not size_T for consistency) */
  size_t                    text_s;                        /* The commentary text section size, including the terminating NULL. */
  char                      *text;                         /* Commentary's text section.  */

} whm_comment_T;
 
/*** Prototypes ***/

/* whm_mem_utils.c */
whm_time_T*    whm_init_time_type  (void);                 /* Allocate memory to a and return a whm_time_T object.         */
void           whm_free_time_type  (whm_time_T *time_o);   /* Free memory of a previously initialized whm_time_T object.   */
whm_queue_T*   whm_init_queue_type (int top_index,         /* Allocate memory to a whm_queue_T object.                     */
				    size_t elem_size);
void           whm_free_queue_type (whm_queue_T *queue);   /* Free memory of a previously initialized whm_queue_T object.  */ 
whm_config_T*  whm_init_config_type(void);                 /* Allocate memory for a whm_config_T object.                   */
void           whm_free_config_type(whm_config_T *config); /* Free memory of a previously initialized whm_config_T object. */
whm_day_T*     whm_init_day_type   (void);                 /* Allocate memory to a whm_day_T object.                       */
void           whm_free_day_type   (whm_day_T *day);       /* Free memory of a previously initialized whm_day_T object.    */
whm_week_T*    whm_init_week_type  (void);                 /* Allocate memory to a whm_week_T object.                      */
void           whm_free_week_type  (whm_week_T *week);     /* Free memory of a previously initialized whm_week_T object.   */
whm_sheet_T*   whm_init_sheet_type (void);                 /* Allocate memory to a whm_sheet_T object.                     */
void           whm_free_sheet_type (whm_sheet_T *sheet);   /* Free memory of a previously initialized whm_sheet_T object.  */
whm_option_T*  whm_init_option_type(void);                 /* Allocate memory to a whm_option_T object.                    */
void           whm_free_option_type(whm_option_T *option); /* Free memory allocated to a whm_option_T object.              */
whm_backup_T*  whm_init_backup_type(void);                 /* Allocate memory to a whm_backup_T* object.                   */
void           whm_free_backup_type(whm_backup_T *bup);    /* Free memory allocated to a whm_backup_T* object.             */
whm_comment_T* whm_init_comment_type(void);                /* Allocate memory to a whm_comment_T* object. */
void           whm_free_comment_type(whm_comment_T* comment); /* Free memory allocated to a whm_comment_T* object. */
int            whm_extend_comment_text(whm_comment_T *comment); /* Extend the text section of the given commentary. */
whm_comment_T** whm_init_comment_arr(size_t numof_comments); /* Allocate memory to an array of commentary objects. */
void           whm_free_comment_arr(whm_comment_T **comments, /* Free memory allocated to an array of commentary objects. */
				    size_t numof_comments);
whm_comment_T** whm_extend_comment_arr(whm_comment_T **comments, /* Extend the given array of commentaries. */
				       size_t *cur_size);

/* whm_gen_utils.c */
int            whm_new_dir         (const char *dir_name); /* Create a new directory if it doesn't already exists.         */
int            whm_get_time        (whm_time_T *time_o);   /* Get the current date in a preinitialized whm_time_T object.  */
int            whm_clr_time        (whm_time_T *time_o);   /* Clear all fields of a whm_time_T object.                     */
char*          whm_get_string      (whm_queue_T *queue);   /* Get a string from queue.                                     */
int            whm_set_string      (whm_queue_T *queue,    /* Add a string to queue.                                       */
				    char *value);
int            whm_clr_string      (whm_queue_T *queue,    /* Clear a string from queue. */
				    int index);
char*          whm_new_backup      (const char *filename,  /* Create a backup of the given file. */
				    char *backupname);
int            whm_rm_backup       (const char *filename); /* Delete the given backup file. */
int            whm_ask_user        (enum whm_question_type questions, /* Ask a question to the user via stdin. */
				    char *answer,
				    size_t answer_s,
				    whm_config_T *config,
				    int pos_ind);
int            whm_new_year_dir    (whm_config_T *config,  /* Verify and create if needed each company's current year dir. */
				    whm_time_T *time_o);
int            whm_find_first_dom  (whm_time_T *time_o,    /* Find the first week day of the month, and its week number. */
				    int *week_num);
int            whm_skip_comments   (char *string,          /* Skip a commentary character sequence. */
				    int *ind,
				    int multi_lines);
int            whm_skip_sheet_comments(whm_sheet_T *sheet, /* Skip and record a given sheet comment sequence. */
				       char *string,
				       int *ind,
				       int multi_lines);
int            whm_validate_name   (char *name,            /* Verify that the given name exists in the configuration file. */
				    whm_config_T **configs,
				    int c_ind);
int            whm_validate_position(char *name,           /* Validate the given position name. */
				     whm_config_T *config);
int            whm_validate_time_field(whm_time_T *time_o,  /* Validate the requested time field's value. */
				       enum whm_time_field_type field);
int            whm_get_month_number(char *month);          /* Returns the month number [1-12] for the corresponding name. */

/* whm_config.c    */
int            whm_new_config      (const char *pathname,  /* Create a new configuration file. */
				    int *config_index,
				    whm_config_T **configs);
int            whm_add_config      (int *config_index,     /* Add a company to the configuration file. */
				    whm_config_T **configs);
int            whm_rm_config       (char *company,         /* Delete the given company from the configuration file. */
				    int *max_config_ind,
				    whm_config_T **configs);
int            whm_read_config     (FILE *stream,          /* Read the configuration file. */
				    int *c_ind,
				    whm_config_T **configs);
int            whm_write_config    (int c_ind,             /* Write the configuration file to disk. */
				    FILE *stream,
				    whm_config_T **configs);
int            whm_modify_config   (char *company,         /* Update one field of the given whm_config_T struct array. */
				    char *position,
				    enum whm_config_field_type field,
				    char *value,
				    int c_ind,
				    whm_config_T **configs);
int            whm_inter_modify_config(int max_config_ind, /* Interactively modify an entry of the configuration file. */
				       whm_config_T **configs);
int            whm_list_config_names (int max_config_ind,  /* List names of active companies in the configuration file. */
				     whm_config_T **configs);
int            whm_list_config_fields(char *company,       /* List a given company's configuration file modifiable entries. */
				      int max_config_ind,
				      whm_config_T **configs);
int            whm_list_config_pos   (whm_config_T *config);/* List a given company's positions names. */
char*          whm_get_company_name  (char *string,        /* Interactively get a company name to edit with whm_modify_config() */
				      size_t string_s,
				      int *c_ind,
				      int max_config_ind,
				      whm_config_T **configs);
int            whm_get_field_name    (char *string,        /* Get the whm_config_T field to edit (enum whm_config_field_type). */
				      int max_config_ind,
				      whm_config_T **configs);
int            whm_get_field_value   (char *value,         /* Get the value corresponding to the given field. */
				      int value_s,
				      char *position,
				      enum whm_config_field_type field,
				      whm_config_T *config);

/* whm_sheet.c   */
int            whm_new_sheet         (whm_sheet_T *sheet,  /* Fill the structure representing an hour sheet.               */
				      size_t *sheet_ind,
				      whm_config_T *config,
				      whm_time_T *time_o);
char*          whm_make_sheet_path   (char *filename,      /* Build an absolute pathname of the given company's latest sheet. */
				      whm_time_T *time_o,
				      whm_config_T *config);
int            whm_print_sheet_head  (FILE *stream,        /* Print a heading message to the given stream.                 */
				      whm_config_T *config,
				      whm_time_T *time_o);
int            whm_print_sheet_cal   (FILE *stream,        /* Print the given hour sheet to the given stream.              */
				      whm_config_T *config,
				      whm_time_T *time_o,
				      whm_sheet_T *sheet);
int            whm_print_sheet_cumul (FILE *stream,        /* Print the cumulative section of an hour sheet.               */
				      whm_config_T *config,
				      whm_time_T *time_o,
				      whm_sheet_T *sheet);
int            whm_write_sheet       (FILE *stream,       /* Write the given sheet to the given stream.                    */
				      whm_config_T *config,
				      whm_time_T *time_o,
				      whm_sheet_T *sheet);
int            whm_read_sheet        (char *pathname,     /* Read the hour sheet living at pathname.                       */
				      whm_config_T *config,
				      whm_time_T *time_o,
				      whm_sheet_T *sheet);
int            whm_parse_sheet       (whm_config_T *config, /* Parse the calendar part of an hour sheet.                   */
				      whm_sheet_T *sheet,
				      char *content);
int            whm_queue_to_sheet    (whm_config_T *config, /* Empty the given queue in the given sheet object.            */
				      whm_queue_T *queue,
				      whm_sheet_T *sheet,
				      int line_count,
				      int week_ind,
				      int pos_ind,
				      int day_ind,
				      int is_cal);
int             whm_update_sheet     (whm_config_T *config, /* Update all calculated values of an hour sheet.              */
				      whm_sheet_T *sheet);
void            whm_reset_totals     (whm_sheet_T *sheet,  /* Reset all sheet totals to -1.0 .                             */
				      whm_config_T *config);
void            whm_get_day_totals   (whm_sheet_T *sheet,  /* Calculate daily amounts of hours and earnings.               */
				      whm_config_T *config);
void            whm_get_week_totals  (whm_sheet_T *sheet,  /* Calculate weekly amounts of hours and earnings.              */
				      whm_config_T *config);
void            whm_get_sheet_cumuls (whm_sheet_T *sheet,  /* Calculate monthly cumulatives.                               */
				      whm_config_T *config);
int             whm_inter_update_sheet(whm_config_T **configs, /* Interactively update active hour sheets.                 */
				       whm_sheet_T **sheets,
				       whm_time_T *time_o,
				       int max_ind);
int             whm_rm_sheet         (whm_config_T *config,/* Reset all fields of the given sheet to their default values. */
				      whm_sheet_T *sheet);
int             whm_set_sheet        (whm_config_T *config,/* Add an entry to the global list of sheets to write to disk.  */
				      whm_sheet_T *sheet);
int             whm_write_sheet_list (whm_time_T *time_o); /* Write to disk all hour sheets in the global list "to_write". */
void            whm_clean_sheet_list (void);               /* NULL out every objects and strings of the global list "to_write". */

/*   whm_main.c   */
int             whm_automatic_mode   (whm_config_T **configs, /* Called when no options are present on the command line.   */
				      whm_sheet_T **sheets,
				      whm_time_T *time_o,
				      int max_ind);
int             whm_parse_options    (int argc, char **argv,/* Parse command line options.                                 */
				      whm_config_T **configs,
				      whm_sheet_T **sheets,
				      whm_time_T *time_o,
				      int *max_ind);
int             whm_get_args         (int argc, char **argv,/* Get command line option's arguments.                        */
				      int *arg_ind,
				      whm_option_T **options,
				      int *opt_ind);
void            whm_sort_op          (whm_option_T **options, /* Sort options using their ->operation field as key.        */
				      int max_opt_ind);
void            whm_help_op          (void);              /* Print a help message to stderr and exit.                      */
int             whm_print_op         (whm_option_T *option,   /* Print the given hour sheet or config file to stdout.      */
				      whm_config_T **configs,
				      whm_sheet_T **sheets,
				      whm_time_T *time_o,
				      int max_ind);
int             whm_update_op        (whm_option_T *option,/* Selectively update an hour sheet. */
				      whm_config_T **configs,
				      whm_sheet_T **sheets,
				      whm_time_T *time_o,
				      int max_ind);
int             whm_add_op           (whm_option_T *option,/* Add a company to the configuration file. */
				      whm_config_T **configs,
				      int *max_ind);
int             whm_del_op           (whm_option_T *option,/* Remove a company from the configuration file. */
				      whm_config_T **configs,
				      int *max_ind);
int             whm_modify_op        (whm_option_T *option,/* Modify a single field of a configuration file entry. */
				      whm_config_T **configs,
				      int max_ind);
void            whm_list_op          (whm_config_T **configs, /* Print on stdout a list of all employer name from the config file. */
				      int max_ind);

void whm_PRINT_config(whm_config_T *config);               /* GDB debugging hook. DO NOT CALL WITHIN A PROGRAM !! */
void whm_PRINT_sheet(whm_sheet_T *sheet,                   /* GDB debugging hook. DO NOT CALL WITHIN A PROGRAM !! */
		     whm_config_T *config);
void whm_PRINT_queue(whm_queue_T *queue);                  /* GDB debugging hook. DO NOT CALL WITHIN A PROGRAM !! */
void whm_PRINT_option(whm_option_T *option);               /* GDB debugging hook. DO NOT CALL WITHIN A PROGRAM !! */
/*** MACROS ***/

/* Remove a trailing newline. */
# define WHM_TRIM_NEWLINE(string) do{			\
    size_t len = strlen(string);			\
    if (string[len-1] == NEWLINE) string[len-1] = '\0';	\
  } while (0);

/* Replace a space by an underscore. */
# define WHM_REPLACE_SPACE(string) do{			\
    size_t i = 0;					\
    while (string[i] != '\0'){				\
      if (string[i] == SPACE) string[i] = U_SCORE;	\
      ++i;						\
    }							\
  } while (0);


#endif /* WHM_MAIN_HEADER_FILE */
