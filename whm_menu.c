/*
 *
 * Work Hour Monitor  -  Main menu interactive utilities. 
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "sutils.h"
#include "whm.h"

extern whm_backup_T *to_write;

/* Display the main menu to stdout. */
void whm_print_main_menu(void)
{
  
  puts("\n\nWork Hour Monitor\nMenu principal:\n");
  puts("\t1 - Entrer le nombre d'heures travaillees aujourd'hui");
  puts("\t2 - Modifier un nombre d'heures entrees precedament");
  puts("\t3 - Recalculer les feuilles de temps");
  puts("\n\t4 - Afficher la feuille de temps d'une companie");
  puts("\t5 - Afficher le somaire mensuel d'une ou plusieurs companies");
  puts("\n\t6 - Afficher le fichier de configuration");
  puts("\t7 - Modifier le fichier de configuration");
  puts("\nEntrer \"fin\" ou \"cancel\" pour annulee la selection.");

} /* whm_print_main_menu() */

void whm_print_positions(whm_config_T *config,
			 whm_sheet_T *sheet)
{
  int i = 0;
  if (!config || !sheet) return;
  printf("%s: ", config->employer);
  do {
    printf("%s ", config->positions[i]);
  } while (++i < config->numof_positions);
  printf("\n");
} /* whm_print_positions() */
    

void whm_print_all_positions(whm_config_T **configs,
			     whm_sheet_T **sheets,
			     int max_ind)
{
  int i = 0, pos_ind = 0;
  
  do {
    if (!configs[i]) return;
    if (configs[i]->status)
      whm_print_positions(configs[i], sheets[i]);
  } while (++i < max_ind);

  return; 
} /* whm_print_company_positions() */


/* Modify the number of worked hours for a previous date. */
int whm_modify_previous_date(whm_config_T **configs,
			     whm_sheet_T **sheets,
			     whm_time_T *time_o,
			     int max_ind,
			     int *c_ind,
			     int *pos_ind)
{
  int i = 0, day_ind = 0, week_ind = 0;
  int date = 0, date_ind = 0;
  double worked_hours = 0;
  char answer[WHM_NAME_STR_S];  /* Initialized by whm_ask_user(). */
  char sheet_path[WHM_MAX_PATHNAME_S];
  char sheet_bup_path[WHM_MAX_PATHNAME_S];
  FILE *stream = NULL;


  if (!configs || !sheets || !time_o 
      || !max_ind || !c_ind  || !pos_ind){
    errno = EINVAL;
    return WHM_ERROR;
  }
  while(1){
    if (whm_ask_user(MODIF_COMPANY_NAME,
		     answer, WHM_NAME_STR_S,
		     NULL, 0) != 0){
      WHM_ERRMESG("Whm_ask_user");
      return WHM_ERROR;
    }
    if (s_strcmp(answer, "list", WHM_NAME_STR_S, LS_ICASE) == 0){
      whm_print_all_positions(configs, sheets, max_ind);
      continue;
    }
    if ((*c_ind = whm_validate_name(answer, configs, max_ind)) == WHM_ERROR){
      WHM_ERRMESG("Whm_validate_name");
      return WHM_ERROR;
    }
    else if (*c_ind == WHM_NOMATCH) {
      fputs("Invalid company name", stderr);
      continue;
    }
    break;
  }
  whm_print_positions(configs[*c_ind], sheets[*c_ind]);

  while(1) {
    if (whm_ask_user(MODIF_POSITION,
		     answer, WHM_NAME_STR_S,
		     NULL, 0) != 0){
      WHM_ERRMESG("Whm_ask_user");
      return WHM_ERROR;
    }
    if ((*pos_ind = whm_validate_position(answer, configs[*c_ind])) == WHM_ERROR){
      WHM_ERRMESG("Whm_validate_position");
      return WHM_ERROR;
    }
    else if (*pos_ind == WHM_NOMATCH) {
      fputs("Invalid position name", stderr);
      continue;
    }
    break;
  }
  
  if (whm_ask_user(MENU_DATE,
		   answer, WHM_NAME_STR_S,
		   NULL, 0) != 0) {
    WHM_ERRMESG("Whm_ask_user");
    return WHM_ERROR;
  }
  if (whm_adjust_time(answer, time_o, WHM_NAME_STR_S) != 0){
    WHM_ERRMESG("Whm_adjust_time");
    return WHM_ERROR;
  }

  if (whm_ask_user(SHEET_WORKED_HOURS,
		   answer, WHM_NAME_STR_S,
		   configs[*c_ind], *pos_ind) != 0){
    WHM_ERRMESG("Whm_ask_user");
    return WHM_ERROR;
  }
  worked_hours = atof(answer);

  /* Get the sheet's week_ind and day_ind from time_o's values. */
  for (; day_ind < 7; day_ind++)
    if (s_strcmp(WHM_EN_DAYS[day_ind], time_o->day, WHM_TIME_STR_S, 0) == 0) break;
  if (day_ind > 6) {
    errno = WHM_INVALIDELEMCOUNT;
    return WHM_ERROR;
  }
  date = atoi(time_o->date);
  for (int day_ind_cp = day_ind; date_ind != date; date_ind++){
    if(++day_ind_cp > 6) {
      day_ind_cp = 0;
      ++week_ind;
    }
  }
  if (week_ind > 5) {
    errno = WHM_INVALIDELEMCOUNT;
    return WHM_ERROR;
  }

  /* 
   * Because we're using a possibly modified time object,
   * we cannot use the global "to_write" list of sheets, as operation
   * on this list all use the original unmodified time object (created in main()).
   * Make a path for the sheet, open it, back it up, update it, write it to disk 
   * and remove the backup before returning successfuly.
   */
  if (whm_make_sheet_path(sheet_path, time_o, configs[*c_ind]) == NULL){
    WHM_ERRMESG("Whm_make_sheet_path");
    return WHM_ERROR;
  }
  if (whm_new_backup((const char*)sheet_path, sheet_bup_path) == NULL){
    WHM_ERRMESG("Whm_new_backup");
    return WHM_ERROR;
  }
  if (whm_read_sheet(sheet_path, configs[*c_ind],
		     time_o, sheets[*c_ind]) != 0){
    WHM_ERRMESG("Whm_read_sheet");
    return WHM_ERROR;
  }
  sheets[*c_ind]->week[week_ind]->day[day_ind]->pos_hours[*pos_ind] = worked_hours;
  if (whm_update_sheet(configs[*c_ind], sheets[*c_ind]) != 0){
    WHM_ERRMESG("Whm_update_sheet");
    return WHM_ERROR;
  }
  if ((stream = fopen(sheet_path, "w")) == NULL) {
    WHM_ERRMESG("Fopen");
    return WHM_ERROR;
  }
  if (whm_write_sheet(stream, configs[*c_ind],
		      time_o, sheets[*c_ind]) != 0) {
    WHM_ERRMESG("Whm_write_sheet");
    goto errjmp;
  }
  fclose(stream);
  stream = NULL;
  if (whm_rm_backup((const char*)sheet_bup_path) != 0){
    WHM_ERRMESG("Whm_rm_backup");
    return WHM_ERROR;
  }
  if (whm_reset_sheet(configs[*c_ind], sheets[*c_ind]) != 0){
    WHM_ERRMESG("Whm_reset_sheet");
    return WHM_ERROR;
  }
  
  return 0;

 errjmp:
  if (stream) {
    fclose(stream);
    stream = NULL;
  }
  return WHM_ERROR;
  

} /* whm_modify_previous_date() */


