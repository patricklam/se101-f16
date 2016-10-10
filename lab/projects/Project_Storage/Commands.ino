#include <inttypes.h>
#include <HardwareSerial.h>
#include "defines.h"
#include "fileSystem.h"

static void waitOnSerialInput()
{
  // Busy wait for a byte.
  while( 0 == Serial.available() )
  {
    // 9600 bit/s = 960 B/s => 1000/960 ms/B = ~1.04167 ms/B = ~1041 us/B
    delayMicroseconds(USECONDS_PER_BYTE);
  }
}

void commandStore()
{
  waitOnSerialInput();
  uint8_t key = Serial.read();
  
  waitOnSerialInput();
  uint8_t const amount = Serial.read();
  
  uint32_t buffer[256];

  waitOnSerialInput();
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
  waitOnSerialInput();
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

