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
int whm_new_sheet(whm_sheet_T  *sheet,
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

} /* whm_new_sheet() */


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
  size_t week_ind = 0, day_ind = 0;
  int pos_ind = 0;
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
  size_t day_ind = 0;
  int pos_ind = 0;
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
  if (s_strcpy(sheet->path, pathname, WHM_MAX_PATHNAME_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }
  sheet->year = atoi(time_o->year);
  sheet->month = atoi(time_o->month);
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

  if (whm_parse_sheet(config, sheet, content) == -1){
    WHM_ERRMESG("Whm_parse_sheet_cal");
    goto errjmp;
  }
  if (content){
    free(content);
    content = NULL;
  }
  return 0;


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


/* Fill a whm_sheet_T object with the given queue's data. */
int whm_queue_to_sheet(whm_config_T *config,
		       whm_queue_T *queue,
		       whm_sheet_T *sheet,
		       int line_count,
		       int week_ind,
		       int pos_ind,
		       int day_ind,
		       int is_cal)
{
  if (!sheet || !config
      || !queue || line_count < 0
      || week_ind < 0 || pos_ind < 0
      || day_ind < 0){
    errno = EINVAL;
    return -1;
  }

  if (is_cal){
    /* 
     * Queue's content depends on the line number:
     * Line 0:                          the week number.
     * Line 1:                          the dates.
     * Line 2 to (numof_positions*2)+1: hours/cash of each positions.
     * Line numof_positions*2+2:        total hours.
     * Line (numof_positions*2)+3:      total earnings.

     * ->numof_positions is garanteed to be at most WHM_DEF_NUMOF_POSITIONS (24 atm).
     * So long as WHM_DEF_NUMOF_POSITIONS is smaller than INT_MAX, casts are good.
     */
    if (line_count == 0) {
      sheet->week[week_ind]->week_number = atoi(whm_get_string(queue));
    }
    else if (line_count == 1) {
      for (day_ind = 0; day_ind < 7; day_ind++)
	sheet->week[week_ind]->day[day_ind]->date = atoi(whm_get_string(queue));
    }
    else if (line_count > 1 && line_count < (config->numof_positions*2)+2) {
      /* 
       * Cause of the way hour sheets are arranged, 
       * hours are on even numbered lines, while
       * earnings are on uneven numbered lines.
       */
      if (line_count % 2){
	for (day_ind = 0; day_ind < 7; day_ind++)
	  sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] = atof(whm_get_string(queue));
	sheet->week[week_ind]->pos_total_earnings[pos_ind] = atof(whm_get_string(queue));
      }
      else{
	for (day_ind = 0; day_ind < 7; day_ind++)
	  sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] = atof(whm_get_string(queue));
	sheet->week[week_ind]->pos_total_hours[pos_ind] = atof(whm_get_string(queue));
      }
    }
    else if (line_count == (config->numof_positions*2)+2){
      for (day_ind = 0; day_ind < 7; day_ind++)
	sheet->week[week_ind]->day[day_ind]->total_hours = atof(whm_get_string(queue));
      sheet->week[week_ind]->total_hours = atof(whm_get_string(queue));
    }
    else if (line_count == (config->numof_positions*2)+3){
      for (day_ind = 0; day_ind < 7; day_ind++)
	sheet->week[week_ind]->day[day_ind]->total_earnings = atof(whm_get_string(queue));
      sheet->week[week_ind]->total_earnings = atof(whm_get_string(queue));
    }
    else {
      errno = WHM_INVALIDELEMCOUNT;
      return -1;
    }
  }
  else {
    /* 
     * Cumulatives:
     * {
     *   Line 0: Total hours followed by total earnings per week day, all positions combined.
     *   Line 1 to numof_positions inclusively: Total hours followed by total earnings per week day, per positions.
     * } x 7
     * {
     *   Line 0: Total hours followed by total earnings for the whole month, all positions combined.
     *   Line 1 to numof_positions inclusively: Total hours followed by total earnings for the whole month, per positions.
     * } x 1
     */
    if (line_count == 0){
      sheet->day_total_hours[day_ind] = atof(whm_get_string(queue));
      sheet->day_total_earnings[day_ind] = atof(whm_get_string(queue));
    }
    else if (line_count > 0 && line_count <= (int)config->numof_positions){
      /* 
       * Here it's safe to use line_count-1 as position counter since 
       * it's garanteed no more than 1 line will be before the begining
       * of positions cumulatives. 
       */
      sheet->day_pos_hours[day_ind][line_count-1] = atof(whm_get_string(queue));
      sheet->day_pos_earnings[day_ind][line_count-1] = atof(whm_get_string(queue));
    }
    else {
      errno = WHM_INVALIDELEMCOUNT;
      return -1;
    }    
  }

  return 0;

} /* whm_queue_to_sheet() */


