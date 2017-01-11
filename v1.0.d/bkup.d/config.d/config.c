/*
 *
 * Work Hour Monitor  -  Main utilities for configuration files.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>

#include "../whm.h"
#include "../whm_errcodes.h"


/* Safely src into a dest of size n. */
void *s_strcpy(char *dest, const char *src, size_t n)
{
  size_t src_s = 0;

  if (dest != NULL                /* We need an already initialized buffer. */
      && src != NULL              /* We need a valid, non-empty source. */
            && src[0] != '\0'
      && n > 0                    /* Destination buffer's size must be bigger than 0, */
      && n < (SIZE_MAX - 1))      /* and smaller than it's type size - 1. */
    ; /* Valid input. */
  else {
    errno = EINVAL;
    return NULL;
  }

  src_s = strlen(src);
  if (src_s > n - 1){ /* -1, we always add a NULL byte at the end of string. */
    errno = EOVERFLOW;
    fprintf(stderr,"%s: Source must be at most destination[n - 1]\n\n", __func__);
    dest = NULL;
    return dest;
  }

  memset(dest, 0, n);
  memcpy(dest, src, src_s);

  return dest;
}

/* 
 * Converts the double integer src into 
 * a previously initialized string dest of 
 * at least dest_s bytes in size.
 * The string is null terminated.
 */
char* s_ftoa(char *dest, size_t dest_s, double src)
{
  if (!dest || dest_s < S_FTOA_MIN_STR_SIZE) {
    errno = EINVAL;
    return NULL;
  }
  memset(dest, 0, dest_s);
  if (snprintf(dest, dest_s-1, "%.2f", src) < 0){
    fprintf(stderr, "%s: Failed to print the given number to the given buffer\n\n", __func__);
    return NULL;
  }
  dest[strlen(dest)] = '\0';

  return dest;
}


/*
 * Converts the integer src into a
 * previously initialized string dest of
 * at least dest_s bytes in size.
 * The string is null terminated.
 */
char* s_itoa(char *dest, size_t dest_s, int src)
{
  if (!dest || dest_s < S_FTOA_MIN_STR_SIZE) {
    errno = EINVAL;
    return NULL;
  }
  memset(dest, 0, dest_s);
  if (snprintf(dest, dest_s, "%d", src) < 0){
    fprintf(stderr, "%s: Failed to print the given number to the given buffer\n\n", __func__);
    return NULL;
  }
  dest[strlen(dest)] = '\0';
  return dest;
}


/*
 * Replaces all spaces within input up to in_s bytes
 * by underscores and copy the results back into input.
 * Takes input string no longer than WHM_MAX_HR_SHEET_LENGHT (default: 6000).
 */
