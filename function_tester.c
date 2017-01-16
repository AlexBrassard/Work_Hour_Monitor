#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "whm.h"


int main (int argc, char **argv)
{

  whm_config_T **configs = NULL;
  int index = 0, i = 0;
  FILE *stream = NULL;
  whm_time_T *time_o = NULL;
  char pathname[WHM_MAX_PATHNAME_S];

  if ((time_o = whm_init_time_type()) == NULL){
    WHM_ERRMESG("Whm_init_time_type");
    return -1;
  }
  if ((configs = malloc(2 * sizeof(whm_config_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    return -1;
  }
  while (i < 2)
    if ((configs[i++] = whm_init_config_type()) == NULL){
      WHM_ERRMESG("Whm_init_config_type");
      return -1;
    }


  if ((stream = fopen(WHM_CONFIGURATION_FILE, "r")) == NULL){
    WHM_ERRMESG("Fopen");
    return -1;
  }
  if (whm_read_config(stream, &index, configs) != 0){
    WHM_ERRMESG("Whm_read_config");
    return -1;
  }
  fclose(stream);
  stream = NULL;
  if (whm_get_time(time_o) == -1){
    WHM_ERRMESG("Whm_get_time");
    return -1;
  }
  for (i = 0; i < 2; i++)
    printf("\nThis sheet's path: %s\n",
	   whm_make_sheet_path(pathname, time_o, configs[i]));
    
  
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

