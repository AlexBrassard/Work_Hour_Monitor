#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "whm.h"


int main (int argc, char **argv)
{

  FILE *stream = NULL;
  char *string = NULL;
  char *new_string = NULL;
  int i = 0, c = 0;


  if ((string = calloc(WHM_HOUR_SHEET_SIZE, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    return -1;
  }
  if ((new_string = calloc(WHM_HOUR_SHEET_SIZE, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    return -1;
  }

  if ((stream = fopen("/home/lappop/.whm.d/SLB.d/2017/SLB_Janvier.sheet", "r")) == NULL){
    WHM_ERRMESG("Fopen");
    return -1;
  }

  fread(string, sizeof(char), WHM_HOUR_SHEET_SIZE, stream);
  if (ferror(stream)) {
    WHM_ERRMESG("Fread");
    return -1;
  }

  while (string[i] != '\0') {
    if (string[i] == NUMBER) whm_skip_comments(string, &i, 0);
    else if (string[i] == SLASH)
      if (string[i+1] != '\0'){
	if (string[i+1] == SLASH) {whm_skip_comments(string, &i, 0);}
	else if (string[i+1] == STAR) {
	  if (whm_skip_comments(string, &i, 1)!=0){
	    WHM_ERRMESG("Whm_skip_comments");
	    return -1;
	  }
	}
      }
    new_string[c] = string[i];
    ++i;
    ++c;
  }
  printf ("string:\n%s\n\nnew_string:\n%s\n\n", string, new_string);

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





