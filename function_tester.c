#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "whm.h"


int main (int argc, char **argv)
{

  whm_time_T *time_o = NULL;
  whm_config_T **configs = NULL;
  whm_sheet_T **sheets = NULL;
  
  

  return 0;

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





