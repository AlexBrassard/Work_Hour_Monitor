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
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#include "whm.h"

/*** Used by whm_queue_to_sheet() ***/
static int total_hours_linenum;
static int total_earnings_linenum;
static int saved_week_ind;
static int pos_with_overtime;
enum whm_sheet_constant_type {
  REG_HOURS = 2,
  REG_EARNINGS,
  TS_HOURS,
  TS_EARNINGS
};
static enum whm_sheet_constant_type current_line_type;
/***                              ***/


/* Global list of sheet to be written to disk (whm_main.c). */
extern whm_backup_T *to_write;

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
    return WHM_ERROR;
  }

  /* 
   * Make an absolute pathname, including a unique 
   * filename for the new hour sheet. 
   */
  if (whm_make_sheet_path(sheet->path, time_o, config) == NULL){
    WHM_ERRMESG("Whm_make_sheet_path");
    return WHM_ERROR;
  }

  /* 
   * Try to open the file at the pathname we just created.
   * If it succeed close the stream and return an error. 
   */
  if ((stream = fopen(sheet->path, "r")) != NULL) {
    fclose(stream);
    stream = NULL;
    errno = WHM_FILEEXIST;
    return WHM_ERROR;
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
    return WHM_ERROR;
  }

  fprintf(stream, "/*\n * Work Hour Monitor\n * Heures travaillees chez %s pour le mois de %s %s.\n \
*\n *\n *\n * Respectez le format et les espacements lors des modifications manuelles de ce fichier.\n *\n */\n\n\n\n",
	  config->employer, WHM_FR_MONTHS[atoi(time_o->month)-1],
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
  int have_done_overtime = 0;
  char temp[WHM_MAX_PATHNAME_S]; /* s_itoa() needs a temporary buffer. */
  
  /* -18s: WHM_NAME_STR_S == 18. */

  if (!stream || !config
      || !time_o || !sheet){
    errno = EINVAL;
    return WHM_ERROR;
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
     * p1_name          p1_worked_hours
     *                  p1_earnings
     * p1_over_time     p1_ot_worked_hours
     *                  p1_ot_earnings
     * p2_name          p2_worked_hours
     *                  p2_earnings
     * p2_over_time     p2_ot_worked_hours
     *                  p2_ot_earnings
     * pN_name          pN_worked_hours
     *                  pN_earnings
     * pN_over_time     pN_ot_worked_hours
     *                  pN_ot_earnings
     * ...
     */

    for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++){
      fprintf(stream, "%-18s ", config->positions[pos_ind]);
      for (day_ind = 0; day_ind < 7; day_ind++){
	fprintf(stream, "%5.5s      ",
		((sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] <= 0)
		 ? WHM_NO_HOUR
		 : s_ftoa(temp, sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind], WHM_NAME_STR_S)));
      }
      fprintf(stream, "  %6.6s",
	      ((sheet->week[week_ind]->pos_total_hours[pos_ind] <= 0)
	       ? WHM_NO_THOUR
	       : s_ftoa(temp, sheet->week[week_ind]->pos_total_hours[pos_ind], WHM_NAME_STR_S)));
      fprintf(stream, "\n                   ");
      for (day_ind = 0; day_ind < 7; day_ind++){
	fprintf(stream, "%6.6s$    ",
		((sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] <= 0)
		 ? WHM_NO_CASH
		 : s_ftoa(temp, sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind], WHM_NAME_STR_S)));
      }
      fprintf(stream, "                   %8.8s$",
	      ((sheet->week[week_ind]->pos_total_earnings[pos_ind] <= 0)
	       ? WHM_NO_TCASH
	       : s_ftoa(temp, sheet->week[week_ind]->pos_total_earnings[pos_ind], WHM_NAME_STR_S)));
      fprintf(stream, "\n");
      /* 
       * Check if overtime was made for any day during the current week.
       * If yes, for each days of the current week, do like above, but with the OT arrays.
       */
      for (day_ind = 0; day_ind < 7; day_ind++)
	if (sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] > 0
	    || sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind] > 0) {
	  have_done_overtime++;
	  break;
	}	  
      if (have_done_overtime) {
	fprintf(stream, "TS:                ");
	for (day_ind = 0; day_ind < 7; day_ind++){
	  fprintf(stream, "%5.5s      ",
		  ((sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] <= 0)
		   ? WHM_NO_HOUR
		   : s_ftoa(temp, sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind], WHM_NAME_STR_S)));
	}
	fprintf(stream, "  %6.6s",
		((sheet->week[week_ind]->pos_ot_total_hours[pos_ind] <= 0)
		 ? WHM_NO_THOUR
		 : s_ftoa(temp, sheet->week[week_ind]->pos_ot_total_hours[pos_ind], WHM_NAME_STR_S)));
	fprintf(stream, "\n                   ");

	for (day_ind = 0; day_ind < 7; day_ind++){
	  fprintf(stream, "%6.6s$    ",
		  ((sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind] <= 0)
		   ? WHM_NO_CASH
		   : s_ftoa(temp, sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind], WHM_NAME_STR_S)));
	}
	fprintf(stream, "                   %8.8s$",
		((sheet->week[week_ind]->pos_ot_total_earnings[pos_ind] <= 0)
		 ? WHM_NO_TCASH
		 : s_ftoa(temp, sheet->week[week_ind]->pos_ot_total_earnings[pos_ind], WHM_NAME_STR_S)));
	fprintf(stream, "\n");
      }
      have_done_overtime = 0;
    }
    /* Daily totals goes here. */
    fprintf(stream, "Total Heures:      ");
    for (day_ind = 0; day_ind < 7; day_ind++){
      fprintf(stream, "%6.6s     ",
	      ((sheet->week[week_ind]->day[day_ind]->total_hours <= 0)
	       ? WHM_NO_THOUR
	       : s_ftoa(temp, sheet->week[week_ind]->day[day_ind]->total_hours, WHM_NAME_STR_S)));
    }
    fprintf(stream, "  %6.6s\n",
	    ((sheet->week[week_ind]->total_hours <= 0)
	     ? WHM_NO_THOUR
	     : s_ftoa(temp, sheet->week[week_ind]->total_hours, WHM_NAME_STR_S)));

    fprintf(stream, "Total Gains:       ");
    for (day_ind = 0; day_ind < 7; day_ind++){
      fprintf(stream, "%8.8s$  ",
	      ((sheet->week[week_ind]->day[day_ind]->total_earnings <= 0)
	       ? WHM_NO_TCASH
	       : s_ftoa(temp, sheet->week[week_ind]->day[day_ind]->total_earnings, WHM_NAME_STR_S)));
    }
    fprintf(stream, "                   %8.8s$\n",
	    ((sheet->week[week_ind]->total_earnings <= 0)
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
    return WHM_ERROR;
  }

  fprintf(stream, "\n/************************************************************************************/\nCumulatifs:\n\n\n");

  /* Keep in mind that at sheet->day_*_* day_index 7 are the monthly totals. */
  for (day_ind = 0; day_ind < 8; day_ind++){
    if (day_ind < 7) fprintf(stream, "%-18s ", WHM_FR_DAYS[day_ind]);
    else fprintf(stream, "\nGrand Total:       ");
    /* Print the total worked hours for all 'day_ind' day this month, all positions combined. */
    fprintf(stream, "%6.6s   ",
	    ((sheet->day_total_hours[day_ind] <= 0)
	     ? WHM_NO_THOUR
	     : s_ftoa(temp, sheet->day_total_hours[day_ind], WHM_NAME_STR_S)));
    /* Print the total earned for all 'day_ind' day this month, all positions combined. */
    fprintf(stream, "%8.8s\n",
	    ((sheet->day_total_earnings[day_ind] <= 0)
	     ? WHM_NO_TCASH
	     : s_ftoa(temp, sheet->day_total_earnings[day_ind], WHM_NAME_STR_S)));

    /* Same as above, but broke down per positions. */
    for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++){
      fprintf(stream, "%-18s ", config->positions[pos_ind]);
      fprintf(stream, "%6.6s   ",
	      ((sheet->day_pos_hours[day_ind][pos_ind] <= 0)
	       ? WHM_NO_THOUR
	       : s_ftoa(temp, sheet->day_pos_hours[day_ind][pos_ind], WHM_NAME_STR_S)));
      fprintf(stream, "%8.8s\n",
	      ((sheet->day_pos_earnings[day_ind][pos_ind] <= 0)
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
    return WHM_ERROR;
  }

  /* Print a header message to the sheet. */
  if (whm_print_sheet_head(stream, config, time_o) != 0) {
    WHM_ERRMESG("Whm_print_sheet_head");
    return WHM_ERROR;
  }
  /* Print the calendar part of the hour sheet. */
  if (whm_print_sheet_cal(stream, config, time_o, sheet) != 0){
    WHM_ERRMESG("Whm_print_sheet_cal");
    return WHM_ERROR;
  }
  /* Call whm_print_sheet_cumul() to print the cumulatives part of the hour sheet. */
  if (whm_print_sheet_cumul(stream, config, time_o, sheet) != 0){
    WHM_ERRMESG("Whm_print_sheet_cumul");
    return WHM_ERROR;
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
    return WHM_ERROR;
  }

  if ((stream = fopen(pathname, "r")) == NULL){
    if (errno != ENOENT)
      WHM_ERRMESG("Fopen");
    return WHM_ERROR;
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
  return WHM_ERROR;

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
  int loc_line_type = 0;
  
  if (!sheet || !config
      || !queue || line_count < 0
      || week_ind < 0 || pos_ind < 0
      || day_ind < 0){
    errno = EINVAL;
    return WHM_ERROR;
  }

  if (is_cal){
    /* saved_week_ind is -1 when it has been reseted by whm_parse_sheet(). */
    if (saved_week_ind == -1) {
      ;
    }
    /* Incremented by whm_parse_sheet() if needed. */
    if (pos_with_overtime > 0){
      total_hours_linenum = ((config->numof_positions*2)+(pos_with_overtime*2)+2);
      total_earnings_linenum = ((config->numof_positions*2)+(pos_with_overtime*2)+3);
      saved_week_ind = week_ind;
    }

    /* 
     * Queue's content depends on the line number:
     * Line 0:                          the week number.
     * Line 1:                          the dates.
     * Line 2 to (numof_positions*2)+1: hours/cash of each positions + overtime (optionally).
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
    else if (line_count > 1 && line_count < total_hours_linenum) {
      /* If saved_week_ind isn't -1, it means we must take account the extra lines of overtime. */
      if (saved_week_ind != -1){
	loc_line_type = current_line_type * pos_with_overtime;
	if (loc_line_type == REG_HOURS * pos_with_overtime){
	  for (day_ind = 0; day_ind < 7; day_ind++)
	    sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] = atof(whm_get_string(queue));
	  sheet->week[week_ind]->pos_total_hours[pos_ind] = atof(whm_get_string(queue));
	}
	else if (loc_line_type == REG_EARNINGS * pos_with_overtime) {
	  for (day_ind = 0; day_ind < 7; day_ind++)
	    sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] = atof(whm_get_string(queue));
	  sheet->week[week_ind]->pos_total_earnings[pos_ind] = atof(whm_get_string(queue));
	}
	else if (loc_line_type == TS_HOURS * pos_with_overtime) {
	  for (day_ind = 0; day_ind < 7; day_ind++)
	    sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] = atof(whm_get_string(queue));
	  sheet->week[week_ind]->pos_ot_total_hours[pos_ind] = atof(whm_get_string(queue));
	}
	else if (loc_line_type == TS_EARNINGS * pos_with_overtime) {
	  for (day_ind = 0; day_ind < 7; day_ind++)
	    sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind] = atof(whm_get_string(queue));
	  sheet->week[week_ind]->pos_ot_total_earnings[pos_ind] = atof(whm_get_string(queue));
	}
	else {
	  errno = WHM_INVALIDELEMCOUNT;
	  return WHM_ERROR;
	}
	if (++current_line_type > TS_EARNINGS) current_line_type = REG_HOURS;
      }
      else {
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
    }
    else if (line_count == total_hours_linenum){
      for (day_ind = 0; day_ind < 7; day_ind++)
	sheet->week[week_ind]->day[day_ind]->total_hours = atof(whm_get_string(queue));
      sheet->week[week_ind]->total_hours = atof(whm_get_string(queue));
    }
    else if (line_count == total_earnings_linenum){
      for (day_ind = 0; day_ind < 7; day_ind++)
	sheet->week[week_ind]->day[day_ind]->total_earnings = atof(whm_get_string(queue));
      sheet->week[week_ind]->total_earnings = atof(whm_get_string(queue));
    }
    else {
      errno = WHM_INVALIDELEMCOUNT;
      return WHM_ERROR;
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
      return WHM_ERROR;
    }    
  }

  return 0;

} /* whm_queue_to_sheet() */


