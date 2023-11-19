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
#include <map>
#include <Wire.h>
#include "../lib/ESP32-A2DP/src/BluetoothA2DPSource.h"

#include "../ArduProf.h"
#include "../peripheral/i2c/I2cA2dp.h"
#include "../data/SoundBuffer.h"
#include "../../AppEvent.h"

///////////////////////////////////////////////////////////////////////////////
class ThreadApp : public ThreadBase
{
public:
    ThreadApp();
    virtual void start(void *);

    static ThreadApp *instance(void)
    {
        return _instance;
    }

protected:
    typedef void (ThreadApp::*handlerFunc)(const Message &);
    std::map<int16_t, handlerFunc> handlerMap;

    virtual void onMessage(const Message &msg);
    virtual void run(void);

private:
    ///////////////////////////////////////////////////////////////////////////
    static ThreadApp *_instance;
    TaskHandle_t taskInitHandle;

    bool isA2dpConnected;
    BluetoothA2DPSource a2dpSource;

    SoundBuffer soundBuffer;

    I2cA2dp i2cA2dp;

    ///////////////////////////////////////////////////////////////////////////
    virtual void setup(void);
    virtual void delayInit(void);

    void onConnectionStateChanged(esp_a2d_connection_state_t state, void *obj);
    void onAudioStateChanged(esp_a2d_audio_state_t state, void *obj);

    ///////////////////////////////////////////////////////////////////////////
    // event handler
    ///////////////////////////////////////////////////////////////////////////
    __EVENT_FUNC_DECLARATION(EventI2c)
    __EVENT_FUNC_DECLARATION(EventNull) // void handlerEventNull(const Message &msg);
};