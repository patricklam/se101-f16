#include <stddef.h>
#include "fileSystem.h"

uint32_t dummyData[10] = { 0xCAFEBABE };

void setup() 
{
  Serial.begin(9600);
  delay(1000);
  EEPROM_Init();

  fileSystemAddFile(10, dummyData, 10 * sizeof(int));
  fileSystemAddFile(11, dummyData, 10 * sizeof(int));
  
  File fileStat = { 0 };
  if( fileSystemStat(10, &fileStat) )
  {
    Serial.println(fileStat.key, DEC);
    Serial.println(fileStat.size, DEC);
  }
}

void commandStore()
{
  uint8_t key = Serial.read();
  uint8_t amount = Serial.read();
  
}

void commandRetrieve()
{
  uint8_t key = Serial.read();
}


void loop() 
{
  
  uint8_t buffer[64] = { 0 };
  uint8_t output[64] = { 0 };

  if(Serial.available())
  {
    char command = Serial.read();
    switch(command)
    {
    case 's':
      commandStore();
      break;
      
    case 'r':
      commandRetrieve();
      break;
      
    default:
      break;
    }
  }
}
