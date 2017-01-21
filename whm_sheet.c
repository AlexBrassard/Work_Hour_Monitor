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

/* 
 * Fill the structures to create a new hour sheet.
 * The sheet itself isn't written to disk now,
 * informations are kept in memory until whm_parse_options() and
 * whm_automatic_mode() are finished performing their operations.
 */
int whm_create_sheet(whm_sheet_T  *sheet,
		     size_t       *sheet_ind, /* First free element of **sheets. */
		     whm_config_T *config,
		     whm_time_T   *time_o)
{
  FILE *stream = NULL;
  int week_number = 0, day_ind = 0, week_ind = 0, date = 1;

  if (!sheet || !config || !time_o || !sheet_ind){
    errno = EINVAL;
    return -1;
  }

  /* 
   * Make an absolute pathname, including a unique 
   * filename for the new hour sheet. 
   */
  if (whm_make_sheet_path(sheet->path, time_o, config) == NULL){
    WHM_ERRMESG("Whm_make_sheet_path");
    return -1;
  }

  /* 
   * Try to open the file at the pathname we just created.
   * If it succeed close the stream and return an error. 
   */
  if ((stream = fopen(sheet->path, "r")) != NULL) {
    fclose(stream);
    stream = NULL;
    errno = WHM_FILEEXIST;
    return -1;
  }

  /*
   * Fill the given sheet according to the information
   * in the given config entry. Increment *sheet_ind
   * when finished. *sheet_ind is garanteed to be the same
   * number, when finished, than c_ind (config_index).
   */
  day_ind = whm_find_first_dom(time_o, &week_number);
  sheet->month = atoi(time_o->month);
  sheet->year  = atoi(time_o->year);
  /* For each weeks, enter the week number. (size_t) */
  for (; week_ind < 6; week_ind++){
    sheet->week[week_ind]->week_number = week_number;
    /* For each days, enter the date. (int)*/
    do {
      if (date <= 31)
	sheet->week[week_ind]->day[day_ind]->date = date++;
    }while (++day_ind < 7);
    day_ind = 0;
    ++week_number;
  }
  ++(*sheet_ind);
    
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


/* Print a predefined message into the given sheet stream. */
int whm_print_sheet_head(FILE *stream,
			 whm_config_T *config,
			 whm_time_T *time_o)
{
  if (!stream || !config || !time_o){
    errno = EINVAL;
    return -1;
  }

  fprintf(stream, "/*\n * Work Hour Monitor\n * Heures travaillees chez %s pour le mois de %s %s.\n \
*\n *\n *\n * Respectez le format et les espacements lors des modifications manuelles de ce fichier.\n *\n */\n\n\n\n",
	  config->employer, WHM_FR_MONTHS[atoi(time_o->month)],
	  time_o->year);
  return 0;
}


/* 
 * Print the calendar part of an hour sheet to the given stream. 
 * Numbers are converted into strings to get more control over 
 * the formating.
 * Had to separate the fprintf statements, apparently it doesn't 
 * like being fed many ternary conditional statement..!
 */
int whm_print_sheet_cal(FILE *stream,
			whm_config_T *config,
			whm_time_T *time_o,
			whm_sheet_T *sheet)
{
  size_t week_ind = 0, day_ind = 0, pos_ind = 0;
  int line_count = 0;
  char temp[WHM_MAX_PATHNAME_S]; /* s_itoa() needs a temporary buffer. */
  /* -18s: WHM_NAME_STR_S == 18. */

  if (!stream || !config
      || !time_o || !sheet){
    errno = EINVAL;
    return -1;
  }

  /* Each week entries in an hour sheet is separated by an empty line "\n\0" */
  for (; week_ind < 6; week_ind++){
    fprintf(stream, "Semaine %zu\n                   ", sheet->week[week_ind]->week_number);
    /* 
     * For each lines we have to loop through all days of the week. 
     * The first line after the week number is the day's names and dates
     * plus the total hours and earnings heading.
     */
    for (day_ind = 0; day_ind < 7; day_ind++){
      fprintf(stream, "%.3s %2s     ",
	      WHM_FR_DAYS[day_ind],
	      ((sheet->week[week_ind]->day[day_ind]->date == -1)
	       ? WHM_NO_DATE
	       : s_itoa(temp, sheet->week[week_ind]->day[day_ind]->date, WHM_MAX_PATHNAME_S)));
    }
    fprintf(stream, "  Total Heures:    Total Gains:\n");

    /* 
     * The config->numof_positions*2 number of lines following the day's names and dates
     * goes like this:
     * position1_name   position1_worked_hours
     *                  position1_earnings
     * position2_name   position2_worked_hours
     *                  position2_earnings
     * positionN_name   positionN_worked_hours
     *                  positionN_earnings
     * ...
     */

    for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++){
      fprintf(stream, "%-18s ", config->positions[pos_ind]);
      for (day_ind = 0; day_ind < 7; day_ind++){
	fprintf(stream, "%5.5s      ",
		((sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] == -1)
		 ? WHM_NO_HOUR
		 : s_ftoa(temp, sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind], WHM_NAME_STR_S)));
      }
      fprintf(stream, "  %6.6s",
	      ((sheet->week[week_ind]->pos_total_hours[pos_ind] == -1)
	       ? WHM_NO_THOUR
	       : s_ftoa(temp, sheet->week[week_ind]->pos_total_hours[pos_ind], WHM_NAME_STR_S)));
      fprintf(stream, "\n                   ");
      for (day_ind = 0; day_ind < 7; day_ind++){
	fprintf(stream, "%6.6s$    ",
		((sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] == -1)
		 ? WHM_NO_CASH
		 : s_ftoa(temp, sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind], WHM_NAME_STR_S)));
      }
      fprintf(stream, "                   %8.8s$",
	      ((sheet->week[week_ind]->pos_total_earnings[pos_ind] == -1) 
	       ? WHM_NO_TCASH
	       : s_ftoa(temp, sheet->week[week_ind]->pos_total_earnings[pos_ind], WHM_NAME_STR_S)));
      
    
      fprintf(stream, "\n");
      
    }
    /* Daily totals goes here. */
    fprintf(stream, "Total Heures:      ");
    for (day_ind = 0; day_ind < 7; day_ind++){
      fprintf(stream, "%6.6s     ",
	      ((sheet->week[week_ind]->day[day_ind]->total_hours == -1)
	       ? WHM_NO_THOUR
	       : s_ftoa(temp, sheet->week[week_ind]->day[day_ind]->total_hours, WHM_NAME_STR_S)));
    }
    fprintf(stream, "  %6.6s\n",
	    ((sheet->week[week_ind]->total_hours == -1)
	     ? WHM_NO_THOUR
	     : s_ftoa(temp, sheet->week[week_ind]->total_hours, WHM_NAME_STR_S)));

    fprintf(stream, "Total Gains:       ");
    for (day_ind = 0; day_ind < 7; day_ind++){
      fprintf(stream, "%8.8s$  ",
	      ((sheet->week[week_ind]->day[day_ind]->total_earnings == -1)
	       ? WHM_NO_TCASH
	       : s_ftoa(temp, sheet->week[week_ind]->day[day_ind]->total_earnings, WHM_NAME_STR_S)));
    }
    fprintf(stream, "                   %8.8s$\n",
	    ((sheet->week[week_ind]->total_earnings == -1)
	     ? WHM_NO_TCASH
	     : s_ftoa(temp, sheet->week[week_ind]->total_earnings, WHM_NAME_STR_S)));
    
    fprintf(stream, "\n\n");
  }
  
  return 0;
} /* whm_print_sheet_cal() */


