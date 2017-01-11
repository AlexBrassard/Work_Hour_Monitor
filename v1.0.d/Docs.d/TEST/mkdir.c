#include <sys/stat.h>


int main(void){
  if (mkdir("/home/lappop/THISTHETEST", 448) == -1) return -1;
  return 0;
}
