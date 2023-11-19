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
#include "./I2cA2dp.h"
#include "./I2cCommand.h"
#include "./I2cResponse.h"
#include "../../ArduProf.h"

#define isVolumeValid(volumne) (volumne >= 0 && volumne <= 100)
#define isSoundValid(param) true

/*
    coral                                 esp32
      |                                     |
      |   I2cCommand::IsA2dpConnected       |
      | ----------------------------------> |
      |                                     |
      |   I2cResponse::A2dpConnected or     |
      |   I2cResponse::A2dpDisconnected     |
      | <---------------------------------- |
      |                                     |
      |                                     |
      |       loop IsA2dpConnected          |
      |       until A2dpConnected           |
      |                                     |
      |                                     |
      |   I2cCommand::PlaySound             |
      |   param: <volumne> <sound>          |
      | ----------------------------------> |
      |                                     |
      |   I2cResponse::Success or           |
      |   I2cResponse::ErrorInvalidParam or |
      |   I2cResponse::ErrorDisconnected    |
      | <---------------------------------- |

    note:
    1. coral sends I2cCommand::PlaySound command every 0.5s
    2. coral re-start I2cCommand::IsA2dpConnected flow once "I2cResponse::ErrorDisconnected" is received
*/

I2cA2dp *I2cA2dp::_instance = nullptr;

I2cA2dp::I2cA2dp(ThreadBase *thread, int16_t eventValue) : _isA2dpConnected(false),
                                                           thread(thread),
                                                           eventValue(eventValue),
                                                           command(I2cCommand::Null),
                                                           param(0),
                                                           reply(I2cResponse::Fail)
{
    _instance = this;

    Wire.onReceive([_instance](int numBytes)
                   {
                    if(_instance) {
                        _instance->onWireReceive(numBytes);
                    } });
    Wire.onRequest([_instance]()
                   { 
                    if(_instance) {
                        _instance->onWireRequest();
                    } });
}

bool I2cA2dp::begin(uint8_t deviceAddr)
{
    bool rst = Wire.begin(deviceAddr);
    return rst;
}

void I2cA2dp::setA2dpConnectionStatus(bool status)
{
    _isA2dpConnected = status;
}

void I2cA2dp::onWireReceive(int numBytes)
{
    int sizeAvailable = Wire.available();
    if (sizeAvailable >= (sizeof(command)))
    {
        command = Wire.read();
        switch (command)
        {
        case I2cCommand::Null:
            reply = I2cResponse::Success;
            break;
        case I2cCommand::IsA2dpConnected:
            reply = _isA2dpConnected ? I2cResponse::A2dpConnected : I2cResponse::A2dpDisconnected;
            LOG_TRACE("I2cCommand::IsA2dpConnected: reply=", reply);
            break;
        case I2cCommand::PlaySound:
        {
            uint8_t paramVolume = Wire.read();
            uint8_t paramSound = Wire.read();

            if (!_isA2dpConnected)
            {
                reply = I2cResponse::ErrorDisconnected;
            }
            else if (isVolumeValid(paramVolume) && isSoundValid(paramSound))
            {
                reply = I2cResponse::Success;

                if (thread)
                {
                    thread->postEvent(eventValue, command, paramVolume, paramSound);
                }
            }
            else
            {
                reply = I2cResponse::ErrorInvalidParam;
            }
            LOG_TRACE("I2cCommand::PlaySound: paramVolume=", paramVolume, ", paramSound=(hex)", DebugLogBase::HEX, paramSound, ", reply=", DebugLogBase::DEC, reply);
            break;
        }
        default:
            reply = I2cResponse::Fail;
            break;
        }

        while (Wire.available())
        {
            uint8_t dummy = Wire.read();
            LOG_TRACE("ignore I2C data=(hex)", DebugLogBase::HEX, dummy);
        }
    }
}

void I2cA2dp::onWireRequest(void)
{
    Wire.write((const uint8_t *)&reply, sizeof(reply));
    command = I2cCommand::Null;
    reply = I2cResponse::Fail;
}