char* whm_replace_spaces(char *input, size_t in_s)
{
  char *temp_in = NULL;
  size_t in_ind = 0, temp_ind = 0;

  if (!input || !in_s || in_s > WHM_MAX_HR_SHEET_LENGHT) {
    errno = EINVAL;
    return NULL;
  }
  if ((temp_in = calloc(in_s, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    return NULL;
  }
  for (in_ind = 0;
       in_ind < strlen(input) && input[in_ind] != '\0';
       in_ind++)
    if (input[in_ind] == SPACE) temp_in[temp_ind++] = '_';
    else temp_in[temp_ind++] = input[in_ind];
  if (s_strcpy(input, temp_in, in_s) == NULL){
    WHM_ERRMESG("S_strcpy");
    free(temp_in);
    temp_in = NULL;
    return NULL;
  }
    free(temp_in);
    temp_in = NULL;
    return input;
} /* whm_replace_spaces() */
  

/* Create a backup of the given file, if it exist. */
int whm_backup_file(const char *pathname, char *npathname)
{
  FILE *filestream = NULL;
  char file_content[WHM_MAX_MAINCONF_LENGHT];
  char new_pathname[WHM_MAX_PATHNAME_LENGHT];
  size_t pathname_len = 0, i = 0, slash_ind = 0, np_ind = 0;

  if (!pathname || !npathname) {
    errno = EINVAL;
    return -1;
  }
  if ((pathname_len = strlen(pathname)) >= WHM_MAX_PATHNAME_LENGHT-3){
    errno = WHM_CONFNAMETOOLONG;
    return -1;
  }
  /* Open and read the file. */
  if ((filestream = fopen(pathname, "r")) == NULL){
    WHM_ERRMESG("Fopen");
    return -1;
  }
  memset(file_content, 0, WHM_MAX_MAINCONF_LENGHT);
  fread(file_content, sizeof(char), WHM_MAX_MAINCONF_LENGHT-1, filestream);
  if (ferror(filestream)){
    WHM_ERRMESG("Fread");
    goto errjmp;
  }
  fclose(filestream);
  filestream = NULL;

  /* 
   * Make the new pathname. A leading and a trailing '#' must be
   * appended to the last part of the pathname (the filename part of the pathname).
   */
  for (i = pathname_len; i > 0; i--)
    if (pathname[i] == '/' && pathname[i-1] != '\\') {
      slash_ind = i;
      break;
    }
  for (i = 0; i < pathname_len; i++){
    /* +2: We want the leading '#' to be after the leading '.' */
    if (i == slash_ind+2) new_pathname[np_ind++] = '#';
    new_pathname[np_ind++] = pathname[i];
  }
  new_pathname[np_ind++] = '#' ;
  new_pathname[np_ind]   = '\0';

  /* Check for an existing backup and remove it. */
  if ((filestream = fopen(new_pathname, "r")) != NULL){
    fclose(filestream);
    filestream = NULL;
    if (remove(new_pathname) != 0) {
      WHM_ERRMESG("Remove");
      goto errjmp;
    }
  }
  /* Create new_pathname in writing mode overwriting any existing one. */
  if ((filestream = fopen(new_pathname, "w")) == NULL){
    WHM_ERRMESG("Fopen");
    goto errjmp;
  }
  fprintf(filestream, "%s", file_content);
  fclose(filestream);
  filestream = NULL;
  /* Make the backup read-only. */
  if (chmod(new_pathname, 0400) != 0) {
    WHM_ERRMESG("Chmod");
    goto errjmp;
  }
  if (s_strcpy(npathname, new_pathname, WHM_MAX_PATHNAME_LENGHT) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }

  return 0;

 errjmp:
  if (filestream){
    fclose(filestream);
    filestream = NULL;
  }
  return -1;
} /* whm_backup_file() */


/* Remove the given backup filename. */
int whm_rm_backup_file(const char *filename)
{
  FILE *filestream = NULL;

  if (!filename) {
    errno = EINVAL;
    return -1;
  }
  if ((filestream = fopen(filename, "r")) == NULL){
    /* fopen already set errno. */
    return -1;
  }
  fclose(filestream);
  filestream = NULL;
  if (remove(filename) != 0) {
    WHM_ERRMESG("Remove");
    return -1;
  }

  return 0;
} /* whm_rm_backup_file() */
    

/* Check whether the given input is considered complete. */
int whm_check_input_done(char *input)
{
  if (strcmp(input, "done") == 0 || strcmp(input, "Done") == 0
      || strcmp(input, "d") == 0 || strcmp(input, "D") == 0)
    return 1;
  else
    return 0;
}


/* 
 * Creates the program's working directory if it doesn't exists
 * from the absolute pathname specified by the WHM_WORK_DIR
 * constant string from whm.h
 */
int whm_create_work_dir(const char *dirname)
{
  FILE *filestream = NULL;
  
  if (!dirname) {
    errno = EINVAL;
    return -1;
  }

  if ((filestream = fopen(dirname, "r")) == NULL){
    if (mkdir(dirname, 00700) != 0){
      WHM_ERRMESG("Mkdir");
      return -1;
    }
    return 0;
  }
  fclose(filestream);
  errno = WHM_DIREXISTS;
  return -1;
  
} /* whm_create_work_dir() */


/* Create the main configuration file when it doesn't exist. */
int whm_create_main_config(const char *filename)
{
  FILE *filestream = NULL, *test_stream = NULL;
  char input[WHM_MAX_CONF_NAME_LENGHT];
  char wage[WHM_MAX_ENTRIES][WHM_WAGE_STRING_LENGHT];
  char positions[WHM_MAX_ENTRIES][WHM_MAX_CONF_NAME_LENGHT];
  char company_path[WHM_MAX_PATHNAME_LENGHT];
  size_t pos_ind, i = 0, numof_positions = 0;
#define WHM_MEMSET_POS() do {						\
    size_t c = 0;							\
    while (c < WHM_MAX_ENTRIES)						\
      memset(positions[c++], 0, WHM_MAX_CONF_NAME_LENGHT);		\
  } while (0);
#define WHM_MEMSET_WAG() do {				\
    size_t c = 0;					\
    while(c < WHM_MAX_ENTRIES)				\
      memset(wage[c++], 0, WHM_WAGE_STRING_LENGHT);	\
  } while (0);
  
  if (filename == NULL) {
    errno = EINVAL;
    return -1;
  }
  /* Check if the file already exists. */
  if ((filestream = fopen(filename, "r")) == NULL){
    /* Create it. */
    if ((filestream = fopen(filename, "w")) == NULL){
      WHM_ERRMESG("Fopen");
      return -1;
    }
  }
  /* It does already exists, we shouldn't be here. */
  else {
    errno = WHM_CONFEXISTS;
    goto errjmp;
  }
  printf("\nWork Hour Monitor: Creation d'un fichier de configuration\n\n");
  /* Print a header message into the new configuration file. */
  fprintf(filestream, "%s\n\n\n", WHM_MAIN_CONF_HEADER);

  /* Ask user which companies to add to the main configuration file. */
  while(1){
    WHM_MEMSET_POS();
    WHM_MEMSET_WAG();
    memset(input, 0, WHM_MAX_CONF_NAME_LENGHT);
    numof_positions = 0;
    printf("'done' ou 'd' quand c'est fini.\n");
    printf("Entrer le nom d'une compagnie a ajouter au fichier de configuration: ");
    if (fgets(input, WHM_MAX_CONF_NAME_LENGHT, stdin) == NULL){
      WHM_ERRMESG("Fgets");
      goto errjmp;
    }
    /* Trim the terminating newline character. */
    WHM_TRIM_TRAILING_NL(input);
    if (whm_check_input_done(input)) break;
    /* Make sure the requested name fits into the main configuration file's format. */
    if (strlen(input) >= WHM_MAX_CONF_NAME_LENGHT) {
      errno = WHM_CONFNAMETOOLONG;
      goto errjmp;
    }
    /* Replace spaces by underscores. */
    if (whm_replace_spaces(input, WHM_MAX_CONF_NAME_LENGHT) == NULL){
      WHM_ERRMESG("Whm_replace_spaces");
      goto errjmp;
    }

    /* Ask the user for the position(s) name(s) and wage(s) per hour. */
    printf("Quel poste(s) occupez-vous chez '%s': ", input);
    pos_ind = 0;
    while(1) {
      if (pos_ind >= WHM_MAX_ENTRIES) {
	errno = WHM_TOOMANYENTRIES;
	goto errjmp;
      }
      if (fgets(positions[pos_ind], WHM_MAX_CONF_NAME_LENGHT, stdin) == NULL){
	WHM_ERRMESG("Fgets");
	goto errjmp;
      }
      /* Trim the newline. */
      WHM_TRIM_TRAILING_NL(positions[pos_ind]);
      if (strlen(positions[pos_ind]) >= WHM_MAX_CONF_NAME_LENGHT){
	errno = WHM_CONFNAMETOOLONG;
	goto errjmp;
      }
      if (whm_check_input_done(positions[pos_ind])) break;
      /* Replace spaces by underscores. */
      if (whm_replace_spaces(positions[pos_ind], WHM_MAX_CONF_NAME_LENGHT) == NULL){
	WHM_ERRMESG("Whm_replace_spaces");
	goto errjmp;
      }
      ++numof_positions;
      /* Ask the user for the wage per hour for the postition just mentioned. */
      printf("\nQuel est votre taux horraire regulier, par heure, pour le poste de '%s': ", positions[pos_ind]);
      if (fgets(wage[pos_ind], WHM_WAGE_STRING_LENGHT, stdin) == NULL){
	WHM_ERRMESG("Fgets");
	goto errjmp;
      }
      WHM_TRIM_TRAILING_NL(wage[pos_ind]);
      if (strlen(wage[pos_ind]) >= WHM_WAGE_STRING_LENGHT) {
	errno = WHM_CONFNAMETOOLONG;
	goto errjmp;
      }
      if (whm_check_input_done(wage[pos_ind])) {
	errno = WHM_INCOMPLETEENTRY;
	goto errjmp;
      }
      /* Replace spaces by underscores. */
      if (whm_replace_spaces(wage[pos_ind], WHM_WAGE_STRING_LENGHT) == NULL){
	WHM_ERRMESG("Whm_replace_spaces");
	goto errjmp;
      }
      ++pos_ind;
      printf("\n\nQuel autres postes occupez-vous chez '%s'?: ", input);
    }
    /* 
     * Make an absolute pathname using the WHM_WORK_DIR constant,
     * the company's name and the WHM_CONFIG_SUFX constant, then create
     * a directory at this location.
     */
    memset(company_path, 0, WHM_MAX_PATHNAME_LENGHT);
    errno = 0;
    WHM_MAKE_PATH(company_path, WHM_WORK_DIR, input);
    if (errno) goto errjmp;
    if (strlen(company_path) + strlen(WHM_CONFIG_SUFX) >= WHM_MAX_PATHNAME_LENGHT){
      errno = WHM_PATHNAMETOOLONG;
      goto errjmp;
    }
    strcat(company_path, WHM_CONFIG_SUFX);
    if ((test_stream = fopen(company_path, "r")) != NULL){
      WHM_ERRMESG("Fopen");
      goto errjmp;
    }
    if (test_stream != NULL){
      fclose(test_stream);
      test_stream = NULL;
    }
    if (mkdir(company_path, 00700) != 0) {
      WHM_ERRMESG("Mkdir");
      goto errjmp;
    }
    /* 
     * Print the freshly fetched values to the new configuration file.
     * Status (active by default), company name, company's working directory pathname,
     * the number of positions occupied in this company, the names of these positions,
     * and the wage for each of these positions.

     * No matter the formating, always terminate with 3 consecutives newlines.
     */
    fprintf(filestream, "1\n%s\n%s\n%zu\n", input, company_path, numof_positions);
    for (i = 0; i < pos_ind; i++) fprintf(filestream, "%s ", positions[i]);
    fprintf(filestream, "\n");
    for (i = 0; i < pos_ind; i++) fprintf(filestream, "%s$ ", wage[i]);

    fprintf(filestream, "\n\n");
    /* Get the next job to add to the main configuration file. */
    
  } /* while(1) */
  
  /* Change the main configuration file's permissions to 0600. */
  if (chmod(filename, 0600) != 0){
    WHM_ERRMESG("Chmod");
    goto errjmp;
  }

  if (filestream) {
    fclose(filestream);
    filestream = NULL;
  }
  if (test_stream != NULL){
    fclose(test_stream);
    test_stream = NULL;
  }

  return 0;

 errjmp:
  if (filestream){
    fclose(filestream);
    filestream = NULL;
  }
  if (test_stream != NULL){
    fclose(test_stream);
    test_stream = NULL;
  }

  return -1;

#undef WHM_MEMSET_WAG
#undef WHM_MEMSET_POS

} /* whm_create_main_config() */


