#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LibFS.h"
 
void usage(char *prog)
{
  printf("USAGE: %s <disk_image_file>\n", prog);
  exit(1);
}
 
int main(int argc, char *argv[])
{
  if (argc != 2) usage(argv[0]);
 
  if(FS_Boot(argv[1]) < 0) {
    printf("ERROR: can't boot file system from file '%s'\n", argv[1]);
    return -1;
  } else printf("file system booted from file '%s'\n", argv[1]);
 
  char* fn;
 
  printf("\nExpected output: SUCCESS\n");
  fn = "/file-1";
  if(File_Create(fn) < 0) printf("ERROR: can't create file '%s'\n", fn);
  else printf("file '%s' created successfully\n", fn);
 
  printf("\nExpected output: SUCCESS\n");
  fn = "/dir-1";
  if(Dir_Create(fn) < 0) printf("ERROR: can't create dir '%s'\n", fn);
  else printf("dir '%s' created successfully\n", fn);
 
  /*printf("\nExpected output: SUCCESS\n");
  fn = "/second-file";
  if(File_Create(fn) < 0) printf("ERROR: can't create file '%s'\n", fn);
  else printf("file '%s' created successfully\n", fn);
 
  printf("\nExpected output: SUCCESS\n");
  fn = "/first-file";
  if(File_Unlink(fn) < 0) printf("ERROR: can't unlink file '%s'\n", fn);
  else printf("file '%s' unlinked successfully\n", fn);*/
 
  printf("\nExpected output: SUCCESS\n");
  fn = "/file-1";
  int fd = File_Open(fn);
  if(fd < 0) printf("ERROR: can't open file '%s'\n", fn);
  else printf("file '%s' opened successfully, fd=%d\n", fn, fd);
 
  printf("\nExpected output: SUCCESS\n");
  fn = "/dir-2";
  if(Dir_Create(fn) < 0) printf("ERROR: can't create dir '%s'\n", fn);
  else printf("dir '%s' create successfully\n", fn);
 
  /*printf("\nExpected output: SUCCESS\n");
  fn = "/dir-1/third-dir";
  if(Dir_Create(fn) < 0) printf("ERROR: can't create dir '%s'\n", fn);
  else printf("dir '%s' create successfully\n", fn);*/
 
  /*printf("\nExpected output: SUCCESS\n");
  fn = "/dir-1/fourth-dir";
  if(Dir_Create(fn) < 0) printf("ERROR: can't create dir '%s'\n", fn);
  else printf("dir '%s' create successfully\n", fn);*/
 
  printf("\nExpected output: SUCCESS\n");
  fn = "/dir-1/dir-3";
  if(Dir_Create(fn) < 0) printf("ERROR: can't create dir '%s'\n", fn);
  else printf("dir '%s' created successfully\n", fn);
 
  printf("\nExpected output: SUCCESS\n");
  fn = "/dir-1/file-2";
  if(File_Create(fn) < 0) printf("ERROR: can't create file '%s'\n", fn);
  else printf("file '%s' created successfully\n", fn);
 
  printf("\nExpected output: SUCCESS\n");
  fn = "/dir-1";
  int sz = Dir_Size(fn);
  printf("Directory size: %d\n", sz);
  char* buffer = malloc(sz);
  int entries = Dir_Read(fn, buffer, sz);
  printf("directory '%s':\n     %-15s\t%-s\n", fn, "NAME", "INODE");
  int idx = 0;
  for(int i=0; i<entries; i++) {
    printf("%-4d %-15s\t%-d\n", i, &buffer[idx], *(int*)&buffer[idx+16]);
    idx += 20;
  }
  free(buffer);
 
  printf("\nExpected output: Fail\n");
  fn = "/file-1";
  fd = File_Open(fn);
  if(fd < 0) printf("ERROR: can't open file '%s'\n", fn);
  else printf("file '%s' opened successfully, fd=%d\n", fn, fd);
 
  printf("\nExpected output: SUCCESS\n");
  char buf[1024]; char* ptr = buf;
  for(int i=0; i<1000; i++) {
    sprintf(ptr, "%d %s", i, (i+1)%10==0 ? "\n" : "");
    ptr += strlen(ptr);
    if(ptr >= buf+1000) break;
  }
  if(File_Write(fd, buf, 1024) != 1024)
    printf("ERROR: can't write 1024 bytes to fd=%d\n", fd);
  else printf("successfully wrote 1024 bytes to fd=%d\n", fd);
 
  printf("\nExpected output: SUCCESS\n");
  if(File_Close(fd) < 0) printf("ERROR: can't close fd %d\n", fd);
  else printf("fd %d closed successfully\n", fd);
 
  printf("\nExpected output: SUCCESS\n");
  fn = "/file-1";
  if(File_Unlink(fn) < 0) printf("ERROR: can't unlink file '%s'\n", fn);
  else printf("file '%s' unlinked successfully\n", fn);
 
 
  if(FS_Sync() < 0) {
    printf("ERROR: can't sync file system to file '%s'\n", argv[1]);
    return -1;
  } else printf("file system sync'd to file '%s'\n", argv[1]);
   
  return 0;
}