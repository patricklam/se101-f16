#include <eeprom.h>
#include <hw_eeprom.h>

#include "fileSystem.h"

static const uint32_t FileSystemMagicCode         = 0xF17E0000;
static const uint32_t FileSystemTableBegin        = 8;
static const uint32_t FileSystemDataBegin         = FileSystemTableBegin + sizeof(union FileSystemTable);

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
  uint32_t                fileFreeEntry;

  // Every Bit represents a Word (4 Bytes) in the File System
  // Helps us easily discover large contiguous regions of free space
  // without having to construct a complex data structure
  uint8_t                 allocTable[FileSystemAllocTableLength];
} fileSystemActiveState;

/**
 * Writes the file systems active state to the EEPROM. This is so that
 * we can resume where we left off in case of a power off.
 */
static void fileSystemWriteCache()
{
  EEPROMProgram(fileSystemActiveState.fileTable.raw, FileSystemTableBegin, sizeof(union FileSystemTable));
  EEPROMProgram((uint32_t*)&fileSystemActiveState.fileFreeEntry, 4, 4);
}

void fileSystemCheck(bool forceCreate)
{
  uint32_t code = 0;
  int32_t freeList = -1;
  EEPROMRead(&code, 0, 4);
  if((code != FileSystemMagicCode) || forceCreate)
  {
    fileSystemCreate();
  }
  EEPROMRead((uint32_t*)&freeList, 4, 4);

  // Cache file system table in memory!
  EEPROMRead( fileSystemActiveState.fileTable.raw, 
              FileSystemTableBegin, 
              sizeof(union FileSystemTable));

  // Cache filesystem properties
  fileSystemActiveState.fileCount = 0;
  fileSystemActiveState.fileFreeEntry = freeList;
  memset(fileSystemActiveState.allocTable, 0, FileSystemAllocTableLength);
  for(int i = 0; i < FileSystemMaximumFileCount; ++i)
  {
    if(fileSystemActiveState.fileTable.entry[i].key)
    {
      int location = fileSystemActiveState.fileTable.entry[i].location;
      int count = fileSystemActiveState.fileTable.entry[i].size;
      int startByte = ((location - FileSystemDataBegin)/ 4) / 32;
      int startBit = ((location - FileSystemDataBegin)/ 4) % 32;
      int bitCount = (count + 3) / 4;
      int xi = startBit;
      for(int i = startByte; i < FileSystemAllocTableLength; ++i)
      {
        for(; xi < 32; xi++)
        {
          bitWrite(fileSystemActiveState.allocTable[i], xi, 1);
        }
        xi = 0;
      }
      fileSystemActiveState.fileCount++;
    }
  }
}

void EEPROM_Init()
{
  EEPROMInit();
  delay(500);
  fileSystemCheck();
}

void fileSystemCreate()
{
  uint32_t value = FileSystemMagicCode;
  int32_t freeListHead = 0;
  EEPROMMassErase(); 
  EEPROMProgram(&value, 0, 4);
  EEPROMProgram((uint32_t*)&freeListHead, 4, 4);
  
  union FileSystemTable table;
  memset(table.raw, 0, sizeof(union FileSystemTable));
  for(int i = 0; i < FileSystemMaximumFileCount; ++i)
  {
    table.entry[i].next = i + 1;
  }
  table.entry[FileSystemMaximumFileCount-1].next = -1;
  EEPROMProgram(table.raw, FileSystemTableBegin, sizeof(union FileSystemTable));
}

int fileSystemRetrieve(uint8_t key, uint32_t * data, size_t bufferLength, uint8_t* readAmount)
{
  File stat;
  if(!fileSystemStat(key, &stat))
  {
    return 1;
  }
  else if(stat.size > bufferLength)
  {
    return 2;
  }
  EEPROMRead(data, stat.location, 4*((stat.size + 3) / 4));
  *readAmount = stat.size;
  return 0;
}

