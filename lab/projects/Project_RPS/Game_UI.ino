#include <stdbool.h>
#include <string.h>

static enum GamePages
{
  Welcome       = 0,
  SelectPlayers = 1,
  RollPlayer    = 2,
  GameResult    = 3,
  Progress      = 4,
  NumberOfPages = 5,
} gameUiPage = Welcome;

const uint32_t SwitchCount = 2;
const uint32_t ButtonCount = 2;
const uint32_t Switches[SwitchCount] = { PA_7, PA_6 };
const uint32_t Buttons[ButtonCount] = { PD_2, PE_0 };
const uint32_t Potentiometer = PE_3;
const size_t   MaximumPlayers = 6;

struct ButtonState
{ 
  bool state;
  bool isRising;
};

typedef enum Action
{
  Rock    = 0,
  Paper   = 1,
  Scissor = 2,
  ActionCount = 3,
} Action;

struct GameState
{
  int playerCount;
  int playerIndex;
  enum Action playerActions[MaximumPlayers];
  bool isRolling;
} activeGame;

static struct InputState
{
  bool                switches[2];
  struct ButtonState  buttons[2];
  float               potentiometer;
} gameInputState;

void GameUIInit()
{
  OrbitOledInit();
  OrbitOledClear();
  OrbitOledClearBuffer();
  OrbitOledSetFillPattern(OrbitOledGetStdPattern(iptnSolid));
  OrbitOledSetDrawMode(modOledSet);

  gameInputState = { 0 };
  activeGame = { 0 };

  for(int i = 0; i < SwitchCount; ++i )
    pinMode(Switches[i], INPUT);
  for(int i = 0; i < ButtonCount; ++i )
    pinMode(Buttons[i], INPUT);
}

static void handlePageProgressBar()
{
  static uint32_t progress = 0;
  if(0 == (progress = (1+progress) % 100))
  {
    OrbitOledClear();
  }
  OrbitOledMoveTo(30, 24);
  OrbitOledDrawString("Loading...");
  
  OrbitOledMoveTo(14, 14);
  OrbitOledSetFillPattern(OrbitOledGetStdPattern(iptnBlank));
  OrbitOledSetDrawMode(modOledAnd);
  OrbitOledFillRect(14 + progress, 16);
  
  OrbitOledMoveTo(14 + progress, 14);
  OrbitOledSetFillPattern(OrbitOledGetStdPattern(iptnSolid));
  OrbitOledSetDrawMode(modOledSet);
  OrbitOledFillRect(14 + progress + 10, 16);
}

static void handlePageRollPlayer()
{
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("Player ");
  OrbitOledDrawChar('1' + activeGame.playerIndex);
  OrbitOledDrawString(" - Shake!");
  
  if(ShakeIsShaking())
  {
    activeGame.isRolling = true;
  }
  else if(activeGame.isRolling)
  {
    delay(1000);
    activeGame.isRolling = false;
    activeGame.playerActions[activeGame.playerIndex++] = (enum Action)(rand() % ActionCount);
  }

  if(activeGame.playerIndex == activeGame.playerCount)
  {
    OrbitOledClearBuffer();
    OrbitOledClear();
    activeGame.playerIndex = 0;
    gameUiPage = GameResult;
  }
}

static void handlePageSelectPlayers()
{
  OrbitOledMoveTo(5, 0);
  OrbitOledDrawString("Use Switches For");
  
  OrbitOledMoveTo(10, 15);
  OrbitOledDrawString("# of Players");

  if(gameInputState.buttons[0].isRising)
  {
    OrbitOledClearBuffer();
    OrbitOledClear();
    activeGame.playerCount = 2 * (gameInputState.switches[1] ? 1 : 0) + 
                             1 * (gameInputState.switches[0] ? 1 : 0) + 
                             2;
    gameUiPage = RollPlayer;
  }
}

static void handlePageWelcome()
{
  OrbitOledMoveTo(5, 0);
  OrbitOledDrawString("Welcome to RPS");
  
  OrbitOledMoveTo(10, 15);
  OrbitOledDrawString("Press BTN1");

  if(gameInputState.buttons[0].isRising)
  {
    OrbitOledClearBuffer();
    OrbitOledClear();
    gameUiPage = SelectPlayers;
  }
}

static void uiInputTick()
{
  for(int i = 0; i < SwitchCount; ++i )
    gameInputState.switches[i] = digitalRead(Switches[i]);
  for(int i = 0; i < ButtonCount; ++i )
  {
    // Only look for Rising Edge Signals.
    bool previousState = gameInputState.buttons[i].state;
    gameInputState.buttons[i].state = digitalRead(Buttons[i]);
    gameInputState.buttons[i].isRising = (!previousState && gameInputState.buttons[i].state);
  }
  gameInputState.potentiometer = analogRead(Potentiometer);
}


bool rpsDidILose(int a, int b)
{
  return (a < b) || (a == Scissor && b == Rock);
}

static void handlePageGameResult()
{
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("Results");

  // Find a loser.
  int lossCount[activeGame.playerCount];
  memset(lossCount, 0, sizeof(int) * activeGame.playerCount);
  for(int i = 0; i < activeGame.playerCount; ++i)
  {
    for(int j = i; j < activeGame.playerCount; ++j)
    {
      lossCount[i] += rpsDidILose(activeGame.playerActions[i], activeGame.playerActions[j]) ? 1 : 0;
      lossCount[j] += rpsDidILose(activeGame.playerActions[j], activeGame.playerActions[i]) ? 1 : 0;
    }
  }
  int maxIndex = 0, maxValue = lossCount[0];
  for(int i = 1; i < activeGame.playerCount; ++i)
  {
    if( maxValue < lossCount[i] )
    {
      maxIndex = i;
      maxValue = lossCount[i];
    }
  }
  
  OrbitOledMoveTo(0, 10);
  OrbitOledDrawChar('P');
  OrbitOledDrawChar('1' + maxIndex);
  OrbitOledDrawString(" lost.");
  
  OrbitOledMoveTo(0, 20);
  for(int i = 0; i < activeGame.playerCount; ++i)
  {
    switch(activeGame.playerActions[i])
    {
    case Rock:
      OrbitOledDrawChar('R');
      break;
    case Paper:
      OrbitOledDrawChar('P');
      break;
    case Scissor:
      OrbitOledDrawChar('S');
      break;
    }
  }
  
  if(gameInputState.buttons[0].isRising)
  {
    OrbitOledClearBuffer();
    OrbitOledClear();
    gameUiPage = Welcome;
  }
}

void GameUITick()
{
  uiInputTick();
  switch(gameUiPage)
  {
  case Welcome:
    handlePageWelcome();
    break;

  case SelectPlayers:
    handlePageSelectPlayers();
    break;

  case RollPlayer:
    handlePageRollPlayer();
    break;

  case GameResult:
    handlePageGameResult();
    break;

  default:
    handlePageProgressBar();
    break;
  }
  OrbitOledUpdate();
}

