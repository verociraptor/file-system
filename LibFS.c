#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <malloc.h>
#include "LibDisk.h"
#include "LibFS.h"
 
// set to 1 to have detailed debug print-outs and 0 to have none
#define FSDEBUG 1
 
#if FSDEBUG
#define dprintf printf
#else
#define dprintf noprintf
void noprintf(char* str, ...) {}
#endif
 
// the file system partitions the disk into five parts:
 
// 1. the superblock (one sector), which contains a magic number at
// its first four bytes (integer)
#define SUPERBLOCK_START_SECTOR 0
 
// the magic number chosen for our file system
#define OS_MAGIC 0xdeadbeef
 
// 2. the inode bitmap (one or more sectors), which indicates whether
// the particular entry in the inode table (#4) is currently in use
#define INODE_BITMAP_START_SECTOR 1
 
// the total number of bytes and sectors needed for the inode bitmap;
// we use one bit for each inode (whether it's a file or directory) to
// indicate whether the particular inode in the inode table is in use
#define INODE_BITMAP_SIZE ((MAX_FILES+7)/8)
#define INODE_BITMAP_SECTORS ((INODE_BITMAP_SIZE+SECTOR_SIZE-1)/SECTOR_SIZE)
 
// 3. the sector bitmap (one or more sectors), which indicates whether
// the particular sector in the disk is currently in use
#define SECTOR_BITMAP_START_SECTOR (INODE_BITMAP_START_SECTOR+INODE_BITMAP_SECTORS)
 
// the total number of bytes and sectors needed for the data block
// bitmap (we call it the sector bitmap); we use one bit for each
// sector of the disk to indicate whether the sector is in use or not
#define SECTOR_BITMAP_SIZE ((TOTAL_SECTORS+7)/8)
#define SECTOR_BITMAP_SECTORS ((SECTOR_BITMAP_SIZE+SECTOR_SIZE-1)/SECTOR_SIZE)
 
// 4. the inode table (one or more sectors), which contains the inodes
// stored consecutively
#define INODE_TABLE_START_SECTOR (SECTOR_BITMAP_START_SECTOR+SECTOR_BITMAP_SECTORS)
 
// an inode is used to represent each file or directory; the data
// structure supposedly contains all necessary information about the
// corresponding file or directory
typedef struct _inode {
  int size; // the size of the file or number of directory entries
  int type; // 0 means regular file; 1 means directory
  int data[MAX_SECTORS_PER_FILE]; // indices to sectors containing data blocks
} inode_t;
 
// the inode structures are stored consecutively and yet they don't
// straddle accross the sector boundaries; that is, there may be
// fragmentation towards the end of each sector used by the inode
// table; each entry of the inode table is an inode structure; there
// are as many entries in the table as the number of files allowed in
// the system; the inode bitmap (#2) indicates whether the entries are
// current in use or not
#define INODES_PER_SECTOR (SECTOR_SIZE/sizeof(inode_t))
#define INODE_TABLE_SECTORS ((MAX_FILES+INODES_PER_SECTOR-1)/INODES_PER_SECTOR)
 
// 5. the data blocks; all the rest sectors are reserved for data
// blocks for the content of files and directories
#define DATABLOCK_START_SECTOR (INODE_TABLE_START_SECTOR+INODE_TABLE_SECTORS)
 
// other file related definitions
 
// max length of a path is 256 bytes (including the ending null)
#define MAX_PATH 256
 
// max length of a filename is 16 bytes (including the ending null)
#define MAX_NAME 16
 
// max number of open files is 256
#define MAX_OPEN_FILES 256
 
// each directory entry represents a file/directory in the parent
// directory, and consists of a file/directory name (less than 16
// bytes) and an integer inode number
typedef struct _dirent {
  char fname[MAX_NAME]; // name of the file
  int inode; // inode of the file
} dirent_t;
 
// the number of directory entries that can be contained in a sector
#define DIRENTS_PER_SECTOR (SECTOR_SIZE/sizeof(dirent_t))
 
// global errno value here
int osErrno;
 
// the name of the disk backstore file (with which the file system is booted)
static char bs_filename[1024];
 
/* the following functions are internal helper functions */
//find min
int min(int num1, int num2){
  return (num1 > num2) ? num2 : num1;
}
 
// check magic number in the superblock; return 1 if OK, and 0 if not
static int check_magic()
{
  char buf[SECTOR_SIZE];
  if(Disk_Read(SUPERBLOCK_START_SECTOR, buf) < 0)
    return 0;
  if(*(int*)buf == OS_MAGIC) return 1;
  else return 0;
}
 
#define CHARBITS (8) //number of bits in a char
 
/*
    The following functions were created by ehenl001@fiu to for bit manipulation
*/
static void set_bit(char *bitmap, int index)
{
    ( bitmap[(index/CHARBITS)] |= (1UL << (index % CHARBITS)));
}
 
static void clear_bit(char *bitmap, int index)
{
    ( bitmap[(index/CHARBITS)] &= ~(1UL << (index % CHARBITS)));
}
 
static int test_bit(char *bitmap, int index)
{
    return ( bitmap[(index/CHARBITS)] & (1UL << (index % CHARBITS))) > 0;
}
 
void yellow(){printf("\033[0;33m");};
void boldYellow(){printf("\033[1m\033[33m");};
void blue(){printf("\033[0;34m");};
void boldBlue(){printf("\033[1m\033[34m");};
void green(){printf("\033[0;32m");};
void boldGreen(){printf("\033[1m\033[32m");};
void red(){printf("\033[0;31m");}
void boldRed(){printf("\033[1m\033[31m");};
void reset(){printf("\033[0m");};
// initialize a bitmap with 'num' sectors starting from 'start'
// sector; all bits should be set to zero except that the first
// 'nbits' number of bits are set to one
// type for inode bitmap sector = 0
// type for sector bitmap sector = 1
static void bitmap_init(int start, int num, int nbits, int type)
{
  /* YOUR CODE */
  green();
  dprintf("bitmap_init(%d, %d, %d)\n",start, num, nbits);
  reset();
 
  //create bitmap with size of arry of chars neccessary to support num bits
  char *bitmap;
 
  //if type then bitmap sector, else inode sector, assign size appropriately
  int size = -1;
  if(type){
     size = SECTOR_BITMAP_SIZE;
  }else{
     size = INODE_BITMAP_SIZE;
  }
  //allocate the neccessary bytes for the num size bitmap
  bitmap = (char *)calloc(size, sizeof(char));
 
  //initialize all bits to 0 (clear all bits)
  for (int i = 0; i < num; i++)
  {
    clear_bit(bitmap,i);
  }
 
  //for nbits set the bit (bit = 1)
   for (int i = 0; i < nbits; i++)
  {
    set_bit(bitmap,i);
  }
 
  //set the bits of the sector that thec current bitmap occupies
  for(int i = start; i < start + num; i++) set_bit(bitmap, i);
 
 // check if bitmap will fit in one sector (SECTOR_SIZE > size(bitmap))
 if(SECTOR_SIZE >= size)
 {
 
     if(Disk_Write(start, bitmap)<0)
          {
            green();
            dprintf("---> Error initializing bitmap, func=bitmap_init\n");
            reset();
          }
      else{
          green();
          dprintf("---> bitmap wrote to disk successfully using 1 sector to store, func=bitmap_init\n");
          reset();
      }
 
 
 }
 else
 {
     int toXfr = size;//track total bytes to write to disk
     int numBytes = 0;
     int i = -1;
     int numSectorUsed = 0;
 
     for (i = 0; i<= size; i+=SECTOR_SIZE)
      {
        char buff[SECTOR_SIZE];
        if(toXfr>SECTOR_SIZE)
        {
            numBytes = SECTOR_SIZE;
        }
        else
        {
            numBytes = toXfr;
        }
        //copy to buff numBytes of bitmap
        if( memcpy(buff, &bitmap[i], numBytes)==NULL){dprintf("error with bitmap copy\n");}
 
        if(Disk_Write(start++, buff)<0)
          {
            green();
            dprintf("---> Error initializing bitmap\n");
            reset();
          }
         numSectorUsed +=1;
         bzero(buff, SECTOR_SIZE);
        toXfr = toXfr - numBytes;//update number of bytes remaining to write to disk
 
      }
      green();
        dprintf("---> bitmap written to disk using %d sectors to store on disk, func=bitmap_init\n", numSectorUsed);
        reset();
 }
  free(bitmap);
  green();
  dprintf("---> mem freed for bitmap, func=bitmap_init\n");
  reset();
}
 