/* 
 * Print the cumulatives part of an hour sheet to the given stream.
 * All numbers are converted into strings to get a better control
 * over the formatting. 
 */
int whm_print_sheet_cumul(FILE *stream,
			  whm_config_T *config,
			  whm_time_T *time_o,
			  whm_sheet_T *sheet)
{
  size_t day_ind = 0, pos_ind = 0;
  char temp[WHM_NAME_STR_S]; /* s_ftoa() needs to be fed a buffer. */

  if (!stream || !config || !time_o || !sheet){
    errno = EINVAL;
    return -1;
  }

  fprintf(stream, "\n/************************************************************************************/\nCumulatifs:\n\n\n");

  /* Keep in mind that at sheet->day_*_* day_index 7 are the monthly totals. */
  for (day_ind = 0; day_ind < 8; day_ind++){
    if (day_ind < 7) fprintf(stream, "%-18s ", WHM_FR_DAYS[day_ind]);
    else fprintf(stream, "\nGrand Total:       ");
    /* Print the total worked hours for all 'day_ind' day this month, all positions combined. */
    fprintf(stream, "%6.6s   ",
	    ((sheet->day_total_hours[day_ind] == -1)
	     ? WHM_NO_THOUR
	     : s_ftoa(temp, sheet->day_total_hours[day_ind], WHM_NAME_STR_S)));
    /* Print the total earned for all 'day_ind' day this month, all positions combined. */
    fprintf(stream, "%8.8s\n",
	    ((sheet->day_total_earnings[day_ind] == -1)
	     ? WHM_NO_TCASH
	     : s_ftoa(temp, sheet->day_total_earnings[day_ind], WHM_NAME_STR_S)));

    /* Same as above, but broke down per positions. */
    for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++){
      fprintf(stream, "%-18s ", config->positions[pos_ind]);
      fprintf(stream, "%6.6s   ",
	      ((sheet->day_pos_hours[day_ind][pos_ind] == -1)
	       ? WHM_NO_THOUR
	       : s_ftoa(temp, sheet->day_pos_hours[day_ind][pos_ind], WHM_NAME_STR_S)));
      fprintf(stream, "%8.8s\n",
	      ((sheet->day_pos_earnings[day_ind][pos_ind] == -1)
	       ? WHM_NO_TCASH
	       : s_ftoa(temp, sheet->day_pos_earnings[day_ind][pos_ind], WHM_NAME_STR_S)));
    }
    fprintf(stream, "\n");
  }

  fprintf(stream, "\n/************************************************************************************/\n");


  return 0;

} /* whm_print_sheet_cumul() */


