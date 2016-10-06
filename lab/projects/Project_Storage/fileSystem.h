#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdbool.h>

typedef struct File
{
  uint8_t key;
  uint8_t size;
} File;

void EEPROM_Init();
void fileSystemCreate();
bool fileSystemStat(uint8_t key, File* status);

#endif
