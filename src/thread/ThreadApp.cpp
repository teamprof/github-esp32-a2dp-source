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
#include <string.h>

#include "../peripheral/i2c/I2cCommand.h"
#include "../../AppEvent.h"
#include "ThreadApp.h"

static int8_t *soundBuf = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////
// #define I2C_DEV_ADDR ((uint8_t)0x55)

////////////////////////////////////////////////////////////////////////////////////////////
#define TARGET_DEVICE_NAME "BTS-06"      // bluetooth name of earphone/speaker
#define LOCAL_DEVICE_NAME "ESP_A2DP_SRC" // bluetooth name of this device

////////////////////////////////////////////////////////////////////////////////////////////

ThreadApp *ThreadApp::_instance = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////
// Thread
////////////////////////////////////////////////////////////////////////////////////////////
// #define RUNNING_CORE 0 // dedicate core 0 for Thread
// #define RUNNING_CORE 1 // dedicate core 1 for Thread
#define RUNNING_CORE ARDUINO_RUNNING_CORE

#define TASK_NAME "ThreadApp"
#define TASK_STACK_SIZE 4096
#define TASK_PRIORITY 3
#define TASK_QUEUE_SIZE 128 // message queue size for app task

#define TASK_INIT_NAME "taskDelayInit"
#define TASK_INIT_STACK_SIZE 4096
#define TASK_INIT_PRIORITY 0

static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
static StaticQueue_t xStaticQueue;

static StackType_t xStack[TASK_STACK_SIZE];
static StaticTask_t xTaskBuffer;

////////////////////////////////////////////////////////////////////////////////////////////
ThreadApp::ThreadApp() : ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                         isA2dpConnected(false),
                         a2dpSource(),
                         soundBuffer(),
                         i2cA2dp(this, EventI2c),
                         handlerMap()
{
    _instance = this;
    handlerMap = {
        __EVENT_MAP(ThreadApp, EventI2c),
        __EVENT_MAP(ThreadApp, EventNull), // {EventNull, &ThreadApp::handlerEventNull},
    };
}

///////////////////////////////////////////////////////////////////////|
__EVENT_FUNC_DEFINITION(ThreadApp, EventI2c, msg) // void ThreadApp::handlerEventI2c(const Message &msg)
{
    // LOG_TRACE("EventI2c(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
    int16_t command = msg.iParam;
    switch (command)
    {
    case I2cCommand::PlaySound:
    {
        if (isA2dpConnected)
        {
            uint8_t paramVolume = (uint8_t)msg.uParam;
            uint8_t paramSound = (uint8_t)msg.lParam;
            a2dpSource.set_volume(paramVolume);
            soundBuffer.updateSoundSignal(paramSound);
        }
        break;
    }

    default:
        LOG_TRACE("unsupported i2c command=", command);
        break;
    }
}