// set the first unused bit from a bitmap of 'nbits' bits (flip the
// first zero appeared in the bitmap to one) and return its location;
// return -1 if the bitmap is already full (no more zeros)
static int bitmap_first_unused(int start, int num, int nbits)
{
  /* YOUR CODE */
 
  green();
  dprintf("bitmap_first_unused(%d, %d, %d)\n", start, num, nbits);
  reset();
  int numSec = num;
  int found = 0;
  int firstUnused = -1;
 
  //determine number of bytes needed to represent bitmap with nbits
  int bmpARRLen = nbits;  
  int currSec = start;
 
  //get number of bits represented by bmp
  int bits;
  if (start == INODE_BITMAP_START_SECTOR){
     bits = 1000;
  }else{
      bits = 10000;
  }
 
  //prep bitmap array
  char *bitmap;
  bitmap = (char*)calloc(bmpARRLen, sizeof(char));
  bzero(bitmap,bmpARRLen);
 
  int index = 0; //track index in bmp where next chunk from buff will be stored
  int numBytes = bmpARRLen; //track remaining bytes to be copied to form complete bitmap
  int bytesToCopy = -1;
  int indexCount = 0;
 
  //read bitmap from disk
  for(int i = 0; i < numSec; i++)
  {
      char buff[SECTOR_SIZE];
      //read each sector and build full bitmap
      if(Disk_Read(currSec, buff) < 0) return -1;
      //copy buff to bitmap
      index = SECTOR_SIZE * i;
 
      if(numBytes<=SECTOR_SIZE)
      {
          bytesToCopy = numBytes;
      }
      else
      {
          bytesToCopy = SECTOR_SIZE;
          numBytes -= SECTOR_SIZE;
      }
 
        for(int j = 0; j<numBytes; j++){
          bitmap[indexCount++] = buff[j];
        }
      bzero(buff, SECTOR_SIZE);
      currSec++;
  }
 
  //search for first bit equal to 0
  for(int i =0; i < bits; i++)
  {
      //if equal to 0 set bit and return i
      if (test_bit(bitmap, i) == 0)
      {
          //set ith bit to 1
          green();
          dprintf("---> attempting to set bit %d\n", i);
          reset();
          set_bit(bitmap, i);
          found = 1;
          firstUnused = i;
      }
      if(found) break;
  }
  //write new bit map to disk
  numBytes = bmpARRLen;
  currSec = start;
  index = 0;
  for(int i = 0; i < numSec; i++)
  {
      char buff[SECTOR_SIZE];
      //update index, beginning of next section of bitmap to copy
      index = SECTOR_SIZE * i;
      //check if remaining bytes to be copied (numBytes) is less than sector size
      if(numBytes <= SECTOR_SIZE)
      {
          bytesToCopy = numBytes;
      }
      else
      {
          bytesToCopy = SECTOR_SIZE;
      }
      //copy from bitmap to buff
      memcpy(buff, &bitmap[index], bytesToCopy);
      //write to currSec full bitmap or part of full bitmap
      if(Disk_Write(currSec, buff) < 0) return -1;
 
      currSec++;
 
      bzero(buff, SECTOR_SIZE);
      //update remaining number of bytes needing to be written to disk
      numBytes -= bytesToCopy;
  }
  //free allocated memory of bitmap
  free(bitmap);
  //check if bitmap corrupted
  if(found ==0 && start==SECTOR_BITMAP_START_SECTOR){
      green();
      dprintf("---> error bitmap corrupted superblock sector found as first unused\n");
      reset();
      return -1;
  }
 
  //if unused is found return its index, else return -1
  if(found)
  {
      green();
      dprintf("---> found first unused bit in nbytes=%d bmp at index=%d\n", nbits, firstUnused);
        reset();
      return firstUnused;
  }
  else
  {
       green();
      dprintf("---> unused bit NOT FOUND in nbytes=%d bmp\n", nbits);
        reset();
      return -1;
  }
}
 
// reset the i-th bit of a bitmap with 'num' sectors starting from
// 'start' sector; return 0 if successful, -1 otherwise
// reset the i-th bit of a bitmap with 'num' sectors starting from
// 'start' sector; return 0 if successful, -1 otherwise
static int bitmap_reset(int start, int num, int ibit)
{
  /* YOUR CODE */
  green();
  dprintf("bitmap_reset(%d, %d, %d)\n", start, num, ibit);
  reset();
  if((start==SECTOR_BITMAP_START_SECTOR) && ((ibit == 0)||(ibit == 1) ||(ibit == 2) || (ibit == 3)||(ibit == 4))){
      green();
    dprintf("---> error attempting to over critical sector=%d\n", ibit);
    dprintf("---> exiting bitmap_reset UNSUCCESSFULLY\n");
    reset();
    return -1;
  }
  int numSec = num;  
  //determine number of bytes needed to represent bitmap with nbits
  int bmpARRLen = -1;
  if (start == INODE_BITMAP_START_SECTOR){
     bmpARRLen = INODE_BITMAP_SIZE;
  }else{
      bmpARRLen = SECTOR_BITMAP_SIZE;
  }  
  int currSec = start;
 
  //prep bitmap array
  char *bitmap;
  bitmap = (char*)calloc(bmpARRLen, sizeof(char));
  bzero(bitmap,bmpARRLen);
 
  int index = 0; //track index in bmp where next chunk from buff will be stored
  int numBytes = bmpARRLen; //track remaining bytes to be copied to form complete bitmap
  int bytesToCopy = -1;
  int indexCount = 0;
 
  //read bitmap from disk
  for(int i = 0; i < numSec; i++)
  {
      char buff[SECTOR_SIZE];
      //read each sector and build full bitmap
      if(Disk_Read(currSec, buff) < 0) return -1;
      //copy buff to bitmap
      index = SECTOR_SIZE * i;
      if(numBytes<=SECTOR_SIZE)
      {
          bytesToCopy = numBytes;
      }
      else
      {
          bytesToCopy = SECTOR_SIZE;
          numBytes -= SECTOR_SIZE;
      }
 
        for(int j = 0; j<numBytes; j++){
          bitmap[indexCount++] = buff[j];
        }
      bzero(buff, SECTOR_SIZE);
      currSec++;
  }
  //checkt if ibit already clear if not clear
  if(test_bit(bitmap, ibit) == 0){
      green();
      dprintf("---> error bit already clear\n");
      reset();
      return -1;
  }else{
      clear_bit(bitmap, ibit);
  }
  //write new bit map to disk
  numBytes = bmpARRLen;
  currSec = start;
  index = 0;
  for(int i = 0; i < numSec; i++)
  {
      char buff[SECTOR_SIZE];
      //update index, beginning of next section of bitmap to copy
      index = SECTOR_SIZE * i;
      //check if remaining bytes to be copied (numBytes) is less than sector size
      if(numBytes <= SECTOR_SIZE)
      {
          bytesToCopy = numBytes;
      }
      else
      {
          bytesToCopy = SECTOR_SIZE;
      }
      //copy from bitmap to buff
      memcpy(buff, &bitmap[index], bytesToCopy);
      //write to currSec full bitmap or part of full bitmap
      if(Disk_Write(currSec, buff) < 0) return -1;
      currSec++;
 
      bzero(buff, SECTOR_SIZE);
      //update remaining number of bytes needing to be written to disk
      numBytes -= bytesToCopy;
  }
  //free allocated memory of bitmap
  free(bitmap);
 
   green();
  dprintf("---> bitmap_reset COMPLETED SUCCESSFULLY\n");
  reset();
  return 0;
}
// return 1 if the file name is illegal; otherwise, return 0; legal
// characters for a file name include letters (case sensitive),
// numbers, dots, dashes, and underscores; and a legal file name
// should not be more than MAX_NAME-1 in length
static int illegal_filename(char* name)
{
  // Check if name is non empty or too long
  if(name==NULL || strlen(name)==0 || strlen(name)>=MAX_NAME)
  {
    dprintf("... Filename is null or too long: %s\n", name);    
    return 1;
  }
 
  // Check if entries only contain allowed elements
  for(int i=0; i < strlen(name);i++){
    if(!(isalpha(name[i]) || isdigit(name[i]) ||
        name[i] == '.' || name[i] == '_' || name[i] == '-'  || name[i] == '/'))
        {
          dprintf("... Invalid characters in filename: %s\n", name);
          return 1; }
  }
 
  return 0;  
}
 
