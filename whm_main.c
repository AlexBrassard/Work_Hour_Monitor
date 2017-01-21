/*
 *
 * Work Hour Monitor  -  Main functions.
 *
 * Version:  1.01
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "whm.h"
#include "whm_error.h"


int main(int argc, char **argv)
{
  FILE *stream = NULL;
  whm_config_T **configs = NULL;
  whm_time_T *time_o = NULL;
  int i = 0, c_ind = 0;
  char config_bkup_name[WHM_MAX_PATHNAME_S];


  /* Initialize the array of configuration entries. */
  if ((configs = malloc(WHM_MAX_CONFIG_ENTRIES * sizeof(whm_config_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  while (i < WHM_MAX_CONFIG_ENTRIES) 
    if ((configs[i++] = whm_init_config_type()) == NULL){
      WHM_ERRMESG("Whm_init_config_type");
      goto errjmp;
    }
  /* Initialize and fill the whm_time_T time object. */
  if ((time_o = whm_init_time_type()) == NULL){
    WHM_ERRMESG("Whm_init_time_type");
    goto errjmp;
  }
  if (whm_get_time(time_o) != 0){
    WHM_ERRMESG("Whm_get_time");
    goto errjmp;
  }

  /* 
   * WHM_WORKING_DIRECTORY and WHM_CONFIGURATION_FILE are in whm.h 
   * If we can't open the program's working directory, create it.
   */
  if ((stream = fopen(WHM_WORKING_DIRECTORY, "r")) == NULL){
    if (whm_new_dir(WHM_WORKING_DIRECTORY) != 0){
      WHM_ERRMESG("Whm_new_dir");
      goto errjmp;
    }
  }
  else {
    fclose(stream);
    stream = NULL;
  }
  
  /* 
   * Read or create the configuration file.
   * In both cases, configs has c_ind-1 number of filled entries,
   * up to WHM_MAX_CONFIG_ENTRIES.
   */
  if ((stream = fopen(WHM_CONFIGURATION_FILE, "r")) == NULL){
  new_config:
    if (whm_new_config(WHM_CONFIGURATION_FILE, &c_ind, configs) != 0) {
      WHM_ERRMESG("Whm_new_config");
      goto errjmp;
    }
  }
  else {
    if (whm_read_config(stream, &c_ind, configs) != 0){
      WHM_ERRMESG("Whm_read_config");
      goto errjmp;
    }
  }
  fclose(stream);
  stream = NULL;
  /* 
   * If nothing was found in the configuration file,
   * warn the user that we're going to recreate it.
   */
  if (!c_ind) {
    fprintf(stderr, "%s: Warning, configuration file exists but contains no entries. Recreating it now\n\n",
	    WHM_PROGRAM_NAME);
    if (remove(WHM_CONFIGURATION_FILE) != 0){
      WHM_ERRMESG("Remove");
      goto errjmp;
    }
    goto new_config;
  }

  /* Make sure this year directory exists for each entries of the config file. */
  for (i = 0; i < c_ind; i++)
    if (whm_new_year_dir(configs[i], time_o) != 0) {
      WHM_ERRMESG("Whm_new_year_dir");
      goto errjmp;
    }

  if (argc > 1){ /* There might be options. */
    /* Use whm_parse_options() to execute options. */
    ;
  }
  /* If the program hasn't terminated yet, execute the automatic mode as well. */
  


  /* 
   * Before making any modifications to the configuration file, do a backup. 
   * This backup is removed only after the configuration file is written to disk.
   */
  if (whm_create_backup(WHM_CONFIGURATION_FILE,
			config_bkup_name) == NULL){
    WHM_ERRMESG("Whm_create_backup");
    goto errjmp;
  }

  /* Write the configuration file to disk. */
  if (whm_write_config(c_ind,
		       WHM_CONFIGURATION_FILE,
		       configs) == -1) {
    WHM_ERRMESG("Whm_write_config");
    goto errjmp;
  }

  /* Remove the configuration file's backup file. */
  if (whm_delete_backup(config_bkup_name) != 0){
    WHM_ERRMESG("Whm_remove_backup");
    goto errjmp;
  }
  
  /* Cleanup before exit. */
  if (configs){
    for (i = 0; i < WHM_MAX_CONFIG_ENTRIES; i++)
      if (configs[i]){
	whm_free_config_type(configs[i]);
	configs[i] = NULL;
      }
    free(configs);
    configs = NULL;
  }
  if (time_o){
    whm_free_time_type(time_o);
    time_o = NULL;
  }

  return 0;

 errjmp:
  if (stream){
    fclose(stream);
    stream = NULL;
  }
  if (configs){
    for (i = 0; i < WHM_MAX_CONFIG_ENTRIES; i++)
      if (configs[i]){
	whm_free_config_type(configs[i]);
	configs[i] = NULL;
      }
    free(configs);
    configs = NULL;
  }
  if (time_o){
    whm_free_time_type(time_o);
    time_o = NULL;
  }

  
  return -1;
}




/* Helper functions to be called only by, and within GDB. */
void whm_PRINT_config(whm_config_T *config)
{
  size_t i = 0;
  if (!config) return ;
  fprintf (stderr, "\nStatus: %zu\nEmployer: %s\nWork Dir: %s\nNumber of positions: %zu\n",
	   config->status, config->employer,
	   config->working_directory,
	   config->numof_positions);

  fprintf(stderr, "Positions Names: ");
  for (i = 0; i < config->numof_positions; i++)
    fprintf(stderr, "%s ", config->positions[i]);

  fprintf(stderr, "\nWages          : ");
  for (i = 0; i < config->numof_positions; i++)
    fprintf(stderr, "%.2f ", config->wages[i]);

  fprintf(stderr, "\nNight prime: %s\nDo pay holidays: %zu\n\n",
	  config->night_prime, config->do_pay_holiday);
}


/* print the content of a whm_sheet_T* object. */
void whm_PRINT_sheet(whm_sheet_T *sheet, whm_config_T *config)
{
  int i = 0, b = 0;
  size_t c = 0;

  if (!sheet || !config) return;

  fprintf(stderr, "\nCompany: %s  Path: %s  Year: %d  Month: %d\nCumulatives:\nHours:     ",
	  config->employer, sheet->path, sheet->year, sheet->month);
  for (i = 0; i < 7; i++){
    fprintf(stderr, "\nDay %d ", i);
    for(c = 0; c < config->numof_positions; c++)
      fprintf(stderr, "%f  ", sheet->day_pos_hours[i][c]);
  }
  fprintf(stderr, "\nEarnings: ");
  for (i = 0; i < 7; i++){
    fprintf(stderr, "\nDay %d ", i);
    for (c = 0; c < config->numof_positions; c++)
      fprintf(stderr, "%f  ", sheet->day_pos_earnings[i][c]);
  }

  for (i = 0; i < 6; i++){
    fprintf(stderr, "\n\nWeek Number: %zu  Total hours: %f  Total earnings: %f\nPer position hours:    ",
	    sheet->week[i]->week_number,
	    sheet->week[i]->total_hours,
	    sheet->week[i]->total_earnings);
    for (c = 0; c < config->numof_positions; c++)
      fprintf(stderr, "%f  ", sheet->week[i]->pos_total_hours[c]);
    fprintf(stderr, "\nPer position earnings: ");
    for (c = 0; c < config->numof_positions; c++)
      fprintf(stderr, "%f  ", sheet->week[i]->pos_total_earnings[c]);

    for (b = 0; b < 7; b++) {
      fprintf(stderr, "\nDate: %d  Total hours: %f  Total earnings: %f\nPer position hours:    ",
	      sheet->week[i]->day[b]->date,
	      sheet->week[i]->day[b]->total_hours,
	      sheet->week[i]->day[b]->total_earnings);
      for (c = 0; c < config->numof_positions; c++)
	fprintf (stderr, "%f  ", sheet->week[i]->day[b]->pos_hours[c]);
      fprintf(stderr, "\nPer position earnings: ");
      for (c = 0; c < config->numof_positions; c++)
	fprintf(stderr, "%f  ", sheet->week[i]->day[b]->pos_earnings[c]);
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
  }

  fprintf(stderr, "\n\n");

} /* whm_PRINT_sheet() */




