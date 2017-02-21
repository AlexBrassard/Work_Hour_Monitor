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
#include <ctype.h>

#include "whm.h"
#include "whm_error.h"

/* 
 * List of sheet_T and config_T objects that have
 * been updated and are ready to be written to disk,
 * including their backup absolute pathname.
 */
whm_backup_T *to_write;



/* All options operations functions are located after main(), near the bottom of this file. */


/*
 * Get the arguments corresponding to an option. 
 * Note: Each _op functions is responsible of making sure it 
 * got the right ammount of expected and required arguments. 
 * The _op functions in question are at near the bottom of this file.
 */
int whm_get_args(int argc, char **argv,
		  int *arg_ind,
		  whm_option_T **options,
		  int *opt_ind)
{
  int arg_name = 0, i = 0;
  char **splitted_arg = NULL;

  if ((splitted_arg = malloc(2 * sizeof(char*))) == NULL){
    WHM_ERRMESG("Malloc");
    return WHM_ERROR;
  }
  for (;i < 2; i++)
    if ((splitted_arg[i] = calloc(WHM_MAX_ARG_STR_S, sizeof(char))) == NULL){
      WHM_ERRMESG("Calloc");
      goto errjmp;
    }
  for (; *arg_ind < argc; (*arg_ind)++) {
    if (argv[*arg_ind][0] == DASH) {
      goto success;
    }
    else
      /* The first string is the argument name, the second is the value. */
      if (s_split(splitted_arg, argv[*arg_ind],
		  2, strlen(argv[*arg_ind]),
		  WHM_MAX_ARG_STR_S, '=') == NULL){
	WHM_ERRMESG("S_split");
	goto errjmp;
      }
    /* Make sure a value was passed via command line. */
    if (splitted_arg[1][0] == '\0') {
      errno = WHM_INVALIDARGVALUE;
      goto errjmp;
    }

    /* Check if the argument name is valid and store it appropriately. */
    for (arg_name = 0; arg_name < WHM_MAX_NUMOF_ARG_NAMES; arg_name++){
      if(s_strcmp(splitted_arg[0], WHM_ARG_NAMES[arg_name],
		  strlen(splitted_arg[0]), 0) == 0){

	switch(arg_name){
	case OPT_NAME:
	  if (s_strcpy(options[*opt_ind]->name, splitted_arg[1], WHM_NAME_STR_S) == NULL){
	    WHM_ERRMESG("S_strcpy");
	    goto errjmp;
	  }
	  break;
	case OPT_POSITION:
	  if (s_strcpy(options[*opt_ind]->position, splitted_arg[1], WHM_NAME_STR_S) == NULL){
	    WHM_ERRMESG("S_strcpy");
	    goto errjmp;
	  }
	  break;
	case OPT_TYPE:
	  options[*opt_ind]->type = atoi(splitted_arg[1]);
	  break;
	case OPT_YEAR:
	  options[*opt_ind]->year = atoi(splitted_arg[1]);
	  break;
	case OPT_MONTH:
	  if (s_strcpy(options[*opt_ind]->month, splitted_arg[1], WHM_NAME_STR_S) == NULL){
	    WHM_ERRMESG("S_strcpy");
	    goto errjmp;
	  }
	  break;
	case OPT_DATE:
	  if (s_strcpy(options[*opt_ind]->date, splitted_arg[1], WHM_NAME_STR_S) == NULL){
	    WHM_ERRMESG("S_strcpy");
	    goto errjmp;
	  }
	  break;
	case OPT_HOURS:
	  options[*opt_ind]->worked_hours = atof(splitted_arg[1]);
	  break;
	case OPT_FIELD:
	  options[*opt_ind]->field = atoi(splitted_arg[1]);
	  break;
	case OPT_VALUE:
	  if (s_strcpy(options[*opt_ind]->value, splitted_arg[1], WHM_VALUE_S) == NULL){
	    WHM_ERRMESG("S_strcpy");
	    goto errjmp;
	  }
	  break;
	  
	  /* Setting an abort call here as it should not ever happen. */
	default: 
	  errno = WHM_INVALIDARGNAME;
	  abort(); 
	}
	break;
      }
    }
  }

 success:
  if (splitted_arg){
    for (i = 0; i < 2; i++)
      if (splitted_arg[i]){
	free(splitted_arg[i]);
	splitted_arg[i] = NULL;
      }
    free(splitted_arg);
    splitted_arg = NULL;
  }
  ++(*opt_ind); /* Get ready for next option. */
  return 0;

 errjmp:
  if (splitted_arg){
    for (i = 0; i < 2; i++)
      if (splitted_arg[i]){
	free(splitted_arg[i]);
	splitted_arg[i] = NULL;
      }
    free(splitted_arg);
    splitted_arg = NULL;
  }
  return WHM_ERROR;
 
} /* whm_get_args() */