/* Parse the content of the given hour sheet content. */
int whm_parse_sheet(whm_config_T *config,
			whm_sheet_T *sheet,
			char *content)
{
  int    line_count = 0;
  int    day_ind  = 0, pos_ind = 0, week_ind = 0;
  int    cont_ind = 0, temp_ind = 0;
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

  /* To remove the 'use of uninitialized value' from valgrind. */
  memset(temp, '\0', WHM_NAME_STR_S);
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
  while(content[cont_ind] != '\0'){

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
      else if (content[cont_ind+1] != '\0' && content[cont_ind+1] == STAR){
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
      if (line_count <= 1 || pos_ind >= (int)config->numof_positions) pos_ind = 0;
      else if ((line_count > 1)
	       && (line_count < (config->numof_positions*2)+1)
	       && (line_count % 2)) pos_ind++;
      /*
       * When parsing the calendar part, day_ind is calculated by
       * whm_queue_to_sheet() itself while when parsing the cumulatives
       * part, this function is handling it.
       * Even though there are only 7 days in a week, the 8th index is used
       * to store the monthly total values.
       */
      if (day_ind >= 8) day_ind = 0; 
      if (whm_queue_to_sheet(config, queue, sheet,
			     line_count, week_ind,
			     pos_ind,
			     ((week_ind < 6) ? 0 : day_ind++),
			     ((week_ind < 6) ? 1 : 0)) != 0){ /* 0 When parsing cumulatives, 1 for calendar. */
	WHM_ERRMESG("Whm_queue_to_sheet");
	goto errjmp;
      }
      ++line_count;
      if (content[cont_ind] == NEWLINE){
	++cont_ind;
	if (++week_ind >= 6) {
	  /* Call whm_parse_sheet_cumul() here then break. We're done. 
	     break;*/
	}
	line_count = 0;
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

    /* 
     * When finding a dash as in '--.--' it means no values,
     * internaly whm is expecting -1.0 when there's no values.
     */
    if (content[cont_ind] == DASH){
      do {
	++cont_ind;
      } while (content[cont_ind] != SPACE && content[cont_ind] != NEWLINE);
      if (s_strcpy(temp, (char*)WHM_NO_VALUES, WHM_NAME_STR_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      temp_ind = strlen(temp)+1;
      continue;
    }
    
    /* Save the dot or digit character into temp[temp_ind]. Skip anything else. */
    if (isdigit(content[cont_ind]) ||  content[cont_ind] == DOT)
      temp[temp_ind++] = content[cont_ind];

    /* Next character. */
    ++cont_ind;
  }

  if (queue){
    whm_free_queue_type(queue);
    queue = NULL;
  }

  return 0;
  
 errjmp:
  if (queue){
    whm_free_queue_type(queue);
    queue = NULL;
  }
  return -1;
  

} /* whm_parse_sheet() */


/*
 * whm_update_sheet()'s job is to recalculate the missing fields depending
 * on the information already present. If hours are present, it uses them to
 * calculate the earnings. If only the earnings are present, hours are calculated from 
 * them.
 * Depending on whether it gets paid or not the 4% is also recalculated.
 */
int whm_update_sheet(whm_config_T *config,
		     whm_sheet_T *sheet)
{
  int day_ind = 0, week_ind = 0, pos_ind = 0;

  if (!config || !sheet){
    errno = EINVAL;
    return -1;
  }

  /* 
   * For each date that's not -1,
   * make sure one of 'hours' or 'earnings' isn't -1 (prioritizing hours).
   * Calculate (always make sure values aren't -1 before using them): 
   *  - The money made that day for each positions 
   *    OR
   *    The hours made that day for each positions
   *  - Add each positions' hours and earnings to their respective total of the current date.
   *  - The current total of hours/earnings + optionaly the 4% of all positions combined.
   *  - The current total of hours/earnings of each separate positions.
   * When each days of the sheet has been recalculated
   *  - The total hours/earning per week day, all positions combined.
   *  - The total hours/earning per week day, per position.
   *  - The grand total hours/earnings for this month, all combined.
   *  - The grand total hours/earnings for this months, per positions.
   */
  whm_reset_totals(sheet, config);
  whm_get_day_totals(sheet, config);
  whm_get_week_totals(sheet, config);
  whm_get_sheet_cumuls(sheet, config);

  return 0;

} /* whm_update_sheet() */


void whm_reset_totals(whm_sheet_T *sheet,
		      whm_config_T *config)
{
  int week_ind = 0, day_ind = 0, pos_ind = 0;
  for (; week_ind < 6; week_ind++) {
    sheet->week[week_ind]->total_hours = -1.0;
    sheet->week[week_ind]->total_earnings = -1.0;
    for (day_ind = 0; day_ind < 7; day_ind++) {
      sheet->day_total_hours[day_ind] = -1.0;
      sheet->day_total_earnings[day_ind] = -1.0;
      sheet->week[week_ind]->day[day_ind]->total_hours = -1.0;
      sheet->week[week_ind]->day[day_ind]->total_earnings = -1.0;
      for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++) {
	sheet->day_pos_hours[day_ind][pos_ind] = -1.0;
	sheet->day_pos_earnings[day_ind][pos_ind] = -1.0;
	sheet->week[week_ind]->pos_total_hours[pos_ind] = -1.0;
	sheet->week[week_ind]->pos_total_earnings[pos_ind] = -1.0;
      }
    }
  }
} /* whm_reset_totals() */


/* 
 * No error checks are made on the arguments.
 * Calculate the totals of hours and earnings for each days of a given sheet.
 */
void whm_get_day_totals(whm_sheet_T *sheet,
			whm_config_T *config)
{
  int week_ind = 0, day_ind = 0, pos_ind = 0;

  for (; week_ind < 6; week_ind++) {

    for (day_ind = 0; day_ind < 7; day_ind++) {
      /* Remember -1 is always the default value since 0 is perfectly valid. */      
      if (sheet->week[week_ind]->day[day_ind]->date == -1) continue;

      for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++) {
	/* First, check if we either got hours or an amount of money to play with. */
	if (sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] == -1
	    && sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] == -1) continue;
	/* 
	 * Always prioritize hours but if there's only an amount
	 * of money present, calculate the number of hours worked from it. 
	 */
	if (sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] > -1)
	  sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] =
	    sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] * config->wages[pos_ind];
	else if (sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] > 0)
	  sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] =
	    sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] / config->wages[pos_ind];
	else
	  sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] = 0;

	/* Update the daily totals. */
	sheet->week[week_ind]->day[day_ind]->total_hours +=
	  ((sheet->week[week_ind]->day[day_ind]->total_hours == -1)
	   ? sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] + 1
	   : sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]);
	sheet->week[week_ind]->day[day_ind]->total_earnings +=
	  ((sheet->week[week_ind]->day[day_ind]->total_earnings == -1)
	   ? sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] + 1
	   : sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]);
      }
    }
  }
      
} /* whm_get_day_totals() */


