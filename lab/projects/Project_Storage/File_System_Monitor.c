#include <FillPat.h>
#include <LaunchPad.h>
#include <OrbitBoosterPackDefs.h>
#include <OrbitOled.h>
#include <OrbitOledChar.h>
#include <OrbitOledGrph.h>
#include "File_System.h"

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
  char strFileCount[7 + 1];
  struct FileSystemStatus stat;

  FileSystemStatus(&stat);
  sprintf(strFileCount, "NUM:%03d", stat.fileCount);
  
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

