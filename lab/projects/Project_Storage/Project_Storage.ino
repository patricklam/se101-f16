#include <stddef.h>
#include "fileSystem.h"

void setup() 
{
  Serial.begin(9600);
  delay(1000);
  
  EEPROM_Init();
  delay(1000);
}

void commandStore()
{
  while( 0 == Serial.available());
  uint8_t key = Serial.read();
  
  while( 0 == Serial.available());
  uint8_t const amount = Serial.read();
  
  uint32_t buffer[256];

  while( 0 == Serial.available());
  size_t amountRead = Serial.readBytes((char*)buffer, amount);
  
  if(!amountRead)
  {
    Serial.write((byte)1);
    return;
  }
  
  if(!fileSystemAddFile(key, buffer, amount))
  {
    Serial.write((byte)2);
    return;
  }

  Serial.write((byte)0);
}

void commandRetrieve()
{
  while(0 == Serial.available());
  uint8_t key = Serial.read();
  
  uint32_t buffer[256];
  byte amount = 0;
  byte errorCode = fileSystemRetrieve(key, buffer, 256*sizeof(uint32_t), &amount);

  Serial.write(&amount, 1);
  if(amount)
  {
    Serial.write((uint8_t*)buffer, amount);
  }
  Serial.write(&errorCode, 1);
}

void commandReset()
{
  fileSystemCheck(true);
}

void loop() 
{
  while(0 == Serial.available());
  
  char command = Serial.read();
  switch(command)
  {
  case 's':
    commandStore();
    break;
    
  case 'r':
    commandRetrieve();
    break;

  case 'e':
    commandReset();
    break;

  case 'l':
    break;
    
  default:
    break;
  }
}
