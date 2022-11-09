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

#include <stdlib.h>
#include <gst/app/gstappsrc.h>

#include "../rdk_gstreamer_utils.h"
#define GST_BUFFER_FLAG_NF_KEYFRAME (GST_BUFFER_FLAG_LAST << 1)
namespace rdk_gstreamer_utils
{
    #define HEAAC_FRAME_SIZE 42
    #define DDP_FRAME_SIZE 32
    const uint32_t      MIN_AUDIO_GAP_SUPPORTED = 35;
    static const int    sAudioChangeGapThresholdMS = 40;
    const uint32_t      WAIT_WHILE_IDLING_MS = 100;
    const uint32_t      OK = 0;
    enum audio_change_state
    {
        AUDCHG_INIT = 0,
        AUDCHG_CMD = 1,
        AUDCHG_SET = 2,
        AUDCHG_ALIGN = 3,
    };
  
    #define HEAAC_PTS_OFFSET_MS (2*HEAAC_FRAME_SIZE*45)
    GstElement * mCurAudioDecoder = NULL;
    gboolean mCurIsAudioAAC = false;

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

    bool IntialVolSettingNeeded_soc()
    {
        return true;
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
        uint32_t ptsFadeDurationInMs;
        gint64 fadeInOffset = 0;

        if(true == audioaac) {
            ptsFadeDurationInMs = HEAAC_FRAME_SIZE;
        } else {
            ptsFadeDurationInMs = DDP_FRAME_SIZE;
            fadeInOffset = DDP_FRAME_SIZE;
        }
        GstElement *audioSink = NULL;

        if(gapduration) {

            g_object_get(pipeline, "audio-sink", &audioSink, NULL);
            if(audioSink) {
                g_object_set(audioSink, "ms12ptsfade-level", (guint32) 0, NULL);
                g_object_set(audioSink, "ms12ptsfade-duration", (guint32) ptsFadeDurationInMs, NULL);
                g_object_set(audioSink, "ms12ptsfade-pts", gapstartpts, NULL);
            }
        }
        if(gapdiscontinuity) {

            g_object_get(pipeline, "audio-sink", &audioSink, NULL);
            if(audioSink) {
            	g_object_set(audioSink, "ms12ptsfade-level", (guint32) 1, NULL);
                g_object_set(audioSink, "ms12ptsfade-duration", (guint32) ptsFadeDurationInMs, NULL);
                g_object_set(audioSink, "ms12ptsfade-pts", (gint64) (gapstartpts + fadeInOffset), NULL);
            }
        }
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

    bool isUIAudioVGAudioMixSupported_soc()
    {
        return true;
    }

    unsigned getNativeAudioFlag_soc()
    {
        /* this is called during _init, so reset locals here */
        mCurAudioDecoder = NULL;
        mCurIsAudioAAC = false;
        return getGstPlayFlag("native-audio");
    }

    bool isPtsOffsetAdjustmentSupported_soc()
    {
        return true;
    }
  
    void updateAudioPtsOffset(void)
    {
        if (mCurAudioDecoder)
        {
            g_object_set(mCurAudioDecoder, "audio pts offset", (mCurIsAudioAAC ? 0 : HEAAC_PTS_OFFSET_MS), NULL);
        }
    }

    /* save the current audio format, update PTS offset in the audio decoder */
    void updateAudioPtsOffset(const std::string& audioCodecString)
    {
        mCurIsAudioAAC = (audioCodecString.compare(0, 4, std::string("mp4a")) == 0);
        updateAudioPtsOffset();
    }

    /* always return AAC offset to the video, so that it will add that PTS offset as a base, move the audio back and forth */

    int getPtsOffsetAdjustment_soc(const std::string& audioCodecString)
    {
        updateAudioPtsOffset(audioCodecString);
        return HEAAC_PTS_OFFSET_MS;
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

    bool audioCodecSwitch(
        const void              *pSampleAttr,
        AudioAttributes         *pAudioAttr,
        uint32_t                *pStatus,
        unsigned int            *pui32Delay,
        llong                   *pAudioChangeTargetPts,
        const llong             *pcurrentDispPts,
        unsigned int            *audio_change_stage,
        GstCaps                 **appsrcCaps,
        bool                    *audioaac,
        bool                    svpenabled,
        GstElement              *aSrc,
        GstElement              *pipeline
    )
    {
        struct timespec ts, now;
        unsigned int reconfig_delay_ms;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        if (*pStatus != OK || pSampleAttr == nullptr) {
            LOG_RGU("No audio data ready yet");
            *pui32Delay = WAIT_WHILE_IDLING_MS;
            return false;
        }

        if (pAudioAttr) {
            const char  *pCodecStr = pAudioAttr->mCodecParam.c_str();
            const char  *pCodecAcc = strstr(pCodecStr, "mp4a");
            bool        isAudioAAC = (pCodecAcc) ? true : false ;
            bool        isCodecSwitch = false;

            LOG_RGU("Audio Attribute format %s  channel %d samp %d, bitrate %d blockAligment %d",
                    pCodecStr, pAudioAttr->mNumberOfChannels,
                    pAudioAttr->mSamplesPerSecond, pAudioAttr->mBitrate,
                    pAudioAttr->mBlockAlignment);

            *pAudioChangeTargetPts = *pcurrentDispPts;
            *audio_change_stage = AUDCHG_ALIGN;
            LOG_RGU("isAudioAAC = %d, *audioaac = %d", isAudioAAC, *audioaac);
            if (*appsrcCaps) {
                gst_caps_unref(*appsrcCaps);
                *appsrcCaps = NULL;
            }

            if (isAudioAAC != *audioaac)
                isCodecSwitch = true;

            configAudioCap_soc(pAudioAttr, audioaac, svpenabled, appsrcCaps);
            {
                gboolean  ret = FALSE;
                GstEvent* flush_start = NULL;
                GstEvent* flush_stop  = NULL;

                flush_start = gst_event_new_flush_start();
                ret = gst_element_send_event(aSrc, flush_start);
                if (!ret)
                    LOG_RGU("ERROR: failed to send flush-start event");

                flush_stop = gst_event_new_flush_stop(TRUE);
                ret = gst_element_send_event(aSrc, flush_stop);
                if (!ret)
                    LOG_RGU("ERROR: failed to send flush-stop event");
            }

            gst_app_src_set_caps(GST_APP_SRC(aSrc), *appsrcCaps);

            if (isCodecSwitch) {
                if ((isPtsOffsetAdjustmentSupported_soc()) && (pAudioAttr != NULL)) {
                    updateAudioPtsOffset(pAudioAttr->mCodecParam);
                }
            }

            clock_gettime(CLOCK_MONOTONIC, &now);
            reconfig_delay_ms = now.tv_nsec > ts.tv_nsec ?
                        (now.tv_nsec-ts.tv_nsec)/1000000 :
                        (1000 - (ts.tv_nsec -now.tv_nsec)/1000000);

            *pAudioChangeTargetPts += (reconfig_delay_ms + sAudioChangeGapThresholdMS);
        }
        else {
            *pui32Delay = 0;
            return false;
        }

        return true;
    }

    bool performAudioTrackCodecChannelSwitch_soc(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup, const void *pSampleAttr, AudioAttributes *pAudioAttr, uint32_t *pStatus, unsigned int *pui32Delay,
                                                 llong *pAudioChangeTargetPts, const llong *pcurrentDispPts, unsigned int *audio_change_stage, GstCaps **appsrcCaps,
                                                 bool *audioaac, bool svpenabled, GstElement *aSrc, bool *ret)
    {
        bool retVal = audioCodecSwitch(pSampleAttr, pAudioAttr, pStatus, pui32Delay, pAudioChangeTargetPts, pcurrentDispPts, audio_change_stage,
        appsrcCaps, audioaac, svpenabled, aSrc, pgstUtilsPlaybackGroup->gstPipeline);
        *ret = retVal;
        return true;
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
        gchar* elementName = gst_element_get_name(element);
        if (elementName && isPtsOffsetAdjustmentSupported_soc()) {
            if (g_strrstr(elementName, "brcmaudiodecoder")) {
                mCurAudioDecoder = element;
                updateAudioPtsOffset();
            }
            g_free(elementName);
        }
    }

    #define GST_FIFO_SIZE_MS (100)
    void audioMixerGetDeviceInfo_soc(uint32_t& preferredFrames, uint32_t& maximumFrames)
    {
        uint64_t maxBytes = GST_FIFO_SIZE_MS * 48 * 4;  // 100ms of PCM data = 4800 frames * 4 bytes

        maximumFrames = maxBytes / 4;
        preferredFrames = maximumFrames / 4;
        maximumFrames = (75 * 48);  // actually set to 75ms to pass NTS (everything else seems fine)
    }

    size_t audioMixerGetBufferDelay_soc(int64_t queuedBytes,int bufferDelayms)
    {
        return (((queuedBytes/256) * 64)  +  (bufferDelayms * 48));
    }

    uint64_t audioMixerGetFifoSize_soc()
    {
        return (GST_FIFO_SIZE_MS * 48 * 4);
    }

    void setVideoSinkMode_soc(GstElement * videoSink)
    {
         return; //no op
    }

    static bool IsH265Stream(std::string codec)
    {
        bool retVal = false;

        if( codec.find(std::string("h265"))  != std::string::npos ||
            codec.find(std::string("hdr10")) != std::string::npos ||
            codec.find(std::string("dvhe"))  != std::string::npos ||
            codec.find(std::string("dvh1"))  != std::string::npos ||
            codec.find(std::string("hvc1"))  != std::string::npos ||
            codec.find(std::string("hev1"))  != std::string::npos ) {
            LOG_RGU("IsH265Stream found H265 stream, requested codec is %s", codec.c_str());
            retVal = true;
        }

        return retVal;
    }

    void configVideoCap_soc(std::string vCodec,uint32_t imageWidth,uint32_t imageHeight,uint32_t frameRateValue,uint32_t frameRateScale,bool svpEnabled,gchar **capsString)
    {
        if (IsH265Stream(vCodec))
        {
            LOG_RGU("####### Using HEVC codec\n");
            *capsString = g_strdup_printf("video/x-h265, alignment=(string)au, stream-format=(string)byte-stream, width=(int)%u, height=(int)%u, framerate=(fraction)%u/%u, enable-svp=(string)%s",
                                imageWidth,imageHeight,frameRateValue,frameRateScale,svpEnabled ? "true" : "false");
        }
        else
        {
            LOG_RGU("####### Using H264 codec\n");
            *capsString = g_strdup_printf("video/x-h264, alignment=(string)au, stream-format=(string)byte-stream, width=(int)%u, height=(int)%u, framerate=(fraction)%u/%u, enable-svp=(string)%s",
                                imageWidth,imageHeight,frameRateValue,frameRateScale,svpEnabled ? "true" : "false");
        }
    }


    uint64_t audioMixerGetQueuedBytes_soc(uint64_t bytesPushed,uint64_t bytesPlayed)
    {
        return (bytesPushed-bytesPlayed);
    }

    void audioMixerConfigurePipeline_soc(GstElement *gstPipeline,GstElement *aSink,GstElement *aSrc,bool attenuateOutput)
    {
        const float AUDIO_VOLUME_SCALE_FACTOR=1.0;
        g_object_set(G_OBJECT(aSink), "volume", 1.0 * AUDIO_VOLUME_SCALE_FACTOR, NULL);
        gst_bin_add_many (GST_BIN (gstPipeline), aSrc, aSink, NULL);
        gst_element_link_many (aSrc, aSink, NULL);
        if(attenuateOutput)
        {
           LOG_RGU("GstAudioMixerOutput: No Audio Equivalence, so attenuating mixer output");
           g_object_set(G_OBJECT(aSink), "volume", 0.5 * AUDIO_VOLUME_SCALE_FACTOR, NULL);
        }
        else
        {
           LOG_RGU("GstAudioMixerOutput: Audio Equivalence On, not attenuating mixer output");
        }
    }

    void switchToAudioMasterMode_soc()
    {
        putenv("GST_BRCM_STC_MODE=audio");
    }

    void setKeyFrameFlag_soc(GstBuffer *gstBuffer,bool val)
    {
	if( val) {
	    GST_BUFFER_FLAG_SET(gstBuffer, GST_BUFFER_FLAG_NF_KEYFRAME);
	}else {
	    GST_BUFFER_FLAG_UNSET(gstBuffer, GST_BUFFER_FLAG_NF_KEYFRAME);
	}
    }

    bool getDelayTimerEnabled_soc()
    {
        return false;
    }

    void SetAudioServerParam_soc(bool enabled)
    {
        return;
    }

} // namespace rdk_gstreamer_utils_soc.cpp

