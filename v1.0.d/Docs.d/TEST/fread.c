#include <stdio.h>
#include <string.h>
#include <errno.h>

const char filename[] = "./test.txt";

int main(void){
  char filecontent[1024];
  FILE *stream = NULL;
  
  memset(filecontent, 0, 1024);

  if ((stream = fopen(filename, "r")) == NULL){
    perror("Fopen");
    return -1;
  }
  fread(filecontent, sizeof(char), 1024, stream);
  if (errno){
    perror("Fread");
    fclose(stream);
    return -1;
  }
  printf("Content:\n%s\n\n", filecontent);
  fclose(stream);
  return 0;
}
