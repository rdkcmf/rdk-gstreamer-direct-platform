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
#include <gst/app/gstappsrc.h>
#include <string>
#include <audio_if.h>
#include <math.h>

namespace rdk_gstreamer_utils
{
    const uint32_t MIN_AUDIO_GAP_SUPPORTED = 35;
    static const int sAudioChangeGapThresholdMS = 40;
    const uint32_t WAIT_WHILE_IDLING = 100;
    const uint32_t OK = 0;
    enum audio_change_state
    {
        AUDCHG_INIT = 0,
        AUDCHG_CMD = 1,
        AUDCHG_SET = 2,
        AUDCHG_ALIGN = 3,
    };
    const char* getAudioDecoderName_soc()
    {
        const char* amlogic_audiodecoder_name  = PLATFORM_AUDIODECODER;
        return amlogic_audiodecoder_name;
    }

    const char* getVideoUnderflowSignalName_soc()
    {
        const char* UnderflowSignalFromPlatform = PLATFORM_BUFFERUNDERFLOW_SIGNAL_VIDEO;
        return UnderflowSignalFromPlatform;
    }

    const char* getAudioUnderflowSignalName_soc()
    {
        const char* UnderflowSignalFromPlatform = PLATFORM_BUFFERUNDERFLOW_SIGNAL_AUDIO;
        return UnderflowSignalFromPlatform;
    }

    void initVirtualDisplayHeightandWidthFromPlatform_soc(unsigned int* mVirtualDisplayHeight, unsigned int* mVirtualDisplayWidth)
    {
        *mVirtualDisplayHeight = PLATFORM_VIRTUALDISPLAY_HEIGHT;
        *mVirtualDisplayWidth = PLATFORM_VIRTUALDISPLAY_WIDTH;
    }

    bool IsAudioFadeSupported_soc()
    {
        return true;
    }

    void EaseAudio_soc(double target, uint32_t duration, rgu_Ease ease)
    {
        int32_t gain = static_cast<int>(20.0f * log10(target) * 128.0f);
        gain = std::max(std::min(gain, 0), -96 * 128);
        LOG_RGU("setVolume target=%f (db %d), duration=%d, ease = %d\n", target, gain / 128, duration, ease);
        const std::string setting = std::string("vol_ease=") + std::to_string(gain) + \
            "," + std::to_string(duration) + "," + std::to_string(static_cast<int>(ease));

        struct audio_hw_device *dev_;
        LOG_RGU("Try Aquire h/w for audio fade");
        audio_hw_load_interface(&dev_);
        if (dev_)
        {
            LOG_RGU("Acquired Audio Fade h/w. Start audio fading !!!");
            dev_->set_parameters(dev_, setting.c_str());
            LOG_RGU("Finished Audio fading. Start Release of h/w");
            dev_->common.close(&dev_->common);
            audio_hw_unload_interface(dev_);
            LOG_RGU("Audio fade H/w release completed !");
        }
        else
            LOG_RGU("Failed to acquire Audio Fade H/w.... !");
    }

    void setVideoProperty_soc(GstElement *pipeline)
    {
        GstElement *audioSink = NULL;
        g_object_get(pipeline, "amlhalasink", &audioSink, NULL);

        if ( audioSink == NULL )
        {
           LOG_RGU("PlaybackGroupNative  audioSink is null,  NOT SETTING VIDEO property");
        }
        else {
           g_object_set(audioSink, "wait-video", TRUE, NULL);
           gst_object_unref(audioSink);
           audioSink = NULL;
        }
    }

    void processAudioGap_soc(GstElement *pipeline,gint64 gapstartpts,gint32 gapduration,gint64 gapdiscontinuity, bool audioaac)
    {
        if (gapduration >= MIN_AUDIO_GAP_SUPPORTED) {
            GstElement *audioSink = NULL;
            g_object_get(pipeline, "amlhalasink", &audioSink, NULL);
            if (audioSink){
                //LOG_RGU( "gap-start-pts/duration: %" PRId64 "/%d, pts_offset=%lld",gapstartpts, gapduration);
                g_object_set(audioSink, "gap-start-pts", gapstartpts, NULL);
                g_object_set(audioSink, "gap-duration", gapduration, NULL);
                g_object_unref(G_OBJECT (audioSink));
            }
        }
    }

