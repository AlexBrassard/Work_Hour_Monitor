/*
 *
 * Work Hour Monitor  -  Hour sheet related functions.
 *
 * Version: 0.01
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "whm.h"

/* Create a new, empty hour sheet. */
int whm_create_sheet(whm_sheet_T  *sheet,
		     whm_config_T *config,
		     whm_time_T   *time_o)
{


  if (!sheet || !config || !time_o){
    errno = EINVAL;
    return -1;
  }

  /* 
   * Make an absolute pathname, including a unique 
   * filename for the new hour sheet. 
   */

  /* 
   * Try to open the file at the pathname we just created.
   * If it succeed close the stream and return an error. 
   */

  /* Use whm_print_sheet_head() to print a heading message to the new file. */

  /* Use whm_print_sheet_cal() to print the calendar part of a sheet to the new file. */

  /* Use whm_print_sheet_cumul() to print the cumulatives part of a sheet to the new file. */

  return 0;

} /* whm_create_sheet() */


/*
 * Create an absolute pathname for a new hour sheet 
 * and return the new name in filename.
 * filename MUST be at least WHM_MAX_PATHNAME_S bytes wide.
 */
char* whm_make_sheet_path(char *filename,
			  whm_time_T *time_o,
			  whm_config_T *config)
{
  char temp[WHM_MAX_PATHNAME_S];
  
  if (!filename || !time_o || !config) {
    errno = EINVAL;
    return NULL;
  }

  /*
   * An hour sheet pathname looks like this:
   * /company'sworkingdir/year/companyname_month.sheet\0
   * +4:                 ^    ^           ^            ^
   */
  if ((strlen(config->working_directory) + strlen(config->employer)
       + strlen(time_o->month) + strlen(WHM_SHEET_SUFFIX) + 4) > WHM_MAX_PATHNAME_S){
    errno = EOVERFLOW;
    return NULL;
  }
  /* Also fills temp with zeros. */
  if (s_strcpy(temp, config->working_directory, WHM_MAX_PATHNAME_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    return NULL;
  }
  strcat(temp, "/");
  strcat(temp, time_o->year);
  strcat(temp, "/");
  strcat(temp, config->employer);
  strcat(temp, "_");
  /* -1: strftime() gives month numbers [1-12]. */
  strcat(temp, WHM_FR_MONTHS[atoi(time_o->month)-1]);
  strcat(temp, WHM_SHEET_SUFFIX);
  /* Fill the caller's buffer with zeros, then copy the new pathname into it. */
  if (s_strcpy(filename, temp, WHM_MAX_PATHNAME_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    return NULL;
  }
  return filename;

} /* whm_make_sheet_path() */






  /* 
   * Verify or create a year directory for a given company name.
   * The name of this directory is always the year in numeric format.
   * Returns successfuly if the directory already exists. 
   */
  int whm_new_year_dir(whm_config_T *config) {
    return 0;
  } /* whm_new_year_dir() */
