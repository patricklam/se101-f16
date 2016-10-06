#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>

typedef struct File
{
  uint16_t  location;
  uint8_t   key;
  uint8_t   size;
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
#define               FileSystemAllocTableLength    FileSystemMaximumBytes / (4 * 8)

union __attribute__ ((packed)) FileSystemTable
{
  struct __attribute__ ((packed))
  {
    uint16_t location   : 16;
    uint8_t key         : 8;
    uint8_t size        : 8;
    int8_t next         : 8;
  } entry[FileSystemMaximumFileCount];
  uint32_t raw[FileSystemMaximumFileCount];
};


#endif