    void enableAudioSwitch_soc(GstElement *pipeline)
    {
        GstElement *audioSink = NULL;
        g_object_get(G_OBJECT (pipeline), "audio-sink", &audioSink, NULL);
        if (audioSink)
        {
            LOG_RGU("setting seamless-audio switch on gstreamer");
            g_object_set(G_OBJECT (audioSink), "seamless-switch", true, NULL);
            g_object_unref(G_OBJECT (audioSink));
        }
    }

    GstElement * configureUIAudioSink_soc(bool TTSenabled)
    {
        GstElement *audioSink = NULL;
        if (!TTSenabled)
        {
            LOG_RGU("configureUIAudioSink_soc: connecting amlhalasink");
            audioSink = gst_element_factory_make ("amlhalasink", "AudioMixer-Sink");
            g_object_set(G_OBJECT(audioSink), "direct-mode", FALSE, NULL);
        }
        else
        {
            LOG_RGU("configureUIAudioSink_soc: : connecting fakesink");
            audioSink = gst_element_factory_make ("fakesink","autoaudiosink");
        }
        return audioSink;
    }

    bool isUIAudioVGAudioMixSupported_soc()
    {
        return false;
    }

    unsigned getNativeAudioFlag_soc()
    {
        return 0; //Amlogic doesnt support HW decode

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

    static void haltAudioPlayback(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup)
    {
        GstState current_state, pending;

        // Get Current State of the Pipeline
        gst_element_get_state(pgstUtilsPlaybackGroup->gstPipeline, &current_state, &pending, GST_CLOCK_TIME_NONE);
        if (current_state == GST_STATE_PLAYING)
            LOG_RGU("OTF -> Pipeline in the play state");
        else if (current_state == GST_STATE_PAUSED)
            LOG_RGU("OTF -> Pipeline in the paused state ");

        // Transition Playsink to Paused
        gst_element_set_state(pgstUtilsPlaybackGroup->curAudioPlaysinkBin, GST_STATE_READY);
        gst_element_get_state(pgstUtilsPlaybackGroup->curAudioPlaysinkBin, &current_state, &pending, GST_CLOCK_TIME_NONE);
        if (current_state == GST_STATE_PAUSED)
            LOG_RGU("OTF -> Current AudioPlaySinkBin State = %d", current_state);

        // Transition Decodebin to Paused
        gst_element_set_state(pgstUtilsPlaybackGroup->curAudioDecodeBin, GST_STATE_PAUSED);
        gst_element_get_state(pgstUtilsPlaybackGroup->curAudioDecodeBin, &current_state, &pending, GST_CLOCK_TIME_NONE);
        if (current_state == GST_STATE_PAUSED)
            LOG_RGU("OTF -> Current DecodeBin State = %d", current_state);


    }

    static void firstTimeSwitchFromAC3toAAC(struct rdkGstreamerUtilsPlaybackGrp* pgstUtilsPlaybackGroup,GstCaps *newAudioCaps)
    {
        GstState current_state, pending;
        GstPad *pTypfdSrcPad = NULL;
        GstPad *pTypfdSrcPeerPad = NULL;
        GstPad *pNewAudioDecoderSrcPad = NULL;
        GstElement *newAudioParse = NULL;
        GstElement *newAudioDecoder = NULL;
        GstElement *newQueue = NULL;
        gboolean linkRet = false;

        // Get Current State of the Pipeline
        gst_element_get_state(pgstUtilsPlaybackGroup->gstPipeline, &current_state, &pending, GST_CLOCK_TIME_NONE);
        if (current_state == GST_STATE_PLAYING)
            LOG_RGU("OTF -> Pipeline in the play state");
        else if (current_state == GST_STATE_PAUSED)
            LOG_RGU("OTF -> Pipeline in the paused state ");

        /* Get the SinkPad of ASink - pTypfdSrcPeerPad */
        if ((pTypfdSrcPad = gst_element_get_static_pad(pgstUtilsPlaybackGroup->curAudioTypefind, "src")) != NULL) // Unref the Pad
            LOG_RGU("OTF -> Current Typefind SrcPad = %p", pTypfdSrcPad);

        if ((pTypfdSrcPeerPad = gst_pad_get_peer(pTypfdSrcPad)) != NULL) // Unref the Pad
            LOG_RGU("OTF -> Current Typefind Src Downstream Element Pad = %p", pTypfdSrcPeerPad);

        // AudioDecoder Downstream Unlink
        if (gst_pad_unlink(pTypfdSrcPad, pTypfdSrcPeerPad) == FALSE)
            LOG_RGU("OTF -> Typefind Downstream Unlink Failed");

        newAudioParse     = gst_element_factory_make ("aacparse", "aacparse");
        newAudioDecoder   = gst_element_factory_make ("avdec_aac", "avdec_aac");
        newQueue          = gst_element_factory_make ("queue", "aqueue");

        // Add new Decoder to Decodebin
        if (gst_bin_add(GST_BIN(pgstUtilsPlaybackGroup->curAudioDecodeBin), newAudioDecoder) == TRUE)
        {
            LOG_RGU("OTF -> Added New AudioDecoder = %p", newAudioDecoder);
        }

        // Add new Parser to Decodebin
        if (gst_bin_add(GST_BIN(pgstUtilsPlaybackGroup->curAudioDecodeBin), newAudioParse) == TRUE)
        {
            LOG_RGU("OTF -> Added New AudioParser = %p", newAudioParse);
        }

        // Add new Queue to Decodebin
        if (gst_bin_add(GST_BIN(pgstUtilsPlaybackGroup->curAudioDecodeBin), newQueue) == TRUE)
        {
            LOG_RGU("OTF -> Added New queue = %p", newQueue);
        }

        if ((pNewAudioDecoderSrcPad = gst_element_get_static_pad(newAudioDecoder, "src")) != NULL) // Unref the Pad
            LOG_RGU("OTF -> New AudioDecoder Src Pad = %p", pNewAudioDecoderSrcPad);

        //Connect decoder to ASINK
        if (gst_pad_link(pNewAudioDecoderSrcPad, pTypfdSrcPeerPad) != GST_PAD_LINK_OK)
            LOG_RGU("OTF -> New AudioDecoder Downstream Link Failed");

        linkRet = gst_element_link_many(newAudioParse, newQueue, newAudioDecoder, NULL);
        if (!linkRet)
            LOG_RGU("OTF -> Downstream Link Failed for typefine, parsr, decoder");

        /* Force Caps */
        LOG_RGU("OTF -> Typefind Setting to READY");
        gst_element_set_state(pgstUtilsPlaybackGroup->curAudioTypefind, GST_STATE_READY);
        g_object_set(G_OBJECT(pgstUtilsPlaybackGroup->curAudioTypefind), "force-caps", newAudioCaps, NULL);
        gst_element_sync_state_with_parent(pgstUtilsPlaybackGroup->curAudioTypefind);
        gst_element_get_state(pgstUtilsPlaybackGroup->curAudioTypefind, &current_state, &pending, GST_CLOCK_TIME_NONE);

        LOG_RGU("OTF -> New Typefind State = %d Pending = %d", current_state, pending);
        LOG_RGU("OTF -> Typefind Syncing with Parent");

        pgstUtilsPlaybackGroup->linkTypefindParser = true;

        /* Update the state */
        gst_element_sync_state_with_parent(newAudioDecoder);
        gst_element_get_state(newAudioDecoder, &current_state, &pending, GST_CLOCK_TIME_NONE);
        LOG_RGU("OTF -> New AudioDecoder State = %d Pending = %d", current_state, pending);

        gst_element_sync_state_with_parent(newQueue);
        gst_element_get_state(newQueue, &current_state, &pending, GST_CLOCK_TIME_NONE);
        LOG_RGU("OTF -> New queue State = %d Pending = %d", current_state, pending);

        gst_element_sync_state_with_parent(newAudioParse);
        gst_element_get_state(newAudioParse, &current_state, &pending, GST_CLOCK_TIME_NONE);
        LOG_RGU("OTF -> New AudioParser State = %d Pending = %d", current_state, pending);

        gst_object_unref(pTypfdSrcPad);
        gst_object_unref(pTypfdSrcPeerPad);
        gst_object_unref(pNewAudioDecoderSrcPad);
        return;
    }

    static bool switchAudioCodec(struct rdkGstreamerUtilsPlaybackGrp* pgstUtilsPlaybackGroup,bool isAudioAAC, GstCaps *newAudioCaps)
    {
        bool ret = false;
        LOG_RGU("Current Audio Codec AAC = %d Same as Incoming audio Codec AAC = %d", pgstUtilsPlaybackGroup->isAudioAAC, isAudioAAC);
        if (pgstUtilsPlaybackGroup->isAudioAAC == isAudioAAC)
        {
            return ret;
        }

        if ((pgstUtilsPlaybackGroup->curAudioDecoder == NULL) && (!(pgstUtilsPlaybackGroup->isAudioAAC)) && (isAudioAAC))
        {
            firstTimeSwitchFromAC3toAAC(pgstUtilsPlaybackGroup,newAudioCaps);
            pgstUtilsPlaybackGroup->isAudioAAC = isAudioAAC;
            return true;
        }

        GstElement *newAudioParse = NULL;
        GstElement *newAudioDecoder = NULL;

        GstPad *newAudioParseSrcPad = NULL;
        GstPad *newAudioParseSinkPad = NULL;
        GstPad *newAudioDecoderSrcPad = NULL;
        GstPad *newAudioDecoderSinkPad = NULL;

        GstPad *audioDecSrcPad = NULL;
        GstPad *audioDecSinkPad = NULL;
        GstPad *audioDecSrcPeerPad = NULL;
        GstPad *audioDecSinkPeerPad = NULL;

        GstPad *audioParseSrcPad = NULL;
        GstPad *audioParseSinkPad = NULL;
        GstPad *audioParseSrcPeerPad = NULL;
        GstPad *audioParseSinkPeerPad = NULL;

        GstState current_state, pending;

        // Get Current State of the Pipeline
        gst_element_get_state(pgstUtilsPlaybackGroup->gstPipeline, &current_state, &pending, GST_CLOCK_TIME_NONE);
        if (current_state == GST_STATE_PLAYING)
            LOG_RGU( "OTF -> Pipeline in the play state");
        else if (current_state == GST_STATE_PAUSED)
            LOG_RGU("OTF -> Pipeline in the paused state ");

        // Get AudioDecoder Src Pads
        if ((audioDecSrcPad = gst_element_get_static_pad(pgstUtilsPlaybackGroup->curAudioDecoder, "src")) != NULL) // Unref the Pad
            LOG_RGU( "OTF -> Current AudioDecoder Src Pad = %p", audioDecSrcPad);

        // Get AudioDecoder Sink Pads
        if ((audioDecSinkPad = gst_element_get_static_pad(pgstUtilsPlaybackGroup->curAudioDecoder, "sink")) != NULL) // Unref the Pad
            LOG_RGU("OTF -> Current AudioDecoder Sink Pad = %p", audioDecSinkPad);

        // Get AudioDecoder Src Peer i.e. Downstream Element Pad
        if ((audioDecSrcPeerPad = gst_pad_get_peer(audioDecSrcPad)) != NULL) // Unref the Pad
            LOG_RGU( "OTF -> Current AudioDecoder Src Downstream Element Pad = %p", audioDecSrcPeerPad);

        // Get AudioDecoder Sink Peer i.e. Upstream Element Pad
        if ((audioDecSinkPeerPad = gst_pad_get_peer(audioDecSinkPad)) != NULL) // Unref the Pad
            LOG_RGU( "OTF -> Current AudioDecoder Sink Upstream Element Pad = %p", audioDecSinkPeerPad);

        // Get AudioParser Src Pads
        if ((audioParseSrcPad = gst_element_get_static_pad(pgstUtilsPlaybackGroup->curAudioParse, "src")) != NULL) // Unref the Pad
            LOG_RGU( "OTF -> Current AudioParser Src Pad = %p", audioParseSrcPad);

        // Get AudioParser Sink Pads
        if ((audioParseSinkPad = gst_element_get_static_pad(pgstUtilsPlaybackGroup->curAudioParse, "sink")) != NULL) // Unref the Pad
            LOG_RGU("OTF -> Current AudioParser Sink Pad = %p", audioParseSinkPad);

        // Get AudioParser Src Peer i.e. Downstream Element Pad
        if ((audioParseSrcPeerPad = gst_pad_get_peer(audioParseSrcPad)) != NULL) // Unref the Peer Pad
            LOG_RGU("OTF -> Current AudioParser Src Downstream Element Pad = %p", audioParseSrcPeerPad);

        // Get AudioParser Sink Peer i.e. Upstream Element Pad
        if ((audioParseSinkPeerPad = gst_pad_get_peer(audioParseSinkPad)) != NULL) // Unref the Peer Pad
            LOG_RGU("OTF -> Current AudioParser Sink Upstream Element Pad = %p", audioParseSinkPeerPad);

        // AudioDecoder Downstream Unlink
        if (gst_pad_unlink(audioDecSrcPad, audioDecSrcPeerPad) == FALSE)
            LOG_RGU("OTF -> AudioDecoder Downstream Unlink Failed");

        // AudioDecoder Upstream Unlink
        if (gst_pad_unlink(audioDecSinkPeerPad, audioDecSinkPad) == FALSE)
            LOG_RGU("OTF -> AudioDecoder Upstream Unlink Failed");

        // AudioParser Downstream Unlink
        if (gst_pad_unlink(audioParseSrcPad, audioParseSrcPeerPad) == FALSE)
            LOG_RGU("OTF -> AudioParser Downstream Unlink Failed");

        // AudioParser Upstream Unlink
        if (gst_pad_unlink(audioParseSinkPeerPad, audioParseSinkPad) == FALSE)
            LOG_RGU("OTF -> AudioParser Upstream Unlink Failed");

        // Current Audio Decoder NULL
        gst_element_set_state(pgstUtilsPlaybackGroup->curAudioDecoder, GST_STATE_NULL);
        gst_element_get_state(pgstUtilsPlaybackGroup->curAudioDecoder, &current_state, &pending, GST_CLOCK_TIME_NONE);
        if (current_state == GST_STATE_NULL)
            LOG_RGU("OTF -> Current AudioDecoder State = %d", current_state);

        // Current Audio Parser NULL
        gst_element_set_state(pgstUtilsPlaybackGroup->curAudioParse, GST_STATE_NULL);
        gst_element_get_state(pgstUtilsPlaybackGroup->curAudioParse, &current_state, &pending, GST_CLOCK_TIME_NONE);
        if (current_state == GST_STATE_NULL)
            LOG_RGU("OTF -> Current AudioParser State = %d", current_state);

        // Remove Audio Decoder From Decodebin
        if (gst_bin_remove(GST_BIN(pgstUtilsPlaybackGroup->curAudioDecodeBin), pgstUtilsPlaybackGroup->curAudioDecoder) == TRUE)
        {
            LOG_RGU("OTF -> Removed AudioDecoder = %p", pgstUtilsPlaybackGroup->curAudioDecoder);
            pgstUtilsPlaybackGroup->curAudioDecoder=NULL;
        }

        // Remove Audio Parser From Decodebin
        if (gst_bin_remove(GST_BIN(pgstUtilsPlaybackGroup->curAudioDecodeBin), pgstUtilsPlaybackGroup->curAudioParse) == TRUE)
        {
            LOG_RGU("OTF -> Removed AudioParser = %p", pgstUtilsPlaybackGroup->curAudioParse);
            pgstUtilsPlaybackGroup->curAudioParse=NULL;
        }
        //TODO
        // Create new Audio Decoder and Parser. The inverse of the current
        if (pgstUtilsPlaybackGroup->isAudioAAC) {
            newAudioParse     = gst_element_factory_make ("ac3parse", "ac3parse");
            newAudioDecoder   = gst_element_factory_make ("identity", "fake_aud_ac3dec");
        }
        else {
            newAudioParse     = gst_element_factory_make ("aacparse", "aacparse");
            newAudioDecoder   = gst_element_factory_make ("avdec_aac", "avdec_aac");
        }

        {
            GstPadLinkReturn gstPadLinkRet = GST_PAD_LINK_OK;
            GstElement *audioParseUpstreamEl = NULL;

            // Add new Decoder to Decodebin
            if (gst_bin_add(GST_BIN(pgstUtilsPlaybackGroup->curAudioDecodeBin), newAudioDecoder) == TRUE)
            {
                LOG_RGU("OTF -> Added New AudioDecoder = %p", newAudioDecoder);
            }

            // Add new Parser to Decodebin
            if (gst_bin_add(GST_BIN(pgstUtilsPlaybackGroup->curAudioDecodeBin), newAudioParse) == TRUE)
            {
                LOG_RGU("OTF -> Added New AudioParser = %p", newAudioParse);
            }

            if ((newAudioDecoderSrcPad = gst_element_get_static_pad(newAudioDecoder, "src")) != NULL) // Unref the Pad
                LOG_RGU("OTF -> New AudioDecoder Src Pad = %p", newAudioDecoderSrcPad);
            if ((newAudioDecoderSinkPad = gst_element_get_static_pad(newAudioDecoder, "sink")) != NULL) // Unref the Pad
                LOG_RGU("OTF -> New AudioDecoder Sink Pad = %p", newAudioDecoderSinkPad);

            // Link New Decoder to Downstream followed by UpStream
            if ((gstPadLinkRet = gst_pad_link(newAudioDecoderSrcPad, audioDecSrcPeerPad)) != GST_PAD_LINK_OK)
                LOG_RGU("OTF -> New AudioDecoder Downstream Link Failed");
            if ((gstPadLinkRet = gst_pad_link(audioDecSinkPeerPad, newAudioDecoderSinkPad)) != GST_PAD_LINK_OK)
                LOG_RGU("OTF -> New AudioDecoder Upstream Link Failed");

            if ((newAudioParseSrcPad = gst_element_get_static_pad(newAudioParse, "src")) != NULL) // Unref the Pad
                LOG_RGU("OTF -> New AudioParser Src Pad = %p", newAudioParseSrcPad);
            if ((newAudioParseSinkPad = gst_element_get_static_pad(newAudioParse, "sink")) != NULL) // Unref the Pad
                LOG_RGU("OTF -> New AudioParser Sink Pad = %p", newAudioParseSinkPad);

            // Link New Parser to Downstream followed by UpStream
            if ((gstPadLinkRet = gst_pad_link(newAudioParseSrcPad, audioParseSrcPeerPad)) != GST_PAD_LINK_OK)
                LOG_RGU("OTF -> New AudioParser Downstream Link Failed %d", gstPadLinkRet);

            if ((audioParseUpstreamEl = GST_ELEMENT_CAST(gst_pad_get_parent(audioParseSinkPeerPad))) == pgstUtilsPlaybackGroup->curAudioTypefind)
            {

                LOG_RGU("OTF -> Typefind Setting to READY");
                gst_element_set_state(audioParseUpstreamEl, GST_STATE_READY);
                g_object_set(G_OBJECT(audioParseUpstreamEl), "force-caps", newAudioCaps, NULL);
                gst_element_sync_state_with_parent(audioParseUpstreamEl);
                gst_element_get_state(audioParseUpstreamEl, &current_state, &pending, GST_CLOCK_TIME_NONE);

                LOG_RGU("OTF -> New Typefind State = %d Pending = %d", current_state, pending);
                LOG_RGU("OTF -> Typefind Syncing with Parent");

                pgstUtilsPlaybackGroup->linkTypefindParser = true;

                gst_object_unref(audioParseUpstreamEl);
            }

            gst_object_unref(newAudioDecoderSrcPad);
            gst_object_unref(newAudioDecoderSinkPad);
            gst_object_unref(newAudioParseSrcPad);
            gst_object_unref(newAudioParseSinkPad);
        }

        gst_object_unref(audioParseSinkPeerPad);
        gst_object_unref(audioParseSrcPeerPad);
        gst_object_unref(audioParseSinkPad);
        gst_object_unref(audioParseSrcPad);

        gst_object_unref(audioDecSinkPeerPad);
        gst_object_unref(audioDecSrcPeerPad);
        gst_object_unref(audioDecSinkPad);
        gst_object_unref(audioDecSrcPad);

        gst_element_sync_state_with_parent(newAudioDecoder);
        gst_element_get_state(newAudioDecoder, &current_state, &pending, GST_CLOCK_TIME_NONE);
        LOG_RGU("OTF -> New AudioDecoder State = %d Pending = %d", current_state, pending);

        gst_element_sync_state_with_parent(newAudioParse);
        gst_element_get_state(newAudioParse, &current_state, &pending, GST_CLOCK_TIME_NONE);
        LOG_RGU("OTF -> New AudioParser State = %d Pending = %d", current_state, pending);

        pgstUtilsPlaybackGroup->isAudioAAC = isAudioAAC;

        return true;
    }

    static void resumeAudioPlayback(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup)
    {
        GstState current_state, pending;

        gst_element_sync_state_with_parent(pgstUtilsPlaybackGroup->curAudioPlaysinkBin);
        gst_element_get_state(pgstUtilsPlaybackGroup->curAudioPlaysinkBin, &current_state, &pending, GST_CLOCK_TIME_NONE);
        LOG_RGU("OTF -> AudioPlaysinkbin State = %d Pending = %d", current_state, pending);

        gst_element_sync_state_with_parent(pgstUtilsPlaybackGroup->curAudioDecodeBin);
        gst_element_get_state(pgstUtilsPlaybackGroup->curAudioDecodeBin, &current_state, &pending, GST_CLOCK_TIME_NONE);
        LOG_RGU("OTF -> Decodebin State = %d Pending = %d", current_state, pending);

    }

    bool performAudioTrackCodecChannelSwitch_soc(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup, const void *pSampleAttr, AudioAttributes *pAudioAttr, uint32_t *pStatus, unsigned int *pui32Delay,
                                                 llong *pAudioChangeTargetPts, const llong *pcurrentDispPts, unsigned int *audio_change_stage, GstCaps **appsrcCaps,
                                                 bool *audioaac, bool svpenabled, GstElement *aSrc, bool *ret)
    {
        struct timespec ts, now;
        unsigned int reconfig_delay_ms;
        clock_gettime(CLOCK_MONOTONIC, &ts);

        if (*pStatus != OK || pSampleAttr == nullptr)
        {
            LOG_RGU("No audio data ready yet");
            printf("\nNo audio data ready yet");
            *pui32Delay = WAIT_WHILE_IDLING;
            *ret = false;
            return true;
        }

        LOG_RGU("Received first audio packet from Netflix after a flush, PTS ");
        printf("\nReceived first audio packet from Netflix after a flush, PTS ");

        if (pAudioAttr)
        {

            const char *pCodecStr = pAudioAttr->mCodecParam.c_str();
            const char *pCodecAcc = strstr(pCodecStr, "mp4a");
            bool isAudioAAC = (pCodecAcc) ? true : false;
            bool isCodecSwitch = false;

            LOG_RGU("Audio Atrtibue format %s  channel %d samp %d, bitrate %d blockAligment %d",
                    pCodecStr, pAudioAttr->mNumberOfChannels,
                    pAudioAttr->mSamplesPerSecond, pAudioAttr->mBitrate,
                    pAudioAttr->mBlockAlignment);

            *pAudioChangeTargetPts = *pcurrentDispPts;
            *audio_change_stage = AUDCHG_ALIGN;

            if (*appsrcCaps)
            {
                gst_caps_unref(*appsrcCaps);
                *appsrcCaps = NULL;
            }

            if (isAudioAAC != *audioaac)
                isCodecSwitch = true;

            configAudioCap(pAudioAttr, audioaac, svpenabled, appsrcCaps);
            {
                gboolean ret = FALSE;
                GstEvent *flush_start = NULL;
                GstEvent *flush_stop = NULL;

                flush_start = gst_event_new_flush_start();
                ret = gst_element_send_event(aSrc, flush_start);
                if (!ret)
                    LOG_RGU("failed to send flush-start event");

                flush_stop = gst_event_new_flush_stop(TRUE);
                ret = gst_element_send_event(aSrc, flush_stop);
                if (!ret)
                    LOG_RGU("failed to send flush-stop event");
            }
            if (!isCodecSwitch)
            {
                LOG_RGU("TRACK SWITCH mAudioAAC = %d", *audioaac);
                gst_app_src_set_caps(GST_APP_SRC(aSrc), *appsrcCaps);
            }
            else
            {
                LOG_RGU("CODEC SWITCH mAudioAAC = %d", *audioaac);
                haltAudioPlayback(pgstUtilsPlaybackGroup);

                if (switchAudioCodec(pgstUtilsPlaybackGroup,*audioaac, *appsrcCaps) == false)
                {
                    LOG_RGU("CODEC SWITCH FAILED switchAudioCodec mAudioAAC = %d", *audioaac);
                }

                gst_app_src_set_caps(GST_APP_SRC(aSrc), *appsrcCaps);

                resumeAudioPlayback(pgstUtilsPlaybackGroup) ;
            }
            clock_gettime(CLOCK_MONOTONIC, &now);
            reconfig_delay_ms = now.tv_nsec > ts.tv_nsec ? (now.tv_nsec - ts.tv_nsec) / 1000000 : (1000 - (ts.tv_nsec - now.tv_nsec) / 1000000);

            (*pAudioChangeTargetPts) += (reconfig_delay_ms + sAudioChangeGapThresholdMS);
            //mInAudioChange.store(false);
        }
        else
        {
            LOG_RGU("####### first audio after change no atrribute drop! ");
            *pui32Delay = 0;
            *ret = false;
            return true;
        }
        *ret = true;
        return true;
    }

    void setAppSrcParams_soc(GstElement *aSrc,MediaType mediatype)
    {
        if (mediatype == MEDIA_VIDEO)
        {
            g_object_set(aSrc, "max-bytes", (guint64) 512 * 1024, NULL);
            g_object_set(aSrc, "max-buffers", (guint64) 250, NULL);
        }
        else
            g_object_set(aSrc, "max-bytes", (guint64) 1 * 64 * 1024, NULL);
        g_object_set(aSrc, "min-percent", (guint) 25, NULL);  //to signal that we need more data before we completely run dry

    }

    void setPixelAspectRatio_soc(GstCaps ** ppCaps,GstCaps *appsrcCaps,uint32_t pixelAspectRatioX,uint32_t pixelAspectRatioY)
    {
        gchar* caps_string = gst_caps_to_string (appsrcCaps);

        caps_string = g_strdup_printf("%s, pixel-aspect-ratio=(fraction)%d/%d", caps_string, pixelAspectRatioX, pixelAspectRatioY);
        *ppCaps = gst_caps_from_string(caps_string);

        g_free(caps_string);
    }

    void onHaveType_cb (GstElement* typefind,guint probability,const GstCaps* caps,gpointer data)
    {
        struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup = (struct rdkGstreamerUtilsPlaybackGrp *)data;
        gchar*          typefindCaps = NULL;

        if (caps == NULL) {
            LOG_RGU("Typefind SRC Pad Caps NULL");
            return;
        }

        typefindCaps = gst_caps_to_string(caps);
        if (typefindCaps) {

            if (g_strrstr(typefindCaps, "audio/")) {

                GstElement* typeFindParent = (GstElement *) gst_element_get_parent(typefind);
                if (typeFindParent) {

                    gchar* elementName = gst_element_get_name(typeFindParent);
                    if (elementName && g_strrstr(elementName, "decodebin")) {
                        pgstUtilsPlaybackGroup->curAudioDecodeBin=typeFindParent;
                        pgstUtilsPlaybackGroup->curAudioTypefind=typefind;
                        if (pgstUtilsPlaybackGroup->linkTypefindParser == true) {
                            if (gst_element_link(typefind, pgstUtilsPlaybackGroup->curAudioParse) == true) {
                                pgstUtilsPlaybackGroup->linkTypefindParser = false;
                            }
                        }
                    }
                    g_free(elementName);

                }
            }
            g_free(typefindCaps);
        }
    }

    void deepElementAdded_soc(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup, GstBin* pipeline, GstBin* bin, GstElement* element)
    {
        if (GST_OBJECT_PARENT (element) == GST_OBJECT_CAST (bin)) {
        gchar* elementName = gst_element_get_name(element);
        if (elementName) {
            if (g_strrstr(elementName, "typefind")) {
                g_signal_connect (G_OBJECT (element), "have-type", G_CALLBACK (onHaveType_cb), pgstUtilsPlaybackGroup);
            }
            g_free(elementName);
        }
        if (GST_OBJECT_CAST (bin) == GST_OBJECT_CAST (pgstUtilsPlaybackGroup->curAudioDecodeBin)) {
            gchar* elementName = gst_element_get_name(element);
            if (elementName) {
                if (g_strrstr(elementName, "parse")) {
                    pgstUtilsPlaybackGroup->curAudioParse=element;
                }
                else if (g_strrstr(elementName, "dec")) {
                    pgstUtilsPlaybackGroup->curAudioDecoder=element;
                }
                g_free(elementName);
            }
        }
        else {
            gchar* elementName = gst_element_get_name(element);
            if (elementName && g_strrstr(elementName, "asink")) {
                GstElement* audioSinkParent = (GstElement *) gst_element_get_parent(element);
                if (audioSinkParent) {
                    gchar* audiSinkParentName = gst_element_get_name(audioSinkParent);
                    if (audiSinkParentName && g_strrstr(audiSinkParentName, "bin")) {
                        pgstUtilsPlaybackGroup->curAudioPlaysinkBin=audioSinkParent;
                    }
                    g_free(audiSinkParentName);
                }
                g_free(elementName);
            }
        }
        }
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
} //namespace rdk_gstreamer_utils_soc.cpp
