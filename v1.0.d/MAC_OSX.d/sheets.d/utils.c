/*
 *
 * Work Hour Monitor  -  Hour sheets related functions.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../whm.h"
#include "../whm_errcodes.h"





/* Print a header message to the given stream. */
int whm_print_sheet_header(whm_job_info_T *job_info,
			   whm_time_T *time_o,
			   FILE *stream)
{
  if (!job_info || !time_o || !stream){
    errno = EINVAL;
    return -1;
  }

  fprintf(stream,
	  "/*\n * Work Hour Monitor\n *\n * %s\n *\n * Nombre d'heures travaillees par jours pour le mois de %s, %s.\n *\n */\n\n",
	  job_info->name, time_o->month, time_o->year);

  return 0;
} /* whm_print_sheet_header() */


/* 
 * Returns the name of the first weekday of the month. 
 * Arguments are all assumed to be valid.
 * time_string must be at least WHM_TIME_O_STRING_LENGHT.
 */
int whm_find_first_dom(whm_time_T *time_o, int *week_num)
{
  int week_day = 0;
  int cur_date = atoi(time_o->date);
  
     
  /* Find the week day number for the current date. */
  while(week_day < 7 && strcmp(time_o->day, WHM_EN_WEEK_DAYS[week_day]) != 0) week_day++;

  /* Find the week day name of the first of the month. */
  while(cur_date != 1) {
    if (--week_day == -1){
      week_day = 6;
      (*week_num)--;
    }
    --cur_date;
  }

  return week_day;
  
} /* whm_find_first_dom() */


/* Print an hour sheet, complete with dates, into the given stream. */
int whm_print_new_sheet(whm_job_info_T *job_info,
			whm_time_T *time_o,
			FILE *stream)
{
  int first_dom = -1, week_num = 0, day_c = 0;
  size_t i = 0, month_day  = 1, week_c = 0;
  if (!job_info || !time_o || !stream) {
    errno = EINVAL;
    return -1;
  }

  week_num = atoi(time_o->week);
  first_dom = whm_find_first_dom(time_o, &week_num);
  while (week_c < 6) {
    fprintf(stream, "\nSemaine %d\n                  ", week_num++);
    day_c = 0;
    if (week_c == 0){
      for (day_c = 0; day_c < 7; day_c++){
	if (day_c < first_dom)
	  fprintf(stream, "%s --%s", WHM_FR_WEEK_DAYS[day_c], "   ");
	else {
	  break;
	}
      }
    }
    while (day_c <= 6){
      if (month_day > 31){ 
	fprintf(stream, "%s --%s", WHM_FR_WEEK_DAYS[day_c++], "   ");
	continue;
      }
      fprintf(stream, "%s %2d%s", WHM_FR_WEEK_DAYS[day_c++], month_day++, "   ");
      __asm__("");
    }
    fprintf(stream, " Total hrs:  Total brut:\n");
    for (i = 0; i < job_info->numof_positions; i++)
    fprintf(stream, "%-17s --.--    --.--    --.--    --.--    --.--    --.--    --.--     ---.--      -----.--$\n",
	    job_info->position_name[i]);
    fprintf(stream, "%-17s --.--    --.--    --.--    --.--    --.--    --.--    --.--     ---.--\n", "Total(h)");
    fprintf(stream, "%-17s ---.--$  ---.--$  ---.--$  ---.--$  ---.--$  ---.--$  ---.--$               -----.--$\n", "Total($)");
    week_c++;
  }
  fprintf(stream, "\n/**********************************************************************/\nCummulatifs:\n");
  for (day_c = 0; day_c < 7; day_c++){
    fprintf(stream, "\n%-17s: ---.--hrs  -----.--$%c\n", WHM_FR_LG_WEEK_DAYS[day_c], SPACE);
    for (i = 0; i < job_info->numof_positions; i++)
      fprintf(stream, "%-17s: ---.--hrs  -----.--$%c\n", job_info->position_name[i], SPACE);
  }
  fprintf(stream, "\n%-17s: ---.--hrs  -----.--$%c\n", "Total", SPACE);
  for (i = 0; i < job_info->numof_positions; i++)
    fprintf(stream, "%-17s: ---.--hrs  -----.--$%c\n", job_info->position_name[i], SPACE);
  fprintf(stream, "\n/**********************************************************************/\n");
  
  return 0;

} /* whm_print_new_sheet() */


/*
 * Make a filename concatenating together:
 * The company's name followed by a '.',
 * the full month name, an '_', the year including centuries,
 * a '.' and the constant whm_hour_sheet_name.
 * Take this new filename and append it to the company's ->work_dir, including a slash
 * in between both.
 * Make sure the total lenght of the new pathname don't exceed our limit. 
 *
 * Abreviated ex: ~/.whm.d/SLB.whmconfig/SLB_december_2016.hour_sheet
 *
 * (+4: for appended punctuation marks, 2x '_', '.' and '/')
 */