/* Parse the content of the given hour sheet content. */
int whm_parse_sheet(whm_config_T *config,
			whm_sheet_T *sheet,
			char *content)
{
  int    line_count = 0, is_cal = 0;
  int    day_ind  = 0, pos_ind = 0, week_ind = 0;
  int    cont_ind = 0, temp_ind = 0, ot_ind = 0;
  whm_queue_T *queue = NULL;
  char   temp[WHM_NAME_STR_S];
  char   ot_string[WHM_NAME_STR_S];

  if (!sheet || !config  || !content){
    errno = EINVAL;
    return WHM_ERROR;
  }
  if ((queue = whm_init_queue_type(WHM_DEF_QUEUE_SIZE, WHM_NAME_STR_S)) == NULL){
    WHM_ERRMESG("Whm_init_queue_type");
    return WHM_ERROR;
  }

  /* Because they're still uninitialized (static, declared after headers). */
  saved_week_ind = -1;
  pos_with_overtime = 0;
  current_line_type = REG_HOURS;
  total_hours_linenum = (config->numof_positions*2)+2;
  total_earnings_linenum = (config->numof_positions*2)+3;

  /* To remove the 'use of uninitialized value' from valgrind. */
  memset(temp, '\0', WHM_NAME_STR_S);
  memset(ot_string, '\0', WHM_NAME_STR_S);
  /* 
   * Only register the digits and dots characters. 
   * The only way to reach the bottom of the loop is 
   * for all conditions to be false.
   * Most 'if' statements aren't followed by 'else if' or 'else'
   * statements cause say an 'outter if' succeeds but one of its
   * 'inner if' fails, I want the conditions bellow to be verified
   * without having to terminate the current loop itteration right away.
   * Felt I have to justify myself..
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
	if (whm_skip_sheet_comments(sheet, content, &cont_ind, 0) != 0){
	  WHM_ERRMESG("Whm_skip_comments");
	  goto errjmp;
	}
	continue;
      }
      /* Multi-lines, C89-style comments. */
      else if (content[cont_ind+1] != '\0' && content[cont_ind+1] == STAR){
	if (whm_skip_sheet_comments(sheet, content, &cont_ind, 1) != 0){
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
     * increment week_ind.
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
      
      /*
       * When parsing the calendar part, day_ind is calculated by
       * whm_queue_to_sheet() itself while when parsing the cumulatives
       * part, this function is handling it.
       * Even though there are only 7 days in a week, the 8th index is used
       * to store the monthly total values.
       */
      if (week_ind < 6) is_cal = 1;
      else is_cal = 0;
      if (whm_queue_to_sheet(config, queue, sheet,
			     line_count, week_ind,
			     pos_ind,
			     day_ind,
			     is_cal) != 0){ /* 0 When parsing cumulatives, 1 for calendar. */
	WHM_ERRMESG("Whm_queue_to_sheet");
	goto errjmp;
      }
      if ((line_count > 1)
	  && (line_count < total_hours_linenum))
	if ((saved_week_ind != -1
	     && pos_with_overtime > 0
	     && current_line_type == REG_HOURS)
	    || (line_count % 2))
	  ++pos_ind;
      ++line_count;

      if (content[cont_ind] == NEWLINE){
	++cont_ind;
	/* Once week_ind is bigger than 6, it doesn't matter anymore. */
	++week_ind;
	/* Reset static variables now. */
	total_hours_linenum = (config->numof_positions*2)+2;
	total_earnings_linenum = (config->numof_positions*2)+3;
	saved_week_ind = -1;
	pos_with_overtime = 0;
	current_line_type = REG_HOURS;
	line_count = 0;
	if (week_ind < 7 || day_ind > 7) day_ind = 0;
	else ++day_ind;
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
      if (ot_ind){
	ot_string[ot_ind] = '\0';
	if (s_strcmp(ot_string, "TS", 2, LS_ICASE) == 0){
	  /* Since seeing a newline increments pos_ind but we aren't done with the previous position. */
	  --pos_ind;	  
	  current_line_type = TS_HOURS;
	  ++pos_with_overtime;
	}
	memset(ot_string, '\0', WHM_NAME_STR_S);
	ot_ind = 0;
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
    if (isalpha(content[cont_ind]))
      ot_string[ot_ind++] = content[cont_ind];
    
    /* Save the dot or digit character into temp[temp_ind]. Skip anything else. */
    else if (isdigit(content[cont_ind]) ||  content[cont_ind] == DOT)
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
  return WHM_ERROR;
  

} /* whm_parse_sheet() */


/*
 * whm_update_sheet()'s job is to recalculate the missing fields depending
 * on the information already present. If hours are present, it uses them to
 * calculate the earnings. If only the earnings are present, hours are calculated from 
 * them.
 * Depending on whether it gets paid or not the 4% is also recalculated.
 */
int whm_update_sheet(whm_config_T *config,
		     whm_sheet_T *sheet,
		     whm_time_T *time_o)
{
  int week_ind = 0, day_ind = 0, pos_ind = 0;
  int ot_limit = 40;
  int wage = 0;
  
  if (!config || !sheet) {
    errno = EINVAL;
    return WHM_ERROR;
  }

  whm_reset_totals(sheet, config);
  whm_reset_overtime(sheet, config);
  if (sheet->week[0]->day[0]->total_hours == -1)
    if (whm_complete_week(sheet, config, time_o) == WHM_ERROR){
      WHM_ERRMESG("Whm_complete_week");
      return WHM_ERROR;
    }

  for (; week_ind < 6; week_ind++) {
    int overtime_reached = 0; /* 1 when overtime starts being calculated. */
    for (day_ind = 0; day_ind < 7; day_ind++) {
      for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++) {
	if (sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] == -1) continue;
	sheet->week[week_ind]->total_hours +=
	  ((sheet->week[week_ind]->total_hours == -1)
	   ? sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] +1
	   : sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]);
	sheet->week[week_ind]->day[day_ind]->total_hours +=
	  ((sheet->week[week_ind]->day[day_ind]->total_hours == -1)
	   ? sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] +1
	   : sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]);

	/* If it's a night shift, add the night prime now if there's one. */

	/* When making overtime. */
	if (sheet->week[week_ind]->total_hours >= ot_limit) {
	  if (!overtime_reached) {
	    ++overtime_reached;
	    sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] =
	      sheet->week[week_ind]->total_hours - ot_limit;
	    sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] -=
	      sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind];	    
	  }
	  else {
	    sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] =
	      sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind];
	    sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] = -1;
	    sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] = -1;
	  }
	  if (sheet->week[week_ind]->total_hours >= 50
	      && config->double_time_after_50) wage = config->wages[pos_ind] * 2;
	  else if (config->time_n_half_after_40) wage = config->wages[pos_ind] + (config->wages[pos_ind]/2);
	  else wage = config->wages[pos_ind];
	  sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind] =
	    sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] * wage;
	  sheet->week[week_ind]->day[day_ind]->total_earnings +=
	    (sheet->week[week_ind]->day[day_ind]->total_earnings == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind]+1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind];
	  sheet->week[week_ind]->total_earnings +=
	    (sheet->week[week_ind]->total_earnings == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind]+1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind];
	  sheet->week[week_ind]->pos_ot_total_earnings[pos_ind] +=
	    (sheet->week[week_ind]->pos_ot_total_earnings[pos_ind] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind] +1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind];
	  sheet->week[week_ind]->pos_ot_total_hours[pos_ind] +=
	    (sheet->week[week_ind]->pos_ot_total_hours[pos_ind] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] + 1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind];
	  /* To be replaced by overtime cumulatives. */
	  sheet->day_total_hours[day_ind] +=
	    (sheet->day_total_hours[day_ind] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind]+1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind];
	  sheet->day_total_earnings[day_ind] +=
	    (sheet->day_total_earnings[day_ind] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind]+1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind];
	  sheet->day_pos_hours[day_ind][pos_ind] +=
	    (sheet->day_pos_hours[day_ind][pos_ind] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind]+1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind];
	  sheet->day_pos_earnings[day_ind][pos_ind] +=
	    (sheet->day_pos_earnings[day_ind][pos_ind] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind]+1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind];
	  sheet->day_total_hours[7] +=
	    (sheet->day_total_hours[7] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind]+1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind];
	  sheet->day_total_earnings[7] +=
	    (sheet->day_total_earnings[7] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind]+1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind];
	  sheet->day_pos_hours[7][pos_ind] +=
	    (sheet->day_pos_hours[7][pos_ind] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind]+1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind];
	  sheet->day_pos_earnings[7][pos_ind] +=
	    (sheet->day_pos_earnings[7][pos_ind] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind]+1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind];
	}
	if (sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] < 0) continue;
	wage = config->wages[pos_ind];
	sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] =
	  sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] * wage;
	sheet->week[week_ind]->day[day_ind]->total_earnings +=
	  (sheet->week[week_ind]->day[day_ind]->total_earnings == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind];
	sheet->week[week_ind]->total_earnings +=
	  (sheet->week[week_ind]->total_earnings == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind];
	sheet->week[week_ind]->pos_total_earnings[pos_ind] +=
	  (sheet->week[week_ind]->pos_total_earnings[pos_ind] == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind];
	sheet->week[week_ind]->pos_total_hours[pos_ind] +=
	  (sheet->week[week_ind]->pos_total_hours[pos_ind] == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind];
	/* Cumulatives */
	sheet->day_total_hours[day_ind] +=
	  (sheet->day_total_hours[day_ind] == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind];
	sheet->day_total_earnings[day_ind] +=
	  (sheet->day_total_earnings[day_ind] == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind];
	sheet->day_pos_hours[day_ind][pos_ind] +=
	  (sheet->day_pos_hours[day_ind][pos_ind] == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind];
	sheet->day_pos_earnings[day_ind][pos_ind] +=
	  (sheet->day_pos_earnings[day_ind][pos_ind] == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind];
	sheet->day_total_hours[7] +=
	  (sheet->day_total_hours[7] == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind];
	sheet->day_total_earnings[7] +=
	  (sheet->day_total_earnings[7] == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind];
	sheet->day_pos_hours[7][pos_ind] +=
	  (sheet->day_pos_hours[7][pos_ind] == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind];
	sheet->day_pos_earnings[7][pos_ind] +=
	  (sheet->day_pos_earnings[7][pos_ind] == -1)
	  ? sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind]+1
	  : sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind];
      }
    }
    if (config->do_pay_holiday && sheet->week[week_ind]->total_earnings > -1)
      sheet->week[week_ind]->total_earnings +=
	(sheet->week[week_ind]->total_earnings * 4)/100;
    
  }

  return 0;

} /* whm_update_sheet() */

