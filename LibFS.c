#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "LibDisk.h"
#include "LibFS.h"

// set to 1 to have detailed debug print-outs and 0 to have none
#define FSDEBUG 0

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

// initialize a bitmap with 'num' sectors starting from 'start'
// sector; all bits should be set to zero except that the first
// 'nbits' number of bits are set to one
static void bitmap_init(int start, int num, int nbits)
{
  /* YOUR CODE */
}

// set the first unused bit from a bitmap of 'nbits' bits (flip the
// first zero appeared in the bitmap to one) and return its location;
// return -1 if the bitmap is already full (no more zeros)
static int bitmap_first_unused(int start, int num, int nbits)
{
  /* YOUR CODE */
  return -1;
}

// reset the i-th bit of a bitmap with 'num' sectors starting from
// 'start' sector; return 0 if successful, -1 otherwise
static int bitmap_reset(int start, int num, int ibit)
{
  /* YOUR CODE */
  return -1;
}

// return 1 if the file name is illegal; otherwise, return 0; legal
// characters for a file name include letters (case sensitive),
// numbers, dots, dashes, and underscores; and a legal file name
// should not be more than MAX_NAME-1 in length
static int illegal_filename(char* name)
{
  // Check if name is non empty or too long
  if(name==NULL || strlen(name)==0 || strlen(name)>=MAX_NAME-1){ return 1; }
  
  // Check if entries only contain allowed elements
  for(int i=0; i < strlen(name);i++){
  	if(!(isalpha(name[i]) || isdigit(name[i]) || 
  		name[i] == '.' || name[i] == '_' || name[i] == '-' )){ return 1; }
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
  char dirent_buffer[SECTOR_SIZE];
  if(group*DIRENTS_PER_SECTOR == parent->size) {
    // new disk sector is needed
    int newsec = bitmap_first_unused(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, SECTOR_BITMAP_SIZE);
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
 
  // check for remaining dirents from parent in that sector (otherwise reset sector bitmap)
  int remainder = parent->size % DIRENTS_PER_SECTOR;
  int group = parent->size/DIRENTS_PER_SECTOR;
  if (remainder == 1) {
    // disk sector has to be freed
    if (bitmap_reset(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, parent->data[group]) < 0) {
    dprintf("... error: free sector in bitmap unsuccessful\n");
    return -1;
    }
    parent->data[group] = 0;
  }
 
  // delete the dirent and write to disk
  char dirent_buffer[SECTOR_SIZE];
  if(Disk_Read(parent->data[group], dirent_buffer) < 0) return -1;
    dprintf("... load disk sector %d for dirent group %d\n", parent->data[group], group);
  int start_entry = group*DIRENTS_PER_SECTOR;
  offset = (parent->size-1)-start_entry;
  dirent_t* dirent = (dirent_t*)(dirent_buffer+offset*sizeof(dirent_t));
  memset(dirent->fname, 0, sizeof(dirent->fname));
  dirent->inode = -1;
  if(Disk_Write(parent->data[group], dirent_buffer) < 0) return -1;
  dprintf("... delete dirent %d (inode=%d) from group %d, update disk sector %d\n",
    parent->size, child_inode, group, parent->data[group]);
 
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
  int pos;   // read/write position
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
      bitmap_init(INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS, 1);
      dprintf("... formatted inode bitmap (start=%d, num=%d)\n",
	     (int)INODE_BITMAP_START_SECTOR, (int)INODE_BITMAP_SECTORS);
      
      // format sector bitmap (reserve the first few sectors to
      // superblock, inode bitmap, sector bitmap, and inode table)
      bitmap_init(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS,
		  DATABLOCK_START_SECTOR);
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
  dprintf("File_Open('%s'):\n", file);
  int fd = new_file_fd();
  if(fd < 0) {
    dprintf("... max open files reached\n");
    osErrno = E_TOO_MANY_OPEN_FILES;
    return -1;
  }

  int child_inode;
  follow_path(file, &child_inode, NULL);
  if(child_inode >= 0) { // child is the one
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
    return fd;
  } else {
    dprintf("... file '%s' is not found\n", file);
    osErrno = E_NO_SUCH_FILE;
    return -1;
  }  
}

int File_Read(int fd, void* buffer, int size)
{
    //check if fd is valid index
    if(fd < 0 || fd > MAX_OPEN_FILES){
	dprintf("... fd=%d out of bound", fd);    
    	osErrno = E_BAD_FD;
	return -1;
    }

    open_file_t file = open_files[fd];

    //check if not an open file
    if(file.inode <= 0){
        dprintf("... fd=%d not an open file\n", fd);
        osErrno = E_BAD_FD;
        return -1;
    }
  int position = file.pos;  
  int remReadSize = 0;

  //check if file size is empty
  if(file.size == 0)
  {
	//none to read  
	return 0;
  }

  //determine how many bytes to read
  if(file.size - position == 0)
  {
      //none to read		  
      return 0;
  }
  else{
      //if less than size is remaining, read only remaining
      //else pick size
      remReadSize = min(file.size - position, size);
  	
  }
  memset(buffer, 0, remReadSize);


  /***read contents of data blocks***/
  
  //load file's inode disk sectors
  int inode = file.inode;
  int inode_sector = INODE_TABLE_START_SECTOR+inode/INODES_PER_SECTOR;
  char inode_buffer[SECTOR_SIZE];
  if(Disk_Read(inode_sector, inode_buffer) < 0) { osErrno = E_GENERAL; return -1; }
    dprintf("... load inode table for inode from disk sector %d\n", inode_sector);

  //get inode
   int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
    int offset = inode-inode_start_entry;
    assert(0 <= offset && offset < INODES_PER_SECTOR);
    inode_t* fileInode = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
    dprintf("... inode %d (size=%d, type=%d)\n",
	    inode, fileInode->size, fileInode->type);
    
    //FILES_PER_SECTOR (SECTOR_SIZE/SIZEOF(open_file_t))
   int i = 0;
   //read data from given datablock pointers
   for(i = file.pos; i < remReadSize; i++){
     if(Disk_Read(fileInode->data[i], (char*)buffer+i*SECTOR_SIZE) < 0){
        dprintf("... error: cant read sector %d\n", fileInode->data[i]);
        osErrno=E_GENERAL;
        return -1;	     
     }
   }

   File_Seek(fd, i);

  
  return remReadSize;
}

int File_Write(int fd, void* buffer, int size)
{
  //check if fd is valid index
    if(fd < 0 || fd > MAX_OPEN_FILES){
        dprintf("... fd=%d out of bound", fd);
        osErrno = E_BAD_FD;
        return -1;
    }

    open_file_t file = open_files[fd];

    //check if not an open file
    if(file.inode <= 0){
        dprintf("... fd=%d not an open file\n", fd);
        osErrno = E_BAD_FD;
        return -1;
    }

    //check if extra data doesn't go over max sectors per file
    if(file.pos + size > MAX_SECTORS_PER_FILE){
	dprintf("... fd=%d cannot add %d data sectors. No more space\n", fd, size);
	osErrno = E_FILE_TOO_BIG;
    }
  
   //load file's inode disk sectors
  int inode = file.inode;
  int inode_sector = INODE_TABLE_START_SECTOR+inode/INODES_PER_SECTOR;
  char inode_buffer[SECTOR_SIZE];
  if(Disk_Read(inode_sector, inode_buffer) < 0) { osErrno = E_GENERAL; return -1; }
    dprintf("... load inode table for inode from disk sector %d\n", inode_sector);

  //get inode
   int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
    int offset = inode-inode_start_entry;
    assert(0 <= offset && offset < INODES_PER_SECTOR);
    inode_t* fileInode = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
    dprintf("... inode %d (size=%d, type=%d)\n",
            inode, fileInode->size, fileInode->type);

   //write data from after the last occupied data pointer
   int i = 0;
   for(i = file.pos; i < size; i++){
      if(Disk_Write(fileInode->data[i], (char*)buffer+i*SECTOR_SIZE) < 0) {
          dprintf("... failed to write buffer data\n");
          osErrno = E_GENERAL;
          return -1;
      }
   /***TODO: need to check if write cannot complete due to lack of space on disk. how?***/	  
   }

   //set new read/write position   
   File_Seek(fd, i);   

  return size;
}

int File_Seek(int fd, int offset)
{
  //check if fd is valid index
  if(fd < 0 || fd > MAX_OPEN_FILES){
    dprintf("... fd=%d out of bound\n", fd);
    osErrno = E_BAD_FD;
    return -1;    
  }
  //check if open file
 if(open_files[fd].inode <= 0) {
    dprintf("... fd=%d not an open file\n", fd);
    osErrno = E_BAD_FD;
    return -1;
  }
    
  //check if offset is within bounds
  if(offset > open_files[fd].size || offset == -1){
     dprintf("... offset=%d out of bound\n", fd);
     osErrno = E_SEEK_OUT_OF_BOUNDS;
    return -1; 
  }

  open_files[fd].pos = offset;

  return open_files[fd].pos;  
}

int File_Close(int fd)
{
  dprintf("File_Close(%d):\n", fd);
  if(0 > fd || fd > MAX_OPEN_FILES) {
    dprintf("... fd=%d out of bound\n", fd);
    osErrno = E_BAD_FD;
    return -1;
  }
  if(open_files[fd].inode <= 0) {
    dprintf("... fd=%d not an open file\n", fd);
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
  int real_size = Dir_Size(path);
  if(real_size<0) return -1;
  if(real_size==0) return 0;
 
  // check if size of buffer is large enough to hold all elements in the directory
  if (size<real_size) {
    osErrno=E_BUFFER_TOO_SMALL;
    return -1;
  }
 
  // initialize buffer
  memset(buffer, 0, size);
 
  // load disk sector from directory
  int inode;
  char inode_buffer[SECTOR_SIZE];
  int parent_inode=follow_path(path, &inode, NULL);
  if(parent_inode<0) return -1;
  int inode_sector = INODE_TABLE_START_SECTOR+inode/INODES_PER_SECTOR;
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for parent inode %d from disk sector %d\n",
     inode, inode_sector);
 
  // get the parent inode
  int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  int offset = inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* dir_inode = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
  dprintf("... get inode %d (size=%d, type=%d)\n",
     inode, dir_inode->size, dir_inode->type);
 
  // read the directory entries into the buffer
  int remainder=dir_inode->size%DIRENTS_PER_SECTOR;
  int group=dir_inode->size/DIRENTS_PER_SECTOR;
 
  // a) completely filled sectors
  for(int i=0; i<group;i++) {
    if(Disk_Read(dir_inode->data[i], (char*)buffer+i*SECTOR_SIZE) < 0) {
        dprintf("... error: cant read sector %d\n", dir_inode->data[i]);
        osErrno=E_GENERAL;
        return -1;
    }
  }
 
  // b) partly filled sector
  if(remainder) {
      if(Disk_Read(dir_inode->data[group], inode_buffer) < 0){
        dprintf("... error: cant read sector %d\n", dir_inode->data[group]);
            osErrno=E_GENERAL;
            return -1;
      }
      strncpy((char*)buffer+group*SECTOR_SIZE, inode_buffer, remainder*sizeof(dirent_t));
  }
 
  return dir_inode->size;
}


