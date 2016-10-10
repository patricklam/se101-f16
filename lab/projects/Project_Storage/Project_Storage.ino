#include <stddef.h>
#include "defines.h"
#include "fileSystem.h"
#include "fileSystemMonitor.h"

void setup() 
{
  Serial.begin(BAUD_RATE);
  fileSystemInit();
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

  default:
    return;
  }

  // Presumably the file system has been modified. Re-render the display.
  FileSystemMonitorUpdate();
}
