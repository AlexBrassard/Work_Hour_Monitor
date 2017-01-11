/*
 *
 * Work Hour Monitor main functions.
 *
 */
#define POSIX_SOURCE /* readdir_r */
#define _BSD_SOURCE  /* readdir_r */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "whm.h"



/* Skip all characters found till we reach a newline. 

   WHY am I not looking for '\0' ??? 
*/
#define WHM_SKIP_LINE(char_ind, buffer) do {		\
    while (buffer[char_ind++] != NEWLINE) continue;	\
  } while (0);

/* Print a pretty header to the main configuration file when creating it. */
#define WHM_PRINT_MAIN_CONFIG_HEADER(stream) do {			\
    fprintf(stream, "# Work Hours Monitor's main configuration file.\n\n"); \
  }while(0);

/*
 * Absolutely no verification is made by this MACRO. It MUST
 * be done prior to using it.
 * Glue a slash in between head and tail, store the whole in dest.
 */
#define WHM_MAKE_PATH(dest, head, tail) do{			\
    char temp[WHM_MAX_PATHNAME_LENGHT];				\
    memset(temp, 0, WHM_MAX_PATHNAME_LENGHT);			\
    if (dest == head || dest == tail) strcpy(temp, dest);	\
    memset(dest, 0, WHM_MAX_PATHNAME_LENGHT);			\
    if (temp[0] != '\0') strcpy(dest, temp);			\
    strcpy(dest, head);						\
    strcat(dest, "/");						\
    strcat(dest, tail);						\
    dest[strlen(dest)] = '\0';					\
  } while(0);

/*
 * Initialize all values of a whm_stack data type's char array to 0.
 * Must be fed the address of the stack.
 */
#define WHM_MEMSET_STACK(stack) do {					\
    size_t i = 0;							\
    while (i < WHM_MAX_ENTRIES) memset(stack->string[i++], 0, WHM_MAX_PATHNAME_LENGHT); \
  } while (0);

/* Initialize to 0 all arrays up to 1 less than the top of stack */
#define WHM_EMPTY_STACK(stack) do {					\
    size_t i = 0;							\
    while(i < stack->tos) memset(stack->string[i++], 0, WHM_MAX_PATHNAME_LENGHT); \
  } while (0);

/* Pass the address of the index of the character to start skipping from. */
#define WHM_SKIP_CONFIG_LINE(array, index) do {			\
    ++(*index);							\
    continue;							\
  } while (array[*index] != '\0' && array[*index] != NEWLINE);

/* Clear all field of a struct whm_job_info data type. */
#define WHM_CLEAR_JOB_INFO(object) do {					\
    size_t c = 0;							\
    memset(object.name, 0, WHM_MAX_PATHNAME_LENGHT);			\
    memset(object.work_dir, 0, WHM_MAX_PATHNAME_LENGHT);		\
    object.status = 0;							\
    while (c < WHM_MAX_NUMOF_WAGES) {					\
      memset(object.position_names[c], 0, WHM_MAX_NAME_LENGHT);		\
      object.wage[c] = 0;						\
      ++c;								\
    }									\
    object.numof_positions = 0;						\
    object.status = 0;							\
  } while(0);

whm_stack* whm_init_config_stack(void)
{
  whm_stack *to_init = NULL;
  size_t i = 0;
  if ((to_init = malloc(sizeof(whm_stack))) == NULL){
    perror("Malloc");
    return NULL;
  }
  if ((to_init->string = malloc(WHM_MAX_ENTRIES * sizeof(char*))) == NULL){
    perror("Malloc");
    goto errjmp;
  }
  for (i = 0; i < WHM_MAX_ENTRIES; i++)
    if ((to_init->string[i] = calloc(WHM_MAX_PATHNAME_LENGHT, sizeof(char))) == NULL){
      perror("Calloc");
      goto errjmp;
    }
  to_init->tos = 0;
  return to_init;

 errjmp:
  if (to_init){
    if (to_init->string){
      for(i = 0; i < WHM_MAX_ENTRIES; i++)
	if (to_init->string[i]){
	  free(to_init->string[i]);
	  to_init->string[i] = NULL;
	}
      free(to_init->string);
      to_init->string = NULL;
    }
    free(to_init);
    to_init = NULL;
  }
  return NULL;
} /* whm_init_config_stack() */