// return the child inode of the given file name 'fname' from the
// parent inode; the parent inode is currently stored in the segment
// of inode table in the cache (we cache only one disk sector for
// this); once found, both cached_inode_sector and cached_inode_buffer
// may be updated to point to the segment of inode table containing
// the child inode; the function returns -1 if no such file is found;
// it returns -2 is something else is wrong (such as parent is not
// directory, or there's read error, etc.)
static int find_child_inode(int parent_inode, char* fname,
          int *cached_inode_sector, char* cached_inode_buffer)
{
  int cached_start_entry = ((*cached_inode_sector)-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  int offset = parent_inode-cached_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* parent = (inode_t*)(cached_inode_buffer+offset*sizeof(inode_t));
  dprintf("... load parent inode: %d (size=%d, type=%d)\n",
   parent_inode, parent->size, parent->type);
  if(parent->type != 1) {
    dprintf("... parent not a directory\n");
    return -2;
  }
 
  int nentries = parent->size; // remaining number of directory entries
  int idx = 0;
  while(nentries > 0) {
    char buf[SECTOR_SIZE]; // cached content of directory entries
    if(Disk_Read(parent->data[idx], buf) < 0) return -2;
    for(int i=0; i<DIRENTS_PER_SECTOR; i++) {
      if(i>nentries) break;
      if(!strcmp(((dirent_t*)buf)[i].fname, fname)) {
  // found the file/directory; update inode cache
  int child_inode = ((dirent_t*)buf)[i].inode;
  dprintf("... found child_inode=%d\n", child_inode);
  int sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
  if(sector != (*cached_inode_sector)) {
    *cached_inode_sector = sector;
    if(Disk_Read(sector, cached_inode_buffer) < 0) return -2;
    dprintf("... load inode table for child\n");
  }
  return child_inode;
      }
    }
    idx++; nentries -= DIRENTS_PER_SECTOR;
  }
  dprintf("... could not find child inode\n");
  return -1; // not found
}
 
// follow the absolute path; if successful, return the inode of the
// parent directory immediately before the last file/directory in the
// path; for example, for '/a/b/c/d.txt', the parent is '/a/b/c' and
// the child is 'd.txt'; the child's inode is returned through the
// parameter 'last_inode' and its file name is returned through the
// parameter 'last_fname' (both are references); it's possible that
// the last file/directory is not in its parent directory, in which
// case, 'last_inode' points to -1; if the function returns -1, it
// means that we cannot follow the path
static int follow_path(char* path, int* last_inode, char* last_fname)
{
  if(!path) {
    dprintf("... invalid path\n");
    return -1;
  }
  if(path[0] != '/') {
    dprintf("... '%s' not absolute path\n", path);
    return -1;
  }
 
  // make a copy of the path (skip leading '/'); this is necessary
  // since the path is going to be modified by strsep()
  char pathstore[MAX_PATH];
  strncpy(pathstore, path+1, MAX_PATH-1);
  pathstore[MAX_PATH-1] = '\0'; // for safety
  char* lpath = pathstore;
 
  int parent_inode = -1, child_inode = 0; // start from root
  // cache the disk sector containing the root inode
  int cached_sector = INODE_TABLE_START_SECTOR;
  char cached_buffer[SECTOR_SIZE];
  if(Disk_Read(cached_sector, cached_buffer) < 0) return -1;
  dprintf("... load inode table for root from disk sector %d\n", cached_sector);
 
  // for each file/directory name separated by '/'
  char* token;
  while((token = strsep(&lpath, "/")) != NULL) {
    dprintf("... process token: '%s'\n", token);
    if(*token == '\0') continue; // multiple '/' ignored
    if(illegal_filename(token)) {
      dprintf("... illegal file name: '%s'\n", token);
      return -1;
    }
    if(child_inode < 0) {
      // regardless whether child_inode was not found previously, or
      // there was issues related to the parent (say, not a
      // directory), or there was a read error, we abort
      dprintf("... parent inode can't be established\n");
      return -1;
    }
    parent_inode = child_inode;
    child_inode = find_child_inode(parent_inode, token,
           &cached_sector, cached_buffer);
    if(last_fname) strcpy(last_fname, token);
  }
  if(child_inode < -1) return -1; // if there was error, abort
  else {
    // there was no error, several possibilities:
    // 1) '/': parent = -1, child = 0
    // 2) '/valid-dirs.../last-valid-dir/not-found': parent=last-valid-dir, child=-1
    // 3) '/valid-dirs.../last-valid-dir/found: parent=last-valid-dir, child=found
    // in the first case, we set parent=child=0 as special case
    if(parent_inode==-1 && child_inode==0) parent_inode = 0;
    dprintf("... found parent_inode=%d, child_inode=%d\n", parent_inode, child_inode);
    *last_inode = child_inode;
    return parent_inode;
  }
}
 
// add a new file or directory (determined by 'type') of given name
// 'file' under parent directory represented by 'parent_inode'
int add_inode(int type, int parent_inode, char* file)
{
  // get a new inode for child
  int child_inode = bitmap_first_unused(INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS, INODE_BITMAP_SIZE);
  if(child_inode < 0) {
    dprintf("... error: inode table is full\n");
    return -1;
  }
  dprintf("... new child inode %d\n", child_inode);
 
  // load the disk sector containing the child inode
  int inode_sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
  char inode_buffer[SECTOR_SIZE];
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for child inode from disk sector %d\n", inode_sector);
 
  // get the child inode
  int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  int offset = child_inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* child = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
 
  // update the new child inode and write to disk
  memset(child, 0, sizeof(inode_t));
  child->type = type;
  if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... update child inode %d (size=%d, type=%d), update disk sector %d\n",
     child_inode, child->size, child->type, inode_sector);
 
  // get the disk sector containing the parent inode
  inode_sector = INODE_TABLE_START_SECTOR+parent_inode/INODES_PER_SECTOR;
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for parent inode %d from disk sector %d\n",
     parent_inode, inode_sector);
 
  // get the parent inode
  inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  offset = parent_inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* parent = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
  dprintf("... get parent inode %d (size=%d, type=%d)\n",
     parent_inode, parent->size, parent->type);
 
  // get the dirent sector
  if(parent->type != 1) {
    dprintf("... error: parent inode is not directory\n");
    return -2; // parent not directory
  }
  int group = parent->size/DIRENTS_PER_SECTOR;
  //check if group has reach max sectors per directory and abort if true
  if (group >= MAX_SECTORS_PER_FILE){
    printf("... all sectors of parent director is filled\n");
    return -1;
  }
  char dirent_buffer[SECTOR_SIZE];
  if(group*DIRENTS_PER_SECTOR == parent->size) {
    // new disk sector is needed
    int newsec = bitmap_first_unused(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, SECTOR_BITMAP_SIZE);
    if(group == MAX_SECTORS_PER_FILE-1){
      dprintf(".... error: all sectors referenced in data attribute of parent inode fully occupied. Parent size: %d\n", parent->size);
      return -1;
    }
    if(newsec < 0) {
      dprintf("... error: disk is full\n");
      return -1;
    }
    parent->data[group] = newsec;
    memset(dirent_buffer, 0, SECTOR_SIZE);
    dprintf("... new disk sector %d for dirent group %d\n", newsec, group);
  } else {
    if(Disk_Read(parent->data[group], dirent_buffer) < 0)
      return -1;
    dprintf("... load disk sector %d for dirent group %d\n", parent->data[group], group);
  }
 
  // add the dirent and write to disk
  int start_entry = group*DIRENTS_PER_SECTOR;
  offset = parent->size-start_entry;
  dirent_t* dirent = (dirent_t*)(dirent_buffer+offset*sizeof(dirent_t));
  strncpy(dirent->fname, file, MAX_NAME);
  dirent->inode = child_inode;
  if(Disk_Write(parent->data[group], dirent_buffer) < 0) return -1;
  dprintf("... append dirent %d (name='%s', inode=%d) to group %d, update disk sector %d\n",
      parent->size, dirent->fname, dirent->inode, group, parent->data[group]);
 
  // update parent inode and write to disk
  parent->size++;
  dprintf("... updating parent inode size: %d", parent->size);
  if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... update parent inode on disk sector %d\n", inode_sector);
 
  return 0;
}
 
