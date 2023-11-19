/* Copyright 2023 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <Wire.h>
#include "../../ArduProf.h"

#define I2C_DEV_ADDR ((uint8_t)0x55) // device address

class I2cA2dp
{
public:
  ///////////////////////////////////////////////////////////////////////
  I2cA2dp(ThreadBase *thread, int16_t event);
  bool begin(uint8_t deviceAddr);

  void setA2dpConnectionStatus(bool status);

protected:
  static I2cA2dp *_instance;

  bool _isA2dpConnected;
  ThreadBase *thread;
  const int16_t eventValue;

  uint8_t command;
  uint8_t param;
  uint8_t reply;

  void onWireReceive(int numBytes);
  void onWireRequest(void);
};