/* 
 * Changes any number of overtime hours to regular hours
 * and sets the overtime fields to -1.
 */
void whm_reset_overtime(whm_sheet_T *sheet,
			whm_config_T *config)
{
  int week_ind = 0, day_ind = 0, pos_ind = 0;
  for(; week_ind < 6; week_ind++)
    for (day_ind = 0; day_ind < 7; day_ind++)
      for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++)
	if (sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] != -1){
	  sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] +=
	    (sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] == -1)
	    ? sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] + 1
	    : sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind];
	  sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] = -1;
	}

} /* whm_reset_overtime() */


/* 
 * When encountering a month wich begin by an incomplete week,
 * (the first of the month is not on a sunday) this function
 * returns the number of hours of the last [incomplete] week
 * of the previous month, or 0 if it hasn't been created yet.
 */
int whm_complete_week(whm_sheet_T *sheet,
		      whm_config_T *config,
		      whm_time_T *time_o)
{
  whm_time_T *loc_time_o = NULL;
  whm_sheet_T *prev_month = NULL;
  int month_ind = 0, week_ind = 5;
  char prev_month_path[WHM_MAX_PATHNAME_S];
  char temp[WHM_TIME_STR_S];
  
  if (!sheet || !config || !time_o) {
    errno = EINVAL;
    return WHM_ERROR;
  }

  if ((loc_time_o = whm_init_time_type()) == NULL) {
    WHM_ERRMESG("Whm_init_type_type");
    return WHM_ERROR;
  }
  if ((prev_month = whm_init_sheet_type()) == NULL) {
    WHM_ERRMESG("Whm_init_sheet_type");
    goto errjmp;
  }

  if (whm_copy_time((sheet->time_o) ? sheet->time_o : loc_time_o,
		    time_o) == NULL){
    WHM_ERRMESG("Whm_copy_time");
    goto errjmp;
  }
  for (month_ind = 0; month_ind < 12; month_ind++)
    if (s_strcmp(loc_time_o->month, WHM_EN_MONTHS[month_ind], WHM_TIME_STR_S, 0) == 0
	|| atoi(loc_time_o->month) == month_ind+1) break;
  if (month_ind > 11) {
    errno = WHM_INVALIDMONTH;
    goto errjmp;
  }
  if (month_ind-1 < 0) month_ind = 11;
  if (s_strcpy(loc_time_o->month, s_itoa(temp, month_ind, WHM_TIME_STR_S), WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }

  memset(prev_month_path, '\0', WHM_MAX_PATHNAME_S);
  if (whm_make_sheet_path(prev_month_path, loc_time_o, config) == NULL){
    WHM_ERRMESG("Whm_make_sheet_path");
    goto errjmp;
  }
  if (whm_read_sheet(prev_month_path, config, time_o, prev_month) == WHM_ERROR){
    if (errno == ENOENT) goto success;
    else {
      WHM_ERRMESG("Whm_read_sheet");
      goto errjmp;
    }
  }
  for (; week_ind >= 0; week_ind--)
    if ((sheet->week[0]->total_hours = prev_month->week[week_ind]->total_hours) != -1) break;
  if (week_ind < 0) week_ind = 0;
  sheet->week[0]->total_earnings = prev_month->week[week_ind]->total_earnings;

 success:
  if (loc_time_o) {
    whm_free_time_type(loc_time_o);
    loc_time_o = NULL;
  }
  if (prev_month) {
    whm_free_sheet_type(prev_month);
    prev_month = NULL;
  }
  return 0;
  
 errjmp:
  if (loc_time_o) {
    whm_free_time_type(loc_time_o);
    loc_time_o = NULL;
  }
  if (prev_month) {
    whm_free_sheet_type(prev_month);
    prev_month = NULL;
  }
  return WHM_ERROR;


} /* whm_complete_week() */
    


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
	sheet->week[week_ind]->pos_ot_total_hours[pos_ind] = -1.0;
	sheet->week[week_ind]->pos_ot_total_earnings[pos_ind] = -1.0;
      }
    }
  }
  sheet->day_total_hours[7] = -1.0;
  sheet->day_total_earnings[7] = -1.0;
  for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++){
    sheet->day_pos_hours[7][pos_ind] = -1.0;
    sheet->day_pos_earnings[7][pos_ind] = -1.0;
  }
} /* whm_reset_totals() */