/* To make it easier to modify the configuration's file formating. */
int whm_pop_in_job_info(whm_job_info_T **job_array,
			size_t *ja_ind,
			whm_stack_T *mc_stack,
			int *numof_positions)
{
  int i = 0;
  
  if (!job_array || !mc_stack || *numof_positions <= 0){
    errno = EINVAL;
    return -1;
  }
  if (*ja_ind >= WHM_MAX_ENTRIES) {
    errno = WHM_STACKFULL;
    return -1;
  }
  /*
   * Wages has been pushed last on the stack,
   * preceded by names of position, the numbers of said positions,
   * the working directory for this company, the name of the company
   * and the status of this company, in this specific order. 
   * Pop them all into their respective job_array[*ja_ind] slots,
   * clean the stack and increment *ja_ind for the next itteration.
   */
  for (i = 0; i < *numof_positions; i++){
    if (s_strcpy(job_array[*ja_ind]->wage[i], whm_pop(mc_stack), WHM_WAGE_STRING_LENGHT) == NULL){
      WHM_ERRMESG("S_strcpy");
      return -1;
    }
  }
  for (i = 0; i < *numof_positions; i++){
    if (s_strcpy(job_array[*ja_ind]->position_name[i], whm_pop(mc_stack), WHM_MAX_CONF_NAME_LENGHT) == NULL){
      WHM_ERRMESG("S_strcpy");
      return -1;
    }
  }
  job_array[*ja_ind]->numof_positions = atoi(whm_pop(mc_stack));
  if (s_strcpy(job_array[*ja_ind]->work_dir, whm_pop(mc_stack), WHM_MAX_PATHNAME_LENGHT) == NULL){
    WHM_ERRMESG("S_strcpy");
    return -1;
  }
  if(s_strcpy(job_array[*ja_ind]->name, whm_pop(mc_stack), WHM_MAX_CONF_NAME_LENGHT) == NULL){
    WHM_ERRMESG("S_strcpy");
    return -1;
  }
  job_array[*ja_ind]->status = atoi(whm_pop(mc_stack));
  
  /* Reset numof_positions, get ready for next config entry. */
  *numof_positions = 0;
  /* Increment job_array's index. */
  (*ja_ind)++;


  return 0;

} /* whm_pop_in_job_info() */


