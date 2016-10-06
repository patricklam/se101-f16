#include <eeprom.h>
#include <hw_eeprom.h>

#include "fileSystem.h"

#define               FileSystemMaximumFileCount    18
#define               FileSystemMaximumBytes        512
#define               FileSystemAllocTableLength    FileSystemMaximumBytes / (4 * 8)

static const uint32_t FileSystemMagicCode         = 0xF17E0000;
static const uint32_t FileSystemTableBegin        = 8;
static const uint32_t FileSystemDataBegin         = FileSystemTableBegin + FileSystemMaximumFileCount * sizeof(uint32_t);


union FileSystemTable
{
  struct
  {
    uint8_t key       : 8;
    uint8_t size      : 8;
    uint8_t location  : 8;
    int8_t next       : 8;
  } entry[FileSystemMaximumFileCount];
  uint32_t raw[FileSystemMaximumFileCount];
};

static struct FileSystemState
{
  uint8_t                 fileCount;
  union FileSystemTable   fileTable;
  uint8_t                 fileFreeEntry;

  // Every Bit represents a Word (4 Bytes) in the File System
  // Helps us easily discover large contiguous regions of free space
  // without having to construct a complex data structure
  uint8_t                 allocTable[FileSystemAllocTableLength];
} fileSystemActiveState;

static void fileSystemCheck()
{
  uint32_t code = 0;
  int32_t freeList = -1;
  EEPROMRead(&code, 0, 4);
  if(code != FileSystemMagicCode)
  {
    fileSystemCreate();
  }
  EEPROMRead((uint32_t*)&freeList, 4, 4);

  Serial.println("Caching File System Table...");
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
      fileSystemActiveState.fileCount++;
    }
  }
  Serial.print("Found ");
  Serial.print(fileSystemActiveState.fileCount);
  Serial.println(" file(s)!");
}

void EEPROM_Init()
{
  EEPROMInit();
  fileSystemCheck();
}

void fileSystemCreate()
{
  Serial.println("Creating filesystem...");
  uint32_t value = FileSystemMagicCode;
  int8_t freeListHead = 0;
  EEPROMMassErase();
  EEPROMProgram(&value, 0, 4);
  EEPROMProgram((uint32_t*)&freeListHead, 4, 4);
  
  union FileSystemTable table;
  for(int i = 0; i < FileSystemMaximumFileCount; ++i)
  {
    table.raw[i] = 0;
    table.entry[i].next = i + 1;
  }
  table.entry[FileSystemMaximumFileCount-1].next = -1;
  EEPROMProgram(table.raw, FileSystemTableBegin, sizeof(union FileSystemTable));
}

bool fileSystemAddFile(uint8_t key, uint32_t * data, size_t count)
{
  if(!key)
    return false;
  if(fileSystemActiveState.fileFreeEntry < 0)
    return false;

  // Find a Location to place it in before we start modifying tables
  // using the allocation table (every bit is worth one word)
  uint8_t const bitsNeeded = (count+3)/4;
  uint8_t bitsFound = 0;
  uint8_t bitsFoundAt = 0;
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
  uint8_t location = FileSystemDataBegin + bitsFoundAt * 4;
  
  int useIndex = fileSystemActiveState.fileFreeEntry;
  fileSystemActiveState.fileFreeEntry = fileSystemActiveState.fileTable.entry[useIndex].next;

  fileSystemActiveState.fileTable.entry[useIndex].key = key;
  fileSystemActiveState.fileTable.entry[useIndex].size = count;
  fileSystemActiveState.fileTable.entry[useIndex].location = location;
  fileSystemActiveState.fileTable.entry[useIndex].next = -1;
  
  EEPROMProgram(data, location, ((count+3)/4) * 4);
  EEPROMProgram(fileSystemActiveState.fileTable.raw, FileSystemTableBegin, sizeof(union FileSystemTable));
  EEPROMProgram((uint32_t*)&fileSystemActiveState.fileFreeEntry, 4, 4);

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
      status->size = fileSystemActiveState.fileTable.entry[i].size;
      return true;
    }
  }
  return false;
}