/*
 * No error checks are made on the arguments.
 * Calculate the amount of hours/earnings for each 
 * weeks of an hour sheet. 
 */
void whm_get_week_totals(whm_sheet_T *sheet,
			 whm_config_T *config)
{
  int week_ind = 0, day_ind = 0, pos_ind = 0;

  for (; week_ind < 6; week_ind++) {
    for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++) {
      for (day_ind = 0; day_ind < 7; day_ind++) {
	if (sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] == -1
	    || sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] == -1) continue;

	sheet->week[week_ind]->pos_total_hours[pos_ind] +=
	  ((sheet->week[week_ind]->pos_total_hours[pos_ind] == -1)
	   ? sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]+1
	   : sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]);
	sheet->week[week_ind]->pos_total_earnings[pos_ind] +=
	  ((sheet->week[week_ind]->pos_total_earnings[pos_ind] == -1)
	   ? sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]+1
	   : sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]);
      }

      if (sheet->week[week_ind]->pos_total_hours[pos_ind] != -1
	  && sheet->week[week_ind]->pos_total_earnings[pos_ind] != -1){
	sheet->week[week_ind]->total_hours +=
	  ((sheet->week[week_ind]->total_hours == -1)
	   ? sheet->week[week_ind]->pos_total_hours[pos_ind]+1
	   : sheet->week[week_ind]->pos_total_hours[pos_ind]);
	sheet->week[week_ind]->total_earnings +=
	  ((sheet->week[week_ind]->total_earnings == -1)
	   ? sheet->week[week_ind]->pos_total_earnings[pos_ind]+1
	   : sheet->week[week_ind]->pos_total_earnings[pos_ind]);
      }

    }
    if (config->do_pay_holiday) sheet->week[week_ind]->total_hours +=
				  ((sheet->week[week_ind]->total_hours * 4)/100);
  }
} /* whm_get_week_totals() */