/*
 * Skip the commentary character sequence that starts at c_ind.
 * A commentary is:
 * Everything from '#' till the end of line;
 * Everything from '//' till the end of line;
 * Everything in between a leading '\/\*' and a closing '\*\/'.
 (backslashes were added to be able to write the comment withtout terminating the one I'm writting right now.)
*/
int whm_skip_comments(char *content,
		      size_t content_s,
		      size_t *c_ind)          /* The first char of the commentary sequence. */
{
  size_t index = *c_ind;
  if (!content || content_s == 0 || *c_ind >= content_s){
    errno = EINVAL;
    return -1;
  }

  switch (content[index]){
  case '#':
    goto skip_to_eol;
    break;
  case '/':
    if (content[index+1] == '/'){
      index++;
      goto skip_to_eol;
    }
    else if(content[index+1] == '*'){
      index++;
      goto skip_multi_lines;
    }
    else {
      errno = WHM_INVALCOMMENT;
      return -1;
    }
    break;
  default:
    errno = WHM_INVALCOMMENT;
    return -1;
    break;
  }

 skip_to_eol:
  while(index < content_s) {
    if (content[index] == '\0'){
      errno = WHM_NOENDOFCOMMENT;
      return -1;
    }
    if (content[index] == NEWLINE) {
      *c_ind = index;
      return 0;
    }
    ++index;
  }
  
 skip_multi_lines:
  while (index < content_s){
    if (content[index] == '\0'){
      errno = WHM_NOENDOFCOMMENT;
      return -1;
    }
    if (content[index] == '*'){
      if (index < content_s-1 && content[index+1] != '\\') {
	if (content[index+1] == '/') {
	  *c_ind = index+1;
	  return 0;
	}
      }
    }
    ++index;
  }
  /* This should not happen: */
  errno = WHM_NOENDOFCOMMENT;
  return -1;
  
} /* whm_skip_comments() */