/* 
 * Writes a single hour sheet to disk. 
 * The stream must be opened beforehand as this
 * function will also be used to print the
 * the sheet to stdout.
 */
int whm_write_sheet(FILE         *stream,
		    whm_config_T *config,
		    whm_time_T   *time_o,
		    whm_sheet_T  *sheet)
{
  int week_ind = 0, day_ind = 0, pos_ind = 0;

  if (!stream || !sheet || !config || !time_o) {
    errno = EINVAL;
    return -1;
  }

  /* Print a header message to the sheet. */
  if (whm_print_sheet_head(stream, config, time_o) != 0) {
    WHM_ERRMESG("Whm_print_sheet_head");
    return -1;
  }
  /* Print the calendar part of the hour sheet. */
  if (whm_print_sheet_cal(stream, config, time_o, sheet) != 0){
    WHM_ERRMESG("Whm_print_sheet_cal");
    return -1;
  }
  /* Call whm_print_sheet_cumul() to print the cumulatives part of the hour sheet. */
  if (whm_print_sheet_cumul(stream, config, time_o, sheet) != 0){
    WHM_ERRMESG("Whm_print_sheet_cumul");
    return -1;
  }

  return 0;

} /* whm_write_sheet() */


/*
 * Read the hour sheet living at pathname, brake down
 * and store the information in the given whm_sheet_T* object.
 * The sheet is read completely and stored in memory until processed.
 */