char* whm_make_sheet_path(char *new_pathname,
			  whm_job_info_T *job_info,
			  whm_time_T *time_o)
{
  char new_path[WHM_MAX_PATHNAME_LENGHT];
  
  
  if (!new_pathname || !job_info || !time_o) {
    errno = EINVAL;
    return NULL;
  }

  if ((strlen(job_info->name) + 4 + strlen(time_o->month)
       + strlen(time_o->year) + strlen(WHM_HOUR_SHEET_NAME))
      + (strlen(job_info->work_dir)) >= WHM_MAX_PATHNAME_LENGHT) {
    errno = WHM_SHEETNAMETOOLONG;
    return NULL;
  }

  if (s_strcpy(new_path, job_info->work_dir, WHM_MAX_PATHNAME_LENGHT) == NULL){
    WHM_ERRMESG("s_strcpy");
    return NULL;
  }
  strcat(new_path, "/");
  strcat(new_path, job_info->name);
  strcat(new_path, "_");
  strcat(new_path, time_o->month);
  strcat(new_path, "_");
  strcat(new_path, time_o->year);
  strcat(new_path, ".");
  strcat(new_path, WHM_HOUR_SHEET_NAME);

  if (s_strcpy(new_pathname, new_path, WHM_MAX_PATHNAME_LENGHT) == NULL){
    WHM_ERRMESG("s_strcpy");
    return NULL;
  }
    
									  

  return new_pathname;

} /* whm_make_sheet_path() */


/*
 * Checks if an hour sheet exists for the given time_string,
 * if not creates one appending WHM_HOUR_SHEET_NAME (whm.h)
 * to the given company name.
 * ex:   ~/.whm.d/SLB.whmconfig/SLB.december_2016.hour_sheet
 */
int whm_create_hour_sheet(whm_job_info_T *job_info,
			  whm_time_T *time_o,
			  char *new_pathname,
			  size_t np_size)
{
  FILE *filestream = NULL;
  

  if (!job_info || !time_o || !new_pathname || !np_size) {
    errno = EINVAL;
    return -1;
  }
  /* Adjust our local copy if the given value's too big. */
  if (np_size > WHM_MAX_PATHNAME_LENGHT) np_size = WHM_MAX_PATHNAME_LENGHT;
  
  if (whm_make_sheet_path(new_pathname, job_info, time_o) == NULL) {
    WHM_ERRMESG("Whm_make_sheet_path");
    goto errjmp;
  }
  
  /* Try to open a file at new_pathname and if we can, fail with WHM_SHEETEXISTS. */
  if ((filestream = fopen(new_pathname, "r")) != NULL){
    errno = WHM_SHEETEXISTS;
    goto errjmp;
  }
  if ((filestream = fopen(new_pathname, "w")) == NULL){
    WHM_ERRMESG("Fopen");
    goto errjmp;
  }

  /* Print a formatted heading message to the new hour sheet. */
  if (whm_print_sheet_header(job_info, time_o, filestream) != 0){
    WHM_ERRMESG("Whm_print_sheet_header");
    goto errjmp;
  }
  if (whm_print_new_sheet(job_info, time_o, filestream) != 0){
    WHM_ERRMESG("Whm_print_new_sheet");
    goto errjmp;
  }

  if (filestream){
    fclose(filestream);
    filestream = NULL;
  }
  return 0;

 errjmp:
  if (filestream){
    fclose(filestream);
    filestream = NULL;
  }
  return -1;
  
} /* whm_create_hour_sheet() */


/*
 * Depending on the given line number,
 * pop in the given month_T object all elements
 * contained within the given stack_T object.
 *
 * The caller is responsible of making sure no '-' appears
 * when we're converting string to int/double.
 */
int whm_pop_in_month_type(size_t line_count,
			  size_t week_ind,
			  size_t numof_positions,
			  size_t cur_pos,
			  whm_month_T *month_info,
			  whm_stack_T *sheet_stack)
{
  int day_ind = 0;
  size_t i = 0;

  if (!month_info || !sheet_stack
      || line_count > WHM_MAX_NUMOF_POSITIONS+4
      || week_ind >= 6){
    errno = EINVAL;
    return -1;
  }
  /* Save the week number. */
  if (line_count == 0){
    month_info->week[week_ind]->week_number = atoi(whm_pop(sheet_stack));
    whm_pop(sheet_stack);
  }

  /* Save the day's names and date number. */
  else if (line_count == 1){
    /* Get rid of "Total hrs:" and "Total brut:" */
    for (i = 0; i < 4; i++) whm_pop(sheet_stack);
    for (day_ind = 6; day_ind >= 0; day_ind--){
      month_info->week[week_ind]->daily_info[day_ind]->daynum = atoi(whm_pop(sheet_stack));
      if (s_strcpy(month_info->week[week_ind]->daily_info[day_ind]->dayname,
		   whm_pop(sheet_stack),
		   WHM_TIME_O_STRING_LENGHT) == NULL){
	WHM_ERRMESG("S_strcpy");
	return -1;
      }
    }
  }

  /* 
   * Save the amount of hours worked for the given position number.
   * +2: 1 for each lines already seen. 
   * Line 0 is the week number line, 
   * line 1 is the weekdays' names and dates.
   * Lines 2 to (numof_positions+2)-1 are the positions lines.
   * Lines numof_positions+2 and numof_positions+3 are the
   * daily total of hours and money made, respectively.
   */
  else if (line_count > 1 && line_count < numof_positions+2){
    month_info->week[week_ind]->weekly_cash[cur_pos] = atof(whm_pop(sheet_stack));
    month_info->week[week_ind]->weekly_hours[cur_pos] = atof(whm_pop(sheet_stack));
    for (day_ind = 6; day_ind >= 0; day_ind--)
      month_info->week[week_ind]->daily_info[day_ind]->pos_hours[cur_pos] = atof(whm_pop(sheet_stack));
    /* The position's name has already been saved in the whm_job_info_T structure. */
    whm_pop(sheet_stack);
  }
  
  /*
   * Save the total of hours, all positions combined, for each days of the week
   * and the total of hours, all positions combined, for the week.
   * +1: The next line after the line of the last position. 
   */
  else if (line_count == (numof_positions+2)){
    month_info->week[week_ind]->total_hours = atof(whm_pop(sheet_stack));
    for (day_ind = 6; day_ind >= 0; day_ind--)
      month_info->week[week_ind]->daily_info[day_ind]->total_hours = atof(whm_pop(sheet_stack));
    /* We don't need the "total(h)" string. */
    whm_pop(sheet_stack);
  }
  
  /*
   * Save the total amount of money made, all positions combined, for each days of the week
   * and the total of money, all positions combined, made during the week.
   * +2: The second next line after the line of the last position. 
   */
  else if (line_count == (numof_positions+3)){
    month_info->week[week_ind]->total_cash = atof(whm_pop(sheet_stack));
    for (day_ind = 6; day_ind >= 0; day_ind--)
      month_info->week[week_ind]->daily_info[day_ind]->total_cash = atof(whm_pop(sheet_stack));
    /* We don't need the "total($)" string. */
    whm_pop(sheet_stack);
  }
  
  /* Else it must be a syntax or logic error. */
  else {
    errno = WHM_INVALIDLINENUM;
    return -1;
  }

  return 0;
      
} /* whm_pop_in_month_type() */