/* 
 * Check that the main configuration file who's name is given by 
 * the caller exists or else create it, read the configuration file
 * and return a whm_job_info_T object containing the parsed information
 * of the main configuration file.
 * first_free_element is a pointer since we want to update the caller's copy.

 * Note that this function is very sensitive to the order and positions of 
 * every characters from the main configuration file.
 */
whm_job_info_T** whm_read_main_config(const char *filename, size_t *ja_ind)
{
  char           main_config_content[WHM_MAX_MAINCONF_LENGHT];
  char           temp[WHM_MAX_PATHNAME_LENGHT];
  FILE           *main_config_stream = NULL;
  whm_job_info_T **job_array = NULL;                           /* Used with *ja_ind. */
  whm_stack_T    *mc_stack = NULL;                             /* main config stack. */
  size_t         content_ind = 0, temp_ind = 0, nl_count = 0, i = 0;
  int            numof_positions = 0, c = 0, numof_entries = 0, temp_status = 0;

  /* 
   * Push value to the given stack, then memset value. 
   * The caller MUST chech errno following a call to this MACRO. 
   */
#define WHM_PUSH_N_CLEAN(stack, value, val_ind, value_len) do {		\
    if (value[0] == '\0' || val_ind == 0) break; /* Do nothing if nothing's in value; */ \
    value[val_ind] = '\0';						\
    if (whm_push(stack, value) != 0) {					\
      WHM_ERRMESG("Whm_push");						\
      break;								\
    }									\
    val_ind = 0;							\
    memset(value, 0, value_len);					\
  } while(0);
  
  
  if (filename == NULL || filename[0] == '\0' || ja_ind == NULL){
    errno = EINVAL;
    return NULL;
  }
  if ((main_config_stream = fopen(filename, "r")) == NULL){
    if (whm_create_main_config(filename) != 0){
      perror("Whm_create_main_config");
      goto errjmp;
    }
    if ((main_config_stream = fopen(filename, "r")) == NULL){
      WHM_ERRMESG("Fopen");
      goto errjmp;
    }
  }
  memset(main_config_content, 0, WHM_MAX_MAINCONF_LENGHT);
  memset(temp, 0, WHM_MAX_PATHNAME_LENGHT);
  if ((mc_stack = whm_init_stack(WHM_MAX_ENTRIES)) == NULL){
    WHM_ERRMESG("Whm_init_stack");
    goto errjmp;
  }
  if ((job_array = malloc(WHM_MAX_ENTRIES * sizeof(whm_job_info_T *))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (i = 0; i < WHM_MAX_ENTRIES; i++)
    if ((job_array[i] = whm_init_job_info()) == NULL){
      WHM_ERRMESG("Whm_init_job_info");
      goto errjmp;
    }
  /* Read the file. */
  fread(main_config_content, sizeof(char), WHM_MAX_MAINCONF_LENGHT, main_config_stream);
  if (ferror(main_config_stream)){
    clearerr(main_config_stream);
    WHM_ERRMESG("Fread");
    goto errjmp;
  }

  /*
   * Each character found is being put into a temporary string 'temp'
   * that, once finding a newline, if there are characters in temp,
   * it is null terminated and pushed on the mc_stack.
   * When 2 consecutive newlines are found and the stack's tos value
   * is bigger than 0, meaning it's not empty, all values on the mc_stack
   * will be poped out in their respective whm_job_info_T fields.
   */
  for (content_ind = 0;
       content_ind < strlen(main_config_content) && main_config_content[content_ind] != '\0';
       content_ind++){
    /* Skip commentaries. */
    if (main_config_content[content_ind] == '/'){
      if (main_config_content[content_ind+1] == '/' || main_config_content[content_ind+1] == '*'){
	if (whm_skip_comments(main_config_content,
			      strlen(main_config_content),
			      &content_ind) != 0) {
	  WHM_ERRMESG("Whm_skip_comments");
	  goto errjmp;
	}
      }
      else {
	goto save_char;
      }
    }

    /* 
     * Position names and wages are handled a little differently, where
     * they are being pushed to the stack when finding a space character instead of a newline.
     */
    else if(main_config_content[content_ind] == SPACE
	    && (nl_count == WHM_MAINCONF_POSNAMES_LINE_NUM
		|| nl_count == WHM_MAINCONF_WAGES_LINE_NUM)) {
      /* Trim the dollar sign for all wages. */
      if (nl_count == WHM_MAINCONF_WAGES_LINE_NUM)
	temp[temp_ind-1] = '\0';
      if (++numof_entries == numof_positions){
	++nl_count;
	numof_entries = 0;
      }
      errno = 0;
      WHM_PUSH_N_CLEAN(mc_stack, temp, temp_ind, WHM_MAX_PATHNAME_LENGHT);
      if (errno){
	WHM_ERRMESG("Whm_push_n_clean");
	goto errjmp;
      }
    }    
    else if(main_config_content[content_ind] == NEWLINE) {
      if (main_config_content[content_ind+1] == NEWLINE) {
	/* Is there something in the stack? */
	if (mc_stack->tos == 0) continue;
	/* If there's something in the temporary string, push it now. */
	if (temp_ind > 0 && temp[0] != '\0'){
	  temp[temp_ind] = '\0';
	  errno = 0;
	  WHM_PUSH_N_CLEAN(mc_stack, temp, temp_ind, WHM_MAX_PATHNAME_LENGHT);
	  if (errno){
	    WHM_ERRMESG("Whm_push_n_clean");
	    goto errjmp;
	  }
	}
	/* Use _pop_in_job_info() to pop all stack's element into their respective fields. */
	if (whm_pop_in_job_info(job_array, ja_ind, mc_stack, &numof_positions) != 0) {
	  WHM_ERRMESG("Whm_pop_in_job_info");
	  goto errjmp;
	}
	nl_count = 0;
      }
      /* NULL terminate, push to mc_stack, then clean the temporary string and continue. */
      else {
	if (temp[0] == '\0' || temp_ind == 0)
	  continue;
	++nl_count;
	temp[temp_ind] = '\0';
	errno = 0;
	/* ->tos > 0 cause mc_stack->string[0] is the status field, also an integer. */
	if (isdigit(temp[0]) && mc_stack->tos > 0) numof_positions = atoi(temp);
	/*	else if (isdigit(temp[0]) && mc_stack->tos == 0)

	  if ((temp_status = atoi(temp)) == 0)
	    WHM_SKIP_CONFIG_ENTRY(main_config_content,
				  &content_ind,
				  strlen(main_config_content));*/
	WHM_PUSH_N_CLEAN(mc_stack, temp, temp_ind, WHM_MAX_PATHNAME_LENGHT);
	if (errno){
	  WHM_ERRMESG("Whm_push_n_clean");
	  goto errjmp;
	}
      }
    }
    /* Add the character to the temporary string and continue. */
    else {
    save_char:
      if (temp_ind >= WHM_MAX_PATHNAME_LENGHT){
	errno = WHM_INVALIDPUSH;
	goto errjmp;
      }
      temp[temp_ind++] = main_config_content[content_ind];
    }
    
  } /* For(conten_ind = 0...) */

  /* Cleanup being yourself! */
  if (main_config_stream){
    fclose(main_config_stream);
    main_config_stream = NULL;
  }
  if (mc_stack){
    whm_free_stack(mc_stack);
    mc_stack = NULL;
  }

  return job_array;
  
 errjmp:
  if (main_config_stream){
    fclose(main_config_stream);
    main_config_stream = NULL;
  }
  if (mc_stack){
    whm_free_stack(mc_stack);
    mc_stack = NULL;
  }
  if (job_array){
    for (i = 0; i < WHM_MAX_ENTRIES; i++)
      if (job_array[i]){
	whm_free_job_info(job_array[i]);
	job_array[i] = NULL;
      }
    free(job_array);
    job_array = NULL;
  }
  

  return NULL;

#undef WHM_PUSH_N_CLEAN
} /* whm_read_main_config() */


/* 
 * Create or overwrite the main configuration file who's name is
 * given by *filename, with values contained in the given **job_array object.
 */
int whm_write_main_config(const char *filename,
			  whm_job_info_T **job_array,
			  size_t *ja_ind)
{
  FILE *filestream = NULL;
  size_t i = 0, c = 0;

  if (!filename || !job_array || !ja_ind) {
    errno = EINVAL;
    return -1;
  }

  if ((filestream = fopen(filename, "w")) == NULL){
    WHM_ERRMESG("Fopen");
    return -1;
  }

  /* Print the header message (whm.h). */
  fprintf(filestream, "%s\n\n\n", WHM_MAIN_CONF_HEADER);
  /* Print the content of the given whm_job_info_T structure. */
  while (i < *ja_ind){
    fprintf(filestream, "%zu\n%s\n%s\n%zu\n",
	    job_array[i]->status,   job_array[i]->name,
	    job_array[i]->work_dir, job_array[i]->numof_positions);
    for(c = 0; c < job_array[i]->numof_positions; c++)
      fprintf(filestream, "%s ", job_array[i]->position_name[c]);
    fprintf(filestream, "\n");
    for (c = 0; c < job_array[i]->numof_positions; c++)
      fprintf(filestream, "%s$ ", job_array[i]->wage[c]);
    fprintf(filestream, "\n\n");
    ++i;
  }

  fclose(filestream);
  filestream = NULL;

  
  return 0;   

} /* whm_write_main_config() */