/* Used when no command line options are present. */
int whm_main_menu(whm_config_T **configs,
		  whm_sheet_T **sheets,
		  whm_time_T *time_o,
		  int *max_ind)
{
  char answer[WHM_NAME_STR_S]; /* whm_ask_user will initialize it. */
  int ret = 0;
  int c_ind = 0, pos_ind = 0;

  if (!configs || !sheets || !time_o || !max_ind || !*max_ind){
    errno = EINVAL;
    return WHM_ERROR;
  }

  while (1) {
    int choice = -1;
    
    whm_print_main_menu();
    if ((ret = whm_ask_user(MAIN_MENU_CHOICE,
			    answer, WHM_NAME_STR_S,
			    NULL, 0)) == WHM_ERROR){
      WHM_ERRMESG("Whm_ask_user");
      return WHM_ERROR;
    }
    else if (ret == WHM_CANCELED) break;
    choice = atoi(answer);
    switch (choice) {
      /* Input worked hours for today. */
    case 1:
      if (whm_inter_update_sheet(configs, sheets,
				 time_o, *max_ind) == WHM_ERROR){
	WHM_ERRMESG("Whm_inter_update_sheet");
	return WHM_ERROR;
      break;
      /* Modify worked hours of a past date. */
    case 2:
      if (whm_modify_previous_date(configs, sheets,
				   time_o, *max_ind,
				   &c_ind, &pos_ind) != 0){
	WHM_ERRMESG("Whm_modify_previous_date");
	return WHM_ERROR;
      }      
      break;
      }
      /* Recalculate the given company's yearly hour sheets. */
    case 3:
      /* open the given year's directory. */
      /* Read and update each sheets. */
      /* write updated sheets to disk. */
      /* Eventualy update the yearly summary (futur feature). */
      break;
    }

  }
    
  
  
  return 0;
  
} /* whm_main_menu() */
