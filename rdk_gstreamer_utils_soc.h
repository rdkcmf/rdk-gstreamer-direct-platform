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
    void processAudioGap_soc(GstElement *pipeline,gint64 gapstartpts,gint32 gapduration,gint64 gapdiscontinuity,bool audioaac);
    void enableAudioSwitch_soc(GstElement *pipeline);
    GstElement * configureUIAudioSink_soc(bool TTSenabled);
    bool isUIAudioVGAudioMixSupported_soc();
    unsigned int getNativeAudioFlag_soc();
    void configAudioCap_soc(AudioAttributes *pAttrib, bool *audioaac, bool svpenabled, GstCaps **appsrcCaps);
    bool performAudioTrackCodecChannelSwitch_soc(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup, const void *pSampleAttr, AudioAttributes *pAudioAttr, uint32_t *pStatus, unsigned int *pui32Delay,
                                                 llong *pAudioChangeTargetPts, const llong *pcurrentDispPts, unsigned int *audio_change_stage, GstCaps **appsrcCaps,
                                                 bool *audioaac, bool svpenabled, GstElement *aSrc, bool *ret);
    void setAppSrcParams_soc(GstElement *aSrc,MediaType mediatype);
    void setPixelAspectRatio_soc(GstCaps ** ppCaps,GstCaps *appsrcCaps,uint32_t pixelAspectRatioX,uint32_t pixelAspectRatioY);
    void deepElementAdded_soc (struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup, GstBin* pipeline, GstBin* bin, GstElement* element);
    void audioMixerGetDeviceInfo_soc(uint32_t& preferredFrames, uint32_t& maximumFrames);
    size_t audioMixerGetBufferDelay_soc(int64_t queuedBytes,int bufferDelayms);
    uint64_t audioMixerGetFifoSize_soc();
    void switchToAudioMasterMode_soc();
} //namespace rdk_gstreamer_utils
