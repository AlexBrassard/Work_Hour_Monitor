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
    return WHM_ERROR;
  }
  /* Return an error if the directory already exists. */
  if ((test_stream = fopen(dir_name, "r")) != NULL){
    errno = WHM_DIREXIST;
    fclose(test_stream);
    test_stream = NULL;
    return WHM_ERROR;
  }
  if (mkdir(dir_name, WHM_DIRECTORY_PERMISSION) != 0){
    WHM_ERRMESG("Mkdir");
    return WHM_ERROR;
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
    return WHM_ERROR;
  }

  if ((t = time(NULL)) == -1){
    WHM_ERRMESG("Time");
    return WHM_ERROR;
  }
  if ((temp = localtime(&t)) == NULL){
    WHM_ERRMESG("Localtime");
    return WHM_ERROR;
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
    return WHM_ERROR;
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
	  return WHM_ERROR;
	}
	break;
      case 1:
	if (s_strcpy(time_o->date, temp_string, WHM_TIME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  return WHM_ERROR;
	}
	break;
      case 2:
	if (s_strcpy(time_o->week, temp_string, WHM_TIME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  return WHM_ERROR;
	}
	break;
      case 3:
	if (s_strcpy(time_o->month, temp_string, WHM_TIME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  return WHM_ERROR;
	}
	break;
      case 4:
	if (s_strcpy(time_o->year, temp_string, WHM_TIME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  return WHM_ERROR;
	}
	break;
      default:
	errno = WHM_BADWORDCOUNT;
	return WHM_ERROR;
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
    return WHM_ERROR;
  }
  memset(time_o, '\0', WHM_TIME_STR_S*5); /* 5: a whm_time_T has 5 fields of identical sizes. */
  return 0;
} /* whm_clr_time() */


/* Copy src time object into dest time object. */
whm_time_T* whm_copy_time(whm_time_T *dest, whm_time_T *src)
{
  if (!src || !dest) {
    errno = EINVAL;
    return NULL;
  }

  if (s_strcpy(dest->day, src->day, WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    return NULL;
  }
  if (s_strcpy(dest->date, src->date, WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    return NULL;
  }
  if (s_strcpy(dest->week, src->week, WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    return NULL;
  }
  if (s_strcpy(dest->month, src->month, WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    return NULL;
  }
  if (s_strcpy(dest->year, src->year, WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    return NULL;
  }
  return dest;

} /* whm_copy_time() */


/* 
 * Adjust the given time object given a new time string
 * of the form dd-mm-yyyy .
 */
int whm_adjust_time(char *time_str,
		    whm_time_T *time_o,
		    size_t time_str_s)
{
  char **date = NULL;
  char strftime_str[WHM_TIME_STR_S];
  char temp_str[WHM_TIME_STR_S];
  int numof_monthly_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int year_day = 0;                /* (1-366) */
  int month = 0;                   /* (1-12)  */
  int day = 0;                     /* (1-31)  */
  int leap_year = 0, year = 0;
  int week_num = 0;
  int current_year = 0, current_month = 0, current_day = 0;
  int day_ind = 0;
  time_t t;
  struct tm *temp = NULL;
  whm_time_T *current_time = NULL;

  if ((current_time = whm_init_time_type()) == NULL){
    WHM_ERRMESG("Whm_init_time_type");
    return WHM_ERROR;
  }
  if (whm_get_time(current_time) == WHM_ERROR){
    WHM_ERRMESG("Whm_get_time");
    goto errjmp;
  }
  if ((date = malloc(3 * sizeof(char*))) == NULL) {
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (int i = 0; i < 3; i++)
    if ((date[i] = calloc(WHM_TIME_STR_S, sizeof(char))) == NULL){
      WHM_ERRMESG("Calloc");
      goto errjmp;
    }

  if (s_split(date, time_str, 3,
	      time_str_s, WHM_TIME_STR_S, '-') == NULL){
    WHM_ERRMESG("S_split");
    goto errjmp;
  }
  /* Date was given in the form: dd-mm-yyyy */
  if (s_strcpy(time_o->date, date[0], WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }
  if (s_strcpy(time_o->month, date[1], WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }
  if (s_strcpy(time_o->year, date[2], WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }
  /* This is ugly, the "field" argument should handle multiple flags. */
  if (!whm_validate_time_field(time_o, T_DATE)
      || !whm_validate_time_field(time_o, T_MONTH)
      || !whm_validate_time_field(time_o, T_YEAR)){
    WHM_ERRMESG("Whm_validate_time_field");
    goto errjmp;
  }
  t = time(NULL);
  temp = localtime(&t);
  if (!temp) {
    WHM_ERRMESG("Localtime");
    goto errjmp;
  }
  memset(strftime_str, '\0', WHM_TIME_STR_S);
  if (strftime(strftime_str, WHM_TIME_STR_S, "%j", temp) == 0){
    WHM_ERRMESG("Strftime");
    goto errjmp;
  }
  week_num = atoi(current_time->week);
  year_day = atoi(strftime_str);
  year = atoi(time_o->year);
  month = atoi(time_o->month);
  day = atoi(time_o->date);
  current_year = atoi(current_time->year);
  current_month = atoi(current_time->month);
  current_day = atoi(current_time->date);
  for (; day_ind < 7; day_ind++)
    if (s_strstr(WHM_EN_DAYS[day_ind], current_time->day,
		 WHM_TIME_STR_S,  LS_ICASE) == 0) break;
  if (day_ind > 6) {
    errno = WHM_INVALIDELEMCOUNT;
    goto errjmp;
  }

  /* 
   * Find time_o's ->day and ->week fields from its
   * ->date, ->month and ->year fields.
   */
  if (year > current_year) { /* Very likely the sheet doesn't exist yet.. */
    errno = WHM_INVALIDYEAR;
    goto errjmp;
  }
  if (month > current_month) {
    while(1) {
      if ((leap_year && year_day > 367) 
	  || (year_day > 366)) {
	++current_year;
	year_day = 1;
	if (s_isleap(current_year)) leap_year = 1;
	else leap_year = 0;
      }
      if (++day_ind > 6) {
	day_ind = 0;
	if (++week_num > 53) week_num = 0;
      }
      ++current_day;
      if (leap_year && current_month == 2){
	if (current_day >= (numof_monthly_days[current_month-1])+1) {
	  current_day = 0;
	  if (++current_month > 12) current_month = 1;
	}
      }
      else {
	if (current_day >= numof_monthly_days[current_month-1]) {
	  current_day = 0;
	  if (++current_month > 12) current_month = 1;
	}
      }

      if (year == current_year
	  && month == current_month
	  && day == current_day) break;
    }
  }
  else if (month == current_month) {
    if (day < current_day) {
      while (current_day != day) {
	if (--day_ind < 0){
	  day_ind = 6;
	  if (--week_num < 1) {
	    week_num = 53;
	    --current_year;
	  }
	}
	--current_day;
      }
    }
    else {
      while (current_day != day){
	if (++day_ind > 6) {
	  day_ind = 0;
	  if (++week_num > 53) {
	    week_num = 1;
	    ++current_year;
	  }
	}
	++current_day;
      }
    }
  }
  else {
    while(1) {
      if (--year_day == 0) {
	--current_year;
	if (s_isleap(current_year)) {
	  leap_year = 1;
	  year_day = 367;
	}
	else {
	  leap_year = 0;
	  year_day = 366;
	}
      }
      if (--day_ind < 0) {
	day_ind = 6;
	if (--week_num == 0) week_num = 53;
      }
      if (--current_day == 0) {
	--current_month;
	if (current_month <= 0) current_month = 12;
	if (current_month == 2 && leap_year)
	  current_day = numof_monthly_days[current_month-1]+1;
	else
	  current_day = numof_monthly_days[current_month-1];
      }
      if (year == current_year
	  && month == current_month
	  && day == current_day) break;
    }
  }
  if (s_strcpy(time_o->day, (char*)WHM_EN_DAYS[day_ind], WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }
  if (s_strcpy(time_o->week, s_itoa(temp_str, week_num, WHM_TIME_STR_S), WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }

  if (current_time){
    whm_free_time_type(current_time);
    current_time = NULL;
  }
  if (date){
    for (int i = 0; i < 3; i++)
      if (date[i]) {
	free(date[i]);
	date[i] = NULL;
      }
    free(date);
    date = NULL;
  }

  return 0;

 errjmp:
  if (current_time){
    whm_free_time_type(current_time);
    current_time = NULL;
  }
  if (date){
    for (int i = 0; i < 3; i++)
      if (date[i]) {
	free(date[i]);
	date[i] = NULL;
      }
    free(date);
    date = NULL;
  }
  return WHM_ERROR;
  
} /* whm_adjust_time() */


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
    return WHM_ERROR;
  }

  if (queue->index > queue->top_index) {
    errno = WHM_FULLQUEUE;
    return WHM_ERROR;
  }

  if (s_strcpy(queue->string[queue->index], value, queue->string_lenght) == NULL){
    WHM_ERRMESG("S_strcpy");
    return WHM_ERROR;
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
    return WHM_ERROR;
  }

  memset(queue->string[index], '\0', queue->string_lenght);
  return 0;
} /* whm_clr_string() */


char* whm_new_backup(const char *filename, char *backupname)
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
} /* whm_new_backup() */


int whm_rm_backup(const char *filename)
{
  if (!filename){
    errno = EINVAL;
    return WHM_ERROR;
  }

  if (remove(filename) == -1){
    WHM_ERRMESG("Remove");
    return WHM_ERROR;
  }
  return 0;
} /* whm_rm_backup() */


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

  case TIME_N_HALF:
    printf("\nRecevez-vous un bonus apres 40 heures (temps et demi)? [Oui/Non]: ");
    break;

  case DOUBLE_TIME:
    printf("\nRecevez-vous un bonus apres 50 heures (temps double)? [Oui/Non]: ");
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

  case SHEET_WORKED_HOURS:
    printf("\nCombien d'heures avez-vous travaillez pour le poste de %s chez %s?: ",
	   config->positions[pos_ind], config->employer);
    break;

  case MAIN_MENU_CHOICE:
    printf("Votre choix: ");
    break;

  case MENU_DATE:
    printf("Entrez la date a modifier (jj-mm-aaaa): ");
    break;

  case MENU_YEAR:
    printf("Entrez l'annee (aaaa): ");
    break;

  case MENU_MONTH:
    printf("Entrez le mois (1-12): ");
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
  /* 
   * "cancel" has a special meaning when whm_ask_user is 
   * used for menus questions, instead of quitting, the user must
   * be brought back to the previous menu or exit if called from the main menu.
   */
  if (s_strcmp(answer, "cancel", answer_s, LS_ICASE) == 0
      || s_strcmp(answer, "exit", answer_s, LS_ICASE) == 0){
    printf("\nCanceled\n\n");
    if (question >= MAIN_MENU_CHOICE) return WHM_CANCELED;
    exit(EXIT_SUCCESS);
  }    
  answer[answer_s-1] = '\0';
  return 0;

 errjmp:
  return WHM_ERROR;

#ifdef WHM_ASK
# undef WHM_ASK
#endif
} /* whm_ask_user() */

/* Makes a pathname for the given year's directory. */
char* whm_make_year_path(whm_config_T *config,
			 whm_time_T *time_o,
			 char *path)
{
  if (!config || !time_o || !path) {
    errno = EINVAL;
    return NULL;
  } 

  /* 
   * The directory path is always:
   * /program's working directory/Company_name.d/year\0
   * +5:                         ^            ^^^     ^
   */
  if ((strlen(WHM_WORKING_DIRECTORY) + strlen(config->employer)
       + strlen(time_o->year) + 5) > WHM_MAX_PATHNAME_S) {
    errno = EOVERFLOW;
    return NULL;
  }
  if (s_strcpy(path, (char*)WHM_WORKING_DIRECTORY, WHM_MAX_PATHNAME_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    return NULL;
  }
  strcat(path, "/");
  strcat(path, config->employer);
  strcat(path, ".d");
  strcat(path, "/");
  strcat(path, time_o->year);

  return path;

} /* whm_make_year_path() */


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

  if (!config || !time_o){
    errno = EINVAL;
    return WHM_ERROR;
  }
  
  if ((stream = fopen(config->working_directory, "r")) == NULL){
    if (whm_new_dir(config->working_directory) != 0){
      WHM_ERRMESG("Whm_new_dir");
      return WHM_ERROR;
    }
  }
  else {
    fclose(stream);
    stream = NULL;
  }
  
  memset(path, '\0', WHM_MAX_PATHNAME_S);
  if (whm_make_year_path(config, time_o, path) == NULL){
    WHM_ERRMESG("Whm_make_year_path");
    return WHM_ERROR;
  }
  if ((stream = fopen(path, "r")) != NULL){
    fclose(stream);
    stream = NULL;
    return 0;
  }
  
  if (whm_new_dir((const char*)path) != 0
      && errno != WHM_DIREXIST){
    WHM_ERRMESG("Whm_new_dir");
    return WHM_ERROR;
  }

  return 0;
} /* whm_new_year_dir() */


/*
 * Find the first weekday of time_o's month,
 * and get the week number of its first week.
 */
int whm_find_first_dom(whm_time_T *time_o,
		       int *week_num)
{
  size_t date = 0, day_ind = 0;
  if (!time_o || !week_num){
    errno = EINVAL;
    return WHM_ERROR;
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
    return WHM_ERROR;
  }
  
  /* Find the first day of month. */
  while (date-- > 1)
    if (day_ind-- == 0) {
      day_ind = 6;
      if ((*week_num)-- == 0){
	errno = WHM_INVALIDELEMCOUNT;
	return WHM_ERROR;
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
    return WHM_ERROR;
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
  return WHM_ERROR;


} /* whm_skip_comments() */


/* Records the positions and skip an hour sheet's commentaries. */
int whm_skip_sheet_comments(whm_sheet_T *sheet,
			   char *string,
			   int *ind,
			   int multi_lines)
{
  char *own_string = NULL;
  int offset_changed = 0;
  size_t string_s = strlen(string)+1;

  if ((own_string = calloc(string_s, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    return WHM_ERROR;
  }
  if (s_strcpy(own_string, string, string_s) == NULL){
    WHM_ERRMESG("S_strpy");
    goto errjmp;
  }
  /* Make sure there's enough room in the comment array else make some. */
  if (sheet->comment_ind >= (int)sheet->numof_comments)
    if (whm_extend_comment_arr(sheet->comments,&(sheet->numof_comments)) == NULL){
      WHM_ERRMESG("Whm_extend_comment_arr");
      goto errjmp;
    }
  /* *ind is the first character of the commentary sequence. */
  sheet->comments[sheet->comment_ind]->b_offset = *ind;  
  /* Skip the sequence to find the end of commentary offset. */
  if (whm_skip_comments(string, ind, multi_lines) != 0){
    WHM_ERRMESG("Whm_skip_comments");
    goto errjmp;
  }
  sheet->comments[sheet->comment_ind]->e_offset = *ind;
  /* Add the begining offset to our copy of string and a NULL byte at the ending offset. */
  own_string[sheet->comments[sheet->comment_ind]->e_offset] = '\0';
  own_string += sheet->comments[sheet->comment_ind]->b_offset;
  ++offset_changed;
  /* Make sure the resulting string fits in the sheet's commentary buffer else extend it. */
  if (strlen(own_string) < WHM_MAX_COMMENT_SIZE){
    while (strlen(own_string) >= sheet->comments[sheet->comment_ind]->text_s)
      if (whm_extend_comment_text(sheet->comments[sheet->comment_ind]) != 0){
	WHM_ERRMESG("Whm_extend_comment_text");
	goto errjmp;
      }
  }
  else {
    errno = WHM_COMMENTTOOLONG;
    goto errjmp;
  }
  /* Copy the resulting string into the sheet's commentary buffer and increment the sheet's comment index. */
  if (s_strcpy(sheet->comments[sheet->comment_ind]->text,
	       own_string,
	       sheet->comments[sheet->comment_ind]->text_s) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }

  if (own_string){
    if (offset_changed)
      own_string -= sheet->comments[sheet->comment_ind]->b_offset;
    free(own_string);
    own_string = NULL;
  }
  (sheet->comment_ind)++;
  return 0;

 errjmp:
  if (own_string){
    if (offset_changed)
      own_string -= sheet->comments[sheet->comment_ind]->b_offset;
    free(own_string);
    own_string = NULL;
  }
  return WHM_ERROR;

} /* whm_skip_sheet_comment() */
			   

/* 
 * Validate a given company name against entries in the configuration file. 
 * Returns -2 -1 or the index of the corresponding config object on error, 
 * failure to match and successful match respectively.
 */
int whm_validate_name(char *name,
		      whm_config_T **configs,
		      int c_ind)
{
  if (!name || name[0] == '\0'
      || !configs || !c_ind) {
    errno = EINVAL;
    return WHM_ERROR;
  }

  while(--c_ind >= 0)
    if (!configs[c_ind]) {
      errno = EINVAL;
      return WHM_ERROR;
    }
    else if (s_strcmp(configs[c_ind]->employer, name,
		      WHM_NAME_STR_S, LS_USPACE) == 0) return c_ind;
  
  return WHM_NOMATCH;
  
} /* whm_validate_name() */


/* 
 * Verify that the given position name is an actual position of
 * the given config->employer.
 * Returns WHM_ERROR, WHM_NOMATCH or the position's index on
 * error, failure to match and successful match respectively.
 */
int whm_validate_position(char *name,
			  whm_config_T *config)
{
  int pos_ind;
  
  if (!name || name[0] == '\0' || !config) {
    errno = EINVAL;
    return WHM_ERROR;
  }

  for (pos_ind = 0; pos_ind < config->numof_positions; pos_ind++)
    if(s_strcmp(config->positions[pos_ind], name, WHM_NAME_STR_S, LS_USPACE) == 0)
      return pos_ind;
  return WHM_NOMATCH;

} /* whm_validate_position() */
      

/* Validate the given field of the given time object. */
int whm_validate_time_field(whm_time_T *time_o,
			    enum whm_time_field_type field)
{
  int day_ind = 0;
  int do_match = 0, num = 0;


  
  if (!time_o) {
    errno = EINVAL;
    return WHM_ERROR;
  }

  switch (field){
  case T_DAY:
    while(day_ind < 7){
      if (s_strstr(WHM_EN_DAYS[day_ind], time_o->day, WHM_TIME_STR_S, LS_ICASE) >= 0){
	++do_match;
	break;
      }
      ++day_ind;
    }
    break;

  case T_DATE:
    if ((num = atoi(time_o->date) > 0) && num < 32)
      ++do_match;
    break;
    
  case T_WEEK:
    if ((num = atoi(time_o->week) > 0) && num < 54)
      ++do_match;
    break;

  case T_MONTH:
    if ((num = atoi(time_o->month) > 0) && num <= 12)
      ++do_match;
    break;

  case T_YEAR:
    if (atoi(time_o->year) > 0)
      ++do_match;
    break;

  default:
    errno = WHM_INVALIDFIELD;
    return WHM_ERROR;
  }

  if (do_match) return 1;
  return 0;

} /* whm_validate_time_field() */


/* Returns -1 on error, else returns the month number corresponding to the given name. */
int whm_get_month_number(char *month)
{
  int num = -1;
  if (!month){
    errno = EINVAL;
    return WHM_ERROR;
  }

  while(++num < 12)
    if ((s_strstr(WHM_FR_MONTHS[num], month, WHM_NAME_STR_S, LS_ICASE) >= 0)
	|| (s_strstr(WHM_EN_MONTHS[num], month, WHM_NAME_STR_S, LS_ICASE) >= 0))
      return (num+1);
  errno = WHM_INVALIDMONTH;
  return WHM_ERROR;

} /* whm_get_month_number() */