int whm_read_sheet(char *pathname,
		   whm_config_T *config,
		   whm_time_T *time_o,
		   whm_sheet_T *sheet)
{
  FILE *stream = NULL;
  char *content = NULL;

  if (!pathname || !config || !time_o || !sheet){
    errno = EINVAL;
    return -1;
  }

  if ((stream = fopen(pathname, "r")) == NULL){
    WHM_ERRMESG("Fopen");
    return -1;
  }
  if ((content = calloc(WHM_HOUR_SHEET_SIZE, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  fread(content, sizeof(char), WHM_HOUR_SHEET_SIZE, stream);
  if (ferror(stream)){
    WHM_ERRMESG("Fread");
    goto errjmp;
  }
  fclose(stream);
  stream = NULL;

  /* Call whm_parse_sheet() now. */


 errjmp:
  if (stream) {
    fclose(stream);
    stream = NULL;
  }
  if (content){
    free(content);
    content = NULL;
  }
  return -1;

} /* whm_read_sheet() */


/* Parse the content of the given hour sheet content. */
int whm_parse_sheet_cal(whm_sheet_T *sheet,
			whm_config_T *config,
			char *content)
{
  int    line_count = 0;
  int    day_ind  = 0, pos_ind = 0, week_ind = 0;
  int    cont_ind = 0, temp_ind = 0;
  size_t numof_lines_bpos = 2;                          /* Number of lines before positions.  */
  size_t numof_lines_apos = 2;                          /* Number of lines after positions.   */
  size_t numof_lines_pos  = config->numof_positions*2;  /* Number of lines for the positions. */
  whm_queue_T *queue = NULL;
  char   temp[WHM_NAME_STR_S];

  if (!sheet || !config  || !content){
    errno = EINVAL;
    return -1;
  }
  if ((queue = whm_init_queue_type(WHM_DEF_QUEUE_SIZE, WHM_NAME_STR_S)) == NULL){
    WHM_ERRMESG("Whm_init_queue_type");
    return -1;
  }
  
  /* 
   * Only register the digits and dots characters. 
   * The only way to reach the bottom of the loop is 
   * for all conditions to be false.
   * Most 'if' statements aren't followed by 'else if' or 'else'
   * statements cause say an 'outter if' succeeds but one of its
   * 'inner if' fails, I want the conditions bellow to be verified
   * without having to terminate the current loop itteration right away.
   * Felt I have to justify myself.
   */
  while(content[cont_ind] != '\0' && content[cont_ind] != EOF){

    /* 
     * Skip comments. 
     * cont_ind is incremented by whm_skip_comments(). 
     */
    if (content[cont_ind] == SLASH || content[cont_ind] == NUMBER){
      /* Single line comments: from '#' to 'EOL' or from '//' to 'EOL' */
      if (content[cont_ind] == NUMBER ||
	  (content[cont_ind+1] != '\0' && content[cont_ind+1] == SLASH)){
	if (whm_skip_comments(content, &cont_ind, 0) != 0){
	  WHM_ERRMESG("Whm_skip_comments");
	  goto errjmp;
	}
	continue;
      }
      /* Multi-lines, C89-style comments. */
      else if (content[cont_ind] != '\0' && content[cont_ind] == STAR){
	if (whm_skip_comments(content, &cont_ind, 1) != 0){
	  WHM_ERRMESG("Whm_skip_comments");
	  goto errjmp;
	}
	continue;
      }
    }

    /* 
     * When hitting a newline, 
     * make sure not to leave a string in temp, make sure the queue
     * isn't empty and fill the whm_sheet_T fields revelant to the
     * line_count. If the next character is another newline,
     * increment week_ind till it reaches 6, then its time to 
     * call whm_parse_sheet_cumul().
     */
    if (content[cont_ind] == NEWLINE) {
      ++cont_ind;
      if (temp_ind){
	if (whm_set_string(queue, temp) != 0){
	  WHM_ERRMESG("Whm_set_string");
	  goto errjmp;
	}
	temp_ind = 0;
      }
      if (queue->is_empty) continue;
      ; /* Call whm_queue_to_sheet() here. */
      if (content[cont_ind] == NEWLINE){
	++cont_ind;
	if (++week_ind >= 6) {
	  /* Call whm_parse_sheet_cumul() here then break. We're done. */
	  break;
	}
      }      
      continue;
    }

    /* 
     * If the character is a space and temp isn't empty, set it in queue.
     * In any cases, skip the space.
     */
    if (content[cont_ind] == SPACE){
      ++cont_ind;
      if (temp_ind){
	if (whm_set_string(queue, temp) != 0){
	  WHM_ERRMESG("Whm_set_string");
	  goto errjmp;
	}
	temp_ind = 0;
      }
      continue;
    }
    
    /* Save the dot or digit character into temp[temp_ind]. Skip anything else. */
    if (isdigit(content[cont_ind]) ||  content[cont_ind] == DOT)
      temp[temp_ind++] = content[cont_ind];

    /* Next character. */
    ++cont_ind;
  }

       
  
 errjmp:
  if (queue){
    whm_free_queue_type(queue);
    queue = NULL;
  }
  return -1;
  

} /* whm_parse_sheet_cal() */
