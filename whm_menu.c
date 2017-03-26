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
  puts("\t3 - Recalculer les feuilles de temps d'une companie");
  puts("\t4 - Recalculer les feuilles de temps de toute les companies");
  puts("\n\t5 - Afficher la feuille de temps d'une companie");
  puts("\t6 - Afficher le somaire mensuel d'une ou plusieurs companies");
  puts("\n\t7 - Afficher le fichier de configuration");
  puts("\t8 - Modifier le fichier de configuration");
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
			     int max_ind)
{
  int i = 0, day_ind = 0, week_ind = 0;
  int c_ind = 0, pos_ind = 0;
  int date = 0, date_ind = 0;
  int fdom_ind = 0;
  double worked_hours = 0;
  char answer[WHM_NAME_STR_S];  /* Initialized by whm_ask_user(). */
  char sheet_path[WHM_MAX_PATHNAME_S];
  char sheet_bup_path[WHM_MAX_PATHNAME_S];
  FILE *stream = NULL;


  if (!configs || !sheets || !time_o || !max_ind ){
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
    if ((c_ind = whm_validate_name(answer, configs, max_ind)) == WHM_ERROR){
      WHM_ERRMESG("Whm_validate_name");
      return WHM_ERROR;
    }
    else if (c_ind == WHM_NOMATCH) {
      fputs("Invalid company name", stderr);
      continue;
    }
    break;
  }
  whm_print_positions(configs[c_ind], sheets[c_ind]);

  while(1) {
    if (whm_ask_user(MODIF_POSITION,
		     answer, WHM_NAME_STR_S,
		     NULL, 0) != 0){
      WHM_ERRMESG("Whm_ask_user");
      return WHM_ERROR;
    }
    if ((pos_ind = whm_validate_position(answer, configs[c_ind])) == WHM_ERROR){
      WHM_ERRMESG("Whm_validate_position");
      return WHM_ERROR;
    }
    else if (pos_ind == WHM_NOMATCH) {
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
  if ((sheets[c_ind]->time_o = whm_init_time_type()) == NULL){
    WHM_ERRMESG("Whm_init_time_type");
    return WHM_ERROR;
  }
  if (whm_copy_time(sheets[c_ind]->time_o, time_o) == NULL){
    WHM_ERRMESG("Whm_copy_time");
    return WHM_ERROR;
  }
  if (whm_adjust_time(answer, sheets[c_ind]->time_o, WHM_NAME_STR_S) != 0){
    WHM_ERRMESG("Whm_adjust_time");
    return WHM_ERROR;
  }

  if (whm_ask_user(SHEET_WORKED_HOURS,
		   answer, WHM_NAME_STR_S,
		   configs[c_ind], pos_ind) != 0){
    WHM_ERRMESG("Whm_ask_user");
    return WHM_ERROR;
  }
  worked_hours = atof(answer);

  /* Get the sheet's week_ind and day_ind from the sheet time_o's values. */
  for (; day_ind < 7; day_ind++)
    if (s_strcmp(WHM_EN_DAYS[day_ind], sheets[c_ind]->time_o->day, WHM_TIME_STR_S, 0) == 0) break;
  if (day_ind > 6) {
    errno = WHM_INVALIDELEMCOUNT;
    return WHM_ERROR;
  }
  date = atoi(sheets[c_ind]->time_o->date);
  fdom_ind = whm_find_first_dom(sheets[c_ind]->time_o, &i); /* i is just a placeholder here. */
  for (; date_ind != date; date_ind++){
    if(++fdom_ind > 6) {
      fdom_ind = 0;
      ++week_ind;
    }
  }
  if (week_ind > 5) {
    errno = WHM_INVALIDELEMCOUNT;
    return WHM_ERROR;
  }
  if (whm_make_sheet_path(sheet_path, sheets[c_ind]->time_o, configs[c_ind]) == NULL){
    WHM_ERRMESG("Whm_make_sheet_path");
    return WHM_ERROR;
  }
  if (whm_read_sheet(sheet_path, configs[c_ind],
		     sheets[c_ind]->time_o, sheets[c_ind]) != 0){
    WHM_ERRMESG("Whm_read_sheet");
    return WHM_ERROR;
  }
  
  sheets[c_ind]->week[week_ind]->day[day_ind]->pos_hours[pos_ind] = worked_hours;
  if (whm_update_sheet(configs[c_ind], sheets[c_ind], time_o) != 0){
    WHM_ERRMESG("Whm_update_sheet");
    return WHM_ERROR;
  }
  
  /* Add the sheet to the global list of sheets to be written to disk. */
  if (whm_set_sheet(configs[c_ind], sheets[c_ind]) != 0){
    WHM_ERRMESG("Whm_set_sheet");
    return WHM_ERROR;
  }

  return 0;

} /* whm_modify_previous_date() */


/* Recalculate each sheets of a single company for a given year. */
int whm_recalculate_year(whm_config_T **configs,
			 whm_time_T *time_o,
			 int max_ind,
			 char *name,
			 char *year)
{
  FILE *stream = NULL;
  char path[WHM_MAX_PATHNAME_S];
  int c_ind = -1, month_ind = 0;
  whm_sheet_T **sheets = NULL;
  whm_time_T *lo_time_o = NULL;


  if ((c_ind = whm_validate_name(name, configs, max_ind)) == WHM_ERROR) {
    WHM_ERRMESG("Whm_validate_name");
    return WHM_ERROR;
  }
  if ((lo_time_o = whm_init_time_type()) == NULL){
    WHM_ERRMESG("Whm_init_time_type");
    goto errjmp;
  }
  if (s_strcpy(lo_time_o->year, year, WHM_TIME_STR_S) == NULL){
    WHM_ERRMESG("S_strcpy");
    goto errjmp;
  }
  if ((sheets = malloc(12 * sizeof(whm_sheet_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (int i = 0; i < 12; i++)
    if ((sheets[i] = whm_init_sheet_type()) == NULL){
      WHM_ERRMESG("Whm_init_sheet_type");
      goto errjmp;
    }

  memset(path, '\0', WHM_MAX_PATHNAME_S);
  if (whm_make_year_path(configs[c_ind], lo_time_o, path) == NULL){
    WHM_ERRMESG("Whm_make_year_path");
    goto errjmp;
  }
  /* Make sure the directory exists. */
  if ((stream = fopen(path, "r")) == NULL) {
    WHM_ERRMESG("Fopen");
    goto errjmp;
  }
  fclose(stream);
  stream = NULL;

  /* Write any modified but not saved hour sheets to disk now. */
  if (whm_write_sheet_list(time_o) != 0){
    WHM_ERRMESG("Whm_write_sheet_list");
    goto errjmp;
  }
  
  for (month_ind = 0; month_ind < 12; month_ind++){
    if (!sheets[month_ind]) {
      errno = EINVAL;
      goto errjmp;
    }
    if ((sheets[month_ind]->time_o = whm_init_time_type()) == NULL){
      WHM_ERRMESG("Whm_init_time_type");
      goto errjmp;
    }
    if (whm_copy_time(sheets[month_ind]->time_o, time_o) == NULL){
      WHM_ERRMESG("Whm_copy_time");
      goto errjmp;
    }
    /* +1: arrays starts at 0, months at 1. */
    s_itoa(sheets[month_ind]->time_o->month, month_ind+1, WHM_TIME_STR_S);
    if (s_strcpy(sheets[month_ind]->time_o->year, year, WHM_TIME_STR_S) == NULL){
      WHM_ERRMESG("S_strcpy");
      goto errjmp;
    }
    if (whm_make_sheet_path(path, sheets[month_ind]->time_o, configs[c_ind]) == NULL){
      WHM_ERRMESG("Whm_make_sheet_path");
      goto errjmp;
    }
    if (whm_read_sheet(path, configs[c_ind],
		       sheets[month_ind]->time_o,
		       sheets[month_ind]) != 0){
      if (errno == ENOENT) continue;
      WHM_ERRMESG("Whm_read_sheet");
      goto errjmp;
    }
    if (whm_update_sheet(configs[c_ind], sheets[month_ind], time_o) != 0){
      WHM_ERRMESG("Whm_update_sheet");
      goto errjmp;
    }
    if (whm_set_sheet(configs[c_ind], sheets[month_ind]) != 0){
      WHM_ERRMESG("Whm_set_sheet");
      goto errjmp;
    }
  }

  if (whm_write_sheet_list(time_o) != 0){
    WHM_ERRMESG("Whm_write_sheet_list");
    goto errjmp;
  }

  if (sheets) {
    for (int i = 0; i < 12; i++)
      if(sheets[i]) {
	whm_free_sheet_type(sheets[i]);
	sheets[i] = NULL;
      }
    free(sheets);
    sheets = NULL;
  }
  if (lo_time_o){
    whm_free_time_type(lo_time_o);
    lo_time_o = NULL;
  }

  return 0;
    
 errjmp:
    if (sheets) {
    for (int i = 0; i < 12; i++)
      if(sheets[i]) {
	whm_free_sheet_type(sheets[i]);
	sheets[i] = NULL;
      }
    free(sheets);
    sheets = NULL;
  }
  if (lo_time_o){
    whm_free_time_type(lo_time_o);
    lo_time_o = NULL;
  }
  return WHM_ERROR;
  
} /* whm_recalculate_year() */


/* Recalculate all sheets of a given year for all active companies. */
int whm_recalculate_year_all(whm_config_T **configs,
			     whm_time_T *time_o,
			     int max_ind)
{
  int c_ind = 0;
  char name[WHM_NAME_STR_S]; /* So we're sure to avoid memory overlapping issues. */
  char year[WHM_TIME_STR_S];

  if (!configs || !time_o || !max_ind){
    errno = EINVAL;
    return WHM_ERROR;
  }

  if (whm_ask_user(MENU_YEAR,
		   year, WHM_TIME_STR_S,
		   NULL, 0) != 0){
    WHM_ERRMESG("Whm_ask_user");
    return WHM_ERROR;
  }

  for (; c_ind < max_ind; c_ind++){
    if (!configs[c_ind]->status) continue;
    if (s_strcpy(name, configs[c_ind]->employer, WHM_NAME_STR_S) == NULL){
      WHM_ERRMESG("S_strcpy");
      return WHM_ERROR;
    }
    if (whm_recalculate_year(configs, time_o,
			     max_ind, name, year) != 0){
      WHM_ERRMESG("Whm_recalculate_year");
      return WHM_ERROR;
    }
  }

  return 0;
  
} /* whm_recalculate_year_all() */


/* Used when no command line options are present. */
int whm_main_menu(whm_config_T **configs,
		  whm_sheet_T **sheets,
		  whm_time_T *time_o,
		  int *max_ind)
{
  char answer[WHM_NAME_STR_S]; /* whm_ask_user will initialize it. */
  char name[WHM_NAME_STR_S];
  char year[WHM_TIME_STR_S];
  char month[WHM_TIME_STR_S];
  whm_option_T *opt = NULL;
  int ret = 0, c_ind = 0;

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
    else if (ret == WHM_CANCELED || ret == WHM_INPUTDONE) break;
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
				   time_o, *max_ind) != 0){
	WHM_ERRMESG("Whm_modify_previous_date");
	return WHM_ERROR;
      }      
      break;
      }

      /* Recalculate the given company's yearly hour sheets. */
    case 3:
      if (whm_ask_user(MODIF_COMPANY_NAME,
		       name, WHM_NAME_STR_S,
		       NULL, 0) != 0){
	WHM_ERRMESG("Whm_ask_user");
	return WHM_ERROR;
      }
      if (whm_ask_user(MENU_YEAR,
		       year, WHM_TIME_STR_S,
		       NULL, 0) != 0){
	WHM_ERRMESG("Whm_ask_user");
	return WHM_ERROR;
      }
      if (whm_recalculate_year(configs, time_o, *max_ind, name, year) != 0){
	WHM_ERRMESG("Whm_recalculate_year");
	return WHM_ERROR;
      }
      break;

      /* Recalculate all sheets of all active companies for a given year. */
    case 4:
      if (whm_recalculate_year_all(configs, time_o, *max_ind) != 0){
	WHM_ERRMESG("Whm_recalculate_year_all");
	return WHM_ERROR;
      }
      break;

      /* Display on stdout the desired company hour sheet for a given year and month. */
    case 5:
      if ((opt = whm_init_option_type()) == NULL){
	WHM_ERRMESG("Whm_init_option_type");
	return WHM_ERROR;
      }
      while (1){
	if (whm_ask_user(MODIF_COMPANY_NAME,
			 name, WHM_NAME_STR_S,
			 NULL, 0) != 0) {
	  WHM_ERRMESG("Whm_ask_user");
	  goto errjmp;
	}
	if (s_strcmp(name, "list", WHM_NAME_STR_S, LS_ICASE) == 0){
	  whm_list_config_names(*max_ind, configs);
	  continue;
	}
	if ((c_ind = whm_validate_name(name, configs, *max_ind)) == WHM_ERROR){
	  WHM_ERRMESG("Whm_validate_name");
	  goto errjmp;
	}
	break;
      }
      if (whm_ask_user(MENU_YEAR,
		       year, WHM_TIME_STR_S,
		       NULL, 0) != 0){
	WHM_ERRMESG("Whm_ask_user");
	goto errjmp;
      }
      if (whm_ask_user(MENU_MONTH,
	 	       month, WHM_TIME_STR_S,
		       NULL, 0) != 0){
	WHM_ERRMESG("Whm_ask_user");
	goto errjmp;
      }
      opt->operation = PRINT;
      if (s_strcpy(opt->name, name, WHM_NAME_STR_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      if (s_strcpy(opt->month, month, WHM_TIME_STR_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      opt->year = atoi(year);
      if (whm_print_op(opt, configs, sheets, time_o, *max_ind) != 0){
	WHM_ERRMESG("Whm_print_op");
	goto errjmp;
      }
      break;
      
    }

  }

  if (opt) {
    whm_free_option_type(opt);
    opt = NULL;
  }
  return 0;

 errjmp:
  if (opt) {
    whm_free_option_type(opt);
    opt = NULL;
  }
  return WHM_ERROR;
  
} /* whm_main_menu() */