// used by both File_Create() and Dir_Create(); type=0 is file, type=1
// is directory
int create_file_or_directory(int type, char* pathname)
{
  int child_inode;
  char last_fname[MAX_NAME];
  int parent_inode = follow_path(pathname, &child_inode, last_fname);
  if(parent_inode >= 0)
   {
    if(child_inode >= 0)
    {
      dprintf("... file/directory '%s' already exists, failed to create\n", pathname);
      osErrno = E_CREATE;
      return -1;
    } else
      {
      if(add_inode(type, parent_inode, last_fname) >= 0)
      {
      dprintf("... successfully created file/directory: '%s'\n", pathname);
      return 0;
      }
      else {
      dprintf("... error: something wrong with adding child inode\n");
      osErrno = E_CREATE;
      return -1;
      }
    }
  } else {
    dprintf("... error: something wrong with the file/path: '%s'\n", pathname);
    osErrno = E_CREATE;
    return -1;
  }
}
 
// remove the child from parent; the function is called by both
// File_Unlink() and Dir_Unlink(); the function returns 0 if success,
// -1 if general error, -2 if directory not empty, -3 if wrong type
int remove_inode(int type, int parent_inode, int child_inode)
{
 
  dprintf("... remove inode %d\n", child_inode);
 
  // load the disk sector containing the child inode
  int inode_sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
  char inode_buffer[SECTOR_SIZE];
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for child inode from disk sector %d\n", inode_sector);
 
  // get the child inode
  int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  int offset = child_inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* child = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
 
  // check for right type
  if(child->type!=type){
    dprintf("... error: the type parameter does not match the actual inode type\n");
    return -3;
  }
 
  // check if child is non-empty directory
  if(child->type==1 && child->size>0){
    dprintf("... error: inode is non-empty directory\n");
    return -2;
  }
 
  // reset data blocks of file
  if(type==0){
 
    /**
      * DEBUGGING PRINT
    */
    dprintf("Inode size: %d, data sector 0: %d\n", child->size, child->data[0]);
 
    char data_buffer[SECTOR_SIZE];
    int sectors = child->size/SECTOR_SIZE+(child->size%SECTOR_SIZE != 0 ? 1 : 0);
    for(int i=0; i<sectors; i++){
      if(Disk_Read(child->data[i], data_buffer) < 0) return -1;
      dprintf("... delete contents of file with inode %d from sector %d\n",
        child_inode, child->data[i]);
      bzero(data_buffer, SECTOR_SIZE);
      if(Disk_Write(child->data[i], data_buffer) < 0) return -1;
      if (bitmap_reset(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, child->data[i]) < 0) {
      dprintf("... error: free sector occupied by file in sector bitmap unsuccessful\n");
      return -1;
      }
    }
 
  }
 
  // delete the child inode
  memset(child, 0, sizeof(inode_t));
  if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... delete inode %d, update disk sector %d\n",
    child_inode, inode_sector);
 
  // get the disk sector containing the parent inode
  inode_sector = INODE_TABLE_START_SECTOR+parent_inode/INODES_PER_SECTOR;
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for parent inode %d from disk sector %d\n",
   parent_inode, inode_sector);
 
  // get the parent inode
  inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  offset = parent_inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* parent = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
  dprintf("... get parent inode %d (size=%d, type=%d)\n",
   parent_inode, parent->size, parent->type);
 
  // check if parent is directory
  if(parent->type != 1) {
    dprintf("... error: parent inode is not directory\n");
    return -3;
  }
 
  // check if parent is non-empty
  if(parent->size < 1) {
    dprintf("... error: parent directory has no entries\n");
    return -1;
  }
 
  // reset bit of child inode in bitmap
  if (bitmap_reset(INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS, child_inode) < 0) {
    dprintf("... error: reset inode in bitmap unsuccessful\n");
    return -1;
  }
 
  int remainder = parent->size % DIRENTS_PER_SECTOR;
  int group = (parent->size)/DIRENTS_PER_SECTOR;
  char dirent_buffer[SECTOR_SIZE];
  dirent_t* dirent;
  int counter = 0;
  int found = 0;
 
  // search for the dirent in every group
  for(int i = 0; i<=group; i++){
 
    if(Disk_Read(parent->data[i], dirent_buffer) < 0) return -1;
      dprintf("... search for child in disk sector %d for dirent group %d\n", parent->data[i], i);
 
    for(int j=0; j<DIRENTS_PER_SECTOR; j++){
      counter ++;
      dirent = (dirent_t*)(dirent_buffer+j*sizeof(dirent_t));
      if(counter < parent->size && (!found)) {
 
        // Case 1: Child node found and last dirent in the same sector
        if(dirent->inode == child_inode && i == group) {
          dirent_t* last_dirent = (dirent_t*)(dirent_buffer+(remainder-1)*sizeof(dirent_t));
          memcpy(dirent, last_dirent, sizeof(dirent_t));
          memset(last_dirent, 0, sizeof(dirent_t));
          if(Disk_Write(parent->data[i], dirent_buffer) < 0) return -1;
          dprintf("... delete dirent (inode=%d) from group %d, update disk sector %d\n",
          child_inode, i, parent->data[i]);
          found=1;
 
        // Case 2: Child node found, but last dirent in other sector
        } else if(dirent->inode == child_inode) {
            char last_dirent_buffer[SECTOR_SIZE];
            if(Disk_Read(parent->data[group], last_dirent_buffer) < 0) return -1;
            dprintf("... load last dirent from parent %d in disk sector %d for dirent group %d\n",
            parent_inode, parent->data[group], group);
            dirent_t* last_dirent = (dirent_t*)(last_dirent_buffer+(remainder-1)*sizeof(dirent_t));
            memcpy(dirent, last_dirent, sizeof(dirent_t));
            memset(last_dirent, 0, sizeof(dirent_t));
            if(Disk_Write(parent->data[i], dirent_buffer) < 0) return -1;
            dprintf("...delete dirent (inode=%d) from group %d, update disk sector %d\n",
              child_inode, i, parent->data[i]);
            if(Disk_Write(parent->data[group], last_dirent_buffer) < 0) return -1;
            dprintf("...copy last dirent with inode %d into group %d, update disk sector %d\n",
              dirent->inode, group-1, parent->data[group]);
            found=1;
        }
 
      } else if(!found) {
 
        // Case 3: Child found and last dirent from parent
        if(dirent->inode == child_inode) {
          memset(dirent, 0, sizeof(dirent_t));
          if(Disk_Write(parent->data[i], dirent_buffer) < 0) return -1;
          dprintf("... delete dirent (inode=%d) from group %d, update disk sector %d\n",
          child_inode, group, parent->data[i]);
          found=1;
 
        // Case 4: Child node not found, and reached last dirent from parent
        } else if(counter >= parent->size) {
          dprintf("Counter: %d, Parent size: %d\n", counter, parent->size);
          dprintf("... error: child inode could not be found in parent directory\n");
          return -1;
        }
      }
    }
  }
 
  // check for remaining dirents from parent in that sector (otherwise reset sector bitmap)
  if (remainder == 1) {
    // disk sector has to be freed
  if(parent->data[group] == 0){dprintf("... parent->data[group]=%d\n", parent->data[group]);}
    if (bitmap_reset(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, parent->data[group]) < 0) {
    dprintf("... error: free sector in bitmap unsuccessful\n");
    return -1;
    }
    parent->data[group] = 0;
  }
 
  // update parent inode and write to disk
  parent->size--;
  if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... update parent inode on disk sector %d\n", inode_sector);
 
  return 0;
}
 
