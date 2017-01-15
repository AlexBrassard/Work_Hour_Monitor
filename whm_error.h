/*
 *
 * Work Hour Monitor  -  Error codes, messages and MACRO.
 *
 * Version: 1.01
 *
 */

#ifndef WHM_ERROR_HEADER_FILE
# define WHM_ERROR_HEADER_FILE

# define WHM_ERRMESG_MAX_LENGHT 128

static const char whm_err_head[] = "Impossible to execute the requested operation: ";

typedef enum whm_errcodes_type {
  WHM_DIREXIST = 200,
  WHM_FILEEXIST,
  WHM_BADWORDCOUNT,
  WHM_EMPTYQUEUE,
  WHM_FULLQUEUE,
  WHM_BADOPTION,
  WHM_TOOMANYENTRIES,
  WHM_BADQUESTION,
  WHM_INVALIDPOSNUM,
  WHM_INVALIDELEMCOUNT,
  WHM_PATHTOOLONG,
  WHM_INVALIDCOMPANY,
  WHM_ISNOTDIGIT,
  WHM_INVALIDFIELD,
  WHM_COMPANYCREATED,
  WHM_INVALIDSTATUS,
  WHM_INVALIDPOSITION,
  WHM_INVALIDWAGE,
  WHM_MISSINGWAGE
 
} whm_errcodes_T;

static const char whm_errmesg[][WHM_ERRMESG_MAX_LENGHT] = {
  "Directory already exists",
  "File already exists",
  "Erroneous word count",
  "Work queue is empty",
  "Work queue is full",
  "Demanded option does not exist",
  "Too many existing entries to add one more",
  "No answer is provided for this question number",
  "Invalid number of positions, must be bigger than zero",
  "Counted an invalid number of elements",
  "New pathname exceed the program's lenght limit",
  "Given company name does not exists",
  "Cannot convert non-digit characters to unsigned integer",
  "The demanded field does not exists",
  "The desired company has been created a few moments ago",
  "Invalid status, must be \"Actif (1)\" or \"Inactif (0)\"",
  "The desired position does not exists",
  "The given wage is invalid",
  "Missing a wage per hour for the given position name"
  
  
};


# define WHM_ERRMESG(string) do {                                       \
    fprintf(stderr, "\nError in '%s()' at line %d, from '%s()':\n%s",		\
	    __func__,  __LINE__, string, whm_err_head);			\
    if (errno && errno >= 200) fprintf(stderr, "%s\n\n", whm_errmesg[errno - 200]); \
    else if (errno) perror(NULL);					\
    else fprintf(stderr, "No error message; errno = %d\n\n", errno);    \
  } while (0);


#endif /* WHM_ERROR_HEADER_FILE */