/* Fill in the "cumulatives" fields of the given _month_T* object. */
int whm_pop_in_cumulatives(size_t line_count,
			   size_t cur_pos,
			   int day_ind,
			   whm_stack_T *stack,
			   whm_month_T *sheet)
{
  errno = 0;
  if (!stack || ! sheet){
    errno = EINVAL;
    return -1;
  }
  if (!line_count){
    if (stack->tos > 1){
      sheet->cumul_cash[day_ind]  = atof(whm_pop(stack));
      sheet->cumul_hours[day_ind] = atof(whm_pop(stack));
    }
    else{
      errno = WHM_USELESS_POP_OP;
    }
  }
  else {
    sheet->per_pos_cash[day_ind][cur_pos]  = atof(whm_pop(stack));
    sheet->per_pos_hours[day_ind][cur_pos] = atof(whm_pop(stack));
  }
  /* The days and positions names has already been recorded. */
  whm_pop(stack);

  return 0;
  
} /* whm_pop_in_cumulatives() */


/*
 * Takes an already opened hour sheet stream and an already initialized char*
 * and returns a string corresponding to the file's content.
 */
char* whm_fetch_hs_content(FILE *stream, char *content)
{
  if (!stream || !content) {
    errno = EINVAL;
    return NULL;
  }

  fread(content, sizeof(char), WHM_MAX_HR_SHEET_LENGHT, stream);
  if (ferror(stream)) {
    WHM_ERRMESG("Fread");
    return NULL;
  }
  content[strlen(content)] = '\0';

  return content;

} /* whm_fetch_hs_content() */


/* Strickly for the 2 following functions. */
# define WHM_PUSH_CLEAN(stack, value, ind) do {	\
    errno = 0;					\
    value[ind] = '\0';				\
    whm_push(stack, value);			\
    memset(value, '\0', WHM_DEF_STACK_SIZE);	\
    ind = 0;					\
  } while (0);
/*
 * Parse the "hours" part of an hour sheet. 
 * Loop over each characters of content.
 * Any '-' is converted to a "-1.0" string, comments are
 * permited: from '#' or '//' till the end of the current line and
 * from '\/\*' to '\*\/' for multi-lines comments, they will be ignored.
 * Words are delimited by the constant SPACE (a space char) and each time
 * one is found, all strings currently in the stack, if there's any, are
 * poped in their corresponding field of the whm_month_t struct. No need
 * to say it's all highly dependent on word positions and line count.
 *
 * Returns the index of the first character following the end of
 * the "hours" part of an hour sheet or 0 on error.
 */