void whm_free_config_stack(whm_stack *stack)
{
  size_t i = 0;
  if (!stack) return;
  if (stack->string){
    for (i = 0; i < WHM_MAX_ENTRIES; i++)
      if (stack->string[i]){
	free(stack->string[i]);
	stack->string[i] = NULL;
      }
    free(stack->string);
    stack->string = NULL;
  }
  free(stack);
} /* whm_free_config_stack() */
  
/* Push a complete string to a stack if there's room and update the top of stack. */
int whm_push(whm_stack *stack, char *value)
{
  if (stack == NULL || value == NULL) {
    errno = EINVAL;
    return -1;
  }
  if (stack->tos >= WHM_MAX_ENTRIES){
    fprintf(stderr, "Config stack reached max capacity.\n");
    return -1;
  }
  if (strlen(value) >= WHM_MAX_PATHNAME_LENGHT) {
    fprintf(stderr, "Value too big for config stack");
    return -1;
  }

  strcpy(stack->string[(stack->tos)++], value);
  return 0;
}

char* whm_pop(whm_stack *stack)
{
  if (!stack) {
    errno = EINVAL;
    return NULL;
  }
  if (stack->tos > 0) --(stack->tos);
  if (stack->tos == 0 && (stack->string[0] == NULL || stack->string[0][0] == '\0')) return NULL;
  else if (stack->tos == 0) return stack->string[0];
  else return stack->string[stack->tos];
  
}

int whm_create_work_dir(void)
{
  if (mkdir(WHM_WORK_DIR, WHM_NEWDIR_PERM) != 0){
    perror("Mkdir");
    return -1;
  }
  return 0;
}

int whm_create_main_config(char *new_config_name)
{
  /* 
   * Double check if the main configuration file already exists.
   * It's name is in the global constant WHM_MAIN_CONF_NAME.
   * This function assumes the WHM_WORK_DIR has successfuly been open
   * and that the file stream is in the work_dir_stream argument.

   * This function is vulnerable to race conditions by its use of readdir.
   * Filenames longer than WHM_MAX_NAME_LENGHT will be silently discarded.
   * This program handles no more than 256 company entries.
   */
  struct dirent *entry = NULL, *res = NULL;
  FILE *file_stream = NULL;
  size_t i = 0, confname_len = 0, company_workdir_len = 0;
  size_t numof_positions = 0;
  char positions_names[WHM_MAX_NUMOF_WAGES][WHM_MAX_NAME_LENGHT];
  char company_name[WHM_MAX_NAME_LENGHT];
  char company_full_path[WHM_MAX_PATHNAME_LENGHT];
  char wage_string[WHM_WAGE_STRING_LENGHT]; 
  float wages[WHM_MAX_NUMOF_WAGES];
  

  if (!new_config_name){
    errno = EINVAL;
    return -1;
  }
  if ((entry = malloc(sizeof(struct dirent))) == NULL){
    perror("Malloc");
    return -1;
  }
  /* The file must be created only if it doesn't already exists. */
  if ((file_stream = fopen(new_config_name, "r")) != NULL){
    goto clean_exit;
  }
  /* Create the file. */
  if ((file_stream = fopen(new_config_name, "w")) == NULL){
    perror("Fopen");
    goto errjmp;
  }
  WHM_PRINT_MAIN_CONFIG_HEADER(file_stream);
  while (1){
    int temp = 0, c = 0;
    int default_company = 0;
    memset(company_name, 0, WHM_MAX_NAME_LENGHT);
    memset(company_full_path, 0, WHM_MAX_PATHNAME_LENGHT);
    numof_positions = 0;
    while (c < WHM_MAX_ENTRIES){
      memset(positions_names[c], 0, WHM_MAX_NAME_LENGHT);
      wages[c] = 0;
      ++c;
    }
    printf("Compagnie(s) a inclure dans le fichier de configuration (Entrer 'done' quand c'est terminer):\n");
    if (fgets(company_name, WHM_MAX_NAME_LENGHT-1, stdin) == NULL){
      perror("Fgets");
      goto errjmp;
    }
    for(i = 0; company_name[i] != '\0'; i++){
      if (company_name[i] == NEWLINE) company_name[i] = '\0';
      else if(isspace(company_name[i])) company_name[i] = '_';
    }
    if (strcmp(company_name, "done") == 0)
      break;
    printf("\n");
 
    /* +2: we're adding a slash '/' and a NULL byte. */
    if ((company_workdir_len =
	 (strlen(company_name) + strlen(WHM_WORK_DIR) + strlen(WHM_CONFIG_SUFX) + 2))
	>= WHM_MAX_PATHNAME_LENGHT){
      fprintf(stderr, "New company pathname too long\n");
      goto errjmp;
    }
    WHM_MAKE_PATH(company_full_path, WHM_WORK_DIR, company_name);
    strcat(company_full_path, WHM_CONFIG_SUFX);

    printf("\nEntrer le nom the votre(vos) position(s) chez %s, 'done' pour terminer:\n", company_name);
    while(numof_positions < WHM_MAX_NUMOF_WAGES){
      if (fgets(positions_names[numof_positions], WHM_MAX_NAME_LENGHT, stdin) == NULL) {
	perror("Fgets");
	goto errjmp;
      }
      for (i = 0; i < WHM_MAX_NAME_LENGHT && positions_names[numof_positions][i] != '\0'; i++)
	if (positions_names[numof_positions][i] == NEWLINE)
	  positions_names[numof_positions][i] = '\0';
      if (strcmp(positions_names[numof_positions], "done") == 0){
	break;
      }
      memset(wage_string, 0, WHM_WAGE_STRING_LENGHT);
      printf("\nQuel est votre salaire, par heure, pour le poste de %s chez %s?: ",
	     positions_names[numof_positions], company_name);
      if (fgets(wage_string, WHM_WAGE_STRING_LENGHT, stdin) == NULL){
	perror("Fgets");
	goto errjmp;
      }
      for (i = 0; i < WHM_WAGE_STRING_LENGHT; i++)
	if (wage_string[i] == NEWLINE) wage_string[i] = '\0';
      wages[numof_positions] = atof(wage_string);
      ++numof_positions;
    }
	   
    /* Set said companies to active (1) by default. */
    fprintf(file_stream, "%s\n%d\n%s\n\n\n", company_name, 1, company_full_path);
  }
 clean_exit:
  free(entry);
  entry = NULL;
  fclose(file_stream);

  return 0;
  
 errjmp:
  if (entry) {
    free(entry);
    entry = NULL;
  }
  if (file_stream)
    fclose(file_stream);
  
  return -1;
} /* whm_create_main_config() */