/* 
 * Gives the number of positions worked in a single given day for a company.
 * Returns 0 when no positions we're worked for the given day.
 * Returns -x when only 1 position has been worked, where x is the negated pos_ind.
 * Returns +x when more than 1 positions were worked, where x is the number of positions worked.
 */
int whm_get_numof_pos_worked(whm_sheet_T *sheet,
			     whm_config_T *config,
			     int week_ind, int day_ind)
{
  int pos_ind = 0, saved_pos_ind = 0, numof_pos_worked = 0;
  for(; pos_ind < config->numof_positions; pos_ind++)
    if (sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] > -1) {
      ++numof_pos_worked;
      saved_pos_ind = pos_ind;
    }

  if (numof_pos_worked > 1) return numof_pos_worked;
  else if (numof_pos_worked == 1) return -saved_pos_ind;
  return 0;

} /* whm_get_numof_pos_worked() */


/* Calculates the overtime hours and earnings and adjust the week data consequently. */
int whm_calculate_overtime(whm_sheet_T *sheet,
			   whm_config_T *config,
			   int ot_type,
			   int week_ind,
			   int last_worked_day)
{
  double wage = 0, diff_from_threshold = 0;
  int ot_threshold = -1;
  int ret = 0, pos_ind = 0, day_ind = last_worked_day;

  /* 
   * Verify that the overtime isn't split between multiple
   * positions for the last worked day.
   */
  errno = 0;
  if ((ret = whm_get_numof_pos_worked(sheet, config, week_ind, last_worked_day)) > 0)
    ; /* Interactively ask user which positions the overtime goes to. */

  else if (ret == 0 && errno) {
    errno = WHM_INVALIDDAY;
    return WHM_ERROR;
  }

  /* 
   * ret is negative, only one position was worked for the given day
   * meaning ret's absolute value is the pos_ind of the worked position. 
   */
  pos_ind = (ret == 0) ? ret : abs(ret);
  switch (ot_type){
  case OT_TIME_N_HALF:
    wage = (config->wages[pos_ind]/2)+config->wages[pos_ind];
    ot_threshold = 40;
    break;
  case OT_DOUBLE_TIME:
    wage = config->wages[pos_ind]*2;
    ot_threshold = 50;
    break;
  default:
    errno = WHM_INVALIDOT;
    return WHM_ERROR;
  }
  /* 
   * Calculate the difference between the total hours of the week
   * and the overtime threshold and substract it from one or more shifts
   * until we reach the threshold.
   */
  diff_from_threshold = sheet->week[week_ind]->total_hours - ot_threshold;

  while (diff_from_threshold > sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind]) {
    sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] =
      sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind];
    sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind] =
      sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] * wage;
    diff_from_threshold -= sheet->week[week_ind]->day[day_ind]->total_hours;
    sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] = 0;
    sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] = 0;
    /* Get the last worked day from the current day index. */
    if ((day_ind = whm_last_worked_day(sheet, config, week_ind, day_ind)) == WHM_ERROR){
      WHM_ERRMESG("Whm_last_worked_day");
      return WHM_ERROR;
    }
  }
  sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] -= diff_from_threshold;
  sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] =
    sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] * config->wages[pos_ind];
  sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] +=
    (sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind] == -1)
    ? diff_from_threshold + 1
    : diff_from_threshold;
  sheet->week[week_ind]->day[day_ind]->pos_ot_earnings[pos_ind] =
    sheet->week[week_ind]->day[day_ind]->pos_ot_hours[pos_ind];
  
  return 0;

} /* whm_calculate_overtime() */


