#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>

typedef struct File
{
  uint16_t    location;
  uint16_t    key;
  uint16_t    size;
} File;

void EEPROM_Init();
void fileSystemCreate();
void fileSystemCheck(bool forceCreate = false);
bool fileSystemStat(uint8_t key, File* status);
bool fileSystemAddFile(uint8_t key, uint32_t * data, size_t count);
void fileSystemRemoveFile(uint8_t key);
int fileSystemRetrieve(uint8_t key, uint32_t * data, size_t bufferLength, uint8_t* readAmount);


#define               FileSystemMaximumFileCount    18
#define               FileSystemMaximumBytes        512
#define               FileSystemAllocTableLength    FileSystemMaximumBytes / (sizeof(uint32_t) * 8)

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
  struct __attribute__ ((packed))
  {
    uint16_t location   : 16;
    uint16_t key        : 16;
    uint16_t size       : 16;
    int16_t next        : 16;
  } entry[FileSystemMaximumFileCount];
  uint32_t raw[sizeof(entry) / 4];
};


#endif