__EVENT_FUNC_DEFINITION(ThreadApp, EventNull, msg) // void ThreadApp::handlerEventNull(const Message &msg)
{
    LOG_TRACE("EventNull(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
}

///////////////////////////////////////////////////////////////////////
void ThreadApp::onMessage(const Message &msg)
{
    // LOG_TRACE("event=", msg.event, ", iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
    auto func = handlerMap[msg.event];
    if (func)
    {
        (this->*func)(msg);
    }
    else
    {
        LOG_TRACE("Unsupported event = ", msg.event, ", iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
    }
}

void ThreadApp::start(void *ctx)
{
    // LOG_TRACE("on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
    configASSERT(ctx);
    _context = ctx;

    _taskHandle = xTaskCreateStaticPinnedToCore(
        [](void *instance)
        { static_cast<ThreadBase *>(instance)->run(); },
        TASK_NAME,
        TASK_STACK_SIZE, // This stack size can be checked & adjusted by reading the Stack Highwater
        this,
        TASK_PRIORITY, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        xStack,
        &xTaskBuffer,
        RUNNING_CORE);
}

void ThreadApp::setup(void)
{
    // LOG_TRACE("on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
    configASSERT(_instance && _instance->context());

    xTaskCreatePinnedToCore(
        [](void *_instance)
        {
            configASSERT(_instance);
            LOG_TRACE("taskDelayInit() on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
            ThreadApp *instance = static_cast<ThreadApp *>(_instance);
            instance->delayInit();
            // vTaskDelay(pdMS_TO_TICKS(100));        // delay 100ms
            vTaskDelete(instance->taskInitHandle); // init completed => delete itself
        },
        TASK_INIT_NAME,
        TASK_INIT_STACK_SIZE, // This stack size can be checked & adjusted by reading the Stack Highwater
        this,
        TASK_INIT_PRIORITY, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        &taskInitHandle,
        ARDUINO_RUNNING_CORE);
}

void ThreadApp::run(void)
{
    LOG_TRACE("on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
    ThreadBase::run();
}

// note: btSerial.start() requires large stack
void ThreadApp::delayInit(void)
{
    LOG_TRACE("on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());

    bool initSoundBuffer = soundBuffer.init();
    configASSERT(initSoundBuffer);

    a2dpSource.set_auto_reconnect(true);
    a2dpSource.set_local_name(LOCAL_DEVICE_NAME);
    a2dpSource.set_on_connection_state_changed([_instance](esp_a2d_connection_state_t state, void *obj)
                                               {
                                                   if (_instance)
                                                   {
                                                       _instance->onConnectionStateChanged(state, obj);
                                                   } });
    a2dpSource.set_on_audio_state_changed([_instance](esp_a2d_audio_state_t state, void *obj)
                                          {
                                            if (_instance)
                                            {
                                                _instance->onAudioStateChanged(state, obj);
                                            } });
    // a2dpSource.set_pin_code();
    a2dpSource.set_volume(80);

    a2dpSource.write_data(&soundBuffer);

    a2dpSource.start(TARGET_DEVICE_NAME);

    bool rst = i2cA2dp.begin(I2C_DEV_ADDR);
    LOG_TRACE("i2cA2dp.begin(0x", DebugLogBase::HEX, I2C_DEV_ADDR, ") returns ", rst);

    // vTaskDelay(pdMS_TO_TICKS(100));
}

void ThreadApp::onAudioStateChanged(esp_a2d_audio_state_t state, void *obj)
{
    switch (state)
    {
    case ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND: // audio stream datapath suspended by remote device
        LOG_TRACE("ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND");
        break;
    case ESP_A2D_AUDIO_STATE_STOPPED: // audio stream datapath stopped
        LOG_TRACE("ESP_A2D_AUDIO_STATE_STOPPED");
        break;
    case ESP_A2D_AUDIO_STATE_STARTED: // audio stream datapath started
        LOG_TRACE("ESP_A2D_AUDIO_STATE_STARTED");
        break;
    default:
        LOG_TRACE("unknown state=", state);
        break;
    }
}

void ThreadApp::onConnectionStateChanged(esp_a2d_connection_state_t state, void *obj)
{
    switch (state)
    {
    case ESP_A2D_CONNECTION_STATE_DISCONNECTED: // connection released
        LOG_TRACE("ESP_A2D_CONNECTION_STATE_DISCONNECTED");
        isA2dpConnected = false;
        i2cA2dp.setA2dpConnectionStatus(false);
        break;
    case ESP_A2D_CONNECTION_STATE_CONNECTING: // connecting remote device
        LOG_TRACE("ESP_A2D_CONNECTION_STATE_CONNECTING");
        break;
    case ESP_A2D_CONNECTION_STATE_CONNECTED: // connection established
        LOG_TRACE("ESP_A2D_CONNECTION_STATE_CONNECTED");
        isA2dpConnected = true;
        i2cA2dp.setA2dpConnectionStatus(true);
        break;
    case ESP_A2D_CONNECTION_STATE_DISCONNECTING: //!< disconnecting remote device
        LOG_TRACE("ESP_A2D_CONNECTION_STATE_DISCONNECTING");
        break;
    default:
        LOG_TRACE("unknown state=", state);
        break;
    }
}