/*
 * When day_ind is bigger or equal to 0, starts at day_ind,
 * else starts at the last weekday (saturday, index 6) and
 * returns the last worked day for the given week.
 */
int whm_last_worked_day(whm_sheet_T *sheet,
			whm_config_T *config,
			int week_ind,
			int day_ind)
{
  int loc_day_ind = 0;
  int pos_ind = 0;

  if (week_ind < 0 || week_ind > 6){
    errno = WHM_INVALIDMONTH;
    return WHM_ERROR;
  }
  if (day_ind > 6 || day_ind < 0) loc_day_ind = 6;
  else if (day_ind > 1) loc_day_ind = day_ind-1;
  else loc_day_ind = 0;
  for (; loc_day_ind >= 0; loc_day_ind--){
    if (sheet->week[week_ind]->day[loc_day_ind]->total_hours > 0)
      return loc_day_ind;
    for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++)
      if (sheet->week[week_ind]->day[loc_day_ind]->pos_ot_hours[pos_ind] > 0)
	return loc_day_ind;
  }

  return WHM_ERROR;
  
} /* whm_last_worked_day() */

/*
 * Look for and calculate the overtime made for each days
 * of each weeks of the given hour sheet.
 */
void whm_get_sheet_overtime(whm_sheet_T *sheet,
			    whm_config_T *config)
{
  int week_ind = 0, day_ind = 0, pos_ind = 0;
  double diff_from_50 = 0, diff_from_40 = 0;
  int numof_pos_worked = 0;
  int saved_pos_ind = -1, saved_day_ind = 0;
  /* 
   * Verify that the overtime gets paid or else return now.
   * If it does, check if the wage gets doubled after 50.
   * If it does, calculate the numbers of hours at double time there are,
   * substract them from the position's total of [regular] hours and from the position's
   * daily total of [regular] hours then add them
   * to the total of overtime worked for this position for the current day.
   * Make sure that the number of overtime hours is smaller than the number
   * of hours worked during the current day, else continue the same process
   * on the last worked day(s).
   * From either the new weekly total or the original, which are now both less than 50 hours,
   * calculate the number of time and a half hours worked. Repeat the same process 
   * as above (put it into a function).
   *
   * When for a single day 2+ positions were worked and the overtime limit is being reached
   * but does not cover the whole work day, ask the user the
   * number of overtime hours worked for each positions.
   */
  if (!config->time_n_half_after_40) return;
  /* For each weeks, check if overtime was made. */
  for (; week_ind < 6; week_ind++) {
    if (sheet->week[week_ind]->total_hours >= 40){
      if ((day_ind = whm_last_worked_day(sheet, config, week_ind, -1)) == WHM_ERROR){
	WHM_ERRMESG("Whm_last_worked_day");
	return;
      }
      /* If more than 50 hours were made, check if wage gets doubled. */      
      if (config->double_time_after_50 && sheet->week[week_ind]->total_hours >= 50){
	if (whm_calculate_overtime(sheet, config, OT_DOUBLE_TIME,
				   week_ind, day_ind) != 0){
	  WHM_ERRMESG("Whm_calculate_overtime");
	  return;
	}
      }
      if (whm_calculate_overtime(sheet, config, OT_TIME_N_HALF,
				 week_ind, day_ind) != 0){
	WHM_ERRMESG("Whm_calculate_overtime");
	return;
      }
    }
  }
      
  
} /* whm_get_sheet_overtime() */


