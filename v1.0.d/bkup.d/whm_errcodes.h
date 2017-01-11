/*
 *
 * Work_hour_monitor specific error codes and messages.
 *
 */

#ifndef WHM_ERROR_CODES
# define WHM_ERROR_CODES
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

# define WHM_ERRMESG_MAX_LENGHT 100                /* Random */

typedef enum {
  WHM_DIREXISTS = 200,
  WHM_CONFEXISTS,
  WHM_SHEETEXISTS,
  WHM_SHEETNAMETOOLONG,
  WHM_CANTREADSHEET,
  WHM_STACKFULL,
  WHM_STACKEMPTY,
  WHM_CONFNAMETOOLONG,
  WHM_TOOMANYENTRIES,
  WHM_INCOMPLETEENTRY,
  WHM_PATHNAMETOOLONG,
  WHM_INVALIDPUSH,
  WHM_INVALCOMMENT,
  WHM_NOENDOFCOMMENT,
  WHM_INVALIDCHAR,
  WHM_BADTIMEVALUE,
  WHM_INVALIDLINENUM,
  WHM_INVALIDHOUR,
  WHM_ENTRYALREADYFILLED,
  WHM_INPUTTOOLONG,
  WHM_INVALIDWEEKIND
  
} whm_errcodes_T;

static const char whm_errmesg[][WHM_ERRMESG_MAX_LENGHT] = {
  "Impossible to create directory: Directory already exists",
  "Impossible to create main configuration file: The file already exists",
  "Impossible to create the requested hour sheet: The file already exists",
  "Impossible to create the requested hour sheet: The new filename is too long",
  "Impossible to read the requested hour sheet",
  "Impossible to push value to stack: Stack is full",
  "Impossible to pop value from stack: Stack is empty",
  "Impossible to add requested name to the main configuration: Name too long",
  "Impossible to add entry: Too many entries",
  "Impossible to continue: Incomplete configuration entry",
  "Impossible to make pathname: Pathname too long",
  "Impossible to push the given value: Value is too big",
  "Impossible to skip commentary: Invalid commentary delimiters",
  "Impossible to skip commentary: Reached end of file before end of commentary sequence",
  "Impossible to continue processing the current file: Found an invalid character",
  "Impossible to create the requested whm_time_T object: Found invalid time value(s)",
  "Impossible to pop elements from the given stack: Invalid line count",
  "Impossible to add the requested hour: Input is not a digit character",
  "Impossible to update entry: Today's entry has already been filled",
  "Impossible to accept input: Input is too long",
  "Impossible to parse the given hour sheet: The week index is bigger than 6"
};


# define WHM_ERRMESG(string) do {					\
    fprintf(stderr, "Error in %s at line %d:\n", __func__, __LINE__);	\
    if (errno && errno >= 200) fprintf(stderr, "%s\n", whm_errmesg[errno - 200]); \
    else if (errno) perror(string);					\
    else fprintf(stderr, "No error message; errno = %d\n", errno);	\
  } while (0);



#endif
