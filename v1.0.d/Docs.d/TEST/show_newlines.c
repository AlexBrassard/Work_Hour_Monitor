#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>



# define BUFFER_UPPER_LIMIT 14096
# define MAX_PATHNAME_LENGHT 1024

int main(int argc, char **argv)
{
  int i = 0, bnl_ind = 0;
  char input;
  FILE *filestream = NULL;
  char *buffer = NULL, *buf_with_nl = NULL;
  char rel_path[MAX_PATHNAME_LENGHT] = "./";

  if ((buffer = calloc((BUFFER_UPPER_LIMIT/2), sizeof(char))) == NULL){
    perror("Calloc");
    return -1;
  }
  if ((buf_with_nl = calloc(BUFFER_UPPER_LIMIT, sizeof(char))) == NULL){
    perror("Calloc");
    goto errjmp;
  }
  if (argc <= 1){
    i = 0;
    while ((input = getchar()) != EOF && input != '\0'){
      if (i+1 >= BUFFER_UPPER_LIMIT/2){
	errno = EOVERFLOW;
	perror("Input");
	goto errjmp;
      }
      buffer[i++] = input;
    }
    buffer[i] = '\0';
  }
  else {
    strcat(rel_path, argv[1]);
    if ((filestream = fopen(rel_path, "r")) == NULL){
      perror("Fopen");
      goto errjmp;
    }
    fread(buffer, sizeof(char), BUFFER_UPPER_LIMIT/2, filestream);
    if (ferror(filestream)){
      perror("Fread");
      goto errjmp;
    }
    if (filestream){
      fclose(filestream);
      filestream = NULL;
    }
  }
  bnl_ind = 0;
  i=0;
  while(i < (int)strlen(buffer)){
    if (buffer[i] == '\n') {
      buf_with_nl[bnl_ind++] = '\\';
      buf_with_nl[bnl_ind++] = 'n';
      buf_with_nl[bnl_ind++] = '\n';
      ++i;
    }
    else
      buf_with_nl[bnl_ind++] = buffer[i++];
  }
  buf_with_nl[bnl_ind] = '\0';
  printf("\nYour file with printed newlines:\n%s\n\n", buf_with_nl);


  if (buffer){
    free(buffer);
    buffer = NULL;
  }
  if (buf_with_nl){
    free(buf_with_nl);
    buf_with_nl = NULL;
  }
  if (filestream){
    fclose(filestream);
    filestream = NULL;
  }
  return 0;

 errjmp:
    if (buffer){
    free(buffer);
    buffer = NULL;
  }
  if (buf_with_nl){
    free(buf_with_nl);
    buf_with_nl = NULL;
  }
  if (filestream){
    fclose(filestream);
    filestream = NULL;
  }
  return -1;
}