/*
 * whm_inter_update_sheet() will prompt user to enter the 
 * number of hours worked for the current day, for all
 * active companies in the configuration file.
 * This function is assuming the sheets to update have already been
 * read in memory (by whm_automatic_mode()) before being called.
 */
int whm_inter_update_sheet(whm_config_T **configs,
			   whm_sheet_T **sheets,
			   whm_time_T *time_o,
			   int max_ind) /* max_ind is 1 more than the last element of both arrays. */
{

  int c_ind = -1, week_ind = 0, day_ind = 0, pos_ind = 0;
  int date = 0, i = 0;
  char answer[WHM_NAME_STR_S];

  if (!configs || !sheets || !time_o || !max_ind){
    errno = EINVAL;
    return WHM_ERROR;
  }

  /*  Get the week and day indexes of the current day. */
  while (day_ind < 7){
    if (s_strstr(WHM_EN_DAYS[day_ind], time_o->day, WHM_TIME_STR_S, 0) >= 0)
      break;
    ++day_ind;
  }
  if (day_ind >= 7) {
    errno = WHM_INVALIDELEMCOUNT;
    return WHM_ERROR;
  }
  date = atoi(time_o->date);
  i = day_ind;
  while (date-- != 0)
    if (i-- <= 0) {
      i = 6;
      ++week_ind;
    }

  /* 
   * For each positions of each active companies in the configuration file,
   * ask the user how many hours were worked for the current day.
   */
  while (++c_ind < max_ind) {
    if (!configs[c_ind]->status) continue;

    /* Set the sheet into the global list. */
    if (whm_set_sheet(configs[c_ind], sheets[c_ind]) != 0){
      WHM_ERRMESG("Whm_set_sheet");
      return WHM_ERROR;
    }
    
    /* 
     * Look at the pos_hours for the current day to see if
     * hours have already been wrote down.
     */
    for (pos_ind = 0; pos_ind < configs[c_ind]->numof_positions; pos_ind++)
      if (sheets[c_ind]->week[week_ind]->day[day_ind]->pos_hours[pos_ind] <= 0) {
	if (whm_ask_user(SHEET_WORKED_HOURS,
			 answer, WHM_NAME_STR_S,
			 configs[c_ind], pos_ind) != 0){
	  WHM_ERRMESG("Whm_ask_user");
	  return WHM_ERROR;
	}
	/* Input the hours. */
	sheets[c_ind]->week[week_ind]->day[day_ind]->pos_hours[pos_ind] = atof(answer);
      }
    /* Update the sheet. */
    if (whm_update_sheet(configs[c_ind], sheets[c_ind], time_o) != 0){
      WHM_ERRMESG("Whm_update_sheet");
      return WHM_ERROR;
    }
  }

  return 0;

} /* whm_inter_update_sheet() */