// representing an open file
typedef struct _open_file {
  int inode; // pointing to the inode of the file (0 means entry not used)
  int size;  // file size cached here for convenience
  int pos;   // read/write position within the data array
  int posByte; //starting byte to read from
} open_file_t;
static open_file_t open_files[MAX_OPEN_FILES];
 
// return true if the file pointed to by inode has already been open
int is_file_open(int inode)
{
  for(int i=0; i<MAX_OPEN_FILES; i++) {
    if(open_files[i].inode == inode)
      return 1;
  }
  return 0;
}
 
// return a new file descriptor not used; -1 if full
int new_file_fd()
{
  for(int i=0; i<MAX_OPEN_FILES; i++) {
    if(open_files[i].inode <= 0)
      return i;
  }
  return -1;
}
 
/* end of internal helper functions, start of API functions */
 
int FS_Boot(char* backstore_fname)
{
  dprintf("FS_Boot('%s'):\n", backstore_fname);
  // initialize a new disk (this is a simulated disk)
  if(Disk_Init() < 0) {
    dprintf("... disk init failed\n");
    osErrno = E_GENERAL;
    return -1;
  }
  dprintf("... disk initialized\n");
 
  // we should copy the filename down; if not, the user may change the
  // content pointed to by 'backstore_fname' after calling this function
  strncpy(bs_filename, backstore_fname, 1024);
  bs_filename[1023] = '\0'; // for safety
 
  // we first try to load disk from this file
  if(Disk_Load(bs_filename) < 0) {
    dprintf("... load disk from file '%s' failed\n", bs_filename);
 
    // if we can't open the file; it means the file does not exist, we
    // need to create a new file system on disk
    if(diskErrno == E_OPENING_FILE) {
      dprintf("... couldn't open file, create new file system\n");
 
      // format superblock
      char buf[SECTOR_SIZE];
      memset(buf, 0, SECTOR_SIZE);
      *(int*)buf = OS_MAGIC;
      if(Disk_Write(SUPERBLOCK_START_SECTOR, buf) < 0) {
  dprintf("... failed to format superblock\n");
  osErrno = E_GENERAL;
  return -1;
      }
      dprintf("... formatted superblock (sector %d)\n", SUPERBLOCK_START_SECTOR);
 
      // format inode bitmap (reserve the first inode to root)
      //type for inode bitmap sector = 0
       dprintf("... calling  bitmap int for inode bitmap with start=%d, num=%d, nbits=1\n",
                    INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS);
      bitmap_init(INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS, 1, 0);
      dprintf("... formatted inode bitmap (start=%d, num=%d)\n",
       (int)INODE_BITMAP_START_SECTOR, (int)INODE_BITMAP_SECTORS);
     
      // format sector bitmap (reserve the first few sectors to
      // superblock, inode bitmap, sector bitmap, and inode table)
      //type for sector bitmap = 1
      dprintf("... calling  bitmap int for sector bitmap with start=%d, num=%d, nbits=%ld\n",
                SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, DATABLOCK_START_SECTOR);
      bitmap_init(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS,
      DATABLOCK_START_SECTOR, 1);
      dprintf("... formatted sector bitmap (start=%d, num=%d)\n",
       (int)SECTOR_BITMAP_START_SECTOR, (int)SECTOR_BITMAP_SECTORS);
     
      // format inode tables
      for(int i=0; i<INODE_TABLE_SECTORS; i++) {
  memset(buf, 0, SECTOR_SIZE);
  if(i==0) {
    // the first inode table entry is the root directory
    ((inode_t*)buf)->size = 0;
    ((inode_t*)buf)->type = 1;
  }
  if(Disk_Write(INODE_TABLE_START_SECTOR+i, buf) < 0) {
    dprintf("... failed to format inode table\n");
    osErrno = E_GENERAL;
    return -1;
  }
      }
      dprintf("... formatted inode table (start=%d, num=%d)\n",
       (int)INODE_TABLE_START_SECTOR, (int)INODE_TABLE_SECTORS);
     
      // we need to synchronize the disk to the backstore file (so
      // that we don't lose the formatted disk)
      if(Disk_Save(bs_filename) < 0) {
  // if can't write to file, something's wrong with the backstore
  dprintf("... failed to save disk to file '%s'\n", bs_filename);
  osErrno = E_GENERAL;
  return -1;
      } else {
  // everything's good now, boot is successful
  dprintf("... successfully formatted disk, boot successful\n");
  memset(open_files, 0, MAX_OPEN_FILES*sizeof(open_file_t));
  return 0;
      }
    } else {
      // something wrong loading the file: invalid param or error reading
      dprintf("... couldn't read file '%s', boot failed\n", bs_filename);
      osErrno = E_GENERAL;
      return -1;
    }
  } else {
    dprintf("... load disk from file '%s' successful\n", bs_filename);
   
    // we successfully loaded the disk, we need to do two more checks,
    // first the file size must be exactly the size as expected (thiis
    // supposedly should be folded in Disk_Load(); and it's not)
    int sz = 0;
    FILE* f = fopen(bs_filename, "r");
    if(f) {
      fseek(f, 0, SEEK_END);
      sz = ftell(f);
      fclose(f);
    }
    if(sz != SECTOR_SIZE*TOTAL_SECTORS) {
      dprintf("... check size of file '%s' failed\n", bs_filename);
      osErrno = E_GENERAL;
      return -1;
    }
    dprintf("... check size of file '%s' successful\n", bs_filename);
   
    // check magic
    if(check_magic()) {
      // everything's good by now, boot is successful
      dprintf("... check magic successful\n");
      memset(open_files, 0, MAX_OPEN_FILES*sizeof(open_file_t));
      return 0;
    } else {      
      // mismatched magic number
      dprintf("... check magic failed, boot failed\n");
      osErrno = E_GENERAL;
      return -1;
    }
  }
}
 
int FS_Sync()
{
  dprintf("FS_Sync():\n");
  if(Disk_Save(bs_filename) < 0) {
    // if can't write to file, something's wrong with the backstore
    dprintf("FS_Sync():\n... failed to save disk to file '%s'\n", bs_filename);
    osErrno = E_GENERAL;
    return -1;
  } else {
    // everything's good now, sync is successful
    dprintf("FS_Sync():\n... successfully saved disk to file '%s'\n", bs_filename);
    return 0;
  }
}
 
int File_Create(char* file)
{
  dprintf("File_Create('%s'):\n", file);
  return create_file_or_directory(0, file);
}
 
int File_Unlink(char* file)
{
  boldBlue();
  dprintf("File_Unlink('%s'):\n", file);
  reset();

  int child_inode;
 
  int parent_inode = follow_path(file, &child_inode, NULL);
 
 
  if(child_inode >= 0) //file exists
  {
     //check if file is open
    if(is_file_open(child_inode)){
       dprintf("... %s is an open file. Cannot unlink\n", file);
       osErrno = E_FILE_IN_USE;
       return -1;
    }
   
   
   int remove = remove_inode(0, parent_inode, child_inode);
   if(remove == -1){
      dprintf("... error: general error when unlinking file\n");
      osErrno = E_GENERAL;
      return -1;
   }
   else if(remove == -3){
      dprintf("... error: no file, incorrect type\n");
      osErrno = E_NO_SUCH_FILE;
      return -1;
   }
   else{
       return 0;
   }
 
  }
 
  else{ //file does not exist
    dprintf("... %s file does not exist\n", file);
    osErrno = E_NO_SUCH_FILE;
    return -1;
 
  }
}
 
