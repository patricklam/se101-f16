#include <Wire.h>

void WireRequestArray(int address, uint8_t* buffer, uint8_t amount);
void WireWriteRegister(int address, uint8_t reg, uint8_t value);
void WireWriteByte(int address, uint8_t value);

static float const    SensorMaximumReading= 512.0;
static float const    SensorMaximumAccel  = 9.81 * 4.0;
static uint8_t const  SensorAccelerometer = 0x1D;
static uint32_t const ShakeThreshold      = 16;
static float          ShakeAccumulator    = 0;

void ShakeInit()
{
  WireWriteRegister(SensorAccelerometer, 0x31, 1);
  WireWriteRegister(SensorAccelerometer, 0x2D, 1 << 3);
}

bool ShakeIsShaking()
{
  return ShakeAccumulator >= ShakeThreshold;
}

void ShakeTick()
{
  size_t const DataLength = 6;
  uint32_t data[DataLength] = { 0 };
  
  WireWriteByte(SensorAccelerometer, 0x32);
  WireRequestArray(SensorAccelerometer, data, DataLength);

  uint16_t xi = (data[1] << 8) | data[0];
  uint16_t yi = (data[3] << 8) | data[2];
  uint16_t zi = (data[5] << 8) | data[4];
  float x = *(int16_t*)(&xi) / SensorMaximumReading * SensorMaximumAccel;
  float y = *(int16_t*)(&yi) / SensorMaximumReading * SensorMaximumAccel;
  float z = *(int16_t*)(&zi) / SensorMaximumReading * SensorMaximumAccel;
  
  ShakeAccumulator = sqrt(x*x + y*y + z*z);
}