/* Reset all fields of a whm_sheet_T* object to their default values. */
int whm_rm_sheet(whm_config_T *config,
		 whm_sheet_T *sheet)
{

  if (!config || !sheet) {
    errno = EINVAL;
    return WHM_ERROR;
  }
  /* 
   * It is possible that the sheet hasn't been written to disk yet,
   * silently ingnore a failed called to unlink with errno set to ENOENT.
   */
  if (unlink(sheet->path) != 0)
    if (errno != ENOENT) {
      WHM_ERRMESG("Unlink");
      return WHM_ERROR;
    }
  if (whm_reset_sheet(config, sheet) != 0) {
    WHM_ERRMESG("Whm_reset_sheet");
    return WHM_ERROR;
  }

  return 0;
} /* whm_rm_sheet() */


/* Reset all whm_sheet_T fields to their default. */
int whm_reset_sheet(whm_config_T *config,
		    whm_sheet_T *sheet)
{
  int week_ind = 0, day_ind = 0, pos_ind = 0;

  if (!sheet || !config) {
    errno = EINVAL;
    return WHM_ERROR;
  }
  memset(sheet->path, '\0', WHM_MAX_PATHNAME_S);
  sheet->year = 0;
  sheet->month = 0;
  for (; week_ind < 6; week_ind++){
    for (day_ind = 0; day_ind < 7; day_ind++){
      for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++){
	sheet->week[week_ind]->day[day_ind]->pos_hours[pos_ind] = -1.0;
	sheet->week[week_ind]->day[day_ind]->pos_earnings[pos_ind] = -1.0;
	sheet->week[week_ind]->pos_total_hours[pos_ind] = -1.0;
	sheet->week[week_ind]->pos_total_earnings[pos_ind] = -1.0;
	sheet->day_pos_hours[day_ind][pos_ind] = -1.0;
	sheet->day_pos_earnings[day_ind][pos_ind] = -1.0;
      }
      sheet->week[week_ind]->day[day_ind]->date = -1.0;
      sheet->week[week_ind]->day[day_ind]->total_hours = -1.0;
      sheet->week[week_ind]->day[day_ind]->total_earnings = -1.0;
      sheet->day_total_hours[day_ind] = -1.0;
      sheet->day_total_earnings[day_ind] = -1.0;
    }
    sheet->week[week_ind]->total_hours = -1.0;
    sheet->week[week_ind]->total_earnings = -1.0;
    sheet->week[week_ind]->week_number = 0;
  }

  return 0;
  
} /* whm_reset_sheet() */


