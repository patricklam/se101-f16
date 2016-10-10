#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <eeprom.h>
#include <hw_eeprom.h>

#include "File_System.h"

static const uint32_t FileSystemMagicCode         = 0xF17E0000;
static const uint32_t FileSystemTableBegin        = 8;
static const uint32_t FileSystemDataBegin         = 8 + sizeof(union FileSystemTable);

struct FileSystemState fileSystemActiveState;

static void fillAllocationTable(unsigned startBit, unsigned bitsLength, unsigned value);

void FileSystemStatus(struct FileSystemStatus * returnVal)
{
  static size_t bitCount[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

  if(!returnVal)
    return;
    
  returnVal->wordsUsed = 0;
  returnVal->maxWords = FileSystemAllocTableLength * 8;
  returnVal->fileCount = fileSystemActiveState.fileCount;
  
  for(unsigned i = 0; i < FileSystemAllocTableLength; ++i)
  {
    uint8_t alloc = fileSystemActiveState.allocTable[i];
    returnVal->wordsUsed += bitCount[alloc >> 4] + bitCount[alloc & 0xF];
  }

  unsigned eI = 0;
  for(unsigned i = 0; i < returnVal->fileCount; ++i)
  {
    while(0 == (fileSystemActiveState.fileTable.entry[eI].key)) 
    { 
      eI++;
    }
    
    returnVal->fileStatus[i].key = fileSystemActiveState.fileTable.entry[eI].key;
    returnVal->fileStatus[i].location = fileSystemActiveState.fileTable.entry[eI].location;
    returnVal->fileStatus[i].size = fileSystemActiveState.fileTable.entry[eI].size;
    eI++;
  }
}

/**
 * Writes the file systems active state to the EEPROM. This is so that
 * we can resume where we left off in case of a power off.
 */
static void FileSystemWriteCache()
{
  EEPROMProgram(fileSystemActiveState.fileTable.raw, FileSystemTableBegin, sizeof(union FileSystemTable));
  EEPROMProgram((uint32_t*)&fileSystemActiveState.fileFreeEntry, 4, 4);
}

void FileSystemCheck(bool forceCreate)
{
  uint32_t code = 0;
  int32_t freeList = -1;
  EEPROMRead(&code, 0, 4);
  if((code != FileSystemMagicCode) || forceCreate)
  {
    FileSystemCreate();
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
  
  for(unsigned i = 0; i < FileSystemMaximumFileCount; ++i)
  {
    if(0 != fileSystemActiveState.fileTable.entry[i].key)
    {
      unsigned location = fileSystemActiveState.fileTable.entry[i].location;
      unsigned count = fileSystemActiveState.fileTable.entry[i].size;
      fillAllocationTable((location - FileSystemDataBegin)/ sizeof(uint32_t), 
                          (count + (sizeof(uint32_t)-1)) / sizeof(uint32_t),
                          1);
      fileSystemActiveState.fileCount++;
    }
  }
}

void FileSystemInit()
{
  EEPROMInit();
  delay(100);
  FileSystemCheck(false);
}

void FileSystemCreate()
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

int FileSystemRetrieve(uint8_t key, uint32_t * data, size_t bufferLength, uint8_t* readAmount)
{
  File stat;
  if(!FileSystemStat(key, &stat))
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

bool FileSystemRemoveFile(uint8_t key)
{
  File stat;
 
 if(!FileSystemStat(key, &stat)) { return false; }

  for(unsigned i = 0; i < FileSystemMaximumFileCount; ++i)
  {
    if(fileSystemActiveState.fileTable.entry[i].key == key)
    {
      fileSystemActiveState.fileTable.entry[i].key = 0;
      fileSystemActiveState.fileTable.entry[i].size = 0;
      fileSystemActiveState.fileTable.entry[i].location = 0;
      fileSystemActiveState.fileTable.entry[i].next = fileSystemActiveState.fileFreeEntry;
      fileSystemActiveState.fileFreeEntry = i;
      fileSystemActiveState.fileCount--;

      fillAllocationTable((stat.location - FileSystemDataBegin) / sizeof(uint32_t),
                          (stat.size + (sizeof(uint32_t)-1)) / sizeof(uint32_t), 
                          0);

      FileSystemWriteCache();
      return true;
    }
  }
  return false;
}

bool FileSystemAddFile(uint8_t key, uint32_t * data, size_t count)
{
  File stat;
  if(!key ||
     (fileSystemActiveState.fileFreeEntry < 0) ||
     FileSystemStat(key, &stat) )
  {
    return false;
  }
  
  // Find a Location to place it in before we start modifying tables
  // using the allocation table (every bit is worth one word)
  unsigned int const bitsNeeded = (count+(sizeof(uint32_t)-1))/sizeof(uint32_t);
  unsigned int bitsFoundAt = 0;
  unsigned int bitsFound = 0;
  for(unsigned i = 0; i < FileSystemAllocTableLength; ++i)
  {
    uint8_t bite = fileSystemActiveState.allocTable[i];
    for(unsigned xi = 0; xi < 8; xi++)
    {
      if((bite >> xi) & 1)
      {
        bitsFoundAt = 8*i + xi + 1;
        bitsFound = 0;
        continue;
      }
      
      if( ++bitsFound >= bitsNeeded )
      {
        uint16_t location = FileSystemDataBegin + bitsFoundAt * sizeof(uint32_t);

        int useIndex = fileSystemActiveState.fileFreeEntry;

        fileSystemActiveState.fileFreeEntry = fileSystemActiveState.fileTable.entry[useIndex].next;
        fileSystemActiveState.fileTable.entry[useIndex].key = key;
        fileSystemActiveState.fileTable.entry[useIndex].size = count;
        fileSystemActiveState.fileTable.entry[useIndex].location = location;
        fileSystemActiveState.fileTable.entry[useIndex].next = -1;
        fileSystemActiveState.fileCount++;
        fillAllocationTable(bitsFoundAt, bitsNeeded, 1);
        
        EEPROMProgram(data, location, bitsNeeded * sizeof(uint32_t));
        FileSystemWriteCache();
        return true;
      }
    }
  }
  return false;
}

bool FileSystemStat(uint8_t key, File* status)
{
  if(!key)
    return false;

  for(unsigned i = 0; i < FileSystemMaximumFileCount; ++i)
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

static void fillAllocationTable(unsigned startBit, unsigned bitsLength, unsigned value)
{
  // Fix the allocation table
  unsigned startByte = startBit / 8;
  unsigned xi = startBit % 8;
  unsigned mask = value ? 1 : 0;
  
  for(unsigned i = startByte; bitsLength && (i < FileSystemAllocTableLength); ++i, xi = 0)
  {
    for(; bitsLength && (xi < 8); xi++, --bitsLength)
    {
      if(mask)
      {
        fileSystemActiveState.allocTable[i] |= (1 << xi);
      }
      else
      {
        fileSystemActiveState.allocTable[i] &= ~(1 << xi);
      }
    }
  }
}
