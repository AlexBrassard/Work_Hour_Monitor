#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

int main(void)
{
  char output[200];
  struct tm *temp = NULL;
  time_t t;

  if ((t = time(NULL)) == -1) {
    perror("Time");
    return errno;
  }
  if ((temp = localtime(&t)) == NULL){
    perror("Localtime");
    return errno;
  }
  /* 
   * %a: Abreviated day of the week, as per the current local.
   * %d: The day of the month from 01 to 31.
   * %B: The full month name according to the current local.
   * %Y: The year including centuries
   * %V: The ISO 8601 week number from 1 to 53
   * %n: A newline character.
   */
  if (strftime(output, sizeof(output), "%a %d %B %Y %V %n", temp) == 0) {
    fprintf(stderr, "Strftime returned 0\n");
    return -1;
  }
  printf("Time string is: %s", output);
  return 0;
}
	    
  
