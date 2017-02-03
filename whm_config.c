/*
 *
 * Work Hour Monitor  -  Configuration related functions.
 *
 * Version: 0.01
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "whm.h"
#include "whm_error.h"

/* Fill the structures needed to later write/create the configuration file. */
int whm_new_config(const char *pathname,
		   int *c_ind,
		   whm_config_T **configs)
{
  FILE *stream = NULL;
  char answer[WHM_NAME_STR_S];

  if (!pathname || !configs || !c_ind){
    errno = EINVAL;
    goto errjmp;
  }
  
  if ((stream = fopen(pathname, "r")) != NULL){
    errno = WHM_FILEEXIST;
    goto errjmp;
  }

  /* 
   * Use whm_add_config() to prompt the user to add company(ies)
   * to the configuration file, up to WHM_MAX_CONFIG_ENTRIES,
   * then create a new working directory for this company.
   */
  printf("\nWork Hour Monitor\nNew configuration file setup\n\n");
  while (*c_ind < WHM_MAX_CONFIG_ENTRIES){
    if (whm_add_config(c_ind, configs) != 0) {
      WHM_ERRMESG("Whm_add_config");
      goto errjmp;
    }
    if (whm_new_dir(configs[*c_ind]->working_directory) != 0
	&& errno != WHM_DIREXIST) {
      WHM_ERRMESG("Whm_new_dir");
      goto errjmp;
    }
    
    ++(*c_ind);
    if (whm_ask_user(ADD_COMPANY, answer, WHM_NAME_STR_S,
		     NULL, 0) != 0){
      WHM_ERRMESG("Whm_ask_user");
      goto errjmp;
    }
    if (answer[0] == 'n' || answer[0] == 'N') break;
  }

  return 0;


 errjmp:
  if (stream){
    fclose(stream);
    stream = NULL;
  }
  return -1;

} /* whm_new_config() */


