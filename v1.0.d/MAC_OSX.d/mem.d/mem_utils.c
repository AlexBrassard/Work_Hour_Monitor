#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "../whm.h"
#include "../whm_errcodes.h"


/* Allocate memory for a whm_stack_T object. */
whm_stack_T* whm_init_stack(size_t size)
{
  whm_stack_T *to_init = NULL;
  size_t i = 0;
  
  if (size >= WHM_MAX_STACK_SIZE) size = WHM_MAX_STACK_SIZE-1;
  if ((to_init = malloc(sizeof(whm_stack_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((to_init->string = malloc(size * sizeof(char*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  while(i < size)
    if ((to_init->string[i++] = calloc(WHM_MAX_PATHNAME_LENGHT, sizeof(char))) == NULL){
      WHM_ERRMESG("Calloc");
      goto errjmp;
    }
  to_init->tos = 0;
  to_init->size = size;

  return to_init;

 errjmp:
  if (to_init){
    if (to_init->string){
      for(i = 0; i < size; i++){
	if (to_init->string[i]){
	  free(to_init->string[i]);
	  to_init->string[i] = NULL;
	}
      }
      free(to_init->string);
      to_init->string = NULL;
    }
    free(to_init);
    to_init = NULL;
  }
  return NULL;

} /* whm_init_stack() */

/* Release memory allocated to a whm_stack_T object. */
void whm_free_stack(whm_stack_T *stack)
{
  size_t i = 0;
  
  if (!stack) return;
  if (stack->string){
    for (i = 0; i < stack->size; i++){
      if (stack->string[i]){
	free(stack->string[i]);
	stack->string[i] = NULL;
      }
    }
    free(stack->string);
    stack->string = NULL;
  }
  free(stack);

} /* whm_free_stack() */


/* Allocate memory for a whm_job_info_T object. */
whm_job_info_T *whm_init_job_info(void)
{
  size_t i = 0;
  whm_job_info_T *to_init = NULL;

  if ((to_init = malloc(sizeof(whm_job_info_T))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  if ((to_init->name = calloc(WHM_MAX_CONF_NAME_LENGHT, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((to_init->work_dir = calloc(WHM_MAX_PATHNAME_LENGHT, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((to_init->position_name = malloc(WHM_MAX_NUMOF_POSITIONS * sizeof(char*))) == NULL) {
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  while (i < WHM_MAX_NUMOF_POSITIONS){
    if ((to_init->position_name[i] = calloc(WHM_MAX_CONF_NAME_LENGHT, sizeof(char))) == NULL){
      WHM_ERRMESG("Calloc");
      goto errjmp;
    }
    ++i;
  }
  if ((to_init->wage = malloc(WHM_MAX_NUMOF_POSITIONS * sizeof(char*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (i = 0; i < WHM_MAX_NUMOF_POSITIONS; i++)
    if ((to_init->wage[i] = calloc(WHM_MAX_CONF_NAME_LENGHT, sizeof(char))) == NULL){
      WHM_ERRMESG("Calloc");
      goto errjmp;
    }
  to_init->numof_positions = 0;
  to_init->status = 0;
  to_init->vacation_pay = 0;

  return to_init;


 errjmp:
  if (to_init){
    if (to_init->wage){
      for(i = 0; i < WHM_MAX_NUMOF_POSITIONS; i++)
	if (to_init->wage[i]){
	  free(to_init->wage[i]);
	  to_init->wage[i] = NULL;
	}
      free(to_init->wage);
      to_init->wage = NULL;
    }
    if (to_init->position_name){
      for (i = 0; i < WHM_MAX_NUMOF_POSITIONS; i++)
	if (to_init->position_name[i]){
	  free(to_init->position_name[i]);
	  to_init->position_name[i] = NULL;
	}
      free(to_init->position_name);
      to_init->position_name = NULL;
    }
    if (to_init->work_dir){
      free(to_init->work_dir);
      to_init->work_dir = NULL;
    }
    if (to_init->name){
      free(to_init->name);
      to_init->name = NULL;
    }
    free(to_init);
    to_init = NULL;
  }
  return NULL;


} /* whm_init_job_info() */
      

/* Release all memory reserved for a single whm_job_info_T object. */
void whm_free_job_info(whm_job_info_T *object)
{
  size_t i = 0;
  if (!object) return;
  if (object->wage){
    for(i = 0; i < WHM_MAX_NUMOF_POSITIONS; i++)
      if (object->wage[i]){
	free(object->wage[i]);
	object->wage[i] = NULL;
      }
    free(object->wage);
    object->wage = NULL;
  }
  if (object->position_name){
    for(i = 0; i < WHM_MAX_NUMOF_POSITIONS; i++)
      if (object->position_name[i]){
	free(object->position_name[i]);
	object->position_name[i] = NULL;
      }
    free(object->position_name);
    object->position_name = NULL;
  }
  if (object->work_dir) {
    free(object->work_dir);
    object->work_dir = NULL;
  }
  if (object->name) {
    free(object->name);
    object->name = NULL;
  }
  free(object);

} /* whm_free_job_info() */


/* 
 * Initialize a variable of type whm_time_T*
 * fetch the time and fill the time object before
 * returning it to the caller.

 * Some values unlikely to change have been hard coded in the function.
 */
whm_time_T* whm_fetch_time(void)
{
#define WHM_TEMP_STRING_LENGHT 256               /* 256: Just a random number, should be enough. */
  time_t     t;
  struct tm  *temp = NULL;
  whm_time_T *time_object = NULL;
  char       time_string[WHM_TIME_STRING_LENGHT];
  char       temp_string[WHM_TIME_O_STRING_LENGHT];    
  size_t i = 0, field_c = 0, ts_ind = 0;
  int month_num = 0;

  /* Allocate memory. */
  if ((time_object = malloc(sizeof(whm_time_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((time_object->day = calloc(WHM_TIME_O_STRING_LENGHT, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((time_object->month = calloc(WHM_TIME_O_STRING_LENGHT, sizeof(char))) == NULL) {
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((time_object->date = calloc(WHM_TIME_O_STRING_LENGHT, sizeof(char))) == NULL) {
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((time_object->year = calloc(WHM_TIME_O_STRING_LENGHT, sizeof(char))) == NULL) {
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((time_object->week = calloc(WHM_TIME_O_STRING_LENGHT, sizeof(char))) == NULL) {
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }

  /* Fetch the time. */
  if ((t = time(NULL)) == -1) {
    WHM_ERRMESG("Time");
    goto errjmp;
  }
  if ((temp = localtime(&t)) == NULL){
    WHM_ERRMESG("Localtime");
    goto errjmp;
  }
  /*
   * %a: The abreviated day name.           [0]
   * %d: The date from 1 to 31.             [1]
   * %m: The month number from 1 to 12      [2]
   * %Y: The year, including centuries.     [3]
   * %V: The ISO 8601 week number, 1 to 53. [4]
   */
  if (strftime(time_string, WHM_TIME_STRING_LENGHT-1, "%a %d %m %Y %V", temp) == 0){
    WHM_ERRMESG("strftime");
    goto errjmp;
  }

  memset(temp_string, 0, WHM_TIME_O_STRING_LENGHT);
  /* Parse the time string, fill and return the time_object. */
  for (i = 0; i <= strlen(time_string)+1; i++){
    /* NULL terminate the temporary string and fill the object when hitting a space character. */
    if (isspace(time_string[i]) || time_string[i] == '\0'){
      temp_string[ts_ind] = '\0';
      switch(field_c){
      case 0:
	if (s_strcpy(time_object->day, temp_string, WHM_TIME_O_STRING_LENGHT) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  goto errjmp;
	}
	break;
      case 1:
	if (s_strcpy(time_object->date, temp_string, WHM_TIME_O_STRING_LENGHT) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  goto errjmp;
	}
	break;
      case 2:
	month_num = atoi(temp_string);
	/* -1: WHM_FR_MONTH_NAMES starts at 0, month numbers at 1. */
	if (s_strcpy(time_object->month, WHM_FR_MONTH_NAMES[month_num-1], WHM_TIME_O_STRING_LENGHT) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  goto errjmp;
	}
	break;
      case 3:
	if (s_strcpy(time_object->year, temp_string, WHM_TIME_O_STRING_LENGHT) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  goto errjmp;
	}
	break;
	
      default:
	break;
      }

      if (field_c++ == 4){
	temp_string[ts_ind] = '\0';
	if (s_strcpy(time_object->week, temp_string, WHM_TIME_O_STRING_LENGHT) == NULL){
	  WHM_ERRMESG("S_strcpy");
	  goto errjmp;
	}
	break;
      }
      ts_ind = 0;
      memset(temp_string, 0, WHM_TIME_O_STRING_LENGHT);      
      continue;
    }
    temp_string[ts_ind++] = time_string[i];
  } 
  
  return time_object;

 errjmp:
  if (time_object){
    if (time_object->day){
      free(time_object->day);
      time_object->day = NULL;
    }
    if (time_object->month){
      free(time_object->month);
      time_object->month = NULL;
    }
    if (time_object->date){
      free(time_object->date);
      time_object->date = NULL;
    }
    if (time_object->year){
      free(time_object->year);
      time_object->year = NULL;
    }
    if (time_object->week){
      free(time_object->week);
      time_object->week = NULL;
    }
    free(time_object);
    time_object = NULL;
  }

  return NULL;
    
#undef WHM_TEMP_STRING_LENGHT
} /* whm_fetch_time() */


/* Release memory allocated to the given whm_time_T* object. */
void whm_free_time(whm_time_T *time_o)
{
  if (!time_o){
    errno = EINVAL;
    return;
  }
  if (time_o->week){
    free(time_o->week);
    time_o->week = NULL;
  }
  if (time_o->year){
    free(time_o->year);
    time_o->year = NULL;
  }
  if (time_o->date){
    free(time_o->date);
    time_o->date = NULL;
  }
  if (time_o->month){
    free(time_o->month);
    time_o->month = NULL;
  }
  if (time_o->day){
    free(time_o->day);
    time_o->day = NULL;
  }

  free(time_o);


  return;
} /* whm_free_time() */
    

/* 
 * Initialize a whm_day_T object, used to hold information
 * on a single day withing an hour sheet. 
 */
whm_day_T* whm_init_day_type(void)
{
  whm_day_T *new_day = NULL;

  if ((new_day = malloc(sizeof(whm_day_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((new_day->dayname = calloc(WHM_TIME_O_STRING_LENGHT, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((new_day->pos_hours = calloc(WHM_MAX_ENTRIES, sizeof(double))) == NULL) {
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  new_day->daynum = -1;
  new_day->total_hours = 0.0;
  new_day->total_cash = 0.0;

  return new_day;


 errjmp:
  if (new_day){
    if(new_day->pos_hours){
      free(new_day->pos_hours);
      new_day->pos_hours = NULL;
    }
    if (new_day->dayname){
      free(new_day->dayname);
      new_day->dayname = NULL;
    }
    free(new_day);
    new_day = NULL;
  }
  return NULL;
} /* whm_init_day_type() */


/* Release memory allocated to a whm_day_T object. */
void whm_free_day_type(whm_day_T *object)
{
  if (!object) return;
  if (object->dayname) {
    free(object->dayname);
    object->dayname = NULL;
  }
  if (object->pos_hours){
    free(object->pos_hours);
    object->pos_hours = NULL;
  }
  free(object);

} /* whm_free_day_type() */


/* 
 * Allocate memory to a whm_week_t structure. 
 * '7' is hard coded in both following functions cause
 * I'm pretty sure there will be no more than 
 * 7 days in a week.
 */
whm_week_T* whm_init_week_type(void)
{
  size_t i = 0;
  whm_week_T *new_week = NULL;

  if ((new_week = malloc(sizeof(whm_week_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((new_week->daily_info = malloc(7 * sizeof(whm_day_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  while (i < 7)
    if ((new_week->daily_info[i++] = whm_init_day_type()) == NULL){
      WHM_ERRMESG("Whm_init_day_type");
      goto errjmp;
    }
  if ((new_week->weekly_hours = calloc(WHM_MAX_NUMOF_POSITIONS, sizeof(double))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((new_week->weekly_cash = calloc(WHM_MAX_NUMOF_POSITIONS, sizeof(double))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
    
  new_week->week_number = 0;
  new_week->total_hours = 0.0;
  new_week->total_cash = 0.0;

  return new_week;

 errjmp:
  if (new_week){
    if (new_week->weekly_cash){
      free(new_week->weekly_cash);
      new_week->weekly_cash = NULL;
    }
    if (new_week->weekly_hours){
      free(new_week->weekly_hours);
      new_week->weekly_hours = NULL;
    }
    if (new_week->daily_info){
      for (i = 0; i < 7; i++)
	if (new_week->daily_info[i]){
	  whm_free_day_type(new_week->daily_info[i]);
	  new_week->daily_info[i] = NULL;
	}
      free(new_week->daily_info);
      new_week->daily_info = NULL;
    }
    free(new_week);
  }
  return NULL;
} /* whm_init_week_type() */


/* Release memory allocated to a whm_week_T object. */
void whm_free_week_type(whm_week_T *object)
{
  size_t i = 0;
  if (!object) return;
  if (object->weekly_cash){
    free(object->weekly_cash);
    object->weekly_cash = NULL;
  }
  if (object->weekly_hours){
    free(object->weekly_hours);
    object->weekly_hours = NULL;
  }
  if (object->daily_info){
    while (i < 7)
      if(object->daily_info[i]){
	whm_free_day_type(object->daily_info[i]);
	object->daily_info[i++] = NULL;
      }
    free(object->daily_info);
    object->daily_info = NULL;
  }
  free(object);
} /* whm_free_week_type() */


/* 
 * Allocate memory to a whm_month_T structure, holding
 * most informations contained within an hour sheet.
 */
whm_month_T* whm_init_month_type(void)
{
  size_t i = 0;
  whm_month_T *object = NULL;

  if ((object = malloc(sizeof(whm_month_T))) == NULL){
    WHM_ERRMESG("Malloc");
    return NULL;
  }
  if ((object->sheet_path = calloc(WHM_MAX_PATHNAME_LENGHT, sizeof(char))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  /* 6: exactly 6 weeks within an hour sheet. */
  if ((object->week = malloc(6 * sizeof(whm_week_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  while (i < 6)
    if ((object->week[i++] = whm_init_week_type()) == NULL){
      WHM_ERRMESG("Whm_init_week_type");
      goto errjmp;
    }
  if ((object->cumul_hours = calloc(8, sizeof(double))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  if ((object->cumul_cash = calloc(8, sizeof(double))) == NULL){
    WHM_ERRMESG("Calloc");
    goto errjmp;
  }
  
  if ((object->per_pos_hours = malloc(8 * sizeof(double*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (i = 0; i < 8; i++)
    if ((object->per_pos_hours[i] = calloc(WHM_MAX_NUMOF_POSITIONS, sizeof(double))) == NULL){
      WHM_ERRMESG("Calloc");
      goto errjmp;
    }
  if ((object->per_pos_cash = malloc(8 * sizeof(double*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (i = 0; i < 8; i++)
    if ((object->per_pos_cash[i] = calloc(WHM_MAX_NUMOF_POSITIONS, sizeof(double))) == NULL){
      WHM_ERRMESG("Calloc");
      goto errjmp;
    }

  return object;

 errjmp:
  if (object){
    if (object->per_pos_cash){
      for (i = 0; i < 8; i++)
	if (object->per_pos_cash[i]){
	  free(object->per_pos_cash[i]);
	  object->per_pos_cash[i] = NULL;
	}
      free(object->per_pos_cash);
      object->per_pos_cash = NULL;
    }
    
    if (object->per_pos_hours){
      for (i = 0; i < 8; i++)
	if (object->per_pos_hours[i]){
	  free(object->per_pos_hours[i]);
	  object->per_pos_hours[i] = NULL;
	}
      free(object->per_pos_hours);
      object->per_pos_hours = NULL;
    }
    if (object->cumul_cash){
      free(object->cumul_cash);
      object->cumul_cash = NULL;
    }
    if (object->cumul_hours) {
      free(object->cumul_hours);
      object->cumul_hours = NULL;
    }
    if (object->week)
      for (i = 0; i < 6; i++)
	if (object->week[i]){
	  whm_free_week_type(object->week[i]);
	  object->week[i] = NULL;
	}
    if (object->sheet_path){
      free(object->sheet_path);
      object->sheet_path = NULL;
    }
    free(object);
  }

  return NULL;
} /* whm_init_month_type() */


/* Release memory allocated to a whm_month_T object. */
void whm_free_month_type(whm_month_T *object)
{
  size_t i = 0;
  if (object){
    if (object->per_pos_cash){
      for (i = 0; i < 8; i++)
	if (object->per_pos_cash[i]){
	  free(object->per_pos_cash[i]);
	  object->per_pos_cash[i] = NULL;
	}
      free(object->per_pos_cash);
      object->per_pos_cash = NULL;
    }
    
    if (object->per_pos_hours){
      for (i = 0; i < 8; i++)
	if (object->per_pos_hours[i]){
	  free(object->per_pos_hours[i]);
	  object->per_pos_hours[i] = NULL;
	}
      free(object->per_pos_hours);
      object->per_pos_hours = NULL;
    }
    if (object->cumul_cash){
      free(object->cumul_cash);
      object->cumul_cash = NULL;
    }
    if (object->cumul_hours) {
      free(object->cumul_hours);
      object->cumul_hours = NULL;
    }

    if (object->week){
      for (i = 0; i < 6; i++)
	if (object->week[i]){
	  whm_free_week_type(object->week[i]);
	  object->week[i] = NULL;
	}
      free(object->week);
      object->week = NULL;
    }
    if (object->sheet_path){
      free(object->sheet_path);
      object->sheet_path = NULL;
    }
    free(object);
  }
  else return;
} /* whm_free_month_type() */
