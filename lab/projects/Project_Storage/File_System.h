#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#ifdef __cplusplus
extern "C"
{
#endif
  
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

#define               FileSystemMaximumFileCount    18
#define               FileSystemMaximumBytes        1024
#define               FileSystemAllocTableLength    FileSystemMaximumBytes / (sizeof(uint32_t) * 8)

typedef struct File
{
  uint16_t    location;
  uint16_t    key;
  uint16_t    size;
} File;

struct FileSystemStatus
{
  size_t      wordsUsed;
  size_t      maxWords;
  size_t      fileCount;
  struct File fileStatus[FileSystemMaximumFileCount];
};

void FileSystemInit();
void FileSystemCreate();
void FileSystemCheck(bool forceCreate);
bool FileSystemStat(uint8_t key, File* status);
bool FileSystemAddFile(uint8_t key, uint32_t * data, size_t count);
bool FileSystemRemoveFile(uint8_t key);
int FileSystemRetrieve(uint8_t key, uint32_t * data, size_t bufferLength, uint8_t* readAmount);
void FileSystemStatus(struct FileSystemStatus *);

/**
 * File System Table Union
 * 
 * The File system table stores a linked list of nodes that either:
 *  1. Are Empty
 *  2. Store information about a particular file.
 * 
 * The linked list structure helps us find the free nodes that can be
 * reused for storing information about files.
 */
union __attribute__ ((packed)) FileSystemTable
{
  struct __attribute__ ((packed)) EntryType
  {
    uint16_t  location    : 16;
    uint16_t  key         : 16;
    uint16_t  size        : 16;
    int16_t   next        : 16;
  } entry[FileSystemMaximumFileCount];
  uint32_t raw[sizeof(struct EntryType) / 4];
};

/**
 * File System State Data Structure
 * Stores, in memory, the number of files, file entries, and next
 * free file entry block to use.
 * 
 * Also contains the allocation bit table which is useful for finding
 * a location to actually write the file!
 */
struct FileSystemState
{
  uint32_t                fileCount;
  union FileSystemTable   fileTable;
  int32_t                 fileFreeEntry;

  // Every Bit represents a Word (4 Bytes) in the File System
  // Helps us easily discover large contiguous regions of free space
  // without having to construct a complex data structure
  uint8_t                 allocTable[FileSystemAllocTableLength];
};

extern struct FileSystemState fileSystemActiveState;

#ifdef __cplusplus
}
#endif

#endif
