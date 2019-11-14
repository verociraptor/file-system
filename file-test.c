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
  if (argc != 2) usage(argv[0]);

  if(FS_Boot(argv[1]) < 0) {
    printf("ERROR: can't boot file system from file '%s'\n", argv[1]);
    return -1;
  } else printf("file system booted from file '%s'\n", argv[1]);

  
   char* fn1;
   char* fn2;

  //Create first file
  fn1 = "/first-file";
  if(File_Create(fn1) < 0) printf("ERROR: can't create file '%s'\n", fn1);
  else printf("file '%s' created successfully\n", fn1);

  //Create second file
  fn2 = "/second-file";
  if(File_Create(fn2) < 0) printf("ERROR: can't create file '%s'\n", fn2);
  else printf("file '%s' created successfully\n", fn2);

  //Open second file
  int fd2 = File_Open(fn2);
  if(fd2 < 0) printf("ERROR: can't open file '%s'\n", fn2);
  else printf("file '%s' opened successfully, fd=%d\n", fn2, fd2);

  //Open first file
  int fd1 = File_Open(fn1);
  if(fd1 < 0) printf("ERROR: can't open file '%s'\n", fn1);
  else printf("file '%s' opened successfully, fd=%d\n", fn1, fd1);

  char buf1[1024]; char* ptr1 = buf1;
  for(int i=0; i<1000; i++) {
    sprintf(ptr1, "%d %s", i, (i+1)%10==0 ? "\n" : "");
    ptr1 += strlen(ptr1);
    if(ptr1 >= buf1+1000) break;
  }

  //Write second file
  if(File_Write(fd2, buf1, 1024) != 1024)
    printf("ERROR: can't write 1024 bytes to fd=%d for file='%s'\n", fd2, fn2);
  else printf("successfully wrote 1024 bytes to fd=%d for file='%s'\n", fd2, fn2);

  //Close second file
  if(File_Close(fd2) < 0)
    printf("ERROR: cannot close fd=%d for file='%s'\n", fd2, fn2);
  else printf("successfully closed fd=%d for file='%s'\n", fd2, fn2);

  char buf2[1024]; char* ptr2 = buf2;
  for(int i=0; i<1000; i++) {
    sprintf(ptr2, "%d %s", i, (i+1)%10==0 ? "\n" : "");
    ptr2 += strlen(ptr2);
    if(ptr2 >= buf2+1000) break;
  }
  
  //Write first file
  if(File_Write(fd1, buf2, 1024) != 1024)
    printf("ERROR: can't write 1024 bytes to fd=%d for file='%s'\n", fd1, fn1);
  else printf("successfully wrote 1024 bytes to fd=%d for file='%s'\n", fd1, fn1);

  //Read second file
  char buffer[1024];
  if(File_Read(fd2, buffer, 1024) < 0)
    printf("ERROR: can't read 1024 bytes to fd=%d for file='%s'\n", fd2, fn2);
  else printf("successfully read 1024 bytes to fd=%d for file='%s'\n", fd2, fn2);

  //Write first file
  char buf3[1024]; char* ptr3 = buf3;
  for(int i=0; i<1000; i++) {
    sprintf(ptr3, "%d %s", i, (i+1)%10==0 ? "\n" : "");
    ptr3 += strlen(ptr3);
    if(ptr3 >= buf3+1000) break;
  }
  
  //Write first file
  if(File_Write(fd1, buf3, 1024) != 1024)
    printf("ERROR: can't write 1024 bytes to fd=%d for file='%s'\n", fd1, fn1);
  else printf("successfully wrote 1024 bytes to fd=%d for file='%s'\n", fd1, fn1);


  //Close first file
  if(File_Close(fd1) < 0)
    printf("ERROR: cannot close fd=%d for file='%s'\n", fd1, fn1);
  else printf("successfully closed fd=%d for file='%s'\n", fd1, fn1);

  //Open first file
  fd1 = File_Open(fn1);
  if(fd1 < 0) printf("ERROR: can't open file '%s'\n", fn1);
  else printf("file '%s' opened successfully, fd=%d\n", fn1, fd1);

  //Read first file
  char buffer2[1024];
  if(File_Read(fd1, buffer2, 1024) < 0)
    printf("ERROR: can't read 1024 bytes to fd=%d for file='%s'\n", fd1, fn1);
  else printf("successfully read 1024 bytes to fd=%d for file='%s'\n", fd1, fn1);


//Unlink first file
 if(File_Unlink(fn1) < 0) {
    printf("ERROR: can't remove file '%s'\n", fn1);
  }
  else printf("file '%s' removed successfully\n", fn1);

//Close first file
if(File_Close(fd1) < 0)
    printf("ERROR: cannot close fd=%d for file='%s'\n", fd1, fn1);
  else printf("successfully closed fd=%d for file='%s'\n", fd1, fn1);


//Unlink second file
 if(File_Unlink(fn2) < 0) {
    printf("ERROR: can't remove file '%s'\n", fn2);
  }
  else printf("file '%s' removed successfully\n", fn2);


  if(FS_Sync() < 0) {
    printf("ERROR: can't sync disk '%s'\n", argv[1]);
    return -3;
  }else printf("file system sync'd to file '%s'\n", argv[1]);

  return 0;
}
