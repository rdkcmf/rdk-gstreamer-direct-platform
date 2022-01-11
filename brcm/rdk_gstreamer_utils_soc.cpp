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
    void setVideoProperty_soc(GstElement *pipeline)
    {
        // no op. To be implemented if required later
        return;
    }

    void processAudioGap_soc(GstElement *pipeline,gint64 gapstartpts,gint32 gapduration)
    {
        // no op. To be implemented if required later
        return;
    }

    void enableAudioSwitch_soc(GstElement *pipeline)
    {
        // no op. To be implemented if required later
        return;
    }

    GstElement * configureUIAudioSink_soc(bool TTSenabled)
    {
        GstElement *audioSink = NULL;
        LOG_RGU("configureUIAudioSink_soc: : connecting autoaudiosink");
        audioSink = gst_element_factory_make ("autoaudiosink","autoaudiosink");
        
        return audioSink;        
    }

    GstElement * getAudioSinkPlaysinkBin_soc(GstElement *element)
    {
        // no op. To be implemented if required later
        return NULL;
    }

    bool isUIAudioVGAudioMixSupported_soc()
    {
        return true;
    }

    std::map<rgu_gstelement,GstElement *> createNewAudioElements_soc(bool isAudioAAC,bool createqueue)
    {
        // no op. To be implemented if required later
        std::map<rgu_gstelement,GstElement *> newAudElements;
        return newAudElements;
    }

    unsigned getNativeAudioFlag_soc()
    {
        return getGstPlayFlag("native-audio"); 
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