size_t whm_fetch_hours(char *content,
		       whm_month_T *sheet,
		       size_t numof_positions)
{
  size_t c_char = 0, tmp_char = 0, content_len = 0;
  size_t week_ind = 0;                         /* There are 6 weeks in every hour sheets. */
  size_t line_count = 0, cur_pos = 0;
  whm_stack_T *stack = NULL;
  char tmp_string[WHM_DEF_STACK_SIZE];

  if (!content || !sheet || !numof_positions){
    errno = EINVAL;
    goto errjmp;
  }
  if ((stack = whm_init_stack(WHM_DEF_STACK_SIZE)) == NULL){
    WHM_ERRMESG("Whm_init_stack");
    goto errjmp;
  }
  memset(tmp_string, '\0', WHM_DEF_STACK_SIZE);
  content_len = strlen(content);

  while(content[c_char] != '\0' && c_char < content_len){
    if (week_ind >= 6) break;
    
    /* Look for commentaries. */
    if ((content[c_char] == SLASH && c_char+1 < content_len
	 && (content[c_char+1] == SLASH || content[c_char+1] == STAR))
	|| (content[c_char] == NUMBER)){
      if (whm_skip_comments(content,
			    content_len,
			    &c_char) == -1){
	WHM_ERRMESG("Whm_skip_comments");
	goto errjmp;
      }
      ++c_char;
      continue;
    }
    if (content[c_char] == SPACE){
      if (tmp_char > 0) {/* Don't try to push an empty string. */
	WHM_PUSH_CLEAN(stack, tmp_string, tmp_char);
	if (errno){
	  WHM_ERRMESG("Whm_push");
	  goto errjmp;
	}
      }
      /* Skip spaces when there's nothing to push. */
      ++c_char;
      continue;
    }

    if (content[c_char] == NEWLINE) {
      if (stack->tos > 0){
	/* Don't leave the temporary string full. */
	if (tmp_char > 0) {
	  WHM_PUSH_CLEAN(stack, tmp_string, tmp_char);
	  if (errno){
	    WHM_ERRMESG("Whm_push");
	    goto errjmp;
	  }
	}
	/* 
	 * When line_count isn't in between WHM_HR_SHEET_POSITIONS_LINE_NUM and
	 * job_info->numof_positions-1, set it to 0, else increment it each time.
	 */
	if (cur_pos >= numof_positions) cur_pos = 0;
	if (whm_pop_in_month_type(line_count,
				  week_ind,
				  numof_positions,
				  (line_count >= WHM_HR_SHEET_POSITIONS_LINE_NUM
				   && line_count <= (numof_positions + WHM_HR_SHEET_POSITIONS_LINE_NUM - 1))
				  ? cur_pos++
				  : 0,
				  sheet,
				  stack) == -1) {
	  WHM_ERRMESG("Whm_pop_in_month_type");
	  goto errjmp;
	}
	++line_count;
	/* 
	 * If a second newline is following the one encountered at this point,
	 * it means we're changing week.
	 */
	if (c_char+1 < content_len
	    && content[c_char+1] == NEWLINE) {
	  ++week_ind;
	  line_count = 0;
	}
      }
      /* Skip a newline when the stack is empty. */
      ++c_char;
      continue;
    }

    /*
     * A dash ('-') means no values are available for this
     * particular field. To make it easier to update the sheets,
     * this value is converted into "-1.0".
     *
     * When there is already something in the temporary string,
     * add the dash to the string like a normal character. 
     */
    if (content[c_char] == DASH && tmp_char == 0){
      while (content[c_char] != SPACE && content[c_char] != NEWLINE) ++c_char;
      if (s_strcpy(tmp_string, "-1.0", WHM_DEF_STACK_SIZE) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      tmp_char = strlen(tmp_string);
      continue;
    }

    /* 
     * If no special characters have been found, add
     * the current character to the temporary string. 
     */
    tmp_string[tmp_char] = content[c_char];
    tmp_char++;
    c_char++;
  }

  
  if (stack){
    whm_free_stack(stack);
    stack = NULL;
  }

  return c_char;

  
 errjmp:
  if (stack){
    whm_free_stack(stack);
    stack = NULL;
  }

  return 0;
  
} /* whm_fetch_hours() */


/* 
 * Parse the "cumulative" part of an hour sheet.
 * Extremely similar to whm_fetch_hours(), I found it easier to make 2 
 * separate functions.
 */
int whm_fetch_cumulatives(char *content,
			  whm_month_T *sheet,
			  size_t numof_positions,
			  size_t c_char)
{
  whm_stack_T *stack = NULL;
  size_t tmp_char = 0, content_len = 0;
  size_t line_count = 0, day_ind = 0, cur_pos = 0;
  char tmp_string[WHM_DEF_STACK_SIZE];

  
  if (!content || !sheet || !numof_positions){
    errno = EINVAL;
    goto errjmp;
  }
  if ((stack = whm_init_stack(WHM_DEF_STACK_SIZE)) == NULL){
    WHM_ERRMESG("Whm_init_stack");
    goto errjmp;
  }
  content_len = strlen(content);

  while (content[c_char] != '\0' && c_char < content_len) {
    
    /* Look for commentaries. */
    if ((content[c_char] == SLASH && c_char+1 < content_len
	 && (content[c_char+1] == SLASH || content[c_char+1] == STAR))
	|| (content[c_char] == NUMBER)){
      if (whm_skip_comments(content,
			    content_len,
			    &c_char) == -1){
	WHM_ERRMESG("Whm_skip_comments");
	goto errjmp;
      }
      ++c_char;
      continue;
    }

    /* 
     * Push the temporary string on the stack or skip a space 
     * when the string is empty.
     */
    if (content[c_char] == SPACE){
      /* Don't try to push an empty string. */
      if (tmp_char > 0){
	WHM_PUSH_CLEAN(stack, tmp_string, tmp_char);
	if (errno) {
	  WHM_ERRMESG("Whm_push");
	  goto errjmp;
	}
      }
      /* Simply skip any spaces when there'snothing to push. */
      ++c_char;
      continue;
    }

    /* 
     * When seeing a newline, pop all elements in the stack in the appropriate fields
     * of the _month_T object. This is highly dependent on the line count.
     */
    if (content[c_char] == NEWLINE) {
      if (tmp_char > 0){
	WHM_PUSH_CLEAN(stack, tmp_string, tmp_char);
	if (errno){
	  WHM_ERRMESG("Whm_push");
	  goto errjmp;
	}
      }
      if (stack->tos > 0) {
	if (cur_pos >= numof_positions) cur_pos = 0;
	if (whm_pop_in_cumulatives(line_count,
				   (line_count >= WHM_HR_SHEET_POSITIONS_LINE_NUM
				    && line_count <= (numof_positions + WHM_HR_SHEET_POSITIONS_LINE_NUM - 1))
				   ? cur_pos++
				   : 0,
				   day_ind,
				   stack,
				   sheet) != 0) {
	  WHM_ERRMESG("Whm_pop_in_cumulatives");
	  goto errjmp;
	}
	++line_count;

	if (c_char+1 < content_len && content[c_char+1] == NEWLINE){
	  /*
	   * day_ind must not be bigger than 7, the 8th element (day_ind == 7)
	   * is the totals for the current month, in the same format as
	   * the previous days.
	   */
	  if (errno != WHM_USELESS_POP_OP)
	    if (++day_ind > 7) break;
	  ++c_char;
	  line_count = 0;
	  continue;
	}	
      }
      /* Skip newlines when there's nothing in the stack to pop. */
      ++c_char;
      continue;
    }

    /* Skip colons. */
    if (content[c_char] == COLON && tmp_char == 0){
      ++c_char;
      continue;
    }

    /* Converts --.-- etc.. into -1.0, see whm_fetch_hour() for explaination. */
    if (content[c_char] == DASH && tmp_char == 0){
      while (content[c_char] != '\0'
	     && content[c_char] != EOF
	     && content[c_char] != SPACE
	     && content[c_char] != NEWLINE) ++c_char;
      if (s_strcpy(tmp_string, "-1.0", WHM_DEF_STACK_SIZE) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      tmp_char = strlen(tmp_string)+1;
      continue;
    }

    /* Add the current character to the temporary string. */
    tmp_string[tmp_char] = content[c_char];
    ++tmp_char;
    ++c_char;
  }

  if (stack) {
    whm_free_stack(stack);
    stack = NULL;
  }

  return 0;

  
 errjmp:
  if (stack) {
    whm_free_stack(stack);
    stack = NULL;
  }

  return -1;

} /* whm_fetch_cumulatives() */
# undef WHM_PUSH_CLEAN

    
/*
 * Read the content of an hour sheet, store
 * all informations found into a whm_month_T structure
 * and return it to the caller.
 * sheet_path is non-NULL only if a new hour sheet has been
 * created using whm_create_hour_sheet.


  MAIN is responsible of looping over all hour sheets
  AND backing them up BEFORE any editing takes place.
  This function must takes only 1 hour sheet at a time.
 */
int whm_read_hour_sheet(whm_month_T *sheet,
			whm_job_info_T *job_info,
			whm_time_T *time_o)
{
  FILE *stream = NULL;
  /* Allocated on the heap not to fill the process' entire stack frame. */
  char *content = NULL;
  size_t c_char = 0;

  if (!sheet || !job_info || !time_o){
    errno = EINVAL;
    return -1;
  }
  if ((content = calloc(WHM_MAX_HR_SHEET_LENGHT, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }

  /* 
   * Try to create the hour sheet if it doesn't exists, 
   * and fill the sheet's pathname.
   */
  if (whm_create_hour_sheet(job_info,
			    time_o,
			    sheet->sheet_path,
			    WHM_MAX_PATHNAME_LENGHT) != -1
      || errno == WHM_SHEETEXISTS)
    ; /* Good. */
  else {
    WHM_ERRMESG("Whm_create_hour_sheet");
    goto errjmp;
  }
  
  /* Open the file for reading only. */
  if ((stream = fopen(sheet->sheet_path, "r")) == NULL){
    WHM_ERRMESG("Fopen");
    goto errjmp;
  }

  /* Fetch the hour sheet's content. */
  memset(content, '\0', WHM_MAX_HR_SHEET_LENGHT);
  if (whm_fetch_hs_content(stream, content) == NULL){
    WHM_ERRMESG("Whm_fetch_hs_content");
    goto errjmp;
  }
  fclose(stream);
  stream = NULL;
  
  /* 
   * Parse the "hours" part of an hour sheet then
   * parse the "cumulatives" part of an hour sheet.
   */
  if ((c_char = whm_fetch_hours(content, sheet, job_info->numof_positions)) == 0){
    WHM_ERRMESG("Whm_fetch_hours");
    goto errjmp;
  }
  if (whm_fetch_cumulatives(content, sheet, job_info->numof_positions, c_char) != 0){
    WHM_ERRMESG("Whm_fetch_cumulatives");
    goto errjmp;
  }
  
  if (content){
    free(content);
    content = NULL;
  }
  return 0;
  
 errjmp:
  if (stream){
    fclose(stream);
    stream = NULL;
  }
  if (content){
    free(content);
    content = NULL;
  }
  return -1;
  
} /* whm_read_hour_sheet() */


/* Prompt the user for the amount of hours worked for today. */
int whm_get_user_hours(whm_month_T *sheet,
		       whm_job_info_T *job_info,
		       whm_time_T *time_o,
		       size_t week_ind, size_t day_ind)
{
  size_t pos_ind = 0, in_ind = 0;
  int in = 0;
  char input[WHM_WAGE_STRING_LENGHT];
 
  if (!sheet || !time_o || !job_info){
    errno = EINVAL;
    return -1;
  }
  
  printf("%s %s, %s\n\n",
	 time_o->date, time_o->month, time_o->year);

  for (; pos_ind < job_info->numof_positions; pos_ind++){
    memset(input, '\0', WHM_WAGE_STRING_LENGHT);
    in_ind = 0;
    printf("\nCombien d'heures avez-vous travaille pour le poste de %s chez %s: ",
	   job_info->position_name[pos_ind], job_info->name);

    while((in = getchar()) != EOF){
      if (in_ind >= WHM_WAGE_STRING_LENGHT){
	errno = WHM_INPUTTOOLONG;
	return -1;
      }
      if (in == NEWLINE){
	input[in_ind] = '\0';
	break;
      }
      if (!isdigit(in)) {
	errno = WHM_INVALIDHOUR;
	return -1;
      }
      input[in_ind++] = in;
    }

    sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind] = atof(input);
    puts("\n");
  }

  return 0;

  
} /* whm_get_user_hours() */


/* 
 * Worked hours for the current day have been inserted
 * in the appropriate fields before a call to this function.
 * Main is solely responsible of prompting the user or not.
 */
int whm_update_hour_sheet(whm_month_T *sheet,
			  whm_job_info_T *job_info,
			  whm_time_T *time_o)
{
  size_t pos_ind = 0;
  size_t week_ind = 0, day_ind = 0;
  double temp_cash = 0.0, temp_hours = 0.0;
  double temp_tot_cash = 0.0, temp_tot_hours = 0.0;
  double vac_pay = 0.0;

  if (!sheet || !job_info || !time_o){
    errno = EINVAL;
    return -1;
  }

  /* Day, then week, then month. */
  for (; week_ind < 5; week_ind++){

    for (day_ind = 0; day_ind < 7; day_ind++){
      temp_hours = 0.0;
      temp_cash  = 0.0;
      vac_pay = 0.0;
      if (sheet->week[week_ind]->daily_info[day_ind]->daynum != -1){
	for (pos_ind = 0; pos_ind < job_info->numof_positions; pos_ind++){
	  if (sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind] == -1) continue;
	  temp_hours += sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind];
	  temp_cash  +=
	    sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind] * atof(job_info->wage[pos_ind]);
	}
	if (temp_hours)
	  sheet->week[week_ind]->daily_info[day_ind]->total_hours = temp_hours;
	else
	  sheet->week[week_ind]->daily_info[day_ind]->total_hours = -1;
	if (temp_cash){
	  sheet->week[week_ind]->daily_info[day_ind]->total_cash = temp_cash;
	}
	else
	  sheet->week[week_ind]->daily_info[day_ind]->total_cash = -1;
      }
    }
    for (pos_ind = 0; pos_ind < job_info->numof_positions; pos_ind++){
      temp_hours = 0.0;
      temp_cash  = 0.0;
      vac_pay = 0.0;
      for (day_ind = 0; day_ind < 7; day_ind++){
	if (sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind] == -1) continue;
	temp_hours += sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind];
      }
      if (temp_hours){
	sheet->week[week_ind]->weekly_hours[pos_ind] = temp_hours;
	sheet->week[week_ind]->weekly_cash[pos_ind] = temp_hours * atof(job_info->wage[pos_ind]);
	if (job_info->vacation_pay){
	  vac_pay = (sheet->week[week_ind]->weekly_cash[pos_ind] * 4)/100;
	  sheet->week[week_ind]->weekly_cash[pos_ind] += vac_pay;
	}
      }
    }
    temp_hours = 0.0;
    temp_cash  = 0.0;
    for (pos_ind = 0; pos_ind < job_info->numof_positions; pos_ind++){
      if (sheet->week[week_ind]->weekly_hours[pos_ind] == -1) continue;
      temp_hours += sheet->week[week_ind]->weekly_hours[pos_ind];
      temp_cash  += sheet->week[week_ind]->weekly_cash[pos_ind];
    }
    if (temp_hours)
      sheet->week[week_ind]->total_hours = temp_hours;
    else
      sheet->week[week_ind]->total_hours = -1;
    if (temp_cash)
      sheet->week[week_ind]->total_cash  = temp_cash;
    else
      sheet->week[week_ind]->total_cash = -1;
  }


  for (day_ind = 0; day_ind < 7; day_ind++){
    temp_tot_hours = 0.0;
    temp_tot_cash = 0.0;
    for (pos_ind = 0; pos_ind < job_info->numof_positions; pos_ind++){
      temp_hours = 0.0;
      temp_cash  = 0.0;
      for (week_ind = 0; week_ind < 6; week_ind++){
	if (sheet->week[week_ind]->daily_info[day_ind]->daynum == -1
	    || sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind] <= 0) continue;
	temp_hours += sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind];
	temp_cash  += sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind]
	  * atof(job_info->wage[pos_ind]);
	
      }
      if (temp_hours) {
	sheet->per_pos_hours[pos_ind][day_ind] = temp_hours;
	temp_tot_hours += temp_hours;
      }
      else sheet->per_pos_hours[pos_ind][day_ind] = -1;
      if (temp_cash) {
	sheet->per_pos_cash[pos_ind][day_ind]  = temp_cash;
	temp_tot_cash  += temp_cash;
      }
      else sheet->per_pos_cash[pos_ind][day_ind] = -1;
    }
    if (temp_tot_hours) sheet->cumul_hours[day_ind] = temp_tot_hours;
    else sheet->cumul_hours[day_ind] = -1;
    if (temp_tot_cash) sheet->cumul_cash[day_ind] = temp_tot_cash;
    else sheet->cumul_cash[day_ind] = temp_tot_cash;
  }
 
  temp_tot_hours = 0.0;
  temp_tot_cash = 0.0;
  for (week_ind = 0; week_ind < 6; week_ind++){
    if (sheet->week[week_ind]->total_hours == -1) continue;
    temp_tot_hours += sheet->week[week_ind]->total_hours;
    temp_tot_cash  += sheet->week[week_ind]->total_cash;
  }
  if (temp_tot_hours > 0){
    sheet->cumul_hours[7] = temp_tot_hours;
    sheet->cumul_cash[7] = temp_tot_cash;
  }
  else{
    sheet->cumul_hours[7] = -1;
    sheet->cumul_cash[7] = -1;
  }

  for (pos_ind = 0; pos_ind < job_info->numof_positions; pos_ind++){
    temp_hours = 0.0;
    temp_cash  = 0.0;
    for (week_ind = 0; week_ind < 6; week_ind++){
      if (sheet->week[week_ind]->weekly_hours[pos_ind] == -1) continue;
      temp_hours += sheet->week[week_ind]->weekly_hours[pos_ind];
      temp_cash  += sheet->week[week_ind]->weekly_cash[pos_ind];
    }
    if (temp_hours > 0){
      sheet->per_pos_hours[pos_ind][7] = temp_hours;
      sheet->per_pos_cash[pos_ind][7] = temp_cash;
    }
    else {
      sheet->per_pos_hours[pos_ind][7] = -1;
      sheet->per_pos_cash[pos_ind][7] = -1;
    }
  }
    
    /*
    for (day_ind = 0; day_ind < 7; day_ind++){
      if (sheet->per_pos_hours[pos_ind][day_ind] == -1) continue;
      temp_hours += sheet->per_pos_hours[pos_ind][day_ind];
    }
    if (temp_hours) {
      sheet->per_pos_hours[pos_ind][7] = temp_hours;
      temp_tot_hours += temp_hours;
      sheet->per_pos_cash[pos_ind][7] = (atof(job_info->wage[pos_ind]) * temp_hours);
      temp_tot_cash += sheet->per_pos_cash[pos_ind][7];
    }
    else {
      sheet->per_pos_hours[pos_ind][7] = -1;
      sheet->per_pos_cash[pos_ind][7] = -1;
    }
  }
  if (temp_tot_hours > 0) sheet->cumul_hours[7] = temp_tot_hours;
  else sheet->cumul_hours[7] = -1;
  if (temp_tot_cash > 0) sheet->cumul_cash[7] = temp_tot_cash;
  else sheet->cumul_cash[7] = -1;
  */								  
	  
  return 0;
    
} /* whm_update_hour_sheet() */

