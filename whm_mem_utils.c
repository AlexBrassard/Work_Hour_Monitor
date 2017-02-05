/*
 *
 * Work Hour Monitor  -  Memory allocation and deallocation functions.
 *
 * Version: 0.01
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "whm.h"
#include "whm_error.h"


whm_time_T* whm_init_time_type(void)
{
  whm_time_T *time_o = NULL;

  if ((time_o = malloc(sizeof(whm_time_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  memset(time_o->date,  '\0', WHM_TIME_STR_S);
  memset(time_o->year,  '\0', WHM_TIME_STR_S);
  memset(time_o->week,  '\0', WHM_TIME_STR_S);
  memset(time_o->month, '\0', WHM_TIME_STR_S);

  return time_o;

} /* whm_init_time_type() */


void whm_free_time_type(whm_time_T *time_o)
{
  if (!time_o) return;

  memset(time_o->date,  '\0', WHM_TIME_STR_S);
  memset(time_o->year,  '\0', WHM_TIME_STR_S);
  memset(time_o->week,  '\0', WHM_TIME_STR_S);
  memset(time_o->month, '\0', WHM_TIME_STR_S);
  free(time_o);

} /* whm_free_time_type() */


whm_queue_T* whm_init_queue_type(int top_index, size_t elem_size)
{
  whm_queue_T *queue = NULL;
  int i = 0;
  
  if (!top_index || !elem_size){
    errno = EINVAL;
    return NULL;
  }
  if ((queue = malloc(sizeof(whm_queue_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((queue->string = malloc(sizeof(char*) * top_index)) == NULL) {
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (; i < top_index; i++)
    if ((queue->string[i] = calloc(elem_size, sizeof(char))) == NULL) {
      WHM_ERRMESG("Calloc");
      goto errjmp;
    }
  queue->index = 0;
  queue->top_index = top_index-1; /* -1: arrays start at 0. */
  queue->string_lenght = elem_size;
  queue->is_empty = 1;
  return queue;

 errjmp:
  if (queue){
    if (queue->string){
      for (i = 0; i < top_index; i++)
	if (queue->string[i]) {
	  free(queue->string[i]);
	  queue->string[i] = NULL;
	}
      free(queue->string);
      queue->string = NULL;
    }
    free(queue);
  }
  return NULL;

} /* whm_init_queue_type() */


void whm_free_queue_type(whm_queue_T *queue)
{
  int i = 0;
  
  if (!queue) return;
  if (queue->string){
    for (; i <= queue->top_index; i++)
      if (queue->string[i]) {
	free(queue->string[i]);
	queue->string[i] = NULL;
      }
    free(queue->string);
    queue->string = NULL;
  }
  free(queue);

} /* whm_free_queue_type() */


whm_config_T* whm_init_config_type(void)
{
  size_t i = 0;
  whm_config_T *config = NULL;

  if ((config = malloc(sizeof(whm_config_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((config->employer = calloc(WHM_NAME_STR_S, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((config->working_directory = calloc(WHM_MAX_PATHNAME_S, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((config->positions = malloc(WHM_DEF_NUMOF_POSITIONS * sizeof(char*))) == NULL) {
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (; i < WHM_DEF_NUMOF_POSITIONS; i++)
    if ((config->positions[i] = calloc(WHM_NAME_STR_S, sizeof(char))) == NULL){
      WHM_ERRMESG("Calloc");
      goto errjmp;
    }
  if ((config->wages = calloc(WHM_DEF_NUMOF_POSITIONS, sizeof(double))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  if ((config->night_prime = calloc(WHM_NAME_STR_S, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  config->status = 0;
  config->numof_positions = 0;
  config->do_pay_holiday = 0;
  return config;

 errjmp:
  if (config){
    if (config->night_prime){
      free(config->night_prime);
      config->night_prime = NULL;
    }
    if (config->wages){
      free(config->wages);
      config->wages = NULL;
    }
    if (config->positions){
      for (i = 0; i < WHM_DEF_NUMOF_POSITIONS; i++)
	if (config->positions[i]){
	  free(config->positions[i]);
	  config->positions[i] = NULL;
	}
      free(config->positions);
      config->positions = NULL;
    }
    if (config->working_directory){
      free(config->working_directory);
      config->working_directory = NULL;
    }
    if (config->employer){
      free(config->employer);
      config->employer = NULL;
    }
    free(config);
  }
  return NULL;

} /* whm_init_config_type() */


void whm_free_config_type(whm_config_T *config)
{
  size_t i = 0;
  
  if (!config) return;
  if (config->night_prime){
    free(config->night_prime);
    config->night_prime = NULL;
  }
  if (config->wages){
    free(config->wages);
    config->wages = NULL;
  }
  if (config->positions){
    for (i = 0; i < WHM_DEF_NUMOF_POSITIONS; i++)
      if (config->positions[i]){
	free(config->positions[i]);
	config->positions[i] = NULL;
      }
    free(config->positions);
    config->positions = NULL;
  }
  if (config->working_directory){
    free(config->working_directory);
    config->working_directory = NULL;
  }
  if (config->employer){
    free(config->employer);
    config->employer = NULL;
  }
  free(config);

} /* whm_free_config_type() */


whm_day_T* whm_init_day_type(void)
{
  whm_day_T *day = NULL;
  size_t i = 0;

  if ((day = malloc(sizeof(whm_day_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((day->pos_hours = malloc(WHM_DEF_NUMOF_POSITIONS * sizeof(double))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  if ((day->pos_earnings = malloc(WHM_DEF_NUMOF_POSITIONS * sizeof(double))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  while (i < WHM_DEF_NUMOF_POSITIONS){
    day->pos_hours[i]    = -1.0;
    day->pos_earnings[i] = -1.0;
    ++i;
  }
  day->date           = -1;
  day->total_hours    = -1.0;
  day->total_earnings = -1.0;
  return day;

 errjmp:
  if (day){
    if (day->pos_earnings){
      free(day->pos_earnings);
      day->pos_earnings = NULL;
    }
    if (day->pos_hours){
      free(day->pos_hours);
      day->pos_hours = NULL;
    }
    free(day);
    day = NULL;
  }
  return NULL;

} /* whm_init_day_type() */


void whm_free_day_type(whm_day_T *day)
{
  if (!day) return;
  if (day->pos_earnings){
    free(day->pos_earnings);
    day->pos_earnings = NULL;
  }
  if (day->pos_hours){
    free(day->pos_hours);
    day->pos_hours = NULL;
  }
  free(day);

} /* whm_free_day_type() */


whm_week_T* whm_init_week_type(void)
{
  whm_week_T *week = NULL;
  size_t i = 0;

  if ((week = malloc(sizeof(whm_week_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((week->pos_total_hours = malloc(WHM_DEF_NUMOF_POSITIONS * sizeof(double))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  if ((week->pos_total_earnings = malloc(WHM_DEF_NUMOF_POSITIONS * sizeof(double))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  if ((week->day = malloc(7 * sizeof(whm_day_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for(; i < 7; i++)
    if ((week->day[i] = whm_init_day_type()) == NULL){
      WHM_ERRMESG("Whm_init_day_type");
      goto errjmp;
    }
  for (i = 0; i < WHM_DEF_NUMOF_POSITIONS; i++){
    week->pos_total_hours[i] = -1.0;
    week->pos_total_earnings[i] = -1.0;
  }
    
  week->total_hours        = -1.0;
  week->total_earnings     = -1.0;
  return week;

 errjmp:
  if (week){
    if (week->day){
      for(i = 0; i < 7; i++)
	if (week->day[i]){
	  whm_free_day_type(week->day[i]);
	  week->day[i] = NULL;
	}
      free(week->day);
      week->day = NULL;
    }
    if (week->pos_total_hours) {
      free(week->pos_total_hours);
      week->pos_total_hours = NULL;
    }
    if (week->pos_total_earnings) {
      free(week->pos_total_earnings);
      week->pos_total_earnings = NULL;
    }
    free(week);
    week = NULL;
  }
  return NULL;

} /* whm_init_week_type() */


void whm_free_week_type(whm_week_T *week)
{
  size_t i = 0;
  if (!week) return;
  if (week->day){
    for(i = 0; i < 7; i++)
      if (week->day[i]){
	whm_free_day_type(week->day[i]);
	week->day[i] = NULL;
      }
    if (week->pos_total_hours) {
      free(week->pos_total_hours);
      week->pos_total_hours = NULL;
    }
    if (week->pos_total_earnings) {
      free(week->pos_total_earnings);
      week->pos_total_earnings = NULL;
    }
    free(week->day);
    week->day = NULL;
  }
  free(week);

} /* whm_free_week_type() */


whm_sheet_T* whm_init_sheet_type(void)
{
  size_t i = 0, c = 0;
  whm_sheet_T *sheet = NULL;

  if ((sheet = malloc(sizeof(whm_sheet_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((sheet->path = calloc(WHM_MAX_PATHNAME_S, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  /* 8: 1 for each week days + 1 for monthly totals. */
  if ((sheet->day_total_hours = malloc(8 * sizeof(double))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  if ((sheet->day_total_earnings = malloc(8 * sizeof(double))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  if ((sheet->day_pos_hours = malloc(8 * sizeof(double*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (; i < 8; i++)
    if ((sheet->day_pos_hours[i] = malloc(WHM_DEF_NUMOF_POSITIONS * sizeof(double))) == NULL){
      WHM_ERRMESG("Malloc");
      goto errjmp;
    }
  if ((sheet->day_pos_earnings = malloc(8 * sizeof(double*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (i = 0; i < 8; i++)
    if ((sheet->day_pos_earnings[i] = malloc(WHM_DEF_NUMOF_POSITIONS * sizeof(double))) == NULL){
      WHM_ERRMESG("Malloc");
      goto errjmp;
    }
  /* 6: 6 weeks in an hour sheet. */
  if ((sheet->week = malloc(6 * sizeof(whm_week_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (i = 0; i < 6; i++)
    if ((sheet->week[i] = whm_init_week_type()) == NULL){
      WHM_ERRMESG("Whm_init_week_type");
      goto errjmp;
    }
  for (c = 0; c < 8; c++){
    for (i = 0; i < WHM_DEF_NUMOF_POSITIONS; i++){
      sheet->day_pos_hours[c][i] = -1.0;
      sheet->day_pos_earnings[c][i] = -1.0;
    }
    sheet->day_total_hours[c] = -1.0;
    sheet->day_total_earnings[c] = -1.0;
  }
  sheet->year  = -1;
  sheet->month = -1;
  return sheet;

 errjmp:
  if (sheet){
    if (sheet->day_pos_hours){
      for(i = 0; i < 8; i++)
	if (sheet->day_pos_hours[i]){
	  free(sheet->day_pos_hours[i]);
	  sheet->day_pos_hours[i] = NULL;
	}
      free(sheet->day_pos_hours);
      sheet->day_pos_hours = NULL;
    }
    if (sheet->day_pos_earnings){
      for(i = 0; i < 8; i++)
	if (sheet->day_pos_earnings[i]){
	  free(sheet->day_pos_earnings[i]);
	  sheet->day_pos_earnings[i] = NULL;
	}
      free(sheet->day_pos_earnings);
      sheet->day_pos_earnings = NULL;
    }
    if (sheet->week){
      for (i = 0; i < 6; i++)
	if (sheet->week[i]){
	  whm_free_week_type(sheet->week[i]);
	  sheet->week[i] = NULL;
	}
      free(sheet->week);
      sheet->week = NULL;
    }
    if (sheet->day_total_hours){
      free(sheet->day_total_hours);
      sheet->day_total_hours = NULL;
    }
    if (sheet->day_total_earnings){
      free(sheet->day_total_earnings);
      sheet->day_total_earnings = NULL;
    }
    if (sheet->path){
      free(sheet->path);
      sheet->path = NULL;
    }
    free(sheet);
    sheet = NULL;
  }
  return NULL;

} /* whm_init_sheet_type() */


void whm_free_sheet_type(whm_sheet_T *sheet)
{
  size_t i = 0;

  if (!sheet) return;
  if (sheet->day_pos_hours){
    for(i = 0; i < 8; i++)
      if (sheet->day_pos_hours[i]){
	free(sheet->day_pos_hours[i]);
	sheet->day_pos_hours[i] = NULL;
      }
    free(sheet->day_pos_hours);
    sheet->day_pos_hours = NULL;
  }
  if (sheet->day_pos_earnings){
    for(i = 0; i < 8; i++)
      if (sheet->day_pos_earnings[i]){
	free(sheet->day_pos_earnings[i]);
	sheet->day_pos_earnings[i] = NULL;
      }
    free(sheet->day_pos_earnings);
    sheet->day_pos_earnings = NULL;
  }
  if (sheet->week){
    for (i = 0; i < 6; i++)
      if (sheet->week[i]){
	whm_free_week_type(sheet->week[i]);
	sheet->week[i] = NULL;
      }
    free(sheet->week);
    sheet->week = NULL;
  }
  if (sheet->day_total_hours){
    free(sheet->day_total_hours);
    sheet->day_total_hours = NULL;
  }
  if (sheet->day_total_earnings){
    free(sheet->day_total_earnings);
      sheet->day_total_earnings = NULL;
  }
  if (sheet->path) {
    free(sheet->path);
    sheet->path = NULL;
  }
  free(sheet);

} /* whm_free_sheet_type() */


whm_option_T *whm_init_option_type()
{
  whm_option_T *option = NULL;

  if ((option = malloc(sizeof(whm_option_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((option->name = calloc(WHM_NAME_STR_S, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((option->position = calloc(WHM_NAME_STR_S, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  option->operation    = NONE;
  option->year         = -1;
  option->month        = -1;
  option->date         = -1;
  option->worked_hours = -1.0;
  return option;

 errjmp:
  if (option){
    if (option->position){
      free(option->position);
      option->position = NULL;
    }
    if (option->name){
      free(option->name);
      option->name = NULL;
    }
    free(option);
    option = NULL;
  }
  return NULL;
} /* whm_init_option_type() */


void whm_free_option_type(whm_option_T *option){
  if (!option) return;
  if (option->position){
    free(option->position);
    option->position = NULL;
  }
  if (option->name){
    free(option->name);
    option->name = NULL;
  }
  free(option);
} /* whm_free_option_type() */


/* Allocate memory to a whm_backup_T* object. */
whm_backup_T* whm_init_backup_type(void)
{
  whm_backup_T *bup = NULL;
  size_t ind = 0;

  if ((bup = malloc(sizeof(whm_backup_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((bup->sheets = malloc(WHM_MAX_CONFIG_ENTRIES * sizeof(whm_sheet_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  if ((bup->configs = malloc(WHM_MAX_CONFIG_ENTRIES * sizeof(whm_config_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  if ((bup->filename = malloc(WHM_MAX_CONFIG_ENTRIES * sizeof(char*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for(; ind < WHM_MAX_CONFIG_ENTRIES; ind++)
    if ((bup->filename[ind] = calloc(WHM_MAX_PATHNAME_S, sizeof(char))) == NULL){
      WHM_ERRMESG("Calloc");
      goto errjmp;
    }
  bup->c_ind = 0;
  bup->size = WHM_MAX_CONFIG_ENTRIES;

  return bup;

 errjmp:
  if (bup) {
     if (bup->filename){
      for (ind = 0; ind < WHM_MAX_CONFIG_ENTRIES; ind++)
	if (bup->filename[ind]) {
	  free(bup->filename[ind]);
	  bup->filename[ind] = NULL;
	}
      free(bup->filename);
      bup->filename = NULL;
    }
    if (bup->configs) {
      free(bup->configs);
      bup->configs = NULL;
    }
    if (bup->sheets) {
      free(bup->sheets);
      bup->sheets = NULL;
    }
    free(bup);
  }
  return NULL;
    

} /* whm_init_backup_type() */


/* Release memory allocated to a whm_backup_T* object. */
void whm_free_backup_type(whm_backup_T *bup)
{
  int i = 0;
  
  if (!bup) return;

  if (bup->filename){
    for (i = 0; i < bup->size; i++)
      if (bup->filename[i]) {
	free(bup->filename[i]);
	bup->filename[i] = NULL;
      }
    free(bup->filename);
    bup->filename = NULL;
  }
  if (bup->configs) {
    free(bup->configs);
    bup->configs = NULL;
  }
  if (bup->sheets) {
    free(bup->sheets);
    bup->sheets = NULL;
  }
  free(bup);
  bup = NULL;

} /* whm_free_backup_type() */