void fileSystemRemoveFile(uint8_t key)
{
  File stat;
 
 if(!fileSystemStat(key, &stat)) { return; }

  int i = 0;
  for(; i < FileSystemMaximumFileCount; ++i)
  {
    if(fileSystemActiveState.fileTable.entry[i].key == key)
    {
      goto FoundFile;
    }
  }
  return;

 FoundFile:
  int useIndex = i;
  int location = stat.location;
  int count = stat.size;
  
  // Erase the appropriate locations in teh allocation table.
  int startByte = ((location - FileSystemDataBegin)/ 4) / 32;
  int startBit = ((location - FileSystemDataBegin)/ 4) % 32;
  int bitCount = (count + 3) / 4;
  int xi = startBit;
  for(int i = startByte; i < FileSystemAllocTableLength; ++i)
  {
    for(; xi < 32; xi++)
    {
      bitWrite(fileSystemActiveState.allocTable[i], xi, 0);
    }
    xi = 0;
  }

  fileSystemActiveState.fileTable.entry[useIndex].key = 0;
  fileSystemActiveState.fileTable.entry[useIndex].size = 0;
  fileSystemActiveState.fileTable.entry[useIndex].location = 0;
  fileSystemActiveState.fileTable.entry[useIndex].next = fileSystemActiveState.fileFreeEntry;
  fileSystemActiveState.fileFreeEntry = useIndex;
  fileSystemActiveState.fileCount--;
  
  fileSystemWriteCache();
}

bool fileSystemAddFile(uint8_t key, uint32_t * data, size_t count)
{
  File stat;
  if(!key ||
     (fileSystemActiveState.fileFreeEntry < 0) ||
     fileSystemStat(key, &stat) )
  {
    return false;
  }
  
  // Find a Location to place it in before we start modifying tables
  // using the allocation table (every bit is worth one word)
  unsigned int const bitsNeeded = (count+3)/4;
  unsigned int bitsFoundAt = 0;
  unsigned int bitsFound = 0;
  for(int i = 0; i < FileSystemAllocTableLength; ++i)
  {
    for(int xi = 0; xi < 32; xi++)
    {
      if(!bitRead(fileSystemActiveState.allocTable[i], xi))
      {
        if( ++bitsFound >= bitsNeeded )
        {
          goto FoundSpace;
        }
      }
      else
      {
        bitsFoundAt = 32*i + xi + 1;
        bitsFound = 0;
      }
    }
  }
  return false;
  
 FoundSpace:
  uint16_t location = FileSystemDataBegin + bitsFoundAt * 4;
  
  int useIndex = fileSystemActiveState.fileFreeEntry;
  fileSystemActiveState.fileFreeEntry = fileSystemActiveState.fileTable.entry[useIndex].next;

  fileSystemActiveState.fileTable.entry[useIndex].key = key;
  fileSystemActiveState.fileTable.entry[useIndex].size = count;
  fileSystemActiveState.fileTable.entry[useIndex].location = location;
  fileSystemActiveState.fileTable.entry[useIndex].next = -1;
  fileSystemActiveState.fileCount++;
  
  EEPROMProgram(data, location, ((count+3)/4) * 4);
  fileSystemWriteCache();

  // Fix the allocation table
  int startByte = ((location - FileSystemDataBegin)/ 4) / 32;
  int startBit = ((location - FileSystemDataBegin)/ 4) % 32;
  int bitCount = (count + 3) / 4;
  int xi = startBit;
  for(int i = startByte; i < FileSystemAllocTableLength; ++i)
  {
    for(; xi < 32; xi++)
    {
      bitWrite(fileSystemActiveState.allocTable[i], xi, 1);
    }
    xi = 0;
  }
  return true;
}

bool fileSystemStat(uint8_t key, File* status)
{
  if(!key)
    return false;

  for(int i = 0; i < FileSystemMaximumFileCount; ++i)
  {
    if(fileSystemActiveState.fileTable.entry[i].key == key)
    {
      status->key = key;
      status->location = fileSystemActiveState.fileTable.entry[i].location;
      status->size = fileSystemActiveState.fileTable.entry[i].size;
      return true;
    }
  }
  return false;
}

