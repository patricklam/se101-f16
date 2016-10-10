#include <FillPat.h>
#include <LaunchPad.h>
#include <OrbitBoosterPackDefs.h>
#include <OrbitOled.h>
#include <OrbitOledChar.h>
#include <OrbitOledGrph.h>
#include "fileSystem.h"

static void drawUsageBar();

void FileSystemMonitorInit()
{
  OrbitOledInit();
  OrbitOledClear();
  OrbitOledClearBuffer();
}

void FileSystemMonitorUpdate()
{
  OrbitOledClear();
  OrbitOledClearBuffer();

  drawUsageBar();
  
  OrbitOledUpdate();
}

static void drawUsageBar()
{
  char strFileCount[10];
  struct FileSystemStatus stat = fileSystemStatus();
  sprintf(strFileCount, "NUM:%02d", stat.fileCount);
  
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("USE:[");
  for(int i = 0; i < 10; ++i) 
  {
    if(i < (10*stat.wordsUsed)/stat.maxWords)
      OrbitOledDrawChar('#');
    else
      OrbitOledDrawChar('-');
  }
  OrbitOledDrawString("]");
  
  OrbitOledMoveTo(0, 10);
  OrbitOledDrawString(strFileCount);
}

