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
#include "SoundBuffer.h"
#include "../ArduProf.h"

#include "./saturday-mono-i8.h"
#include "./off-mono-i8.h"
#include "./bright-mono-i8.h"
#include "./beep-mono-i8.h"
#include "./bell-mono-i8.h"

SoundBuffer *SoundBuffer::_instance = nullptr;

OneChannel8BitSoundData SoundBuffer::soundLaneLeft;
OneChannel8BitSoundData SoundBuffer::soundLaneMiddle;
OneChannel8BitSoundData SoundBuffer::soundLaneRight;
OneChannel8BitSoundData SoundBuffer::soundEdgePool;
OneChannel8BitSoundData SoundBuffer::soundError;

SoundBuffer::SoundBuffer() : OneChannel8BitSoundData(nullptr, (SAMPLING_RATE * BUFFER_DURATION / 1000), true),
                             _mutex(xSemaphoreCreateMutex())
{
    _instance = this;
    memset(_dataSlot, 0, sizeof(_dataSlot));
}

SoundBuffer::~SoundBuffer()
{
}

bool SoundBuffer::init(void)
{
    soundEdgePool.setData((int8_t *)off_mono_i8_raw, off_mono_i8_raw_len);
    soundEdgePool.setLoop(true);

    soundLaneLeft.setData((int8_t *)bright_mono_i8_raw, bright_mono_i8_raw_len);
    soundLaneLeft.setLoop(true);
    soundLaneMiddle.setData((int8_t *)beep_mono_i8_raw, beep_mono_i8_raw_len);
    soundLaneMiddle.setLoop(true);
    soundLaneRight.setData((int8_t *)bell_mono_i8_raw, bell_mono_i8_raw_len);
    soundLaneRight.setLoop(true);

    soundError.setData((int8_t *)saturday_mono_i8_raw, saturday_mono_i8_raw_len);
    soundError.setLoop(true);

    return true;
}

int32_t SoundBuffer::get2ChannelData(int32_t pos, int32_t len, uint8_t *data)
{
    int32_t result_len = 0;
    int32_t frame_count = len / 4;
    int32_t frameNum = pos / 4;
    int32_t soundLength = this->count();

    if (len > 0 && frameNum < soundLength)
    {
        Frame *framePtr = (Frame *)data;
        int32_t slotNum;
        while (result_len < frame_count)
        {
            slotNum = getSlotNumFromFrameNum(frameNum);
            int32_t slotPos = frameNum % SAMPLING_PER_SLOT;
            int32_t count = readSlotData(slotNum, slotPos, frame_count - result_len, framePtr);
            result_len += count;
            framePtr += count;
            frameNum = (frameNum + count) % soundLength;
        }
    }
    return result_len * 4;
}

void SoundBuffer::updateSoundSignal(uint8_t soundData)
{
    I2cParam::Sound sound{
        .byte{
            .data = soundData}};

    if (sound.byte.data == 0)
    {
        LOG_TRACE("sound.bit.data == 0");
        clearAllSlots();
    }
    else
    {
        if (sound.bit.edgeTop || sound.bit.edgeBottom)
        {
            LOG_TRACE("sound.bit.edgeTop/edgeBottom");
            setSoundDataPtr(SLOT_EDGE_POOL, &soundEdgePool);
        }

        if (sound.bit.laneMiddle)
        {
            LOG_TRACE("sound.bit.laneMiddle");
            setSoundDataPtr(SLOT_LANE_MIDDLE, &soundLaneMiddle);
        }
        else if (sound.bit.laneLeft)
        {
            LOG_TRACE("sound.bit.laneLeft");
            setSoundDataPtr(SLOT_LANE_LEFT, &soundLaneLeft);
        }
        else if (sound.bit.laneRight)
        {
            LOG_TRACE("sound.bit.laneRight");
            setSoundDataPtr(SLOT_LANE_RIGHT, &soundLaneRight);
        }

        if (sound.bit.lostConnection)
        {
            LOG_TRACE("sound.bit.lostConnection");
            setSoundDataPtr(SLOT_ERROR, &soundError);
        }
    }
}

void SoundBuffer::clearAllSlots(void)
{
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
    {
        LOG_TRACE("xSemaphoreTake failed!");
        return;
    }

    for (int i = 0; i < DIM_DATA_SLOT; i++)
    {
        _dataSlot[i].data = nullptr;
    }

    if (xSemaphoreGive(_mutex) != pdTRUE)
    {
        LOG_TRACE("xSemaphoreGive failed!");
    }
}

OneChannel8BitSoundData *SoundBuffer::getSoundDataPtr(int32_t slot)
{
    configASSERT(slot >= 0 && slot < TOTAL_SLOTS);

    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
    {
        LOG_TRACE("xSemaphoreTake failed!");
        return nullptr;
    }

    OneChannel8BitSoundData *dataPtr = _dataSlot[slot].data;

    if (xSemaphoreGive(_mutex) != pdTRUE)
    {
        LOG_TRACE("xSemaphoreGive failed!");
    }

    return dataPtr;
}

bool SoundBuffer::setSoundDataPtr(int32_t slot, OneChannel8BitSoundData *soundData)
{
    configASSERT(slot >= 0 && slot < TOTAL_SLOTS);

    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
    {
        LOG_TRACE("xSemaphoreTake failed!");
        return false;
    }

    DataSlot *dataSlotPtr = &_dataSlot[slot];
    dataSlotPtr->data = soundData;

    if (xSemaphoreGive(_mutex) != pdTRUE)
    {
        LOG_TRACE("xSemaphoreGive failed!");
    }

    return true;
}

int32_t SoundBuffer::readSlotData(int32_t slot, int32_t index, int32_t length, Frame *framePtr)
{
    int32_t result_len = 0;
    OneChannel8BitSoundData *dataPtr = getSoundDataPtr(slot);
    length = std::min(length, SAMPLING_PER_SLOT - index);
    while (length-- > 0)
    {
        if (dataPtr)
        {
            if (dataPtr->getData((index), *framePtr))
            {
                result_len++;
            }
            else
            {
                configASSERT(false);
                break;
            }
        }
        else
        {
            framePtr->channel1 = 0;
            framePtr->channel2 = 0;
            result_len++;
        }
        index++;
        framePtr++;
    }
    return result_len;
}
