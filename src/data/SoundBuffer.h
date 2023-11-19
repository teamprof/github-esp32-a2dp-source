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
#include "../lib/ESP32-A2DP/src/BluetoothA2DPSource.h"
#include "../lib/ESP32-A2DP/src/SoundData.h"

#include "../base/peripheral/PeriodicTimer.h"
#include "../peripheral/i2c/I2cParam.h"

#include "./DataSlot.h"

class SoundBuffer : public OneChannel8BitSoundData
{
public:
    // +------+------+------+------+------+
    // | slot | slot | slot | slot | slot |
    // | 0.1s | 0.1s | 0.1s | 0.1s | 0.1s |
    // +------+------+------+------+------+
    //
    // |----------------------------------|
    //                 0.5s
    static const int16_t SLOT_EDGE_POOL = 1;
    static const int16_t SLOT_LANE_LEFT = 2;
    static const int16_t SLOT_LANE_MIDDLE = 2;
    static const int16_t SLOT_LANE_RIGHT = 2;
    static const int16_t SLOT_ERROR = 3;

    static const int32_t SAMPLING_RATE = 44100;
    static const int32_t SLOT_DURATION = 100; // in units of ms

    static const int32_t BUFFER_DURATION = 500; // in units of ms

    static const int32_t SAMPLING_PER_SLOT = SAMPLING_RATE * SLOT_DURATION / 1000;
    static const int32_t TOTAL_SLOTS = (BUFFER_DURATION / SLOT_DURATION);

    static const int32_t DIM_DATA_SLOT = 10; // max number of dataSlot

    SoundBuffer();
    ~SoundBuffer();

    bool init(void);
    int32_t get2ChannelData(int32_t pos, int32_t len, uint8_t *data);
    void updateSoundSignal(uint8_t soundData);

private:
    static SoundBuffer *_instance;

    DataSlot _dataSlot[DIM_DATA_SLOT];

    SemaphoreHandle_t _mutex;

    static OneChannel8BitSoundData soundLaneLeft;
    static OneChannel8BitSoundData soundLaneMiddle;
    static OneChannel8BitSoundData soundLaneRight;
    static OneChannel8BitSoundData soundEdgePool;
    static OneChannel8BitSoundData soundError;

    void clearAllSlots(void);

    OneChannel8BitSoundData *getSoundDataPtr(int32_t slot);
    bool setSoundDataPtr(int32_t slot, OneChannel8BitSoundData *soundData);

    inline int32_t getSlotNumFromFrameNum(int32_t frameNumber)
    {
        return (frameNumber / SAMPLING_PER_SLOT) % TOTAL_SLOTS;
    }

    int32_t readSlotData(int32_t slotNum, int32_t slotPos, int32_t length, Frame *framePtr);
};
