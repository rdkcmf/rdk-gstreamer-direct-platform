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

#include <string>

namespace rdk_gstreamer_utils
{
    void initVirtualDisplayHeightandWidthFromPlatform_soc(unsigned int* mVirtualDisplayHeight, unsigned int* mVirtualDisplayWidth);
    const char* getAudioDecoderName_soc();
    const char* getAudioUnderflowSignalName_soc();
    const char* getVideoUnderflowSignalName_soc();
    bool IsAudioFadeSupported_soc();
    void EaseAudio_soc(double target, uint32_t duration, rgu_Ease ease);
    bool isPtsOffsetAdjustmentSupported_soc();
    int getPtsOffsetAdjustment_soc(const std::string& audioCodecString);
    void setVideoProperty_soc(GstElement *pipeline);
    void processAudioGap_soc(GstElement *pipeline,gint64 gapstartpts,gint32 gapduration);
    void enableAudioSwitch_soc(GstElement *pipeline);
    GstElement * configureUIAudioSink_soc(bool TTSenabled);
    GstElement * getAudioSinkPlaysinkBin_soc(GstElement *element);
    bool isUIAudioVGAudioMixSupported_soc();
    std::map<rgu_gstelement,GstElement *> createNewAudioElements_soc(bool isAudioAAC,bool createqueue);
    unsigned int getNativeAudioFlag_soc();
} //namespace rdk_gstreamer_utils