int File_Open(char* file)
{
  boldBlue();
  dprintf("File_Open('%s'):\n", file);
  reset();

  int fd = new_file_fd();
 
  if(fd < 0) {
    dprintf("... max open files reached\n");
    osErrno = E_TOO_MANY_OPEN_FILES;
    return -1;
  }
 
  int child_inode;
  follow_path(file, &child_inode, NULL);
 
  if(child_inode >= 0) { // child is the one, file exists
    //check if file is already open
    if(is_file_open(child_inode)){
      dprintf("... error: %s is an open file already.\n", file);
      osErrno = E_FILE_IN_USE;
      return -1;
    }
    // load the disk sector containing the inode
    int inode_sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
    char inode_buffer[SECTOR_SIZE];
    if(Disk_Read(inode_sector, inode_buffer) < 0) { osErrno = E_GENERAL; return -1; }
    dprintf("... load inode table for inode from disk sector %d\n", inode_sector);
 
    // get the inode
    int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
    int offset = child_inode-inode_start_entry;
    assert(0 <= offset && offset < INODES_PER_SECTOR);
    inode_t* child = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
    dprintf("... inode %d (size=%d, type=%d)\n",
      child_inode, child->size, child->type);
 
    if(child->type != 0) {
      dprintf("... error: '%s' is not a file\n", file);
      osErrno = E_GENERAL;
      return -1;
    }
 
    // initialize open file entry and return its index
    open_files[fd].inode = child_inode;
    open_files[fd].size = child->size;
    open_files[fd].pos = 0;
    open_files[fd].posByte = 0;
 
 
       
 
    return fd;
  } else {
    dprintf("... file '%s' is not found\n", file);
    osErrno = E_NO_SUCH_FILE;
    return -1;
  }  
}

//Case 1: Size to read is bigger than the remaining size of file ->
//        ask user if they wish to read the remaining size or nothing at all
//Case 2: size to read is less than or equal to total file size -> read size to read
int File_Read(int fd, void* buffer, int size)
{
  boldBlue();
  dprintf("File_Read(%d, buffer, %d):\n", fd, size);
  reset();
 
  //check if fd is valid index
  if(fd < 0 || fd > MAX_OPEN_FILES){
    blue();
    dprintf("... fd=%d out of bound", fd);  
    reset();  
    osErrno = E_BAD_FD;
    return -1;
  }
 
  open_file_t file = open_files[fd];
 
  //check if not an open file
  if(file.inode <= 0){
    blue();
    dprintf("... fd=%d not an open file\n", fd);
    reset();
    osErrno = E_BAD_FD;
    return -1;
  }
 
   
  //check if file size is empty
  if(file.size == 0)
  {
    //none to read
    blue();
    dprintf("... file fd=%d is empty\n", fd);
    reset();
    return 0;
  }
 
  /***determine how many sectors can actually be read***/
 
  //none to read, position at end of file
  if(file.size - ((file.pos+1)*SECTOR_SIZE-(SECTOR_SIZE-file.posByte)) == 0)
  {
    blue();
    dprintf("... file fd=%d is at end of file\n", fd);  
    reset();  
    return 0;
  }
  //something to read
  //remaining file size left to read
  int remFileSize = file.size - ((file.pos+1)*SECTOR_SIZE-(SECTOR_SIZE-file.posByte));
  int sectorsToRead = 0;
 
  int sizeToRead = 0;
 
  sizeToRead = min(remFileSize,size);

  //let user know that reading less than the initial size given
  if(sizeToRead == remFileSize){
    printf("... file fd=%d can only read remaining bytes of %d, not %d bytes\n", fd, remFileSize, size);
  }
 
 
  //if less than size is remaining, read only remaining
  //else pick initial given size to read
 
 
  //if posByte is at part of current sector
  if(file.posByte != 0){
    sectorsToRead++;//read only one sector
    if(SECTOR_SIZE - file.posByte < sizeToRead){//need to read more sectors
      int remSize = sizeToRead - (SECTOR_SIZE - file.posByte);
      sectorsToRead += remSize/SECTOR_SIZE;
 
      //check for remainder
      if(remSize%SECTOR_SIZE){
        sectorsToRead++;
      }
    }
  }
  else{//if posByte is at beginning of a sector
    sectorsToRead = sizeToRead/SECTOR_SIZE;
 
    //check remainder
    if(sizeToRead%SECTOR_SIZE){
      sectorsToRead++;
    }
  }
   
  blue();
  dprintf("... sectors to read=%d with size to read=%d of file size=%d at data[%d] at byte position=%d\n",
            sectorsToRead, sizeToRead, file.size, file.pos, file.posByte);
  reset();
 
 /***get file inode***/
 
  //load file's inode disk sectors
  int inode = file.inode;
  int inode_sector = INODE_TABLE_START_SECTOR+inode/INODES_PER_SECTOR;
  char inode_buffer[SECTOR_SIZE];
  if(Disk_Read(inode_sector, inode_buffer) < 0) { osErrno = E_GENERAL; return -1; }
 
  blue();
  dprintf("... load inode table for inode from disk sector %d\n", inode_sector);
  reset();
 
  //get inode
  int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  int offset = inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* fileInode = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
 
  blue();
  dprintf("... inode %d (size=%d, type=%d)\n",
            inode, fileInode->size, fileInode->type);
  reset();
   
  int position = file.pos;
 
  /***read contents of data blocks***/
 
  //temp buffer to read entire sector in
  char tempBuff[SECTOR_SIZE];
  memset(buffer,0,sizeToRead);
 
  //will position buffer ptr to next available space to copy data into
  int ctrSize = 0;
  for(int i = 0; i < sectorsToRead; i++){
    bzero(tempBuff, SECTOR_SIZE);
 
    if(Disk_Read(fileInode->data[position], tempBuff) < 0){
      blue();
      dprintf("... error: can't read sector %d\n", fileInode->data[position]);
      reset();
      osErrno=E_GENERAL;
      return -1;      
    }
 
    blue();
    dprintf("... Reading data[%d]=%d at positionByte=%d\n",
              position, fileInode->data[position], file.posByte);
    reset();
 
    int currBytes = 0;  //keep track of what's currently read from sector
 
    if(file.posByte != 0){//starting out at arbitrary point in currsector
      currBytes = SECTOR_SIZE - file.posByte;
    }
    else if(i == sectorsToRead - 1){//at last sector to read from
      currBytes = sizeToRead - ctrSize;
    }  
    else{//full sectors
      currBytes = SECTOR_SIZE;
    }
   
    blue();
    dprintf("... Copying data of %d bytes at %d into buffer ptr at %d\n",
                currBytes, file.posByte, ctrSize);
    reset();
 
    //copy what's needed into buffer from buff
    memcpy(buffer+ctrSize, tempBuff+file.posByte, currBytes);
   
    ctrSize += currBytes;
 
    //specify new byte position if didn't read the entire last sector
    if(i == sectorsToRead - 1 && file.posByte + currBytes < SECTOR_SIZE){
      file.posByte = currBytes;
    }
    else{//new byte position and block position if read entire sectors
      position++;
      file.posByte = 0;
    }
 
 
  }
 
 
  //set new read/write position and new posbyte to read at  
  open_files[fd].pos = position;
  open_files[fd].posByte = file.posByte;  
 
  blue();
  dprintf("... file=%d is now at pos=%d with byte pos=%d\n", fd, open_files[fd].pos, open_files[fd].posByte);
 
  dprintf("... successfully read %d sectors with size=%d\n", sectorsToRead, sizeToRead);
  reset();
 
  return sizeToRead;
 
}
 
//helper function
//gets user input on what action to take related to overwriting a file
//when the file recently opens and is non-empty
int toOverwriteOrNot(int fd){
  int action = 0;
  do{
    printf("File of fd=%d is non-empty. Do you\n"
          "1. wish to overwrite from position 0 (will delete entire file's contents)?\n"
          "2. wish to append data from first empty position?\n"
          "3. wish to not write?\n"
          "Please input 1, 2 or 3 indicating your desired action.\n", fd);
   
    scanf("%d", &action);
  }while(action != 1 && action != 2 && action != 3);
 
  return action;
}
 
