#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "LibFS.h"
 
void usage(char *prog)
{
  printf("USAGE: %s <disk_image_file>\n", prog);
  exit(1);
}
 
int main(int argc, char *argv[])
{
 
  time_t rawtime;
  struct tm * timeinfo;
 
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  printf ( "Custom Test for file system\nCurrent local time and date: %s\n",
  asctime (timeinfo) );
 
  if (argc != 2) usage(argv[0]);
 
  if(FS_Boot(argv[1]) < 0) {
    printf("ERROR: can't boot file system from file '%s'\n", argv[1]);
    return -1;
  } else printf("file system booted from file '%s'\n", argv[1]);

  char* dir;
  printf("\nExpected output: SUCCESS\n");
  dir = "/dir-1";
  if(Dir_Create(dir) < 0) printf("ERROR: can't create dir '%s'\n", dir);
  else printf("dir '%s' created successfully\n", dir);
 
  
  printf("\nExpected output: SUCCESS\n");
  for(int i = 1; i <= 30; i++){
    char fn[256];
    strcpy(fn, dir);
    strcat(fn, "/file-");
    char currFile[5];
    sprintf(currFile, "%d", i);
    strcat(fn, currFile);
    printf("%s", fn);
    if(File_Create(fn) < 0){ printf("ERROR: can't create file '%s'\n", fn); return -1;}
    else printf("file '%s' created successfully\n", fn);
  }

  if(FS_Sync() < 0) {
    printf("ERROR: can't sync file system to file '%s'\n", argv[1]);
    return -1;
  } else printf("file system sync'd to file '%s'\n", argv[1]);
 
  return 0;
 }