/** Main Program **/

int main (int argc, char **argv)
{
  whm_job_info job_info[WHM_MAX_ENTRIES];   /* Contains all information on each found active jobs. */
  whm_stack *main_config_stack;             /* MAKE A MACRO TO MEMSET ITS ARRAY */
  bool main_config_found = false;           /* True when a main configuration file is found. */
  char *main_config_content = NULL;
  char main_config_pathname[WHM_MAX_PATHNAME_LENGHT];
  char string_to_push[WHM_MAX_PATHNAME_LENGHT];
  FILE *main_config_stream = NULL;
  DIR *dir_stream = NULL;
  size_t i = 0, main_config_content_len = 0, stp_ind = 0;
  size_t job_info_ind = 0;                  /* Struct job_info's index. */
  struct dirent *res = NULL, *entry = NULL;

  
  if ((entry = malloc(sizeof(struct dirent) + WHM_MAX_NAME_LENGHT)) == NULL){
    perror("Malloc");
    return -1;
  }
  if ((main_config_content = calloc(WHM_MAX_MAINCONF_LENGHT, sizeof(char))) == NULL){
    perror("Calloc");
    goto errjmp;
  }
  if ((main_config_stack = whm_init_config_stack()) == NULL){
    perror("Whm_init_config_stack");
    goto errjmp;
  }
  /* Check if the WHM_WORK_DIR directory exists, if not create it. */
  if ((dir_stream = opendir(WHM_WORK_DIR)) == NULL){
    if (errno == ENOENT) {
      if (whm_create_work_dir() != 0){
	perror("Whm_create_work_dir");
	goto errjmp;
      }
      if ((dir_stream = opendir(WHM_WORK_DIR)) == NULL){
	perror("Opendir");
	goto errjmp;
      }
    }
    /* Another error code than ENOENT poped up. */
    else {
      perror("Opendir");
      goto errjmp;
    }
  }
  closedir(dir_stream);
  dir_stream = NULL;

  /* Concatenate the main configuration's filename out of constants from whm.h. */
  if (strlen(WHM_WORK_DIR) + strlen(WHM_MAIN_CONF_NAME) + 2 >= WHM_MAX_PATHNAME_LENGHT){
    fprintf(stderr, "Main configuration file's pathname too long.\n");
    goto errjmp;
  }
  WHM_MAKE_PATH(main_config_pathname, WHM_WORK_DIR, WHM_MAIN_CONF_NAME);
  /* If the main configuration file doesn't exists, create it. */
  if ((main_config_stream = fopen(main_config_pathname, "r")) == NULL){
    if (errno == ENOENT){
      if (whm_create_main_config(main_config_pathname) == -1){
	perror("Whm_create_main_config");
	goto errjmp;
      }
      if ((main_config_stream = fopen(main_config_pathname, "r")) == NULL){
	perror("Fopen");
	goto errjmp;
      }
    }
    else {
      perror("Fopen");
      goto errjmp;
    }
  }
  /* Read and parse the content of the main configuration file. */
  fread(main_config_content, sizeof(char), WHM_MAX_MAINCONF_LENGHT, main_config_stream);
  if (ferror(main_config_stream)){
    perror("Fread");
    goto errjmp;
  }
  main_config_content_len = strlen(main_config_content);
  memset(string_to_push, 0, WHM_MAX_PATHNAME_LENGHT);
  while(i < main_config_content_len && main_config_content[i] != '\0'){
    /* Skip comments. */
    if (main_config_content[i] == '#') WHM_SKIP_CONFIG_LINE(main_config_content, &i);
    if (main_config_content[i] == NEWLINE){
      /* 
       * When finding 2 consecutive newlines, fill in the first available
       * entry of the whm_job_info type array, this should free the main_config_stack
       * of any strings it may content and put back its ->tos at 0.
       * Get the next job entry from main_config_content.
       */
      if (i > 2 && main_config_content[i-1] == NEWLINE && main_config_content[i-2] == NEWLINE) {
	if (job_info_ind >= WHM_MAX_ENTRIES){
	  fprintf(stderr, "Too many entries to fit in the job info array.\n");
	  goto errjmp;
	}
	/* Fill in the structs job_info, we'll be working strickly on those from now one. */
	strcpy(job_info[job_info_ind].work_dir,whm_pop(main_config_stack));
	job_info[job_info_ind].status = atoi(whm_pop(main_config_stack));
	strcpy(job_info[job_info_ind].name, whm_pop(main_config_stack));
	/* 
	 * If the job status of the current entry isn't active, 
	 * clean up the entry we'll reuse it for an active one.
	 */
	if (job_info[job_info_ind].status == 0){
	  WHM_CLEAR_JOB_INFO(job_info[job_info_ind]);
	  ++i;
	  continue;
	}
	++job_info_ind;
	++i;
	continue;
      }
      if (stp_ind > 0){
	string_to_push[stp_ind] = '\0';
	if (strlen(string_to_push) >= WHM_MAX_PATHNAME_LENGHT){
	  fprintf(stderr, "Main configuration file: Entry too long\n");
	  goto errjmp;
	}
	if(whm_push(main_config_stack, string_to_push) == -1){
	  perror("Whm_push");
	  goto errjmp;
	}
	memset(string_to_push, 0, WHM_MAX_PATHNAME_LENGHT);
	stp_ind = 0;
	++i;
	continue;
      }
      else{
	++i;
	continue;
      }
    }
    string_to_push[stp_ind++] = main_config_content[i++];
  }
  if (argc > 1){
    /* Use Getops to handle options here. */
  }

  /* Cleanup. */
  if (entry) {
    free(entry);
    entry = NULL;
  }
  if (main_config_content){
    free(main_config_content);
    main_config_content = NULL;
  }
  if (main_config_stream)
    fclose(main_config_stream);
  if (main_config_stack)
    whm_free_config_stack(main_config_stack);
  
  return 0;

 errjmp:
  if (entry){
    free(entry);
    entry = NULL;
  }
  if (main_config_content){
    free(main_config_content);
    main_config_content = NULL;
  }
  if (main_config_stream)
    fclose(main_config_stream);
  if (main_config_stack)
    whm_free_config_stack(main_config_stack);
  if (dir_stream != NULL)
    closedir(dir_stream);
  return -1;
}
