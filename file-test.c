#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LibFS.h"

void usage(char *prog)
{
  printf("USAGE: %s [disk] file\n", prog);
  exit(1);
}

int main(int argc, char *argv[])
{
  char *diskfile, *path;
  if(argc != 2 && argc != 3) usage(argv[0]);
  if(argc == 3) { diskfile = argv[1]; path = argv[2]; }
  else { diskfile = "default-disk"; path = argv[1]; }

  if(FS_Boot(diskfile) < 0) {
    printf("ERROR: can't boot file system from file '%s'\n", diskfile);
    return -1;
  }
  
  fn = "/first-file";
  if(File_Create(fn) < 0) printf("ERROR: can't create file '%s'\n", fn);
  else printf("file '%s' created successfully\n", fn);

  fn = "/second-file";
  if(File_Create(fn) < 0) printf("ERROR: can't create file '%s'\n", fn);
  else printf("file '%s' created successfully\n", fn);

  fn = "/second-file";
  int fd = File_Open(fn);
  if(fd < 0) printf("ERROR: can't open file '%s'\n", fn);
  else printf("file '%s' opened successfully, fd=%d\n", fn, fd);

  char buf[1024]; char* ptr = buf;
  for(int i=0; i<1000; i++) {
    sprintf(ptr, "%d %s", i, (i+1)%10==0 ? "\n" : "");
    ptr += strlen(ptr);
    if(ptr >= buf+1000) break;
  }
  if(File_Write(fd, buf, 1024) != 1024)
    printf("ERROR: can't write 1024 bytes to fd=%d\n", fd);
  else printf("successfully wrote 1024 bytes to fd=%d\n", fd);

  char buffer[1024];
  if(File_Read(fd, buffer, 1024) != 1024)
    printf("ERROR: can't read 1024 bytes to fd=%d\n", fd);
  else printf("successfully read 1024 bytes to fd=%d\n", fd);

  if(File_Unlink(path) < 0) {
    printf("ERROR: can't remove file '%s'\n", path);
    return -2;
  }
  printf("file '%s' removed successfully\n", path);

  if(FS_Sync() < 0) {
    printf("ERROR: can't sync disk '%s'\n", diskfile);
    return -3;
  }
  return 0;
}