//ask user if want to write in only remaining available space or not
//case: when size > sizeRem
int ifWriteRem(int fd, int size, int sizeRem){
  int action = 0;

  do{
    printf("File of fd=%d cannot write requested size=%d bytes. It has a space of size=%d bytes remaining. Do you\n"
            "1. wish to write %d bytes from data given?\n"
            "2. wish to not write at all?\n"
            "Please input 1 or 2 indicating your desired actions.\n", fd, size, sizeRem, sizeRem);
    scanf("%d", &action);
  }while(action != 1 && action != 2);

  return action;
}

//case 1: file_write first called on empty file -> no checks, write into file
//case 2: file_write first called on a recently opened non-empty file 
//        or file ptr is at arbitrary point within file contents
//        -> check if user wishes to overwrite
//        if 1 -> overwrite from given position
//        if 2 -> write only from the first empty position
//        if 3 -> do not overwrite
//Case 3: file_write called with a size bigger than remaining available space in a file 
//        -> check if user wishes to write into remaining space 
//        if 1 -> write into remaining space from data given 
//        if 2 -> do not write
int File_Write(int fd, void* buffer, int size)
{
  boldBlue();
  dprintf("File_Write(%d, buffer, %d):\n",fd, size);
  reset();

  //check if fd is valid index
  if(fd < 0 || fd > MAX_OPEN_FILES){
    blue();
    dprintf("... error: fd=%d out of bound\n", fd);
    reset();
    osErrno = E_BAD_FD;
    return -1;
  }

  open_file_t file = open_files[fd];

  //check if not an open file
  if(file.inode <= 0){
    blue();
    dprintf("... error: fd=%d not an open file\n", fd);
    reset();
    osErrno = E_BAD_FD;
    return -1;
  }
    
  //check if file size is full
  if(file.size == MAX_SECTORS_PER_FILE*SECTOR_SIZE)
  {
    blue();
    dprintf("... error: file fd=%d is full\n", fd);  
    reset();
    osErrno = E_FILE_TOO_BIG;
    return 0;
  }

  //load file's inode disk sectors
  int inode = file.inode;
  int inode_sector = INODE_TABLE_START_SECTOR+inode/INODES_PER_SECTOR;
  char inode_buffer[SECTOR_SIZE];
  if(Disk_Read(inode_sector, inode_buffer) < 0) { osErrno = E_GENERAL; return -1; }

  blue();
  dprintf("... load inode table for inode from disk sector %d\n", inode_sector);
  reset();

  //get inode
  int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  int offset = inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* fileInode = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
  blue();
  dprintf("... inode %d (size=%d, type=%d)\n",
            inode, fileInode->size, fileInode->type);
  reset();
   
   /***check if user wishes to overwrite or not***/

  //write data from after the given data pointer
  int position = file.pos;
  int action = 0;
  int positionByte = file.posByte;

  //check cases for potential overwriting:
  //if recently opened file is non-empty
  //if current pointer is at arbitrary point within file contents
  if(file.size - ((file.pos+1)*SECTOR_SIZE-(SECTOR_SIZE-file.posByte)) != 0){
    //check if user wishes to overwrite
    action = toOverwriteOrNot(fd);
    if(action == 3){
      blue();
      printf("... User wishes to not overwrite already existing file. No write done\n");
      reset();
      return 0;
    }
    else if(action == 2){ //write only from the first empty position
      position = file.size/SECTOR_SIZE;
      positionByte = 0;

      if(file.size%SECTOR_SIZE){
        positionByte = file.size%SECTOR_SIZE;
      }
      blue();
      dprintf("... User wishes to write from first empty byte position=%d in block position=%d with file size=%d\n",
              positionByte, position, file.size);
      reset();
    }
    else{//wishes to overwrite. delete file contents
      int pos = 0;
      char dataBuffer[SECTOR_SIZE];
      bzero(dataBuffer, SECTOR_SIZE);
      while(fileInode->data[pos] != 0){
        // disk sector has to be freed
        if(Disk_Write(fileInode->data[pos], dataBuffer) < 0) return -1;
        if (bitmap_reset(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, fileInode->data[pos]) < 0) {
          blue();
          dprintf("... error: free sector occupied by file in sector bitmap unsuccessful\n");
          reset();
          return -1;
        }
        pos++;
      }
      position = 0;
      positionByte = 0;
      file.size = 0;
      fileInode->size = 0;
      blue();
      dprintf("... User wishes to overwrite entire file. Starting at block position=%d with file size=%d\n",
              position, file.size);
      reset();
    }
  }

  int sizeToWrite = size;

  //check if the rem space left is less than size to write
  if(SECTOR_SIZE*MAX_SECTORS_PER_FILE - file.size < size){
    blue();
    dprintf("... error: fd=%d of size=%d at block position=%d at byte position=%d cannot add size=%d bytes.\n" 
              "Ask user what they want to do\n", fd, file.size, position, positionByte, size);
    reset();
    if(ifWriteRem(fd, size, SECTOR_SIZE*MAX_SECTORS_PER_FILE - file.size) == 1){
      sizeToWrite = SECTOR_SIZE*MAX_SECTORS_PER_FILE - file.size;
      blue();
      dprintf("... User chose to write only %d bytes to file=%d\n", sizeToWrite, fd);
      reset();
    }
    else{
      blue();
      dprintf("... User chose to not write remaining %d bytes. Ending call\n", SECTOR_SIZE*MAX_SECTORS_PER_FILE - file.size);
      osErrno = E_FILE_TOO_BIG;
      return 0;
    }
  }

  /***determine how many sectors need to be written in***/

  int sectorsToWrite = 0;

  if(positionByte != 0){//if posByte is at part of current sector
    sectorsToWrite++;
    if(SECTOR_SIZE - positionByte < sizeToWrite){//need to write more sectors
      int remSize = sizeToWrite - (SECTOR_SIZE - positionByte);
      sectorsToWrite += remSize/SECTOR_SIZE;

      //check for remainder
      if(remSize%SECTOR_SIZE){
        sectorsToWrite++;
      }
    }
  }
  else{//if posByte is at beginning of a sector
    sectorsToWrite = sizeToWrite/SECTOR_SIZE;

    //check remainder
    if(sizeToWrite%SECTOR_SIZE){
      sectorsToWrite++;
    }
  }

  blue();
  dprintf("... extra sectors needed=%d for fd=%d at data[%d] at byte position=%d\n", sectorsToWrite,
                  fd, position, positionByte);
  reset();

  /***write into data blocks***/
  
  char tempBuff[SECTOR_SIZE];
  int ctrSize = 0;
  for(int i = 0; i < sectorsToWrite; i++){
    //find first sector to use if starting at new position
    int newsec = 0;
    if(positionByte == 0){
      newsec = bitmap_first_unused(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, SECTOR_BITMAP_SIZE);
      
      //check if space exists on disk for write
      if(newsec < 0){
        blue();
        dprintf("... error: no space on disk, write cannot complete\n");
        reset();
        osErrno = E_NO_SPACE;
        return -1;
      }
      fileInode->data[position] = newsec;
    } 
    
    bzero(tempBuff, SECTOR_SIZE);
    if(Disk_Read(fileInode->data[position], tempBuff) < 0){
      blue();
      dprintf("... error: can't read sector %d\n", fileInode->data[position]);
      reset();
      osErrno=E_GENERAL;
      return -1;       
    }
    
    blue();
    dprintf("... going to write into disk in sector %d\n", newsec);
    reset();

    int currBytes = 0;  //keep track of what's currently needed to extract from buffer

    if(positionByte != 0){//starting out at arbitrary point in currsector
      currBytes = SECTOR_SIZE - positionByte;
    }
    else if(i == sectorsToWrite - 1){//at last sector to write into 
      currBytes = sizeToWrite - ctrSize;
    }   
    else{//full sectors
      currBytes = SECTOR_SIZE;
    }
    
    blue();
    dprintf("... copying data %d bytes from ptr position %d in buffer into tempbuff at position %d\n",
            currBytes, ctrSize, positionByte);
    reset();

    //copy what's needed from buffer into tempbuff
    memcpy(tempBuff+positionByte, buffer+ctrSize, currBytes);

    ctrSize += currBytes;//where to next extract from buffer

    if(Disk_Write(fileInode->data[position], tempBuff) < 0) {
      blue();
      dprintf("... error: failed to write buffer data\n");
      reset();
      osErrno = E_GENERAL;
      return -1;
    }
    blue();
    dprintf("... Writing data[%d]=%d\n", position, fileInode->data[position]);
    reset();

    //specify new byte position if didn't write the entire last sector
    if(i == sectorsToWrite - 1 && positionByte + currBytes < SECTOR_SIZE){
      positionByte = currBytes;
    }
    else{//new byte position and block position if read entire sectors
      position++;
      positionByte = 0;
    }
  } 

  /***update file and file inode***/

  //update file and inode size
  open_files[fd].size = file.size + sizeToWrite;
  open_files[fd].pos = position;
  open_files[fd].posByte = positionByte;

  //set new inode size
  fileInode->size = open_files[fd].size;
 

  //write to disk the updated inode
  if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
  blue();
  dprintf("... update child inode %d (size=%d, type=%d), update disk sector %d\n",
                  inode, fileInode->size, fileInode->type, inode_sector); 

  dprintf("... file=%d is now at block pos=%d and byte position=%d with size=%d\n", 
              fd, open_files[fd].pos, open_files[fd].posByte, open_files[fd].size);
  reset();
  return sizeToWrite;
}
int File_Seek(int fd, int offset)
{
  boldBlue();
  dprintf("File_Seek(%d, %d):\n", fd, offset);
  reset();
  //check if fd is valid index
  if(fd < 0 || fd > MAX_OPEN_FILES){
    dprintf("... error: fd=%d out of bound\n", fd);
    osErrno = E_BAD_FD;
    return -1;    
  }
  //check if open file
 if(open_files[fd].inode <= 0) {
    dprintf("... error: fd=%d not an open file\n", fd);
    osErrno = E_BAD_FD;
    return -1;
  }
   
  //check if offset is within bounds and if not at end of file
  if(offset >= open_files[fd].size || offset == -1){
     dprintf("... error: offset=%d out of bound\n", fd);
     osErrno = E_SEEK_OUT_OF_BOUNDS;
    return -1;
  }
 
  open_files[fd].pos = offset/SECTOR_SIZE;
  open_files[fd].posByte = offset%SECTOR_SIZE;
 
 dprintf("... file=%d now at position=%d at byte position=%d\n",
          fd, open_files[fd].pos, open_files[fd].posByte);

  return open_files[fd].pos;  
}
 