/*
 * No error checks are made on the arguments.
 * Calculate the cumulatives per week day and per position for
 * the current month. 
 */
void whm_get_sheet_cumuls(whm_sheet_T *sheet,
			  whm_config_T *config)
{
  int week_ind = 0, day_ind = 0, pos_ind = 0;

  /* Index 7 is the grand totals for this month. */
  for (; day_ind < 7; day_ind++) {
    for (week_ind = 0; week_ind < 6; week_ind++) {
      for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++) {
	if (sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] != -1
	    && sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] != -1) {
	  sheet->day_pos_hours[day_ind][pos_ind] +=
	    ((sheet->day_pos_hours[day_ind][pos_ind] == -1)
	     ? sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]+1
	     : sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]);
	  sheet->day_pos_earnings[day_ind][pos_ind] +=
	    ((sheet->day_pos_earnings[day_ind][pos_ind] == -1)
	     ? sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]+1
	     : sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]);
	}
      }
      if (sheet->week[week_ind]->day[day_ind]->total_hours != -1
	  && sheet->week[week_ind]->day[day_ind]->total_earnings != -1){
	sheet->day_total_hours[day_ind] +=
	  ((sheet->day_total_hours[day_ind] == -1)
	   ? sheet->week[week_ind]->day[day_ind]->total_hours+1
	   : sheet->week[week_ind]->day[day_ind]->total_hours);
	sheet->day_total_earnings[day_ind] +=
	  ((sheet->day_total_earnings[day_ind] == -1)
	   ? sheet->week[week_ind]->day[day_ind]->total_earnings+1
	   : sheet->week[week_ind]->day[day_ind]->total_earnings);
      }
    }
  }
  for (week_ind = 0; week_ind < 6; week_ind++) {
    for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++) {
      if (sheet->week[week_ind]->pos_total_hours[pos_ind] != -1
	  && sheet->week[week_ind]->pos_total_earnings[pos_ind] != -1) {
	sheet->day_pos_hours[7][pos_ind] +=
	  ((sheet->day_pos_hours[7][pos_ind] == -1)
	   ? sheet->week[week_ind]->pos_total_hours[pos_ind]+1
	   : sheet->week[week_ind]->pos_total_hours[pos_ind]);
	sheet->day_pos_earnings[7][pos_ind] +=
	  ((sheet->day_pos_earnings[7][pos_ind] == -1)
	   ? sheet->week[week_ind]->pos_total_earnings[pos_ind]+1
	   : sheet->week[week_ind]->pos_total_earnings[pos_ind]);
      }
    }
    if (sheet->week[week_ind]->total_hours != -1
	&& sheet->week[week_ind]->total_earnings != -1) {
      sheet->day_total_hours[7] +=
	((sheet->day_total_hours[7] == -1)
	 ? sheet->week[week_ind]->total_hours+1
	 : sheet->week[week_ind]->total_hours);
      sheet->day_total_earnings[7] +=
	((sheet->day_total_earnings[7] == -1)
	 ? sheet->week[week_ind]->total_earnings+1
	 : sheet->week[week_ind]->total_earnings);
    }
  }

} /* whm_get_sheet_cumuls() */







/*
 * whm_inter_update_sheet() will prompt user to enter the 
 * number of hours worked for the current day, for all
 * active companies in the configuration file.
 * This function is assuming the sheets to update have already been
 * read in memory before being called.

 **** This functions is mostly to be called only from whm_automatic_mode() ****

 */
int whm_inter_update_sheet(whm_config_T **configs,
			   whm_sheet_T **sheets,
			   whm_time_T *time_o,
			   int max_ind)
{

  int c_ind = -1;
  char answer[WHM_NAME_STR_S];

  if (!configs || !sheets || !time_o || !max_ind){
    errno = EINVAL;
    return -1;
  }

  while (++c_ind < max_ind) {
    ;
    /* 
     * For each positions of each active companies in the configuration file,
     * ask the user how many hours were worked for the current day.
     */
  }
