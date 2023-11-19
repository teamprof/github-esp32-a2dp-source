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
#include "./src/ArduProf.h"
#include "./src/thread/ThreadApp.h"
#include "./src/peripheral/i2c/I2cCommand.h"
#include "./AppContext.h"

///////////////////////////////////////////////////////////////////////////////
static AppContext appContext = {0};

///////////////////////////////////////////////////////////////////////////////
static void initGlobalVar(void)
{
}

static void createTasks(void)
{
    static ThreadApp threadApp;
    appContext.threadApp = &threadApp;
    threadApp.start(&appContext);
}

static void printAppInfo(void)
{
    PRINTLN("===============================================================================");
    PRINTLN("chipModel=", ESP.getChipModel(), ", chipRevision=", ESP.getChipRevision(), ", flash size=", ESP.getFlashChipSize(),
            "\r\nNumber of cores=", ESP.getChipCores(), ", SDK version=", ESP.getSdkVersion());
    PRINTLN("===============================================================================");
}

void setup()
{
    /////////////////////////////////////////////////////////////////////////////
    // initial serial port to 115200bps
    Serial.begin(115200);
    while (!Serial)
    {
        delay(100);
    }

    // set log output to serial port, and init log params such as log_level
    LOG_SET_LEVEL(DefaultLogLevel);
    // LOG_SET_LEVEL(DebugLogLevel::LVL_TRACE);
    // LOG_SET_LEVEL(DebugLogLevel::LVL_NONE);
    LOG_SET_DELIMITER("");
    LOG_ATTACH_SERIAL(Serial);
    /////////////////////////////////////////////////////////////////////////////

    printAppInfo();

    initGlobalVar();
    createTasks();

    // LOG_TRACE("setup done");

    // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

static void testSound(void)
{
    delay(500);

    uint8_t volume = 50;
    I2cParam::Sound sound = {0};

    static int count = 0;
    switch (count++ / 4)
    {
    case 0:
        sound.bit.laneLeft = 1;
        break;
    case 1:
        sound.bit.laneMiddle = 1;
        break;
    case 2:
        sound.bit.laneRight = 1;
        break;
    }
    count %= 16;

    appContext.threadApp->postEvent(EventI2c, I2cCommand::PlaySound, volume, sound.byte.data);
}

void loop()
{
    // testSound();
    delay(1000);
}