/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2019 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "../rdk_gstreamer_utils.h"
namespace rdk_gstreamer_utils
{
    const char* getAudioDecoderName_soc()
    {
        const char* bcm_audiodecoder_name  = PLATFORM_AUDIODECODER;
        return bcm_audiodecoder_name;
    }

    const char* getVideoUnderflowSignalName_soc()
    {
        const char* UnderflowSignalFromPlatform = PLATFORM_BUFFERUNDERFLOW_SIGNAL;
        return UnderflowSignalFromPlatform;
    }

    const char* getAudioUnderflowSignalName_soc()
    {
        const char* UnderflowSignalFromPlatform = PLATFORM_BUFFERUNDERFLOW_SIGNAL;
        return UnderflowSignalFromPlatform;
    }

    void initVirtualDisplayHeightandWidthFromPlatform_soc(unsigned int* mVirtualDisplayHeight, unsigned int* mVirtualDisplayWidth)
    {
        *mVirtualDisplayHeight = PLATFORM_VIRTUALDISPLAY_HEIGHT;
        *mVirtualDisplayWidth = PLATFORM_VIRTUALDISPLAY_WIDTH;
    }

    bool IsAudioFadeSupported_soc()
    {
        return false;
    }

    void EaseAudio_soc(double target, uint32_t duration, rgu_Ease ease)
    {
        // no op
        return;
    }

    bool isPtsOffsetAdjustmentSupported_soc()
    {
        return true;
    }

    int getPtsOffsetAdjustment_soc(const std::string& audioCodecString)
    {
        unsigned int HEAAC_FRAME_SIZE_IN_MS = 42;
        bool isAudioAAC = (audioCodecString.compare(std::string("mp4a"))==0);
        int ptsoffset = (isAudioAAC) ? (2*HEAAC_FRAME_SIZE_IN_MS*45) : 0;

        return ptsoffset;
    }

} // namespace rdk_gstreamer_utils_soc.cpp

