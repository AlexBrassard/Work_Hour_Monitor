/*
 *
 * Work Hour Monitor  -  General utilities.
 *
 * Version: 0.01
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "whm.h"
#include "whm_error.h"
#include "sutils.h"


int whm_new_dir(const char *dir_name)
{
  FILE *test_stream = NULL;
  
  if (!dir_name) {
    errno = EINVAL;
    return -1;
  }
  /* Return an error if the directory already exists. */
  if ((test_stream = fopen(dir_name, "r")) != NULL){
    errno = WHM_DIREXIST;
    fclose(test_stream);
    test_stream = NULL;
    return -1;
  }
  if (mkdir(dir_name, WHM_DIRECTORY_PERMISSION) != 0){
    WHM_ERRMESG("Mkdir");
    return -1;
  }
  return 0;
  
} /* whm_new_dir() */
    
   
int whm_get_time(whm_time_T *time_o)
{
  time_t t;
  struct tm *temp = NULL;
  char temp_string[WHM_TIME_STR_S];
  char time_string[WHM_STRFTIME_STR_S];
  size_t i = 0, tmp = 0, time_str_len = 0, word_count = 0;
  
  if (!time_o){
    errno = EINVAL;
    return -1;
  }

  if ((t = time(NULL)) == -1){
    WHM_ERRMESG("Time");
    return -1;
  }
  if ((temp = localtime(&t)) == NULL){
    WHM_ERRMESG("Localtime");
    return -1;
  }
  /*
   * %a: The abreviated day name.           [0]
   * %d: The date from 1 to 31.             [1]
   * %V: The ISO 8601 week number, 1 to 53. [4]
   * %m: The month number from 1 to 12      [2]
   * %Y: The year, including centuries.     [3]
   */
  memset(time_string, '\0', WHM_STRFTIME_STR_S);
  errno = 0;
  if (strftime(time_string, WHM_STRFTIME_STR_S, "%a %d %V %m %Y", temp) == 0
      && errno){
    WHM_ERRMESG("Strftime");
    return -1;
  }
  
  /*
   * temp_string is ready to be copied to the appropriate field
   * of the given whm_time_T object when hitting a space within time_string.
   */
  time_str_len = strlen(time_string);
  while (i <= time_str_len) {
    if (time_string[i] == '\0' || time_string[i] == SPACE){
      temp_string[tmp] = '\0';
      switch(word_count) {
      case 0:
	if (s_strcpy(time_o->day, temp_string, WHM_TIME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  return -1;
	}
	break;
      case 1:
	if (s_strcpy(time_o->date, temp_string, WHM_TIME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  return -1;
	}
	break;
      case 2:
	if (s_strcpy(time_o->week, temp_string, WHM_TIME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  return -1;
	}
	break;
      case 3:
	if (s_strcpy(time_o->month, temp_string, WHM_TIME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  return -1;
	}
	break;
      case 4:
	if (s_strcpy(time_o->year, temp_string, WHM_TIME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  return -1;
	}
	break;
      default:
	errno = WHM_BADWORDCOUNT;
	return -1;
      }
      ++word_count;
      ++i;
      tmp = 0;
      if (time_string[i] == '\0' || word_count > 4) break;
      memset(temp_string, '\0', WHM_TIME_STR_S);
      continue;
    }
    temp_string[tmp] = time_string[i];
    ++i;
    ++tmp;
  }

  return 0;
} /* whm_get_time() */


int whm_clr_time(whm_time_T *time_o)
{
  if (!time_o) {
    errno = EINVAL;
    return -1;
  }
  memset(time_o, '\0', WHM_TIME_STR_S*5); /* 5: a whm_time_T has 5 fields of identical sizes. */
  return 0;
} /* whm_clr_time() */


char* whm_get_string(whm_queue_T *queue)
{
  char *temp;
  int i = 1;
  if (!queue) {
    errno = EINVAL;
    return NULL;
  }
  if (queue->index <= 0) {
    errno = WHM_EMPTYQUEUE;
    queue->is_empty = 1;
    return NULL;
  }

  if (queue->index == 1){
    queue->index--;
    queue->is_empty = 1;
    return queue->string[0];
  }

  temp = queue->string[0];
  while (i < queue->index){
    queue->string[i-1] = queue->string[i];
    i++;
  }    
  queue->string[queue->index-1] = temp;
  return queue->string[--(queue->index)];

} /* whm_get_string() */


int whm_set_string(whm_queue_T *queue, char *value)
{
  if (!queue || !value || value[0] == '\0') {
    errno = EINVAL;
    return -1;
  }

  if (queue->index > queue->top_index) {
    errno = WHM_FULLQUEUE;
    return -1;
  }

  if (s_strcpy(queue->string[queue->index], value, queue->string_lenght) == NULL){
    WHM_ERRMESG("S_strcpy");
    return -1;
  }
  queue->is_empty = 0;
  queue->index++;
  /*  Not sure the following line is good practice... */
  memset(value, '\0', strlen(value));

  return 0;

} /* whm_set_string() */


int whm_clr_string(whm_queue_T *queue, int index)
{
  if (!queue || index > queue->top_index) {
    errno = EINVAL;
    return -1;
  }

  memset(queue->string[index], '\0', queue->string_lenght);
  return 0;
} /* whm_clr_string() */


char* whm_create_backup(const char *filename, char *backupname)
{
  FILE *stream = NULL, *bstream = NULL;
  char *file_content = NULL;
  
  if (!filename || !backupname){
    errno = EINVAL;
    return NULL;
  }
  /* Check if the new filename fits in the program's limit and create it. */
  if ((strlen(filename) + strlen(WHM_BKUP_SUFFIX) + 1) > WHM_MAX_PATHNAME_S){
    errno = EOVERFLOW;
    return NULL;
  }
  /* Cast is to avoid compiler's warning about discarding const qualifier. */
  if (s_strcpy(backupname, (char*)filename, WHM_MAX_PATHNAME_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    return NULL;
  }
  strcat(backupname, WHM_BKUP_SUFFIX);
  if ((file_content = calloc(BUFSIZ, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    return NULL;
  }
  /* Open and read the file to backup, then write it in the new file. */
  if ((stream = fopen(filename, "r")) == NULL){
    WHM_ERRMESG("Fopen");
    goto errjmp;
  }
  errno = 0;
  fread(file_content, BUFSIZ-1, sizeof(char), stream);
  if (ferror(stream) && errno) {
    WHM_ERRMESG("Fread");
    goto errjmp;
  }
  if ((bstream = fopen(backupname, "w")) == NULL){
    WHM_ERRMESG("Fopen");
    goto errjmp;
  }
  if (fwrite(file_content, strlen(file_content), sizeof(char), bstream) == 0){
    WHM_ERRMESG("Fwrite");
    goto errjmp;
  }
  fclose(stream);
  stream = NULL;
  fclose(bstream);
  bstream = NULL;
  /* Make sure the backup is read-only. */
  if (chmod(backupname, 0400) != 0){
    WHM_ERRMESG("Chmod");
    goto errjmp;
  }

  free(file_content);
  file_content = NULL;
  return backupname;

 errjmp:
  
  if (file_content){
    free(file_content);
    file_content = NULL;
  }
  if (stream){
    fclose(stream);
    stream = NULL;
  }
  if (bstream){
    fclose(bstream);
    bstream = NULL;
  }
  return NULL;
} /* whm_create_backup() */


int whm_delete_backup(const char *filename)
{
  if (!filename){
    errno = EINVAL;
    return -1;
  }

  if (remove(filename) == -1){
    WHM_ERRMESG("Remove");
    return -1;
  }
  return 0;
} /* whm_delete_backup() */


int whm_ask_user(enum whm_question_type question,
		 char *answer,
		 size_t answer_s,
		 whm_config_T *config,
		 int pos_ind)
{
  size_t i = 0;
  
  if (!answer || question < 0 || !answer_s){
    errno = EINVAL;
    goto errjmp;
  }
  errno = 0;
#define WHM_ASK(ans) do {					\
    while (1){							\
      if (fgets(answer, answer_s, stdin) == NULL){		\
	WHM_ERRMESG("Fgets");					\
	goto errjmp;						\
      }								\
      if (answer[0] != '\n' && answer[0] != '\0') break;	\
    }								\
  } while (0);
  
  memset(answer, '\0', answer_s);
  switch(question){
    /* Building the configuration file. */
  case EMPLOYER:
    printf("\nEntrer le nom d'une companie: ");
    break;
    
  case POSITION:
    if (!config || !config->employer) {
      errno = EINVAL;
      goto errjmp;
    }
    printf("\nNommez un poste que vous occupez chez %s: ", config->employer);
    break;

  case POSITION2:
    if (!config || !config->employer){
      errno = EINVAL;
      goto errjmp;
    }
    printf("\nEntrez le nom d'un autre poste que vous occupez chez %s: ", config->employer);
    break;

  case WAGE:
    if (!config || !config->positions[pos_ind]) {
      errno = EINVAL;
      goto errjmp;
    }
    printf("Quel est votre taux horraire, par heure, pour le poste de %s?: ", config->positions[pos_ind]);
    break;

  case NIGHT_PRIME:
    printf("\nObtenez-vous une prime de nuit?\n-1 : Aucun quart de nuit.\n 0 : Aucune prime.\n 0+: Montant de la prime.\n   : ");
    break;

  case HOLIDAY_PAY:
    printf("\nObtenez-vous votre 4%% de vacance a chaque paie? [Oui/Non]: ");
    break;

  case ADD_COMPANY:
    printf("\nVoulez-vous ajouter une autre compagnie? [Oui/Non]: ");
    break;

    /* Configuration file entry modification. */
  case MODIF_COMPANY_NAME:
    printf("\nEntrez le nom de la compagnie a modifier.\n[Entrer \"list\" pour une liste d'entrees disponible]: ");
    break;

  case MODIF_CONFIG_FIELD:
    printf("\nQuel entree voulez-vous modifier?\n[Entrer \"list\" pour une liste d'entrees disponible]: ");
    break;

  case MODIF_UNKNOWN_COMPANY:
    printf("\nLa compagnie demandee n'existe pas,\nvoulez-vous cree une nouvelle entree pour celle-ci? [Oui/Non]: ");
    break;

  case MODIF_POSITION:
    printf("\nQuel position souhaitez-vous modifier?: ");
    break;

  case FIELD_STATUS:
    printf("\nEntrer le nouveau status [Actif/Inactif]: ");
    break;

  case FIELD_EMPLOYER:
    printf("\nEntrer le nouveau nom pour la compagnie %s [%d chars max]: ",
	   config->employer, WHM_NAME_STR_S-1);
    break;

  case FIELD_WAGE:
    printf("\nEntrer le nom du poste a modifier suivi d'une espace et du salaire, par heure, pour ce poste: ");
    break;

  case FIELD_POSITION:
    if (!config){
      errno = EINVAL;
      goto errjmp;
    }
    printf("\nPour ajouter un nouveau poste, entrer le nom du nouveau\
 poste chez %s,\nsuivi d'une espace et du salaire, par heure, pour celui-ci.\n\n",
	   config->employer);
    printf("Pour modifier le nom d'un poste existant chez %s,\nentrer le vieux nom du poste,\
 suivi d'une espace et du nouveau nom pour le poste.\n\n",
	   config->employer);
    printf("Pour supprimer un poste existant, entrer le nom du poste seulement.\n: ");
    break;

  case FIELD_NIGHT_PRIME:
    printf("\n-1 : Aucun quart de nuit.\n 0 : Aucune prime de nuit.\n 0+: Montant de la prime de nuit\n   : ");
    break;

  case FIELD_HOLIDAY_PAY:
    printf("\nRecevez-vous votre 4%% de vacance a chaque paie? [0: non/1: oui]: ");
    break;

  default:
    errno = WHM_BADQUESTION;
    goto errjmp;
  }
  WHM_ASK(answer);
  WHM_TRIM_NEWLINE(answer);
  WHM_REPLACE_SPACE(answer);
  while (i < WHM_NUMOF_EOI_STRINGS)
    if (strcmp(answer, WHM_END_OF_INPUT[i++]) == 0)
      return WHM_INPUTDONE;
  answer[answer_s-1] = '\0';
  return 0;

 errjmp:
  return -1;

#ifdef WHM_ASK
# undef WHM_ASK
#endif
} /* whm_ask_user() */


/*                                                                                                                                    
 * Verify or create a year directory for a given company name.                                                                        
 * The name of this directory is always the year in numeric format.                                                                   
 * Returns successfuly if the directory already exists.                                                                               
 */
int whm_new_year_dir(whm_config_T *config,
		     whm_time_T *time_o)
{
  FILE *stream = NULL;
  char path[WHM_MAX_PATHNAME_S];
  
  if (!config || !time_o) {
    errno = EINVAL;
    return -1;
  } 

  /* 
   * The directory path is always:
   * /program's working directory/Company_name.d/year\0
   * +5:                         ^            ^^^     ^
   */
  if ((strlen(WHM_WORKING_DIRECTORY) + strlen(config->employer)
       + strlen(time_o->year) + 5) > WHM_MAX_PATHNAME_S) {
    errno = EOVERFLOW;
    return -1;
  }
  if (s_strcpy(path, (char*)WHM_WORKING_DIRECTORY, WHM_MAX_PATHNAME_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    return -1;
  }
  strcat(path, "/");
  strcat(path, config->employer);
  strcat(path, ".d");
  strcat(path, "/");
  strcat(path, time_o->year);
  if ((stream = fopen(path, "r")) != NULL){
    fclose(stream);
    stream = NULL;
    return 0;
  }
  
  if (whm_new_dir((const char*)path) != 0
      && errno != WHM_DIREXIST){
    WHM_ERRMESG("Whm_new_dir");
    return -1;
  }

  return 0;
} /* whm_new_year_dir() */


/*
 * Find the first weekday of time_o's month,
 * and get the week number of the its first week 
 */
int whm_find_first_dom(whm_time_T *time_o,
		       int *week_num)
{
  size_t date = 0, day_ind = 0;
  if (!time_o || !week_num){
    errno = EINVAL;
    return -1;
  }
  
  /* Keep in mind that Sunday is day 0, Saturday is day 6. */
  date      = atoi(time_o->date);
  *week_num = atoi(time_o->week);
  
  /* Determine todays day name. */
  for (; day_ind < 6; day_ind++)
    if (strstr(WHM_EN_DAYS[day_ind], time_o->day) != NULL)
      break;

  /* There was a problem. */
  if (day_ind > 6) {
    errno = WHM_INVALIDELEMCOUNT;
    return -1;
  }
  
  /* Find the first day of month. */
  while (date-- > 1)
    if (day_ind-- == 0) {
      day_ind = 6;
      if ((*week_num)-- == 0){
	errno = WHM_INVALIDELEMCOUNT;
	return -1;
      }			       
    }

  return day_ind;
} /* whm_find_first_dom() */


/* 
 * Skip a commentary sequence, move *index to the next character
 * following the end of comment.
 *
 * A commentary may be one of these sequences:
 * From '#' till the end of line;                       Single line
 * from '//' till the end of line;                      Single line
 * from the leading '\/\*' till the trailing '\*\/'.    Multi-lines
 * An non-terminated multi-lines sequence is a fatal error.
 *
 * This function trusts its caller to detect what kind of comments
 * it is dealing with and pass this infomation along in the multi_lines
 * argument, which is bigger than 0 if the sequence is a multi-lines sequence,
 * <= 0 if it's a single line sequence.
 */
int whm_skip_comments(char *string,
		      int *ind,
		      int multi_lines)
{


  if (!string || !ind) {
    errno = EINVAL;
    return -1;
  }
  
  if (multi_lines > 0)
    /* Reaching end of string before end of comment can't be forgiven. */
    while (1) {
      if (string[*ind] == STAR) {
	if (string[(*ind)+1] != '\0' && string[(*ind)+1] != EOF){
	  if (string[(*ind)+1] == SLASH) {
	    (*ind) += 2;
	    break;
	  }
	}
	else {
	  goto invalid_comment;
	}
      }
      ++(*ind);
    }
  else
    while(string[*ind] != NEWLINE) {
      if (string[*ind] == '\0') break;
      ++(*ind);
    }

  return 0;

 invalid_comment:
  errno = WHM_INVALIDCOMMENT;
  return -1;


} /* whm_skip_comments() */