/* Called when options are present on the command line. */
int whm_parse_options(int argc, char **argv,
		      whm_config_T **configs,
		      whm_sheet_T **sheets,
		      whm_time_T *time_o,
		      int *max_ind)
{
  int i = 0, arg_ind = 0, opt_ind = 0, optnames_ind = 0;
  int opt_not_found = 0;
  whm_option_T **options = NULL;

  if (!configs || !sheets  || !time_o
      || !max_ind || !(*max_ind) || argc < 1 || !argv){
    errno = EINVAL;
    return WHM_ERROR;
  }
  else if (argc >= WHM_MAX_NUMOF_OPTIONS) {
    errno = WHM_TOOMANYOPTIONS;
    return WHM_ERROR;
  }

  if ((options = malloc(WHM_MAX_NUMOF_OPTIONS * sizeof(whm_option_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    return WHM_ERROR;
  }
  for (; i < WHM_MAX_NUMOF_OPTIONS; i++)
    if ((options[i] = whm_init_option_type()) == NULL){
      WHM_ERRMESG("Whm_init_option_type");
      goto errjmp;
    }
  ++arg_ind;
  while (arg_ind < argc) {
    if (argv[arg_ind][0] == DASH){
      opt_not_found = 1;
      for (optnames_ind = 0; optnames_ind < WHM_PROG_NUMOF_OPTIONS; optnames_ind++)
	if (s_strcmp(WHM_SHORT_OPTIONS[optnames_ind], argv[arg_ind],
		     WHM_SHORT_OPTION_S, 0) == 0
	    || s_strcmp(WHM_LONG_OPTIONS[optnames_ind], argv[arg_ind],
			WHM_LONG_OPTION_S, 0) == 0){
	  opt_not_found = 0;
	  options[opt_ind]->operation = optnames_ind;
	  ++arg_ind;
	  if (whm_get_args(argc, argv, &arg_ind, options, &opt_ind) != 0){
	    WHM_ERRMESG("Whm_get_args");
	    goto errjmp;
	  }
	  break;
	}

      if (opt_not_found) {
	fprintf(stderr, "\nOption [%s] not found\n", argv[arg_ind]);
	errno = EINVAL;
	goto errjmp;
      }
    }
    else {
      /* 
       * Encountering a command line argument not begining with a dash here is a mistake,
       * only whm_get_args() should ever see and process those.
       */
      errno = WHM_NODASHOPTION;
      goto errjmp;
    }
  }

  /* Sort the operations using their ->operation field as key. */
  whm_sort_op(options, opt_ind);

  /* Go over each options executing the appropriate operation. */
  for (i = 0; i < opt_ind; i++)
    switch(options[i]->operation) {
    case HELP:
      /* Call whm_help_op() here. */
      break;
    case PRINT:
      if (whm_print_op(options[i], configs, sheets, time_o, *max_ind) != 0){
	WHM_ERRMESG("Whm_print_op");
	goto errjmp;
      }
      break;
    case UPDATE:
      if (whm_update_op(options[i], configs, sheets, time_o, *max_ind) != 0){
	WHM_ERRMESG("Whm_update_op");
	goto errjmp;
      }
      break;
    case ADD:
      if (whm_add_op(options[i], configs, max_ind) != 0){
	WHM_ERRMESG("Whm_add_op");
	goto errjmp;
      }
      break;
    case DELETE:
      if (whm_del_op(options[i], configs, max_ind) != 0){
	WHM_ERRMESG("Whm_del_op");
	goto errjmp;
      }
      break;
    case MODIFY:
      if (whm_modify_op(options[i], configs, *max_ind) != 0){
	WHM_ERRMESG("Whm_modify_op");
	goto errjmp;
      }
      break;
    case LIST:
      whm_list_op(configs, *max_ind);
      break;
    default:
      /* Should never reach this statement, a bad operation should be caught a little above. */
      /* not happening. fix it. */
      errno = EINVAL;
      abort();
    }
      

  /* Do some cleaning. */
  if (options){
    for (i = 0; i < WHM_MAX_NUMOF_OPTIONS; i++)
      if (options[i]){
	whm_free_option_type(options[i]);
	options[i] = NULL;
      }
    free(options);
    options = NULL;
  }

  return 0;

 errjmp:
  if (options){
    for (i = 0; i < WHM_MAX_NUMOF_OPTIONS; i++)
      if (options[i]){
	whm_free_option_type(options[i]);
	options[i] = NULL;
      }
    free(options);
    options = NULL;
  }

  return WHM_ERROR;

} /* whm_parse_options() */


/* 
 * Called when no command line options are present. 
 * to_write is a global whm_backup_T* initialized by main().
 */
int whm_automatic_mode(whm_config_T **configs,
		       whm_sheet_T **sheets,
		       whm_time_T *time_o,
		       int max_ind)            /* One more than the last elements of sheets && configs. */
{
  /* 
   * For each entries of config, make their sheet path.
   * Read each sheets. 
   * If we can't read the sheet, make sure the company's status 
   * is set to active before creating a sheet.
   * Backup the sheets.
   * Add the sheet's backup filename to the global list of backup filenames.
   * Interactively update the sheets.
   */
  size_t c_ind = 0;
  char path[WHM_MAX_PATHNAME_S];
  char bup_path[WHM_MAX_PATHNAME_S];

    
  if (!sheets || !configs || !max_ind || !time_o){
    errno = EINVAL;
    return WHM_ERROR;
  }

  for (; (int)c_ind < max_ind; c_ind++) {
    if (whm_make_sheet_path(path, time_o, configs[c_ind]) == NULL){
      WHM_ERRMESG("Whm_make_sheet_path");
      return WHM_ERROR;
    }

    /* Read or create the sheet if it doesn't exists and the entry is set to active. */
    if (whm_read_sheet(path, configs[c_ind], time_o, sheets[c_ind]) != 0){
      if (errno != ENOENT){
	WHM_ERRMESG("Whm_read_sheet");
	return WHM_ERROR;
      }
      else if (configs[c_ind]->status){
	if (whm_new_sheet(sheets[c_ind], &c_ind,
			  configs[c_ind], time_o) != 0){
	  WHM_ERRMESG("Whm_new_sheet");
	  return WHM_ERROR;
	}
      }
      else continue;
    }
  }
  
  if (whm_inter_update_sheet(configs, sheets, time_o, max_ind) != 0){
    WHM_ERRMESG("Whm_inter_update_sheet");
    return WHM_ERROR;
  }

  return 0;
  
} /* whm_automatic_mode */


/* Work Hour Monitor. */
int main(int argc, char **argv)
{
  FILE *stream = NULL;
  whm_config_T **configs = NULL;
  whm_sheet_T ** sheets = NULL;
  whm_time_T *time_o = NULL;
  int i = 0, c_ind = 0;
  char config_bkup_name[WHM_MAX_PATHNAME_S];

  /* 
   * To prevent Valgrind from complaining that 
   * Syscall open(filename) points to uninitialized bytes.
   */
  memset(config_bkup_name, '\0', WHM_MAX_PATHNAME_S);

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
  if ((sheets = malloc(WHM_MAX_CONFIG_ENTRIES * sizeof(whm_sheet_T*))) == NULL){
    WHM_ERRMESG("Malloc");
    goto errjmp;
  }
  for (i = 0; i < WHM_MAX_CONFIG_ENTRIES; i++)
    if ((sheets[i] = whm_init_sheet_type()) == NULL){
      WHM_ERRMESG("Whm_init_sheet_type");
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
  /* Allocate memory for global structures. */
  if ((to_write = whm_init_backup_type()) == NULL){
    WHM_ERRMESG("Whm_init_backup_type");
    goto errjmp;
  }

  /* 
   * WHM_WORKING_DIRECTORY and WHM_CONFIGURATION_FILE are in whm_sysdep.h 
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
  if (stream){
    fclose(stream);
    stream = NULL;
  }
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

  /* 
   * Note !
   * Don't read the hour sheets now. _parse_options needs to be able
   * to selectively open sheets so filling the array now isn't helping.
   * Once _parse_options is finished, all sheets opened must be updated and written
   * to disk and closed. _automatic_mode's job is to open all config entries' latest sheet
   * to check if it's up to date and if not, update it.
   */

  
  if (argc > 1){ /* There might be options. */
    if (whm_parse_options(argc, argv, configs,
			 sheets, time_o, &c_ind) != 0){
      WHM_ERRMESG("Whm_parse_option");
      goto errjmp;
    }
  }
  else 
    if (whm_automatic_mode(configs, sheets, time_o, c_ind) != 0){
      WHM_ERRMESG("Whm_automatic_mode");
      goto errjmp;
    }
  
  /* Write every sheet that has been modified to disk now. */
  if (whm_write_sheet_list(time_o) != 0){
    WHM_ERRMESG("Whm_write_sheet_list");
    goto errjmp;
  }

  /* 
   * Before making any modifications to the configuration file, do a backup. 
   * This backup is removed only after the configuration file is written to disk.

   This might not be needed since whm_automatic_mode() isn't modifying the configuration file
   in any ways. A user must use the proper options to safely modify the configuration file.

   */
  if ((stream = fopen(WHM_CONFIGURATION_FILE, "r")) != NULL){
    fclose(stream);
    stream = NULL;
    if (whm_new_backup(WHM_CONFIGURATION_FILE,
		       config_bkup_name) == NULL){
      WHM_ERRMESG("Whm_new_backup");
      goto errjmp;
    }
  }
  /* Open and write the configuration file to disk. */
  if ((stream = fopen(WHM_CONFIGURATION_FILE, "w")) == NULL){
    WHM_ERRMESG("Fopen");
    goto errjmp;
  }
  if (whm_write_config(c_ind,
		       stream,
		       configs) == -1) {
    WHM_ERRMESG("Whm_write_config");
    goto errjmp;
  }
  fclose(stream);
  stream = NULL;
  if ((stream = fopen(config_bkup_name, "r")) != NULL){
    fclose(stream);
    stream = NULL;
    /* Remove the configuration file's backup file. */
    if (whm_rm_backup(config_bkup_name) != 0){
      WHM_ERRMESG("Whm_rm_backup");
      goto errjmp;
    }
  }
  /* Cleanup before exit. */
  if (to_write) {
    whm_free_backup_type(to_write);
    to_write = NULL;
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
  if (sheets){
    for (i = 0; i < WHM_MAX_CONFIG_ENTRIES; i++)
      if (sheets[i]) {
	whm_free_sheet_type(sheets[i]);
	sheets[i] = NULL;
      }
    free(sheets);
    sheets = NULL;
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
  if (to_write) {
    whm_free_backup_type(to_write);
    to_write = NULL;
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
  if (sheets){
    for (i = 0; i < WHM_MAX_CONFIG_ENTRIES; i++)
      if (sheets[i]) {
	whm_free_sheet_type(sheets[i]);
	sheets[i] = NULL;
      }
    free(sheets);
    sheets = NULL;
  }
  if (time_o){
    whm_free_time_type(time_o);
    time_o = NULL;
  }
  
  return WHM_ERROR;
}

/* Options operations functions. */

/* When calling for help, this is what you get. */
void whm_help_op(void)
{

  /* Must think of a better way to do this.. */
  fputs("\nWork Hour Monitor\n\n\n", stderr);
  fputs("Options disponibles:\n\n", stderr);
  fputs("\t-h\t--help\t\t\t\tCe message.\n\n", stderr);
  fputs("\t-p\t--print [nom] [annee] [mois]\n", stderr);
  fputs("\t\t\t\t\t\tImprime a l'ecran la plus recente feuille de temps de \"nom\"\n", stderr);
  fputs("\t\t\t\t\t\t\"mois\" et \"annee\" sont optionels et servent a preciser\n", stderr);
  fputs("\t\t\t\t\t\tle mois et l'annee, respectivement, de la feuille desiree.\n", stderr);
  fputs("\t\t\t\t\t\tSi \"config\" est donnee comme \"nom\", WHM imprime le fichier de\n", stderr);
  fputs("\t\t\t\t\t\tconfiguration a l'ecran et quitte.\n\n", stderr);
  fputs("\t-u\t--update [nom] [position] [heures] [date]\n", stderr);
  fputs("\t\t\t\t\t\tMet a jour la plus recente feuille de temps de \"nom\".\n", stderr);
  fputs("\t\t\t\t\t\t\"position\", \"heures\" et \"date\" sont optionnels et servent\n", stderr);
  fputs("\t\t\t\t\t\ta preciser la position a mettre a jour, le nombres d'heures travaillees\n", stderr);
  fputs("\t\t\t\t\t\tet la date a laquel les modifications s'appliquent, respectivement.\n", stderr);
  fputs("\t\t\t\t\t\tLe parametre \"heures\" dois obligatoirement etre precede par \"position\"\n", stderr);
  fputs("\t\t\t\t\t\tsinon une erreur est retourne a l'utilisateur et le programme quitte.\n\n", stderr);
  fputs("\t-a\t--add\t\t\t\tAjouter une companie au fichier de configuration [Interactif].\n\n", stderr);
  fputs("\t-d\t--delete [type] [nom] [annee] [mois]\n", stderr);
  fputs("\t\t\t\t\t\tLorsque \"type\" indique \"config\", supprime l'entree \"nom\" du fichier\n", stderr);
  fputs("\t\t\t\t\t\tde configuration. Lorsque \"type\" indique \"sheet\", supprime\n", stderr);
  fputs("\t\t\t\t\t\tla plus recente feuille de temps de \"nom\". \"annee\" et \"mois\" servent\n", stderr);
  fputs("\t\t\t\t\t\ta preciser le mois et l'annee, respectivement, de la feuille de temps a supprimer.\n\n", stderr);
  fputs("\t-m\t--modify [nom] [position] [champ] [valeur]\n", stderr);
  fputs("\t\t\t\t\t\tModifie les valeurs de configuration de la companie \"nom\".\n", stderr);
  fputs("\t\t\t\t\t\t\"position\", \"champ\" et \"valeur\" sont optionnels et servent\n", stderr);
  fputs("\t\t\t\t\t\ta preciser la position a laquel la modification s'applique, le champ\n", stderr);
  fputs("\t\t\t\t\t\tdu fichier de configuration a modifier et la nouvelle valeur a assigner a ce\n", stderr);
  fputs("\t\t\t\t\t\tchamp, respectivement. \"valeur\" dois obligatoirement etre precede de \"champ\"\n", stderr);
  fputs("\t\t\t\t\t\tsinon une erreur est retournee et le programme quitte.\n\n", stderr);
  fputs("\t-l\t--list\t\t\t\tAffiche une liste de noms des companies presentes dans le fichier de configuration\n\n\n", stderr);

  exit(EXIT_SUCCESS);

} /* whm_help_op() */


/* 
 * Print the lastest or a specific hour sheet, 
 * or print the configuration file to stdout.
 */
int whm_print_op(whm_option_T *option,
		 whm_config_T **configs,
		 whm_sheet_T **sheets,
		 whm_time_T *time_o,
		 int c_ind)
{
  int ind = 0, num = 0, valid = 0;
  char temp[WHM_TIME_STR_S];
  char path[WHM_MAX_PATHNAME_S];

  if (!option || !option->name || option->name[0] == '\0'){
    errno = EINVAL;
    return WHM_ERROR;
  }
  /* Check if the given "company" name is "config". */
  if (s_strcmp(option->name, "config",
	       WHM_NAME_STR_S, LS_ICASE) == 0){
    if (whm_write_config(c_ind, stdout, configs) != 0){
      WHM_ERRMESG("Whm_write_config");
      return WHM_ERROR;
    }
    return 0;
  }

  /* Make sure the given option name exists. */
  if ((ind = whm_validate_name(option->name, configs, c_ind)) == WHM_ERROR) {
    WHM_ERRMESG("Whm_validate_name");
    return WHM_ERROR;
  }
  else if (ind == WHM_NOMATCH) {
    errno = WHM_INVALIDCOMPANY;
    return WHM_ERROR;
  }
  
  /* If the year and/or month argument was given, modify the time object accordingly. */
  if (option->year != -1)
    if (s_strcpy(time_o->year,
		 s_itoa(temp, option->year, WHM_TIME_STR_S),
		 WHM_TIME_STR_S) == NULL){
      WHM_ERRMESG("S_strcpy");
      return WHM_ERROR;
    }
  if (option->month[0] != '\0'){
    if (!isdigit(option->month[0])){
      if ((num = whm_get_month_number(option->month)) == WHM_ERROR){
	WHM_ERRMESG("Whm_get_month_number");
	return WHM_ERROR;
      }
      if (s_strcpy(option->month,
		   s_itoa(temp, num, WHM_TIME_STR_S),
		   WHM_TIME_STR_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	return WHM_ERROR;
      }
    }
    if (s_strcpy(time_o->month, option->month, WHM_TIME_STR_S) == NULL){
      WHM_ERRMESG("S_strcpy");
      return WHM_ERROR;
    }
    if ((valid = whm_validate_time_field(time_o, T_MONTH)) <= 0){
      if (valid == 0) errno = WHM_INVALIDVALUE;
      WHM_ERRMESG("Whm_validate_time_field");
      return WHM_ERROR;
    }
  }
  /* Make the required sheet's pathname. */
  if (whm_make_sheet_path(path, time_o, configs[ind]) == NULL){
    WHM_ERRMESG("Whm_make_sheet_path");
    return WHM_ERROR;
  }
  /* Read the required sheet. */
  if (whm_read_sheet(path, configs[ind], time_o, sheets[ind]) != 0){
    WHM_ERRMESG("Whm_read_sheet");
    return WHM_ERROR;
  }
  
  /* Print the required sheet. */
  if (whm_write_sheet(stdout, configs[ind], time_o, sheets[ind]) != 0) {
    WHM_ERRMESG("Whm_write_sheet");
    return WHM_ERROR;
  }

  return 0;
} /* whm_print_op() */


/* Update a single field of the configuration file. */
int whm_update_op(whm_option_T *option,
		  whm_config_T **configs,
		  whm_sheet_T **sheets,
		  whm_time_T *time_o,
		  int max_ind)
{
  char answer[WHM_NAME_STR_S];
  char temp_str[WHM_TIME_STR_S];
  char sheet_path[WHM_MAX_PATHNAME_S];
  int c_ind = 0, pos_ind = 0, i = 0;
  int d_ind = 0, s_ind = 0, day_ind = 0, week_ind = 0;
  int word_c = 0, num = 0;
  int valid = 0;

  if (!option || !configs || !sheets
      || !max_ind || option->name[0] == '\0'){
    errno = EINVAL;
    return WHM_ERROR;
  }
  
  /* Validate the given name. */
  if ((c_ind = whm_validate_name(option->name, configs, max_ind)) == WHM_ERROR){
    WHM_ERRMESG("Whm_validate_name");
    goto errjmp;
  }
  else if (c_ind == WHM_NOMATCH){
    errno = WHM_INVALIDCOMPANY;
    goto errjmp;
  }

  /* 
   * Check if there are more than 1 position in the given company.
   * If yes make sure the ->position field of the option object isn't empty else
   * ask the user for the position to modify and validate it.
   */
  if (option->position[0] == '\0'){
    if (whm_ask_user(MODIF_POSITION, answer, WHM_NAME_STR_S,
		     configs[c_ind], 0) == WHM_ERROR) {
      WHM_ERRMESG("Whm_ask_user");
      goto errjmp;
    }
    if ((pos_ind = whm_validate_position(answer, configs[c_ind])) == WHM_ERROR){
      WHM_ERRMESG("Whm_validate_position");
      goto errjmp;
    }
  }
  else
    if ((pos_ind = whm_validate_position(option->position, configs[c_ind])) == WHM_ERROR){
      WHM_ERRMESG("Whm_validate_position");
      goto errjmp;
    }
  
  /* 
   * Check if the ->date field of the option object is empty.
   * If not change the given time object accordingly.
   */
  if (option->date[0] != '\0'){
    /* Loop over date, separate date number from month number and year. */
    if (isdigit(option->date[0])) {
      memset(temp_str, '\0', WHM_TIME_STR_S);
      for(; d_ind < WHM_NAME_STR_S; d_ind++){
	if (option->date[d_ind] != '\0' && isdigit(option->date[d_ind]))
	  temp_str[s_ind++] = option->date[d_ind];
	else {
	  /* 
	   * There are at most 3 words in a date string, in this order:
	   * date_word/-month_word/-year_word.
	   */
	  temp_str[s_ind] = '\0';
	  switch(word_c){
	  case 0:
	    if (s_strcpy(time_o->date, temp_str, WHM_TIME_STR_S) == NULL){
	      WHM_ERRMESG("S_strcpy");
	      goto errjmp;
	    }
	    if ((valid = whm_validate_time_field(time_o, T_DATE)) <= 0){
	      if (valid == 0) errno = WHM_INVALIDVALUE;
	      WHM_ERRMESG("Whm_validate_time_field");
	      goto errjmp;
	    }
	    break;
	  case 1:
	    if (s_strcpy(time_o->month, temp_str, WHM_TIME_STR_S) == NULL){
	      WHM_ERRMESG("S_strcpy");
	      goto errjmp;
	    }
	    if ((valid = whm_validate_time_field(time_o, T_MONTH)) <= 0){
	      if (valid == 0) errno = WHM_INVALIDVALUE;
	      WHM_ERRMESG("Whm_validate_time_field");
	      goto errjmp;
	    }
	    break;
	  case 2:
	    if (s_strcpy(time_o->year, temp_str, WHM_TIME_STR_S) == NULL){
	      WHM_ERRMESG("S_strcpy");
	      goto errjmp;
	    }
	    if ((valid = whm_validate_time_field(time_o, T_YEAR)) <= 0){
	      if (valid == 0) errno = WHM_INVALIDVALUE;
	      WHM_ERRMESG("Whm_validate_time_field");
	      goto errjmp;
	    }
	    break;
	  default:
	    goto end_of_loop;
	  }
	  s_ind = 0;
	  memset(temp_str, '\0', WHM_NAME_STR_S);
	  ++word_c;
	}
      }
    }
  }
  /* If date wasn't present, check if month and/or year was given. */
  else if (option->year >= 0 || option->month[0] != '\0') {
    if (option->year >= 0){
      if (s_strcpy(time_o->year, s_itoa(temp_str, option->year, WHM_TIME_STR_S), WHM_TIME_STR_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      if ((valid = whm_validate_time_field(time_o, T_YEAR)) <= 0){
	if (valid == 0) errno = WHM_INVALIDVALUE;
	WHM_ERRMESG("Whm_validate_time_field");
	goto errjmp;
      }
      /* Check if the given year's directory exists or create it. */
      if (whm_new_year_dir(configs[c_ind], time_o) != 0){
	WHM_ERRMESG("Whm_new_year_dir");
	goto errjmp;
      }
    }
    /* Month can be given as a name or as digits, make sure in the end we deal with digits. */
    if (option->month[0] != '\0'){
      if (!isdigit(option->month[0])){
	if ((num = whm_get_month_number(option->month)) == WHM_ERROR){
	  WHM_ERRMESG("Whm_get_month_number");
	  goto errjmp;
	}
	if (s_strcpy(option->month,
		     s_itoa(temp_str, num, WHM_TIME_STR_S),
		     WHM_NAME_STR_S) == NULL){
	  WHM_ERRMESG("S_strcpy");
	}
      }
      if (s_strcpy(time_o->month, option->month, WHM_TIME_STR_S) == NULL){
	WHM_ERRMESG("S_strcpy");
	goto errjmp;
      }
      if ((valid = whm_validate_time_field(time_o, T_MONTH)) <= 0){
	if (valid == 0) errno = WHM_INVALIDVALUE;
	WHM_ERRMESG("Whm_validate_time_field");
	goto errjmp;
      }
    }
  }
 end_of_loop:
  /* 
   * Check if the ->worked_hours field of the option object is empty, if it is
   * ask the user for the amount of hours worked for the given date.
   */
  if (option->worked_hours == -1){
    if (whm_ask_user(SHEET_WORKED_HOURS, answer, WHM_NAME_STR_S,
		     configs[c_ind], pos_ind) != 0){
      WHM_ERRMESG("Whm_ask_user");
      goto errjmp;
    }
    option->worked_hours = atof(answer);
  }
  /* Make the sheet's path. */
  if (whm_make_sheet_path(sheet_path, time_o, configs[c_ind]) == NULL){
    WHM_ERRMESG("Whm_make_sheet_path");
    goto errjmp;
  }

  /* Read the hour sheet. */
  if (whm_read_sheet(sheet_path, configs[c_ind],
		     time_o, sheets[c_ind]) != 0){
    WHM_ERRMESG("Whm_read_sheet");
    goto errjmp;
  }
  /* Get the week and day indexes of the time object's values. */
  for (day_ind = 0; day_ind < 7; day_ind++)
    if (s_strstr(WHM_FR_DAYS[day_ind], time_o->day, WHM_TIME_STR_S, LS_ICASE) >= 0) break;
    else if (s_strstr(WHM_EN_DAYS[day_ind], time_o->day, WHM_TIME_STR_S, LS_ICASE) >= 0) break;
  i = day_ind;
  d_ind = atoi(time_o->date);
  while (--d_ind > 0)
    if (i-- <= 0){
      i = 6;
      ++week_ind;
    }
  
  /* Change the worked hours values for the given position. */
  sheets[c_ind]->week[week_ind]->day[day_ind]->pos_hours[pos_ind] = option->worked_hours;
  
  /* Update the sheet. */
  if (whm_update_sheet(configs[c_ind], sheets[c_ind]) != 0){
    WHM_ERRMESG("Whm_update_sheet");
    goto errjmp;
  }
  
  /* Set the sheet into the "write list" */
  if (whm_set_sheet(configs[c_ind], sheets[c_ind]) != 0){
    WHM_ERRMESG("Whm_set_sheet");
    goto errjmp;
  }

  return 0;
  

 errjmp:
  return WHM_ERROR;

  
} /* whm_update_op() */
		  

/* Add a company to the configuration file. */
int whm_add_op(whm_option_T *option, /* Passing *option to make it clear it's an option operation. */
	       whm_config_T **configs,
	       int *max_ind)
{

  if (whm_add_config(max_ind, configs) != 0){
    WHM_ERRMESG("Whm_add_config");
    return WHM_ERROR;
  }
  ++(*max_ind);
  return 0;

} /* whm_add_op() */


/* 
 * Delete an existing company from the configuration file
 * but keep all it's directories and hour sheets.
 */
int whm_del_op(whm_option_T *option,
	       whm_config_T **configs,
	       int *max_ind)
{
  /* Make sure name is given and valid. */
  if (option->name[0] == '\0'){
    errno = WHM_INVALIDVALUE;
    return WHM_ERROR;
  }
  if (whm_validate_name(option->name, configs, *max_ind) == WHM_ERROR){
    WHM_ERRMESG("Whm_validate_name");
    return WHM_ERROR;
  }
  if (whm_rm_config(option->name, max_ind, configs) != 0){
    WHM_ERRMESG("Whm_rm_config");
    return WHM_ERROR;
  }
  return 0;

} /* whm_del_op() */


/* Modify a single field in an already existing entry of the configuration file. */
int whm_modify_op(whm_option_T *option,
		  whm_config_T **configs,
		  int max_ind)
{

  int c_ind = -1, field = -1, valid = -1;
  
  /* Make sure name is given and valid. */
  if (option->name[0] == '\0'){
    errno = WHM_INVALIDVALUE;
    return WHM_ERROR;
  }
  if ((c_ind = whm_validate_name(option->name, configs, max_ind)) == WHM_ERROR){
    WHM_ERRMESG("Whm_validate_name");
    return WHM_ERROR;
  }
  else if (c_ind == WHM_NOMATCH){
    errno = WHM_INVALIDCOMPANY;
    return WHM_ERROR;
  }

  /* Make sure that field was given else ask for it. */
  if (option->field == F_NONE)
    if ((option->field = whm_get_field_name(option->name, max_ind, configs)) == WHM_ERROR){
      WHM_ERRMESG("Whm_get_field_name");
      return WHM_ERROR;
    }
  /* 
   * Make sure that if field is F_POSITION or F_WAGE,
   * and if the configuration entry has more than 1 position,
   * position name was given and valid. 
   */
  if(option->field == F_POSITION || option->field == F_WAGE){
    if (option->position[0] == '\0')
    ask_again:
      if (whm_ask_user(MODIF_POSITION, option->position,
		       WHM_NAME_STR_S, configs[c_ind],
		       0) == WHM_ERROR){
	WHM_ERRMESG("Whm_ask_user");
	return WHM_ERROR;
      }
    if (s_strstr("liste", option->position, WHM_NAME_STR_S, LS_ICASE) >= 0){
      if (whm_list_config_pos(configs[c_ind]) != 0){
	WHM_ERRMESG("Whm_list_config_pos");
	return WHM_ERROR;
      }
      goto ask_again;
    }

    if ((valid = whm_validate_position(option->position, configs[c_ind])) == WHM_ERROR){
      WHM_ERRMESG("Whm_validate_position");
      return WHM_ERROR;
    }
    else if (valid == WHM_NOMATCH){
      errno = WHM_INVALIDPOSITION;
      return WHM_ERROR;
    }
  }
  /* Make sure that value was given else ask for it. */
  if (option->value[0] == '\0')
    if (whm_get_field_value(option->value, WHM_NAME_STR_S, option->position,
			    option->field, configs[c_ind]) == WHM_ERROR){
      WHM_ERRMESG("Whm_get_field_value");
      return WHM_ERROR;
    }
  if (whm_modify_config(option->name, option->position,
			option->field, option->value,
			c_ind, configs) == WHM_ERROR){
    WHM_ERRMESG("Whm_modify_config");
    return WHM_ERROR;
  }

  return 0;

} /* whm_modify_op() */


/* Print a list of all names in the configuration file. */
void whm_list_op(whm_config_T **configs,
		 int max_ind)
{
  if (whm_list_config_names(max_ind, configs) != 0)
    WHM_ERRMESG("Whm_list_config_names");
  
} /* whm_list_op */


/* 
 * Sort operations according to these priorities:
 * - Help message (program then exit.)
 * - Add/Delete/Modify configuration file.
 * - Update/Delete hour sheet.
 * - Print/list hour sheet /config name /config entry.
 */
void whm_sort_op(whm_option_T **options,
		 int max_opt_ind)
{
  whm_option_T *smallest = NULL, *temp = NULL;

  int cur = 0, low = 0;

  for (low = 0; low < max_opt_ind; low++)
    for (cur = low; cur < max_opt_ind; cur++)
      if (options[cur]->operation < options[low]->operation){
	temp = options[cur];
	options[cur] = options[low];
	options[low] = temp;
      }
  
} /* whm_sort_op() */
    


/*** Helper functions to be called only by, and within GDB. ***/

void whm_PRINT_config(whm_config_T *config)
{
  int i = 0;
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
  int c = 0;

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


/* Print the content of a whm_queue_T type object. */
void whm_PRINT_queue(whm_queue_T *queue)
{
  size_t i = 0;
  if (!queue) return;

  fprintf(stderr, "Is empty: %zu\nString lenght: %zu\nTop index: %d\nIndex: %d\n",
	  queue->is_empty, queue->string_lenght,
	  queue->top_index, queue->index);
  while (i < (size_t)queue->index){
    fprintf(stderr, "string[%zu]: %s\n", i, queue->string[i]);
    ++i;
  }
  fprintf(stderr, "\n");

} /* whm_PRINT_queue() */


void whm_PRINT_option(whm_option_T *option)
{
  if (!option) return;
  fprintf(stderr, "Operation: %d\nName: %s\nPosition: %s\nYear: %d\nMonth: %s\nDate: %s\nHours: %f\n\n",
	  option->operation,
	  option->name, option->position,
	  option->year, option->month,
	  option->date, option->worked_hours);
} /* whm_PRINT_option() */
