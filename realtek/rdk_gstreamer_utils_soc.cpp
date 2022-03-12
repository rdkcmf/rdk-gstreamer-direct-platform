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
        const char* realtek_audiodecoder_name  = PLATFORM_AUDIODECODER;
        return realtek_audiodecoder_name;
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

    void processAudioGap_soc(GstElement *pipeline,gint64 gapstartpts,gint32 gapduration,gint64 gapdiscontinuity,bool audioaac)
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
        LOG_RGU("configureUIAudioSink_soc: : connecting alsasink");
        audioSink = gst_element_factory_make ("alsasink","alsasink");

        return audioSink;
    }

    bool isUIAudioVGAudioMixSupported_soc()
    {
        return true;
    }

    unsigned  getNativeAudioFlag_soc()
    {
        return getGstPlayFlag("native-audio");
    }

    bool isPtsOffsetAdjustmentSupported_soc()
    {
        return false;
    }

    int getPtsOffsetAdjustment_soc(const std::string& audioCodecString)
    {
        return 0;
    }

    void configAudioCap_soc(AudioAttributes *pAttrib, bool *audioaac, bool svpenabled, GstCaps **appsrcCaps)
    {
        gchar *caps_string;
        LOG_RGU("Config audio codec %s sampling rate %d channel %d alignment %d",
                pAttrib->mCodecParam.c_str(),
                pAttrib->mSamplesPerSecond,
                pAttrib->mNumberOfChannels,
                pAttrib->mBlockAlignment);

        if (pAttrib->mCodecParam.compare(0, 4, std::string("mp4a")) == 0)
        {
            LOG_RGU("####### Using AAC\n");
            caps_string = g_strdup_printf("audio/mpeg, mpegversion=4, enable-svp=(string)%s", svpenabled ? "true" : "false");
            *audioaac = true;
        }
        else
        {
            LOG_RGU("####### Using EAC3 \n");

            caps_string = g_strdup_printf("audio/x-eac3, framed=(boolean)true, rate=(int)%u, channels=(int)%u, alignment=(string)frame, enable-svp=(string)%s",
                                          pAttrib->mSamplesPerSecond,
                                          pAttrib->mNumberOfChannels,
                                          svpenabled ? "true" : "false");
            *audioaac = false;
        }
        *appsrcCaps = gst_caps_from_string(caps_string);
        g_free(caps_string);
    }

    bool performAudioTrackCodecChannelSwitch_soc(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup, const void *pSampleAttr, AudioAttributes *pAudioAttr, uint32_t *pStatus, unsigned int *pui32Delay,
                                                 llong *pAudioChangeTargetPts, const llong *currentDispPts, unsigned int *audio_change_stage, GstCaps **appsrcCaps,
                                                 bool *audioaac, bool svpenabled, GstElement *aSrc, bool *ret)
    {
        return false;
    }


    void setAppSrcParams_soc(GstElement *aSrc,MediaType mediatype)
    {
        if (mediatype == MEDIA_VIDEO)
            g_object_set(aSrc, "max-bytes", (guint64) 512 * 1024, NULL);
        else
            g_object_set(aSrc, "max-bytes", (guint64) 1 * 64 * 1024, NULL);
    }

    void setPixelAspectRatio_soc(GstCaps ** ppCaps,GstCaps *appsrcCaps,uint32_t pixelAspectRatioX,uint32_t pixelAspectRatioY)
    {
        return;
    }

    void deepElementAdded_soc (struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup,GstBin* pipeline, GstBin* bin, GstElement* element)
    {
        return;
    }

    /**
    *  Time            Size            Frames
    * ------------------------------------------
    *  48ms           9216 bytes       2304 frames
    */
    #define GST_FIFO_SIZE_MS (48)
    void audioMixerGetDeviceInfo_soc(uint32_t& preferredFrames, uint32_t& maximumFrames)
    {
        uint64_t maxBytes = GST_FIFO_SIZE_MS * 48 * 4;  // 48ms of PCM data = 2304 frames * 4 bytes

        maximumFrames = maxBytes / 4;
        preferredFrames = maximumFrames / 4;

    }

    size_t audioMixerGetBufferDelay_soc(int64_t queuedBytes,int bufferDelayms)
    {
        return ((queuedBytes/256) * 64 +  (bufferDelayms)*48);
    }

    uint64_t audioMixerGetFifoSize_soc()
    {
        return (GST_FIFO_SIZE_MS * 48 * 4);
    }
  
    void setKeyFrameFlag_soc(GstBuffer *gstBuffer,bool val)
    {
        //nop
	return;
    }

    bool getDelayTimerEnabled_soc()
    {
        return true;
    }

    void switchToAudioMasterMode_soc()
    {
        // no op. To be implemented if required later
        return;
    }

    void SetAudioServerParam_soc(bool enabled)
    {
        return;
    }
} // namespace rdk_gstreamer_utils
