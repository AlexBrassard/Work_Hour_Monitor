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
  int i = 0, week_day = 0;
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
 * Read the content of an hour sheet, store
 * all informations found into a whm_month_T structure
 * and return it to the caller.
 * sheet_path is non-NULL only if a new hour sheet has been
 * created using whm_create_hour_sheet.
 */
int whm_read_hour_sheet(whm_month_T **sheet_info,
			size_t numof_sheets,
			whm_job_info_T **job_info,
			whm_time_T *time_o)
{
  size_t sht_char = 0, tmp_char = 0, sheet_content_len = 0, day_ind = 0;
  size_t sheet_line_count = 0, week_ind = 0, cur_pos = 0, sheet_ind = 0;
  FILE *sheet_stream = NULL;
  whm_stack_T *sheet_stack = NULL;
  char loc_sheet_path[WHM_MAX_PATHNAME_LENGHT];
  /* Defined on the heap not to fill all available stack space for this process. */
  char *sheet_content = NULL;
  char temp_string[WHM_DEF_STACK_SIZE];

  if (!time_o || !sheet_info
      || !numof_sheets || !job_info){
    errno = EINVAL;
    return -1;
  }
  /* 
   * Try to create the file, if it already exists, 
   * _create_hour_sheet will return WHM_SHEETEXISTS.
   */
  if ((sheet_content = calloc(WHM_MAX_HR_SHEET_LENGHT, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    return -1;
  }
  while (sheet_ind < numof_sheets){
    if (whm_create_hour_sheet(job_info[sheet_ind], time_o,
			      loc_sheet_path,
			      WHM_MAX_PATHNAME_LENGHT) != -1
	|| errno == WHM_SHEETEXISTS){
      ; /* Good. */
    }
    else {
      WHM_ERRMESG("Whm_create_hour_sheet");
      return -1;
    }

    if ((sheet_stack = whm_init_stack(WHM_DEF_STACK_SIZE)) == NULL){
      WHM_ERRMESG("Whm_init_stack");
      goto errjmp;
    }
    if (s_strcpy(sheet_info[sheet_ind]->sheet_path, loc_sheet_path, WHM_MAX_PATHNAME_LENGHT) == NULL){
      WHM_ERRMESG("S_strcpy");
      goto errjmp;
    }
    if ((sheet_stream = fopen(loc_sheet_path, "r")) == NULL){
      WHM_ERRMESG("Fopen");
      goto errjmp;
    }
    
    /* Read the hour sheet. */
    fread(sheet_content, sizeof(char), WHM_MAX_HR_SHEET_LENGHT-1, sheet_stream);
    if (ferror(sheet_stream)){
      WHM_ERRMESG("Fread");
      goto errjmp;
    }
    sheet_content_len = strlen(sheet_content);
    fclose(sheet_stream);
    sheet_stream = NULL;
    memset(temp_string, 0, WHM_DEF_STACK_SIZE);
    
    
    while(sht_char < sheet_content_len){
      if (week_ind >= 6) break; /* to enter the next loop and fill cumulatives. */
      /* Skip comments. */
      if (sht_char+1 < sheet_content_len){
	if (sheet_content[sht_char] == '/'){
	  if (sheet_content[sht_char+1] == '*'
	      || sheet_content[sht_char+1] == '/'){
	    if (whm_skip_comments(sheet_content,
				  sheet_content_len,
				  &sht_char) == -1){
	      WHM_ERRMESG("Whm_skip_comments");
	      goto errjmp;
	    }
	    sht_char++;
	    continue;
	  }
	}
	if(sheet_content[sht_char] == '#'){
	  if (whm_skip_comments(sheet_content,
				sheet_content_len,
				&sht_char) == -1){
	    WHM_ERRMESG("Whm_skip_comments");
	    goto errjmp;
	  }
	  sht_char++;
	  continue;
	}
      }
      if (sheet_content[sht_char] == SPACE && tmp_char > 0){
	/* When seeing a space, NULL terminate the string, push it on the stack. */
      push_char:
	temp_string[tmp_char] = '\0';
	if (whm_push(sheet_stack, temp_string) != 0) {
	  WHM_ERRMESG("Whm_push");
	  goto errjmp;
	}
	memset(temp_string, 0, WHM_DEF_STACK_SIZE);
	tmp_char = 0;
	if (sheet_content[sht_char] != NEWLINE) sht_char++;
	continue;
      }
      else if (sheet_content[sht_char] == SPACE){
	++sht_char;
	continue;
      }
      if (sheet_content[sht_char] == NEWLINE){
	/* 
	 * When seeing a newline, pop the current stack in the appropriate fields
	 * of the whm_month_T object.
	 */
	if (sheet_stack->tos > 0){
	  if (temp_string[0] != '\0') {
	    temp_string[tmp_char] = '\0';
	    if (whm_push(sheet_stack, temp_string) != 0) {
	      WHM_ERRMESG("Whm_push");
	      goto errjmp;
	    }
	    memset(temp_string, 0, WHM_DEF_STACK_SIZE);
	    tmp_char = 0;
	    /*	    sht_char++;*/
	  }
	  if (cur_pos == job_info[sheet_ind]->numof_positions) cur_pos = 0;
	  if (whm_pop_in_month_type(sheet_line_count,
				    week_ind,
				    job_info[sheet_ind]->numof_positions,
				    cur_pos++ ,
				    sheet_info[sheet_ind],
				    sheet_stack) != 0){
	    WHM_ERRMESG("Whm_pop_in_month_type");
	    goto errjmp;
	  }
	  /* When seeing 2 consecutives newlines, increment week_ind and continue */				
	  if (sht_char+1 < sheet_content_len
	      && sheet_content[sht_char+1] == NEWLINE){
	    ++week_ind;
	    sht_char += 2;
	    sheet_line_count = 0;
	    continue;
	  }
	  sht_char++;
	  sheet_line_count++;
	  continue;
	  
	}
	/* Else the stack is empty, just skip the newline. */
	else {
	  sht_char++;
	  continue;
	}
      }
      if (sheet_content[sht_char] == '-'){
	if (s_strcpy(temp_string, "-1.0", WHM_DEF_STACK_SIZE) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  goto errjmp;
	}
	while(sheet_content[sht_char] == '-' || sheet_content[sht_char] == '.' || sheet_content[sht_char] == '$') ++sht_char;
	tmp_char = strlen(temp_string)+1;
	goto push_char;
	/*	tmp_char = strlen(temp_string)+1;
		sht_char++;*/
	continue;
      }
      
      
      
      /* If no special characters are found, add the current one to the temporary string. */
      temp_string[tmp_char] = sheet_content[sht_char];
      ++tmp_char;
      ++sht_char;
    }
    /* 
     * From the sht_char character, loop till the end of the hour sheet.
     * Lines are in the form of:
     Day full name(FR) : hours  cash$
     pos[0]name        : hours  cash$
     ...
     
    */
    memset(temp_string, 0, WHM_DEF_STACK_SIZE);
    tmp_char = 0;
    sheet_line_count = 0;
    while (sheet_content[sht_char] != '\0' && sheet_content[sht_char] != EOF) {
      /* Skip comments */
      if (sht_char+1 < sheet_content_len){
	if (sheet_content[sht_char] == '/'){
	  if (sheet_content[sht_char+1] == '*'
	      || sheet_content[sht_char+1] == '/'){
	    if (whm_skip_comments(sheet_content,
				  sheet_content_len,
				  &sht_char) == -1){
	      WHM_ERRMESG("Whm_skip_comments");
	      goto errjmp;
	    }
	    sht_char++;
	    continue;
	  }
	}
	if(sheet_content[sht_char] == '#'){
	  if (whm_skip_comments(sheet_content,
				sheet_content_len,
				&sht_char) == -1){
	    WHM_ERRMESG("Whm_skip_comments");
	    goto errjmp;
	  }
	  sht_char++;
	  continue;
	}
      }
      
      if (sheet_content[sht_char] == SPACE){
	/* 
	 * If there's something in temp_string, push it on the stack,
	 * else simply skip the space.
	 */
	if (tmp_char > 0){
	  if (whm_push(sheet_stack, temp_string) != 0) {
	    WHM_ERRMESG("Whm_push");
	    goto errjmp;
	  }
	  memset(temp_string, 0, WHM_DEF_STACK_SIZE);
	  tmp_char = 0;
	}
	sht_char++;      
	continue;
      }
      
      if (sheet_content[sht_char] == NEWLINE){
	/* Don't leave the temporary string full. */
	if (tmp_char > 0) {
	  if (whm_push(sheet_stack, temp_string) != 0) {
	    WHM_ERRMESG("Whm_push");
	    goto errjmp;
	  }
	  memset(temp_string, 0, WHM_DEF_STACK_SIZE);
	  tmp_char = 0;
	}
	if (sheet_stack->tos > 0){
	  if (cur_pos >= job_info[sheet_ind]->numof_positions) cur_pos = 0;
	  errno = 0;
	  if (whm_pop_in_cumulatives(sheet_line_count,
				     ((sheet_line_count > 0) ? cur_pos++ : 0),
				     day_ind,
				     sheet_stack,
				     sheet_info[sheet_ind]) != 0) {
	    WHM_ERRMESG("Whm_pop_in_cumulatives");
	    goto errjmp;
	  }
	  if (sheet_content[sht_char+1] == NEWLINE) {
	    /* 
	     * Increment and make sure that day_ind is not bigger than 7. 
	     * The 8th item (index 7) is the total for the month, with the
	     * same format as the previous days.
	     */
	    if (errno != WHM_USELESS_POP_OP)
	      if (++day_ind > 7) break;	    
	    sht_char++;
	    sheet_line_count = 0;
	    continue;
	  }
	  sheet_line_count++;
	}
	sht_char++;
	continue;
      }
      if ((sheet_content[sht_char] == '-' || sheet_content[sht_char] == ':') && tmp_char == 0){
	if (sheet_content[sht_char] == '-'){
	  if (s_strcpy(temp_string, "-1.0", WHM_DEF_STACK_SIZE) == NULL){
	    WHM_ERRMESG("S_strcpy");
	    goto errjmp;
	  }
	  tmp_char = strlen(temp_string)+1;
	}
	while(sheet_content[sht_char] != SPACE) sht_char++;
	continue;
      }
      temp_string[tmp_char++] = sheet_content[sht_char++];
    }
      
    ++sheet_ind; /* Get next entry. */
  }
  
  if (sheet_stack){
    whm_free_stack(sheet_stack);
    sheet_stack = NULL;
  }
  if (sheet_content){
    free(sheet_content);
    sheet_content = NULL;
  }    
  
  return 0;
  

 errjmp:
  if (sheet_stream){
    fclose(sheet_stream);
    sheet_stream = NULL;
  }
  if (sheet_stack){
    whm_free_stack(sheet_stack);
    sheet_stack = NULL;
  }
    if (sheet_content){
    free(sheet_content);
    sheet_content = NULL;
  }


  return -1;
  
} /* whm_read_hour_sheet() */


/* 
 * Update the given hour sheet, _month_T structure.
 * This function must be interactive unless whm is
 * invoked using the -u or --update switches, in which 
 * case it must automaticaly update the given sheet with the
 * given information.
 * When interactive is > 0, the user is prompted on stdout
 * for revelant informations else the -u or --update switch 
 * must have been used else it's a mistake.

 * -------> NO checks are made for overflow <------

 *
 */
int whm_update_hour_sheet(size_t interactive,
			  whm_month_T *hour_sheet,
			  whm_job_info_T *job_info,
			  whm_time_T *time_o)
{
  size_t pos_ind = 0, input_char = 0, i = 0, week_ind = 0, week_day = 0;
  int c = 0;
  int in;
  char input[WHM_TIME_O_STRING_LENGHT];
  double earnings[WHM_MAX_NUMOF_POSITIONS];
  double week_hours = 0.0, week_cash = 0.0;
  

  if (!hour_sheet || !job_info || !time_o){
    errno = EINVAL;
    return -1;
  }
  /* Find today's entry, represented by week_ind and week_day */
  for (week_ind = 0; week_ind < 6; week_ind++)
    if (atoi(time_o->week) == hour_sheet->week[week_ind]->week_number) break;
  for (week_day = 0; week_day < 7; week_day++)
    if (strcmp(time_o->day, WHM_EN_WEEK_DAYS[week_day]) == 0) break;

  /* 
   * Verify if the entry has already been filled. 
   * In a loop so that in the future we can selectively update
   * positions within an entry. 
   * At the moment it's either all or nothing.
   */
  for (i = 0; i < job_info->numof_positions; i++){
    if (hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i] != -1) {
      errno = WHM_ENTRYALREADYFILLED;
      return -1;
    }
  }
      
  if (interactive){
    printf("\n\n%s\t\t%s %s, %s\n\n",
	   job_info->name, time_o->date, time_o->month, time_o->year);
    while (pos_ind < job_info->numof_positions){
      printf("\nCombien d'heures avez-vous travaille aujourd'hui en tant que %s: ",
	     job_info->position_name[pos_ind]);
      memset(input, 0, WHM_TIME_O_STRING_LENGHT);
      input_char = 0;
      while ((in = getchar())){
	if (in == NEWLINE || in == EOF) break;
	if (input_char >= WHM_TIME_O_STRING_LENGHT){
	  errno = WHM_INPUTTOOLONG;
	  return -1;
	}
	if (isdigit(in))
	  input[input_char++] = in;
	else {
	  errno = WHM_INVALIDHOUR;
	  return -1;
	}
      }
      /* Update hours. */
      hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[pos_ind] = atof(input);
      
      ++pos_ind;
    }
  }
  /* 
   * Else assume the -u or --update switch was used
   * and that main passed the appropriate arguments to us.
   */
  else {
    ;
  }

  /* Calculate the amount of money made for each positions. */
  for (i = 0; i < job_info->numof_positions; i++)
    earnings[i] = (atof(job_info->wage[i])) * hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i];

  /* Conditionals are required since we use the value -1 to represent entries with no values (--.--, etc) */
  for (i = 0; i < job_info->numof_positions; i++){
    /* Calculate the daily amount of money and the hours made, all combined. */
    hour_sheet->week[week_ind]->daily_info[week_day]->total_cash +=
      (hour_sheet->week[week_ind]->daily_info[week_day]->total_cash == -1)
      ? earnings[i]+1
      : earnings[i];
    hour_sheet->week[week_ind]->daily_info[week_day]->total_hours +=
      (hour_sheet->week[week_ind]->daily_info[week_day]->total_hours == -1) 
      ? hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i]+1
      : hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i];

    /* Update the weekly amount of hours and money made for each positions. */
    hour_sheet->week[week_ind]->weekly_cash[i] +=
      (hour_sheet->week[week_ind]->weekly_cash[i] == -1)
      ? earnings[i]+1
      : earnings[i];
    hour_sheet->week[week_ind]->weekly_hours[i] +=
      (hour_sheet->week[week_ind]->weekly_hours[i] == -1)
      ? hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i]+1
      : hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i];

    /* Update the weekly amount of hours and money made, all combined. */
    hour_sheet->week[week_ind]->total_cash +=
      (hour_sheet->week[week_ind]->total_cash == -1)
      ? earnings[i]+1
      : earnings[i];
    hour_sheet->week[week_ind]->total_hours +=
      (hour_sheet->week[week_ind]->total_hours == -1)
      ? hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i]+1
      : hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i];
    /* Update the monthly entries corresponding to the current day. */
    hour_sheet->cumul_hours[week_day] +=
      (hour_sheet->cumul_hours[week_day] == -1)
      ? hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i]+1
      : hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i];
    hour_sheet->cumul_cash[week_day] +=
      (hour_sheet->cumul_cash[week_day] == -1)
      ? earnings[i]+1
      : earnings[i];
    hour_sheet->per_pos_hours[week_day][i] +=
      (hour_sheet->per_pos_hours[week_day][i] == -1)
      ? hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i]+1
      : hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i];
    hour_sheet->per_pos_cash[week_day][i] +=
      (hour_sheet->per_pos_cash[week_day][i] == -1)
      ? earnings[i]+1
      : earnings[i];

    /* Update the monthly totals. */
    hour_sheet->cumul_hours[7] +=
      (hour_sheet->cumul_hours[7] == -1)
      ? hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i]+1
      : hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i];
    hour_sheet->cumul_cash[7] +=
      (hour_sheet->cumul_cash[7] == -1)
      ? earnings[i]+1
      : earnings[i];
    hour_sheet->per_pos_hours[7][i] +=
      (hour_sheet->per_pos_hours[7][i] == -1)
      ? hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i]+1
      : hour_sheet->week[week_ind]->daily_info[week_day]->pos_hours[i];
    hour_sheet->per_pos_cash[7][i] +=
      (hour_sheet->per_pos_cash[7][i] == -1)
      ? earnings[i]+1
      : earnings[i];
    
  }


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
  
    /* Print the week number ,two newlines and 18 spaces to keep columns aligned. */
    fprintf(stream, "Semaine %*d\n\n                  ", WHM_DATE_WIDTH, sheet->week[week_ind]->week_number);

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
    fprintf(stream, "%-17s: %6shrs  ", WHM_FR_LG_WEEK_DAYS[day_ind],
	    (sheet->cumul_hours[day_ind] == -1)
	    ? WHM_NO_T_HOUR
	    : s_ftoa(temp_string, FTOA_S, sheet->cumul_hours[day_ind]));
    fprintf(stream, "%9s$\n",
	    (sheet->cumul_cash[day_ind] == -1)
	    ? WHM_NO_T_CASH
	    : s_ftoa(temp_string, FTOA_S, sheet->cumul_cash[day_ind]));

    for(pos_ind = 0; pos_ind < job_info->numof_positions; pos_ind++){
      fprintf(stream, "%-17s: %6shrs  ", job_info->position_name[pos_ind],
	      (sheet->per_pos_hours[day_ind][pos_ind] == -1)
	      ? WHM_NO_T_HOUR
	      : s_ftoa(temp_string, FTOA_S, sheet->per_pos_hours[day_ind][pos_ind]));
      fprintf(stream, "%9s$\n",
	      (sheet->per_pos_cash[day_ind][pos_ind] == -1)
	      ? WHM_NO_T_CASH
	      : s_ftoa(temp_string, FTOA_S, sheet->per_pos_cash[day_ind][pos_ind]));
    }
    fprintf(stream, "\n");
  }
  /* The monthly totals are at cumul_*[7]. */
  fprintf(stream, "%-17s: %6shrs  ", "Total",
	  (sheet->cumul_hours[7] == -1)
	  ? WHM_NO_T_HOUR
	  : s_ftoa(temp_string, FTOA_S, sheet->cumul_hours[7]));
  fprintf(stream, "%9s$\n",
	  (sheet->cumul_cash[7] == -1)
	  ? WHM_NO_T_CASH
	  : s_ftoa(temp_string, FTOA_S, sheet->cumul_cash[7]));
  
  for(pos_ind = 0; pos_ind < job_info->numof_positions; pos_ind++){
    fprintf(stream, "%-17s: %6shrs  ", job_info->position_name[pos_ind],
	    (sheet->per_pos_hours[7][pos_ind] == -1)
	    ? WHM_NO_T_HOUR
	    : s_ftoa(temp_string, FTOA_S, sheet->per_pos_hours[7][pos_ind]));
    fprintf(stream, "%9s$\n",
	    (sheet->per_pos_cash[7][pos_ind] == -1)
	    ? WHM_NO_T_CASH
	    : s_ftoa(temp_string, FTOA_S, sheet->per_pos_cash[7][pos_ind]));
  }

  fprintf(stream, "\n/**********************************************************************/\n\n");
  
  return 0;
  
#undef FTOA_S
}