/* Interactively add a single company to the configuration file. */
int whm_add_config(int *c_ind,                   /* Index of the first free element of configs. */
		   whm_config_T **configs)
{
  char *answer = NULL;
  char pathname[WHM_MAX_PATHNAME_S];
  int element_c = 0, ret = 0;


  if (!c_ind || !configs) {
    errno = EINVAL;
    return -1;
  }

  if (*c_ind >= WHM_MAX_CONFIG_ENTRIES) {
     errno = WHM_TOOMANYENTRIES;
    return -1;
  }
  if ((answer = calloc(WHM_NAME_STR_S, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    return -1;
  }

  /*
   * Refer to whm.h for informations on the whm_config_T data type.
   * Long story short, there are 4 elements in the structure that 
   * the user must give information about.
   */
  printf("\nAjout d'une compagnie au fichier de configuration.\n\n");
  
  element_c = 0;
  while (element_c < 4) {
    int pos_ind = 0;
    ret = 0;
    memset(answer, '\0', WHM_NAME_STR_S);
    
    switch(element_c) {
      /* Get the employer's name. */
    case 0:
      if (whm_ask_user(EMPLOYER, answer, WHM_NAME_STR_S,
		       configs[*c_ind], 0) != 0) {
	WHM_ERRMESG("Whm_ask_user");
	goto errjmp;
      }
      if (s_strcpy(configs[*c_ind]->employer, answer, WHM_NAME_STR_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      break;
      
      /* Ask whether there are night shifts and if yes, the amount of the night bonus. */
    case 1:
    input1:
      if (whm_ask_user(NIGHT_PRIME, answer, WHM_NAME_STR_S,
		       configs[*c_ind], 0) != 0){
	WHM_ERRMESG("Whm_ask_user");
	goto errjmp;
      }
      if (isdigit(answer[0]) || answer[0] == DOT || answer[0] == DASH){
	if (s_strcpy(configs[*c_ind]->night_prime, answer, WHM_NAME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  goto errjmp;
	}
      }
      else {
	fprintf(stderr, "\nPlease enter a valid number (-1 or 0.00 and up).\n\n");
	goto input1;
      }
      break;
      
      /* Ask whether the employer pays on each paycheck the 4% normaly cummulated for vacations. */
    case 2:
      if (whm_ask_user(HOLIDAY_PAY, answer, WHM_NAME_STR_S,
		       configs[*c_ind], 0) != 0 ) {
	WHM_ERRMESG("Whm_ask_user");
	goto errjmp;
      }
      if (answer[0] == 'o' || answer[0] == 'O')
	configs[*c_ind]->do_pay_holiday = 1;
      else
	configs[*c_ind]->do_pay_holiday = 0;
      break;
      
      /* Get the name(s) of occupied position(s) with the previously mentioned employer. */
    case 3:
      printf("\nEntrez \"done\", \"exit\" ou \"fin\" lorsque termine.\n");
      /* whm_ask_user sets errno to 0 everytime it's run. */
      while (1){
	if (pos_ind > 0){
	  if ((ret = whm_ask_user(POSITION2, answer, WHM_NAME_STR_S,
				  configs[*c_ind], pos_ind)) != 0
	      && ret != WHM_INPUTDONE){
	    WHM_ERRMESG("Whm_ask_user");
	    goto errjmp;
	  }
	}
	else {
	  if ((ret = whm_ask_user(POSITION, answer, WHM_NAME_STR_S,
				  configs[*c_ind], pos_ind)) != 0
	      && ret != WHM_INPUTDONE){
	    WHM_ERRMESG("Whm_ask_user");
	    goto errjmp;
	  }
	}
	if (ret != WHM_INPUTDONE){
	  if (s_strcpy(configs[*c_ind]->positions[pos_ind], answer, WHM_NAME_STR_S) == NULL){
	    WHM_ERRMESG("S_strcpy");
	    goto errjmp;
	  }
	  /* Get the wage for the position just mentioned. */
	input2:
	  if (whm_ask_user(WAGE, answer, WHM_NAME_STR_S,
			   configs[*c_ind], pos_ind) != 0){
	    WHM_ERRMESG("Whm_ask_user");
	    goto errjmp;
	  }
	  /* Make sure input doesn't begin by anything else than a dot or a digit. */
	  if (isdigit(answer[0]) || answer[0] == DOT){
	    configs[*c_ind]->wages[pos_ind] = atof(answer);
	  }
	  else {
	    fprintf(stderr, "\nPlease enter a valid number (0.00 and up).\n\n");
	    goto input2;
	  }
	  ++pos_ind; /* Next position. */
	}
	else break;
      }
      /* Make sure at least one position was given. */
      if (pos_ind <= 0) {
	errno = WHM_INVALIDPOSNUM;
	goto errjmp;
      }
      /* Save the number of positions. */
      configs[*c_ind]->numof_positions = (size_t)pos_ind;
      break;
      
    default:
      errno = WHM_INVALIDELEMCOUNT;
      goto errjmp;
    }
    if (ret == WHM_INPUTDONE) break;
    ++element_c;
  }
  
  /* 
   * Build the new company's working directory pathname.
   * Ex: "WHM_WORKING_DIRECTORY'/'config[*c_ind]->employer'.'d\0"
   * +4:                        ^                          ^ ^ ^ = 4;
   * s_strcpy adds the terminating NULL byte.
   */
  if ((strlen(WHM_WORKING_DIRECTORY) + strlen(configs[*c_ind]->employer) + 4) >= WHM_MAX_PATHNAME_S){
    errno = WHM_PATHTOOLONG;
    goto errjmp;
  }
  if (s_strcpy(pathname, (char*)WHM_WORKING_DIRECTORY, WHM_MAX_PATHNAME_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }
  strcat(pathname, "/");
  strcat(pathname, configs[*c_ind]->employer);
  strcat(pathname, ".d");
  if (s_strcpy(configs[*c_ind]->working_directory, pathname, WHM_MAX_PATHNAME_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }

  /* Set the status of the new company to active. */
  configs[*c_ind]->status++;

  if (answer){
    free(answer);
    answer = NULL;
  }
  
  return 0;

 errjmp:
  if (answer){
    free(answer);
    answer = NULL;
  }

  return -1;
  
} /* whm_add_config() */


/* Fill a whm_config_T object with relevant data. */
int whm_get_config_entry(whm_config_T *config,
			 FILE *stream)
{
  size_t line_c_ind = 0, word_c_ind = 0, pos_ind = 0;
  char line[WHM_LINE_BUFFER_S], word[WHM_NAME_STR_S];
  int line_count = 0;
  
  if (!stream) {
    errno = EINVAL;
    return -1;
  }

  for (line_count = 0; line_count < 8; line_count++){
    memset(line, '\0', WHM_LINE_BUFFER_S);
    memset(word, '\0', WHM_NAME_STR_S);
    pos_ind = 0;

    /* Read a line then parse it. */
    errno = 0;
    if (fgets(line, WHM_LINE_BUFFER_S, stream) == NULL) {
      if (feof(stream)) return 1;
      else if (errno || ferror(stream)){
	WHM_ERRMESG("Fgets");
	goto errjmp;
      }
    }

    /* Substract 1 cause line_count gets incremented every end of loop. */
    if (line[0] == NEWLINE) {
      --line_count;
      continue;
    }
    else WHM_TRIM_NEWLINE(line);

    /* 
     * Only check if config isn't NULL here so we can detect 
     * EOF even when config is in fact NULL. 
     */
    if (!config) {
      errno = EINVAL;
      return -1;
    }

    /* Populate the whm_config_T object's fields, depending the line we're at. */
    switch(line_count){
    case 0:
      config->status = atoi(line);
      break;

    case 1:
      WHM_REPLACE_SPACE(line);
      if (s_strcpy(config->employer, line, WHM_NAME_STR_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      break;

    case 2:
      if (s_strcpy(config->working_directory,
		   line, WHM_LINE_BUFFER_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      break;

    case 3:
      config->numof_positions = atoi(line);
      break;

    case 4:
      for (line_c_ind = 0; line_c_ind < WHM_LINE_BUFFER_S; line_c_ind++){
	if (line[line_c_ind] == SPACE || line[line_c_ind] == '\0') {
	  word[word_c_ind] = '\0';
	  if (s_strcpy(config->positions[pos_ind++], word, WHM_NAME_STR_S) == NULL){
	    WHM_ERRMESG("S_strcpy");
	    goto errjmp;
	  }
	  word_c_ind = 0;
	  if (line[line_c_ind] == '\0' || line[line_c_ind+1] == '\0') break;
	  memset(word, '\0', WHM_NAME_STR_S);
	  continue;
	}
	word[word_c_ind++] = line[line_c_ind];
      }
      break;

    case 5:
      for (line_c_ind = 0; line_c_ind < WHM_LINE_BUFFER_S; line_c_ind++){
	if (line[line_c_ind] == SPACE || line[line_c_ind] == '\0') {
	  config->wages[pos_ind++] = atof(word);
	  word_c_ind = 0;
	  if (line[line_c_ind] == '\0' || line[line_c_ind+1] == '\0') break;
	  memset(word, '\0', WHM_NAME_STR_S);
	  continue;
	}
	word[word_c_ind++] = line[line_c_ind];
      }
      break;

    case 6:
      if (s_strcpy(config->night_prime, line, WHM_NAME_STR_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      break;

    case 7:
      config->do_pay_holiday = atoi(line);
      break;

    default:
      errno = WHM_INVALIDELEMCOUNT;
      goto errjmp;
    }
  }

  return 0;

 errjmp:
  return -1;
} /* whm_get_config_entry() */
  


/* Read and parse the configuration file. */
int  whm_read_config(FILE *stream,
		     int *c_ind,
		     whm_config_T **configs)
{
  long config_offset = 0;
  size_t header_len = strlen(WHM_CONFIG_HEADER_MSG);
  int ret = 0;
  
  if (!stream || !c_ind || !configs){
    errno = EINVAL;
    return -1;
  }
  /* 
   * Place the file position indicator at the next
   * character after the configuration file header message.
   */
  if (header_len < LONG_MAX) config_offset = (long)header_len;
  else config_offset = LONG_MAX-1;
  if (fseek(stream, config_offset, SEEK_SET) == -1){
    WHM_ERRMESG("Fseek");
    return -1;
  }
  
  /*
   * Fill each whm_config_T objects with the information found in
   * the configuration file. whm_get_config_entry will return
   * 0 after an entry has been filled, and exactly 1 when reached EOF.
   */

  for (*c_ind = 0; *c_ind < WHM_MAX_CONFIG_ENTRIES; (*c_ind)++){
    if ((ret = whm_get_config_entry(configs[*c_ind], stream)) == -1){
      WHM_ERRMESG("Whm_get_config_entry");
      return -1;
    }
    else if (ret == 1) break;
  }

  return 0;

} /* whm_read_config() */


/*
 * Write to disk all informations found in the whm_config_T* array. 
 * c_ind is 1 more than the last element of configs.
 */
int whm_write_config(int c_ind,
		     const char *config_path,
		     whm_config_T **configs)
{
  if (!c_ind || !config_path || !configs) {
    errno = EINVAL;
    return -1;
  }

  FILE *stream = NULL;
  int cur_ind = 0, i = 0;


  /*
   * Open the configuration file given by config_path in writing mode,
   * print the configuration's header message then each objects
   * of the array, each fields of the struct on a separate line.
   * An empty line (just a newline) will separate each companies.
   */

  if ((stream = fopen(config_path, "w")) == NULL){
    WHM_ERRMESG("Fopen");
    return -1;
  }
  fprintf(stream, "%s", WHM_CONFIG_HEADER_MSG);
  for (; cur_ind < c_ind; cur_ind++){
    if (!configs[cur_ind]) break;
    fprintf(stream, "%zu\n%s\n%s\n%zu\n",
	    configs[cur_ind]->status, configs[cur_ind]->employer,
	    configs[cur_ind]->working_directory,
	    configs[cur_ind]->numof_positions);
    
    for (i = 0; i < (int)configs[cur_ind]->numof_positions; i++)
      fprintf(stream, "%s ", configs[cur_ind]->positions[i]);
    fprintf(stream, "\n");
    for (i = 0; i < (int)configs[cur_ind]->numof_positions; i++)
      fprintf(stream, "%.2f ", configs[cur_ind]->wages[i]);
    fprintf(stream, "\n%s\n%zu\n\n",
	    configs[cur_ind]->night_prime,
	    configs[cur_ind]->do_pay_holiday);
  }
   
  fclose(stream);
  stream = NULL;
  if (chmod(config_path, 0600) != 0){
    WHM_ERRMESG("Chmod");
    return -1;
  }


  return 0;

} /* whm_write_config() */


/* Modify a single field of a single entry in the configuration file. */
int whm_modify_config(char *company,
		      char *position,
		      enum whm_config_field_type field,
		      char *value,
		      int c_ind,
		      whm_config_T **configs)
{
  int pos_ind = 0;
  int pos_found = 0, company_found = 0;
  char new_path[WHM_MAX_PATHNAME_S];
  char *temp_string = NULL;

  if (!company || c_ind < 0){
    errno = EINVAL;
    return -1;
  }

  /* 
   * Switch on the field, convert value to an appropriate data type if needed
   * and update the field's value with the caller's value.
   */
  switch(field){
  case F_STATUS:
    if (value[0] == 'a' || value[0] == 'A' || value[0] == '1')
      configs[c_ind]->status = 1;
    else 
      configs[c_ind]->status = 0;
    break;

  case F_EMPLOYER:
    if (s_strcpy(configs[c_ind]->employer, value, WHM_NAME_STR_S) == NULL){
      WHM_ERRMESG("S_strcpy");
      return -1;
    }
    /* 
     * Build the new company's working directory pathname.
     * Ex: "WHM_WORKING_DIRECTORY'/'config[*c_ind]->employer'.'d\0"
     * +4:                        ^                          ^ ^ ^ = 4;
     * s_strcpy adds the terminating NULL byte.
     */
    if ((strlen(WHM_WORKING_DIRECTORY) + strlen(configs[c_ind]->employer) + 4)
	>= WHM_MAX_PATHNAME_S) {
      errno = WHM_PATHTOOLONG;
      return -1;
    }
    if (s_strcpy(new_path, (char*)WHM_WORKING_DIRECTORY, WHM_MAX_PATHNAME_S) == NULL){
      WHM_ERRMESG("S_strcpy");
      return -1;
    }
    strcat(new_path, "/");
    strcat(new_path, configs[c_ind]->employer);
    strcat(new_path, ".d");
    /* 
     * Rename the working directory with the new company name. 
     * Existing hour sheets keeps their original company name.
     */
    if (rename(configs[c_ind]->working_directory, new_path) == -1){
      WHM_ERRMESG("Rename");
      return -1;
    }
    /* Save the new pathname. */
    if (s_strcpy(configs[c_ind]->working_directory, new_path, WHM_MAX_PATHNAME_S) == NULL){
      WHM_ERRMESG("S_strcpy");
      return -1;
    }
    break;

  case F_POSITION:
    /* 
     * Make sure the given position name isn't null,
     * check against the existing position names. If it matches and value isn't
     * null, replace the matched position name by value, else if value is null,
     * remove position name from the entry.
     * If position name doesn't match, add it to the given company and make sure
     * value is a double integer indicating the wage for this new position.
     */
    if (!position) {
      errno = EINVAL;
      return -1;
    }
    for (; pos_ind < (int)configs[c_ind]->numof_positions; pos_ind++)
      if (s_strcmp(configs[c_ind]->positions[pos_ind], position, WHM_NAME_STR_S, LS_ICASE) == 0){
	pos_found++;
	break;
      }
    if (pos_found) {
      if (value && value[0] != '\0') {
	if (s_strcpy(configs[c_ind]->positions[pos_ind], position, WHM_NAME_STR_S) == NULL){
	  WHM_ERRMESG("s_strcpy");
	  return -1;
	}
      }
      else {
	/* Reajust the positions and wages arrays. */
	memset(configs[c_ind]->positions[pos_ind], '\0', WHM_NAME_STR_S);
	temp_string = configs[c_ind]->positions[pos_ind];
	while (pos_ind+1 < (int)configs[c_ind]->numof_positions){
	  configs[c_ind]->positions[pos_ind] = configs[c_ind]->positions[pos_ind+1];
	  configs[c_ind]->wages[pos_ind] = configs[c_ind]->wages[pos_ind+1];
	  pos_ind++;
	}
	configs[c_ind]->positions[pos_ind] = temp_string;
	temp_string = NULL;
      }
    }
    /* The position name did not exist. */
    else {
      if (!value || !position || position[0] == '\0') {
	errno = EINVAL;
	return -1;
      }
      /* Return an error for now. Later will ask for the missing wage itself. */
      else if (value[0] == '\0') {
	errno = WHM_MISSINGWAGE;
	return -1;
      }
      else if (!isdigit(value[0])){
	errno = WHM_ISNOTDIGIT;
	return -1;
      }
      else if (configs[c_ind]->numof_positions+1 >= WHM_DEF_NUMOF_POSITIONS) {
	errno = WHM_TOOMANYENTRIES;
	return -1;
      }
      if (s_strcpy(configs[c_ind]->positions[configs[c_ind]->numof_positions],
		   position, WHM_NAME_STR_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	return -1;
      }
      configs[c_ind]->wages[configs[c_ind]->numof_positions] = atof(value);
      configs[c_ind]->numof_positions++;
    }
    break;

  case F_WAGE:
    if (!position) {
      errno = EINVAL;
      return -1;
    }
    /* Make sure the given position exist. */
    for (; pos_ind < (int)configs[c_ind]->numof_positions; pos_ind++)
      if (s_strcmp(configs[c_ind]->positions[pos_ind], position, WHM_NAME_STR_S, LS_ICASE) == 0){
	pos_found++;
	break;
      }
    if (!pos_found) {
      errno = WHM_INVALIDPOSITION;
      return -1;
    }
    if (!isdigit(value[0])){
      errno = WHM_ISNOTDIGIT;
      return -1;
    }
    configs[c_ind]->wages[pos_ind] = atof(value);
    break;
		     	     

  case F_NIGHT_PRIME:
    if (!isdigit(value[0]) && value[0] != DOT && value[0] != DASH){
      errno = WHM_ISNOTDIGIT;
      return -1;
    }
    if (s_strcpy(configs[c_ind]->night_prime, value, WHM_NAME_STR_S) == NULL){
      WHM_ERRMESG("S_strcpy");
      return -1;
    }
    break;

  case F_HOLIDAY_PAY:
    if (!value){
      errno = EINVAL;
      return -1;
    }
    if (!isdigit(value[0])){
      errno = WHM_ISNOTDIGIT;
      return -1;
    }
    if (value[0] >= '1') configs[c_ind]->do_pay_holiday = 1;
    else configs[c_ind]->do_pay_holiday = 0;
    break;

  default:
    errno = WHM_INVALIDFIELD;
    return -1;
  }
  return 0;
} /* whm_modify_config() */


/* 
 * Interactively get a company name to feed 
 * to whm_modify_config() or whm_add_config().
 * A null return value with errno set to WHM_COMPANYCREATED
 * indicates the company was created and its entry does
 * not need to be modified and the new company name is stored in string.
 */
char* whm_get_company_name(char *string, size_t string_s,
			   int *c_ind,
			   int max_config_ind,
			   whm_config_T **configs)
{
  char answer[WHM_NAME_STR_S];
  int  company_found = 0;

  if (!string || !string_s || !max_config_ind
      || !max_config_ind || !configs) {
    errno = EINVAL;
    return NULL;
  }

  while (1) {
  ask_company_name:
    if (whm_ask_user(MODIF_COMPANY_NAME, string, string_s,
		     NULL, 0) != 0) {
      WHM_ERRMESG("Whm_ask_user");
      return NULL;
    }
    if (strstr(string, "list") != NULL){
      if (whm_list_config_names(max_config_ind, configs) != 0){
	WHM_ERRMESG("Whm_list_config_names");
	return NULL;
      }
      continue;
    }
    else break;
  }

  /* 
   * Verify the given company name exist, 
   * else propose to create an entry for it.  
   */
  for (*c_ind = 0; *c_ind < max_config_ind; (*c_ind)++)
    if (s_strcmp(string, configs[*c_ind]->employer, WHM_NAME_STR_S, LS_ICASE) == 0){
      ++company_found;
      break;
    }

  if (!company_found) {
    if (whm_ask_user(MODIF_UNKNOWN_COMPANY, answer,
		     WHM_NAME_STR_S, NULL, 0) != 0){
      WHM_ERRMESG("Whm_ask_user");
      return NULL;
    }
    if (answer[0] == 'o' || answer[0] == 'O'
	|| answer[0] == 'y' || answer[0] == 'Y'){
      if (whm_add_config(&max_config_ind, configs) != 0){
	WHM_ERRMESG("Whm_add_config");
	return NULL;
      }
      errno = WHM_COMPANYCREATED;
      return NULL;
    }
    else goto ask_company_name;
  }
  
  return string;

} /* whm_get_company_name() */


/* Interactively ask the field to edit from the configuration entry. */
int whm_get_field_name(char *company,
		       int max_config_ind,
		       whm_config_T **configs)
{
  char value[WHM_NAME_STR_S];
  int intvalue = 0;
  
  if (!company || !max_config_ind || !configs) {
    errno = EINVAL;
    return -1;
  }

  while (1) {
    if (whm_ask_user(MODIF_CONFIG_FIELD, value,
		     WHM_NAME_STR_S,
		     NULL, 0) != 0) {
      WHM_ERRMESG("Whm_ask_user");
      return -1;
    }
    if (strstr(value, "list") != NULL){
      if (whm_list_config_fields(company, max_config_ind, configs) != 0){
	WHM_ERRMESG("Whm_list_config_fields");
	return -1;
      }
    }
    else if (isdigit(value[0])) break;
  }

  return ((intvalue = atoi(value)));

} /* whm_get_field_name() */


/* Interactively modify one or more entries from the configuration file. */
int whm_inter_modify_config(int max_config_ind,
			    whm_config_T **configs)
{

#define WHM_VALUE_S WHM_NAME_STR_S*2+1
  char company[WHM_NAME_STR_S], position[WHM_NAME_STR_S], temp[WHM_NAME_STR_S];
  char value[WHM_VALUE_S]; /* Some values requires 2 names and a space inbetween. */
  int c_ind = 0, i = 0, value_ind = 0;
  int field = -1;

  if (!max_config_ind || !configs){
    errno = EINVAL;
    return -1;
  }

  memset(temp, '\0', WHM_NAME_STR_S);
  puts("\nWork Hour Monitor\nConfiguration file modification\n\n");
  
  /* Ask user which company to modify. */
  if (whm_get_company_name(company, WHM_NAME_STR_S,
			   &c_ind,
			   max_config_ind,
			   configs) == NULL){
    /* Return successfuly if we had to create a new entry for the given name. */
    if (errno == WHM_COMPANYCREATED) return 0;
    WHM_ERRMESG("Whm_get_company_name");
    return -1;
  }

  /* Ask which field to modify (out of ones allowed to be modified). */
  if ((field = whm_get_field_name(company, max_config_ind, configs)) == -1){
    WHM_ERRMESG("Whm_get_field_name");
    return -1;
  }

  /* 
   * Switch on the field asking the remaining
   * informations (postion name, old/new value etc). 
   */
  switch(field){
  case F_STATUS:
    if (whm_ask_user(FIELD_STATUS, value,
		     WHM_VALUE_S, configs[c_ind], 0) == -1){
      WHM_ERRMESG("Whm_ask_user");
      return -1;
    }
    break;

  case F_EMPLOYER:
    if (whm_ask_user(FIELD_EMPLOYER, value,
		     WHM_VALUE_S, configs[c_ind], 0) == -1){
      WHM_ERRMESG("Whm_ask_user");
      return -1;
    }
    break;
    
  case F_POSITION:
    /* 
     * The answer to this question can take 3 forms:
     * position_name\0                             Delete position name.
     * old_position_name new_position_name\0       Replace old_position_name by new_position_name.
     * new_position_name new_wage\0                Add the position new_position_name with wage new_wage.
     */
    if (whm_ask_user(FIELD_POSITION, value,
		     WHM_VALUE_S, configs[c_ind], 0) == -1){
      WHM_ERRMESG("Whm_ask_user");
      return -1;
    }
    /* The first part of value is always the positions name. */
    for (value_ind = 0; value[value_ind] != '\0'; value_ind++){
      if (value[value_ind] == SPACE){
	value_ind++;
	break;
      }
      position[i++] = value[value_ind];
    }
    position[i] = '\0';
    /* 
     * If the terminating NULL byte hasn't been reached yet, this is 
     * either the new position name or the wage. Whm_modify_config() 
     * knows how to differentiate them. 
     */
    for (i = 0; value[value_ind] != '\0'; value_ind++)
      temp[i++] = value[value_ind];
    if (s_strcpy(value, temp, WHM_VALUE_S) == NULL){
      WHM_ERRMESG("S_strcpy");
      return -1;
    }
    break;
    
  case F_WAGE:
    if (whm_ask_user(FIELD_WAGE, value,
		     WHM_VALUE_S,
		     NULL, 0) == -1){
      WHM_ERRMESG("Whm_ask_user");
      return -1;
    }
    /* 
     * Whm_modify_config will verify position name is valid,
     * all there is to be done is to separate the wage and position name.
     */
    for(value_ind = 0; value[value_ind] != '\0'; value_ind++){
      if (value[value_ind] == SPACE) {
	if (i > 0) {
	  position[i] = '\0';
	  break;
	}
      }
      position[i++] = value[value_ind];
    }
    
    for(i = 0; value[value_ind] != '\0'; value_ind++)
      temp[i++] = value[value_ind];
    if (i > 0){
      if (s_strcpy(value, temp, WHM_VALUE_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	return -1;
      }
    }
    else{
      errno = WHM_INVALIDWAGE;
      return -1;
    }
    break;

  case F_NIGHT_PRIME:
    if (whm_ask_user(FIELD_NIGHT_PRIME, value,
		     WHM_VALUE_S,
		     NULL, 0) == -1){
      WHM_ERRMESG("Whm_ask_user");
      return -1;
    }
    break;

  case F_HOLIDAY_PAY:
    if (whm_ask_user(FIELD_HOLIDAY_PAY, value,
		     WHM_VALUE_S,
		     NULL, 0) == -1){
      WHM_ERRMESG("Whm_ask_user");
      return -1;
    }
    break;

  default:
    errno = WHM_INVALIDFIELD;
    return -1;
  }

  /* Make the call to whm_modify_config() to apply the requested modifications. */
  if (whm_modify_config(company, position,
			field, value,
			c_ind,
			configs) == -1){
    WHM_ERRMESG("Whm_modify_config");
    return -1;
  }

  return 0;

#undef WHM_VALUE_S
#undef WHM_GET_ONE_VALUE
#undef WHM_GET_TWO_VALUES
} /* whm_inter_modify_config() */


/* Print to stdout a list of all company names within the configuration file. */
int whm_list_config_names(int max_config_ind,
			  whm_config_T **configs)
{
  int c_ind = 0;

  if (!configs || max_config_ind < 0){
    errno = EINVAL;
    return -1;
  }
  puts("\nWork Hour Monitor\nList of active companies\n");
  while (c_ind < max_config_ind) {
    if (!configs[c_ind]) break;
    if (!configs[c_ind]->status) {
      ++c_ind;
      continue;
    }
    printf("%s\n", configs[c_ind]->employer);
    ++c_ind;
  }

  return 0;
} /* whm_list_config_names() */


/* 
 * Print to stdout all modifiable fields of a given 
 * company's configuration file entry and their respective values.
 */
int whm_list_config_fields(char *company,
			   int max_config_ind,
			   whm_config_T **configs)
{
  int c_ind = 0, i = 0;
  char temp[WHM_NAME_STR_S];

  if (!company || max_config_ind < 0 || !configs){
    errno = EINVAL;
    return -1;
  }

  while (c_ind < max_config_ind){
    if (!configs[c_ind]) break;
    if (s_strcmp(configs[c_ind]->employer, company, WHM_NAME_STR_S, LS_ICASE) == 0){
      printf("\n\nWork Hour Monitor\nList of modifiable fields for %s's configuration file entry.\n\n\n", company);

      printf("0 - %-18s %zu\n1 - %-18s %s\n2 - %-18s ", "Status:", configs[c_ind]->status,
	     "Employer:", configs[c_ind]->employer,
	     "Positions names:");
      for (; i < configs[c_ind]->numof_positions; i++)
	printf("%-18s ", configs[c_ind]->positions[i]);

      printf("\n3 - %-18s ", "Position wages:");
      for (i = 0; i < configs[c_ind]->numof_positions; i++)
	printf("%.2f              ", configs[c_ind]->wages[i]);
      printf("\n4 - %-18s %s\n5 - %-18s %s\n",
	     "Night bonus:", configs[c_ind]->night_prime,
	     "Holiday pay:", (configs[c_ind]->do_pay_holiday > 0) ? "Yes" : "No");

      puts("\n");
      return 0;
    }
    ++c_ind;
  }
  errno = WHM_INVALIDCOMPANY;
  return -1;
} /* whm_list_config_fields() */
  

/* 
 * Delete the given company name and all related informations from 
 * the configuration file.
 * This will NOT remove the company's directory and hours sheets.
 * A user must do this manualy.
 */
int whm_rm_config(char *company,
		  int *max_config_ind,
		  whm_config_T **configs)
{
  int c_ind = 0, i = 0;
  whm_config_T *temp = NULL;

  if (!company || !max_config_ind || !*max_config_ind || !configs){
    errno = EINVAL;
    return -1;
  }
  
  while (c_ind < *max_config_ind) {
    if (s_strcmp(company, configs[c_ind]->employer, WHM_NAME_STR_S, LS_ICASE) == 0){
      /* Move the matched entry to the end of the array. */
      temp = configs[c_ind];
      while (c_ind+1 < *max_config_ind) {
	configs[c_ind] = configs[c_ind+1];
	++c_ind;
      }
      configs[++c_ind] = temp;
      /* Overwrite some information that could be sensitive. */
      memset(configs[c_ind]->employer, '\0', WHM_NAME_STR_S);
      memset(configs[c_ind]->working_directory, '\0', WHM_MAX_PATHNAME_S);
      for (i = 0; i < configs[c_ind]->numof_positions; i++){
	memset(configs[c_ind]->positions[i], '\0', WHM_NAME_STR_S);
	configs[c_ind]->wages[i] = 0;
      }
      configs[c_ind] = NULL;
      --(*max_config_ind);
      return 0;
    }
    ++c_ind;
  }
  errno = WHM_INVALIDCOMPANY;
  return -1;
} /* whm_delete_config() */