int File_Close(int fd)
{
  dprintf("File_Close(%d):\n", fd);
  if(0 > fd || fd > MAX_OPEN_FILES) {
    dprintf("... error: fd=%d out of bound\n", fd);
    osErrno = E_BAD_FD;
    return -1;
  }
  if(open_files[fd].inode <= 0) {
    dprintf("... error: fd=%d not an open file\n", fd);
    osErrno = E_BAD_FD;
    return -1;
  }
 
 
 
  dprintf("... file closed successfully\n");
  open_files[fd].inode = 0;
  return 0;
}
 
int Dir_Create(char* path)
{
  dprintf("Dir_Create('%s'):\n", path);
  return create_file_or_directory(1, path);
}
 
int Dir_Unlink(char* path)
{
   dprintf("Dir_Unlink('%s'):\n", path);
     // no empty path and no root directory allowed
  if(path==NULL) {
    dprintf("... error: empty path (NULL) for directory unlink");
    osErrno = E_GENERAL;
    return -1;
  }
 
  if(strcmp(path, "/")==0) {
    dprintf("... error: not allowed to unlink root directory");
    osErrno = E_ROOT_DIR;
    return -1;
  }
 
  // find parent and children (if theres any)
  int child_inode;
  int parent_inode = follow_path(path, &child_inode, NULL);
  if(parent_inode < 0) {
    dprintf("... error: directory '%s' not found\n", path);
      osErrno = E_NO_SUCH_DIR;
      return -1;
  }
 
  int remove = remove_inode(1, parent_inode, child_inode);
 
  if (remove==-1) {
    dprintf("... error: general error when unlinking directory\n");
    osErrno = E_GENERAL;
    return -1;
  } else if (remove==-3) {
    dprintf("... error: no directory, wrong type\n");
    osErrno = E_NO_SUCH_DIR;
    return -1;
  } else if (remove==-2) {
    dprintf("... error: directory %s not empty", path);
    osErrno = E_DIR_NOT_EMPTY;
    return -1;
  } else {
    return 0;
  }
 
 
}
 
int Dir_Size(char* path)
{
  dprintf("Dir_Size('%s'):\n", path);
  // no empty path allowed
  if(path==NULL) {
    dprintf("... error: empty path (NULL) given as parameter\n");
    osErrno = E_GENERAL;
    return -1;
  }
 
  // directory has to exist
  int child_inode;
  int parent_inode = follow_path(path, &child_inode, NULL);
  if(parent_inode < 0) {
    dprintf("... error: directory '%s' not found\n", path);
      osErrno = E_NO_SUCH_DIR;
      return -1;
  }
 
  // load the disk sector containing the child inode
  int inode_sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
  char inode_buffer[SECTOR_SIZE];
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for child inode from disk sector %d\n", inode_sector);
 
  // get the child inode
  int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  int offset = child_inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* child = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
 
  // check for type
  if (child->type!=1) {
    dprintf("... error: wrong type, path leads to file\n");
    osErrno = E_GENERAL;
    return -1;
  }
 
  return child->size*sizeof(dirent_t);
}
 
int Dir_Read(char* path, void* buffer, int size)
{
  dprintf("Dir_Read('%s', buffer, %d):\n", path, size);
 
  // no empty path allowed
  if(path==NULL) {
    dprintf("... error: empty path (NULL) given as parameter\n");
    osErrno = E_GENERAL;
    return -1;
  }
 
  // directory has to exist
  int inode;
  int parent_inode = follow_path(path, &inode, NULL);
  if(parent_inode < 0) {
    dprintf("... error: directory '%s' not found\n", path);
      osErrno = E_NO_SUCH_DIR;
      return -1;
  }
 
  // check if size parameter matches the actual directory size
  int act_size = Dir_Size(path);
  if (size>act_size) {
    dprintf("... error: size parameter %d does not match actual directory size of %d bytes.\n",
      size, act_size);
    osErrno=E_GENERAL;
    return -1;
  }
 
  // check if size of buffer is large enough to hold all elements in the directory
  int buf_size = (int)malloc_usable_size(buffer);
  if (size>buf_size) {
    dprintf("... error: buffer provided has size %d, but %d bytes required\n",
      buf_size, size);
    osErrno=E_BUFFER_TOO_SMALL;
    return -1;
  }
 
  // initialize buffer
  bzero(buffer, size);
 
  // load disk sector from directory
  char inode_buffer[SECTOR_SIZE];
  int inode_sector = INODE_TABLE_START_SECTOR+inode/INODES_PER_SECTOR;
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for parent inode %d from disk sector %d\n",
     inode, inode_sector);
 
  // get the directory inode
  int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  int offset = inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* dir_inode = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
  dprintf("... get inode %d (size=%d, type=%d)\n",
     inode, dir_inode->size, dir_inode->type);
 
  // read the directory entries into the buffer
  int remainder=dir_inode->size%DIRENTS_PER_SECTOR;
  int group=dir_inode->size/DIRENTS_PER_SECTOR;
  int buf_offset = (int)(SECTOR_SIZE-SECTOR_SIZE%sizeof(dirent_t));
 
  // a) completely filled sectors
  for(int i=0; i<group;i++) {
    if(Disk_Read(dir_inode->data[i], buffer+i*buf_offset) < 0) {
        dprintf("... error: cant read sector %d\n", dir_inode->data[i]);
        osErrno=E_GENERAL;
        return -1;
    }
  }
 
  // b) partly filled sector
  if(remainder) {
      char buff[SECTOR_SIZE];
      if(Disk_Read(dir_inode->data[group], buff) < 0){
        dprintf("... error: cant read sector %d\n", dir_inode->data[group]);
            osErrno=E_GENERAL;
            return -1;
      }
 
    memcpy(buffer+group*buf_offset, buff, remainder*sizeof(dirent_t));
   
  }
 
  return dir_inode->size;
}