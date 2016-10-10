#include <stddef.h>
#include "defines.h"
#include "File_System.h"
#include "File_System_Monitor.h"

void setup() 
{
  Serial.begin(BAUD_RATE);
  FileSystemInit();
  FileSystemMonitorInit();
  FileSystemMonitorUpdate();
}

void loop() 
{
  if(!Serial.available())
  {
    delayMicroseconds(USECONDS_PER_BYTE);
    return;
  }
  
  // Notice that rendering is *blocked* until the I/O operation
  // is complete. This is reasonable since the file system state 
  // should not be changing anyway!
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
    commandList();
    break;

  case 'd':
    commandDelete();
    break;

  case 'u':
    // Allocation table dump.
    for(int i = 0; (i < FileSystemAllocTableLength); ++i)
    {
      for(unsigned xi = 0; (xi < 8); xi++)
      {
        Serial.print((fileSystemActiveState.allocTable[i] >> xi) & 1, DEC);
      }
      delay(10);
      Serial.println();
    }
    return;

  default:
    return;
  }

  // Presumably the file system has been modified. Re-render the display.
  FileSystemMonitorUpdate();
}