/* 
 * Takes a whm_job_info_T* object, a whm_month_T* object, 
 * a whm_time_T* object and a FILE* stream. Outputs one
 * formatted hour sheet to the given stream.
 */
int whm_print_hour_sheet(whm_job_info_T *job_info,
			 whm_month_T *sheet,
			 whm_time_T *time_o,
			 FILE *stream)
{
  size_t week_ind = 0, day_ind = 0, pos_ind = 0;
  char temp_string[S_FTOA_MIN_STR_SIZE];

#define FTOA_S S_FTOA_MIN_STR_SIZE
  
  if (!job_info || !sheet
      || !time_o   || ! stream){
    errno = EINVAL;
    return -1;
  }
  
  /* Print a heading message to the sheet. */
  if (whm_print_sheet_header(job_info, time_o, stream) != 0){
    WHM_ERRMESG("Whm_print_sheet_header");
    return -1;
  }

  /* For each 6 weeks of an hour sheet: */
  while(week_ind < 6){
  
    /* Print the week number, a newline and 18 spaces to keep columns aligned. */
    fprintf(stream, "Semaine %*d\n                  ", WHM_DATE_WIDTH, sheet->week[week_ind]->week_number);

    /* Print the days names and dates, followed by 2 strings and a newline. */
    for(day_ind = 0; day_ind < 7; day_ind++)
      fprintf(stream, "%s %2s    ", WHM_FR_WEEK_DAYS[day_ind],
	      (sheet->week[week_ind]->daily_info[day_ind]->daynum == -1)
	      ? WHM_NO_DATE
	      : s_itoa(temp_string, FTOA_S, (double)sheet->week[week_ind]->daily_info[day_ind]->daynum));
    fprintf(stream, "%s:   %s:\n", " Total hrs", "Total brut");

    /* 
     * For each positions, print its name, the number of hours worked for each 
     * days of the week, then the total number of hours worked and
     * the total amount of money made and a newline.
     */
    for (pos_ind = 0; pos_ind < job_info->numof_positions; pos_ind++){
      fprintf(stream, "%-17s ", job_info->position_name[pos_ind]);

      for (day_ind = 0; day_ind < 7; day_ind++)
	fprintf(stream, "%-5s     ",
		(sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind] == -1)
		? WHM_NO_HOUR
		: s_ftoa(temp_string, FTOA_S, sheet->week[week_ind]->daily_info[day_ind]->pos_hours[pos_ind]));

      fprintf(stream, " %-6s       ",
	      (sheet->week[week_ind]->weekly_hours[pos_ind] == -1)
	      ? WHM_NO_T_HOUR
	      : s_ftoa(temp_string, FTOA_S, sheet->week[week_ind]->weekly_hours[pos_ind]));
      fprintf(stream, "%9s$\n",
	      (sheet->week[week_ind]->weekly_cash[pos_ind] == -1)
	      ? WHM_NO_T_CASH
	      : s_ftoa(temp_string, FTOA_S, sheet->week[week_ind]->weekly_cash[pos_ind]));
    }

    /* 
     * Print the total of hours, all positions combined, for each days of the week,
     * followed by the total of hours, all positions and days combined.
     */
    fprintf(stream, "%-17s ", "Total(h)");
    for (day_ind = 0; day_ind < 7; day_ind++)
      fprintf(stream, "%-5s     ",
	      (sheet->week[week_ind]->daily_info[day_ind]->total_hours == -1)
	      ? WHM_NO_HOUR
	      : s_ftoa(temp_string, FTOA_S, sheet->week[week_ind]->daily_info[day_ind]->total_hours));

    fprintf(stream, " %-6s\n",
	    (sheet->week[week_ind]->total_hours == -1)
	    ? WHM_NO_T_HOUR
	    : s_ftoa(temp_string, FTOA_S, sheet->week[week_ind]->total_hours));

    /*
     * Print the total amount of money, all positions combined, for each days of the week
     * followed by the total amount of money made this week, all days and positions combined.
     */
    fprintf(stream, "%-17s ", "Total($)");
    for (day_ind = 0; day_ind < 7; day_ind++)
      fprintf(stream, "%6s$   ",
	      (sheet->week[week_ind]->daily_info[day_ind]->total_cash == -1)
	      ? WHM_NO_CASH
	      : s_ftoa(temp_string, FTOA_S, sheet->week[week_ind]->daily_info[day_ind]->total_cash));
    fprintf(stream, "              %9s$\n\n",
	    (sheet->week[week_ind]->total_cash == -1)
	    ? WHM_NO_T_CASH
	    : s_ftoa(temp_string, FTOA_S, sheet->week[week_ind]->total_cash));

    ++week_ind;
  }
  fprintf(stream, "\n/**********************************************************************/\nCummulatifs:\n");

  /* 
   * Print a summary for each cummulated week days,
   * all positions combined and per positions.
   */
  for (day_ind = 0; day_ind < 7; day_ind++){
    fprintf(stream, "%-17s: ", WHM_FR_LG_WEEK_DAYS[day_ind]);
    fprintf(stream, "%6shrs  ",
	    (sheet->cumul_hours[day_ind] <= 0)
	    ? WHM_NO_T_HOUR
	    : s_ftoa(temp_string, FTOA_S, sheet->cumul_hours[day_ind]));
    fprintf(stream, "%9s$\n",
	    (sheet->cumul_cash[day_ind] <= 0)
	    ? WHM_NO_T_CASH
	    : s_ftoa(temp_string, FTOA_S, sheet->cumul_cash[day_ind]));

    for(pos_ind = 0; pos_ind < job_info->numof_positions; pos_ind++){
      fprintf(stream, "%-17s: ", job_info->position_name[pos_ind]);
      fprintf(stream, "%6shrs  ",
	      (sheet->per_pos_hours[pos_ind][day_ind] <= 0)
	      ? WHM_NO_T_HOUR
	      : s_ftoa(temp_string, FTOA_S, sheet->per_pos_hours[pos_ind][day_ind]));
      fprintf(stream, "%9s$\n",
	      (sheet->per_pos_cash[pos_ind][day_ind] <= 0)
	      ? WHM_NO_T_CASH
	      : s_ftoa(temp_string, FTOA_S, sheet->per_pos_cash[pos_ind][day_ind]));
    }
    fprintf(stream, "\n");
  }
  /* The monthly totals are at cumul_*[7]. */
  fprintf(stream, "%-17s: ","Total");
  fprintf(stream, "%6shrs  ",
	  (sheet->cumul_hours[7] <= 0)
	  ? WHM_NO_T_HOUR
	  : s_ftoa(temp_string, FTOA_S, sheet->cumul_hours[7]));
  fprintf(stream, "%9s$\n",
	  (sheet->cumul_cash[7] <= 0)
	  ? WHM_NO_T_CASH
	  : s_ftoa(temp_string, FTOA_S, sheet->cumul_cash[7]));
  
  for(pos_ind = 0; pos_ind < job_info->numof_positions; pos_ind++){
    fprintf(stream, "%-17s: ", job_info->position_name[pos_ind]);
    fprintf(stream, "%6shrs  ",
	    (sheet->per_pos_hours[pos_ind][7] <= 0)
	    ? WHM_NO_T_HOUR
	    : s_ftoa(temp_string, FTOA_S, sheet->per_pos_hours[pos_ind][7]));
    fprintf(stream, "%9s$\n",
	    (sheet->per_pos_cash[pos_ind][7] <= 0)
	    ? WHM_NO_T_CASH
	    : s_ftoa(temp_string, FTOA_S, sheet->per_pos_cash[pos_ind][7]));
  }

  fprintf(stream, "\n/**********************************************************************/\n\n");
  
  return 0;
  
#undef FTOA_S
}