/* 
 * Set a sheet in the global list 'to_write' of sheet to be written to disk. 
 * The backup filename must be copied manualy.
 */
int whm_set_sheet(whm_config_T *config,
		  whm_sheet_T *sheet)
{
  whm_config_T **realloc_c = NULL;
  whm_sheet_T **realloc_s = NULL;
  char **realloc_f = NULL;
  int new_size = 0;
  
  if (!config || !sheet){
    errno = EINVAL;
    return WHM_ERROR;
  }

  if (to_write->c_ind+1 >= to_write->size){
    if ((new_size = to_write->size * 2) >= INT_MAX){
      errno = EOVERFLOW;
      return WHM_ERROR;
    }    
    if ((realloc_c = realloc(to_write->configs, new_size * sizeof(whm_config_T*))) == NULL){
      WHM_ERRMESG("Realloc");
      return WHM_ERROR;
    }
    if ((realloc_s = realloc(to_write->sheets, new_size * sizeof(whm_sheet_T*))) == NULL){
      WHM_ERRMESG("Realloc");
      goto errjmp;
    }
    if ((realloc_f = realloc(to_write->filename, new_size * sizeof(char*))) == NULL){
      WHM_ERRMESG("Realloc");
      goto errjmp;
    }
    to_write->configs = realloc_c;
    to_write->sheets = realloc_s;
    to_write->filename = realloc_f;
    to_write->size = new_size;
  }
  
  to_write->configs[to_write->c_ind] = config;
  to_write->sheets[to_write->c_ind] = sheet;
  ++(to_write->c_ind);

  return 0;

 errjmp:
  if (realloc_c) {
    free(realloc_c);
    realloc_c = NULL;
  }
  if (realloc_s) {
    free(realloc_s);
    realloc_s = NULL;
  }
  if (realloc_f) {
    free(realloc_f);
    realloc_f = NULL;
  }
  return WHM_ERROR;

} /* whm_set_sheet() */


/* Write to disk every hour sheet contained in the global to_write object. */
int whm_write_sheet_list(whm_time_T *time_object)
{
  int i = 0;
  FILE *stream = NULL;
  whm_time_T *time_o = NULL;

  if (!time_object) {
    errno = EINVAL;
    return WHM_ERROR;
  }
  
  for (; i < to_write->c_ind; i++){
    /* Backup the sheet only if it already exist. */
    if ((stream = fopen(to_write->sheets[i]->path, "r")) != NULL){
      fclose(stream);
      stream = NULL;
      if (whm_new_backup(to_write->sheets[i]->path,
			 to_write->filename[i]) == NULL){
	WHM_ERRMESG("Whm_new_backup");
	goto errjmp;
      }
    }
    /* 
     * If the ->time_o field of the current sheet isn't NULL use it,
     * else use the time object passed by the caller. 
     */
    if (to_write->sheets[i]->time_o == NULL) time_o = time_object;
    else time_o = to_write->sheets[i]->time_o;
    if ((stream = fopen(to_write->sheets[i]->path, "w")) == NULL) {
      WHM_ERRMESG("Fopen");
      return WHM_ERROR;
    }
    if (whm_write_sheet(stream, to_write->configs[i],
			time_o, to_write->sheets[i]) != 0){
      WHM_ERRMESG("Whm_write_sheet");
      goto errjmp;
    }
    fclose(stream);
    stream = NULL;
    if (chmod(to_write->sheets[i]->path, 0600) != 0){
      WHM_ERRMESG("Chmod");
      goto errjmp;
    }
    /* The sheet is saved to disk, remove the backup if there's one. */
    if (to_write->filename[i][0] != '\0')
      if (whm_rm_backup(to_write->filename[i]) != 0){
	WHM_ERRMESG("Whm_rm_backup");
	goto errjmp;
      }
  }
  
  whm_clean_sheet_list();

  return 0;

 errjmp:
  if (stream) {
    fclose(stream);
    stream = NULL;
  }

  return WHM_ERROR;

} /* whm_write_sheet_list() */

void whm_clean_sheet_list(void)
{
  while (--(to_write->c_ind) >= 0){
    to_write->sheets[to_write->c_ind] = NULL;
    to_write->configs[to_write->c_ind] = NULL;
    /* Do not NULL out filename as each strings must be freed when the time comes. */
    memset(to_write->filename[to_write->c_ind], '\0', WHM_MAX_PATHNAME_S);
  }
  to_write->c_ind = 0;

} /* whm_clean_sheet_list() */


/* 
 * Print a summary of the worked hours and earning made for a given
 * company name for a given period of time (weekly/monthly/yearly).
 */
