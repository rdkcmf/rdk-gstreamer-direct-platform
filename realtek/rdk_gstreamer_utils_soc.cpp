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
#include <gst/app/gstappsrc.h>

#include "../rdk_gstreamer_utils.h"
namespace rdk_gstreamer_utils
{
    #define GST_AV_SYNC_OFFSET    (65)
    #define GST_PRELOAD_TIME       (30)

    enum rgu_audio_change_state {
        AUDCHG_INIT = 0,
        AUDCHG_CMD = 1,
        AUDCHG_SET = 2,
        AUDCHG_ALIGN = 3,
    };

    GstElement *mCurAudioSink;

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

    bool IntialVolSettingNeeded_soc()
    {
        return false;
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
        if (gapduration!=0){
            LOG_RGU( "gap-start-pts/duration: %lld/%d",gapstartpts, gapduration);
            GstElement *audioSink = NULL;
            g_object_get(pipeline, "audio-sink", &audioSink, NULL);
            if (audioSink) {
                g_object_set(audioSink, "gap-start-pts", gapstartpts, NULL);
                g_object_set(audioSink, "gap-duration", gapduration, NULL);
                g_object_unref(G_OBJECT (audioSink));
            }
        }
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
        LOG_RGU("configureUIAudioSink_soc: : connecting rtkaudiosink");
        audioSink = gst_element_factory_make ("rtkaudiosink","rtkaudiosink");
        g_object_set(G_OBJECT(audioSink), "media-tunnel",  FALSE, NULL);
        g_object_set(G_OBJECT(audioSink), "audio-service",  TRUE, NULL);

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

            caps_string = g_strdup_printf("audio/mpeg, mpegversion=4, secure-stream=audio, enable-svp=(string)%s", svpenabled ? "true" : "false");
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

    static bool getGstPlayerState(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup,GstState *gst_state, GstState *gst_pending, int msTimeout)
    {
      bool retVal  = false;
      GstStateChangeReturn  ret = GST_STATE_CHANGE_FAILURE;
      ret = gst_element_get_state (pgstUtilsPlaybackGroup->gstPipeline,
                                   gst_state,
                                   gst_pending,
                                   msTimeout * GST_MSECOND);
      if(ret == GST_STATE_CHANGE_FAILURE) {
        LOG_RGU("getGstPlayerState: state change failed, get_state returned: %d, state: %s pending: %s\n",
                    ret, gst_element_state_get_name(*gst_state), gst_element_state_get_name(*gst_pending));
      }
      else if(ret == GST_STATE_CHANGE_SUCCESS) {
        LOG_RGU("getGstPlayerState: state change succeeded, get_state returned: %d, state: %s pending: %s\n",
                  ret, gst_element_state_get_name(*gst_state), gst_element_state_get_name(*gst_pending));
        retVal = true;
      }
      else {
        LOG_RGU("getGstPlayerState: state change still pending, get_state returned: %d, state: %s pending: %s\n",
                  ret, gst_element_state_get_name(*gst_state), gst_element_state_get_name(*gst_pending));
      }
      return retVal;
    }

    static void setAudioSinkSync (bool bsync)
    {
        LOG_RGU("OTF -> setAudioSinkSync(%s)", (bsync?"true":"false"));
        if(NULL != mCurAudioSink)
        {
            g_object_set(mCurAudioSink, "async", bsync, NULL);
        }
        else
        {
            LOG_RGU("OTF -> setAudioSinkSync() mCurAudioSink is NULL.");
        }
        LOG_RGU("OTF -> setAudioSinkSync() Done.");
    }

    static void setAudioMasterSkip (struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup)
    {
      LOG_RGU("OTF -> setAudioMasterSkip");
      if(NULL != pgstUtilsPlaybackGroup->curAudioDecoder)
      {
        g_object_set(pgstUtilsPlaybackGroup->curAudioDecoder, "avsync-audio-skip", true, NULL);
      }
      else
      {
        LOG_RGU("OTF -> setAudioMasterSkip() pgstUtilsPlaybackGroup->curAudioDecoder is NULL.");
      }
      LOG_RGU("OTF -> setAudioMasterSkip() Done.");
    }

    static bool pauseAudioPlayback (struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup)
    {
        GstState current_state, pending;

        LOG_RGU("OTF -> pauseAudioPlayback()");
        // Get Current State of the Pipeline
        LOG_RGU("OTF -> pauseAudioPlayback(): Get Current State of gstPipeline");
        getGstPlayerState(pgstUtilsPlaybackGroup, &current_state, &pending, 0);

        if (current_state == GST_STATE_PLAYING)
            LOG_RGU("OTF -> Pipeline in the play state");
        else if (current_state == GST_STATE_PAUSED)
            LOG_RGU("OTF -> Pipeline in the paused state ");
        else
            LOG_RGU("OTF -> Pipeline in the play state");

        if(pgstUtilsPlaybackGroup->curAudioPlaysinkBin != NULL)
        {
           if(GST_STATE_PAUSED != current_state)
           {
               // Transition Playsink to Paused
               LOG_RGU("OTF -> pausePlayback(): Transition Playsink (%s) to Paused",  gst_element_get_name(pgstUtilsPlaybackGroup->curAudioPlaysinkBin));
               gst_element_set_state(pgstUtilsPlaybackGroup->curAudioPlaysinkBin, GST_STATE_PAUSED);
               gst_element_get_state(pgstUtilsPlaybackGroup->curAudioPlaysinkBin, &current_state, &pending, GST_CLOCK_TIME_NONE);
               if (current_state == GST_STATE_PAUSED)
                   LOG_RGU("OTF -> Current AudioPlaySinkBin State = %d", current_state);
           }
           else
           {
               LOG_RGU("OTF -> pausePlayback(): Playsink (%s) is already Paused",  gst_element_get_name(pgstUtilsPlaybackGroup->curAudioPlaysinkBin));
           }
        }

        if(pgstUtilsPlaybackGroup->curAudioPlaysinkBin != NULL)
        {
            if(GST_STATE_PAUSED != current_state)
            {
                // Transition Decodebin to Paused
                LOG_RGU("OTF -> pausePlayback():  Transition Decodebin (%s) to Paused",  gst_element_get_name(pgstUtilsPlaybackGroup->curAudioPlaysinkBin));
                gst_element_set_state(pgstUtilsPlaybackGroup->curAudioPlaysinkBin, GST_STATE_PAUSED);
                gst_element_get_state(pgstUtilsPlaybackGroup->curAudioPlaysinkBin, &current_state, &pending, GST_CLOCK_TIME_NONE);
                if (current_state == GST_STATE_PAUSED)
                    LOG_RGU("OTF -> Current DecodeBin State = %d", current_state);
            }
           else
           {
                LOG_RGU("OTF -> pausePlayback(): Decodebin (%s) is already Paused",  gst_element_get_name(pgstUtilsPlaybackGroup->curAudioPlaysinkBin));
           }
        }

        LOG_RGU("OTF -> pauseAudioPlayback() Done");
        return true;
    }

    static bool resumeAudioPlayback (struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup)
    {
        GstState current_state, pending;

        LOG_RGU("OTF -> resumeAudioPlayback()");

        // Get Current State of the Pipeline
        LOG_RGU("OTF -> resumeAudioPlayback(): Get Current State of gstPipeline");
        getGstPlayerState(pgstUtilsPlaybackGroup, &current_state, &pending, 0);

        LOG_RGU("OTF -> resumeAudioPlayback(): gstPipeline state: '%s'", gst_element_state_get_name(current_state));
        if ((current_state == GST_STATE_PAUSED) || (current_state == GST_STATE_PLAYING))
        {
            LOG_RGU("OTF -> resumeAudioPlayback(): Setting CurAudioPlaysinkBin State to '%s'", gst_element_state_get_name(current_state));
            gst_element_set_state(pgstUtilsPlaybackGroup->curAudioPlaysinkBin, current_state);

            LOG_RGU("OTF -> resumeAudioPlayback(): Setting CurAudioDecodebin State to '%s'", gst_element_state_get_name(current_state));
            gst_element_set_state(pgstUtilsPlaybackGroup->curAudioDecodeBin, current_state);
        }
        else
        {
            LOG_RGU("OTF -> resumeAudioPlayback(): Syncing CurAudioPlaysinkBin State with parent");
            gst_element_sync_state_with_parent(pgstUtilsPlaybackGroup->curAudioPlaysinkBin);
            LOG_RGU("OTF -> resumeAudioPlayback(): Syncing CurAudioDecodebin State with parent");
            gst_element_sync_state_with_parent(pgstUtilsPlaybackGroup->curAudioDecodeBin);
        }

        LOG_RGU("OTF -> resumeAudioPlayback(): Checking CurAudioPlaysinkBin State");
        gst_element_get_state(pgstUtilsPlaybackGroup->curAudioPlaysinkBin, &current_state, &pending, 100 * GST_MSECOND);
        LOG_RGU("OTF -> AudioPlaysinkbin State = %d Pending = %d", current_state, pending);

        LOG_RGU("OTF -> resumeAudioPlayback(): Checking CurAudioDecodeBin State");
        gst_element_get_state(pgstUtilsPlaybackGroup->curAudioDecodeBin, &current_state, &pending, 100 * GST_MSECOND);
        LOG_RGU("OTF -> Decodebin State = %d Pending = %d", current_state, pending);

        LOG_RGU("OTF -> resumeAudioPlayback() Done.");
        return true;
    }

    static GstClockTime currentPosition(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup)
    {
      gint64 currentPts = GST_CLOCK_TIME_NONE;

      GstQuery* query = gst_query_new_position(GST_FORMAT_TIME);
      if (gst_element_query(pgstUtilsPlaybackGroup->gstPipeline, query))
          gst_query_parse_position(query, 0, &currentPts);

      gst_query_unref(query);

      return currentPts;
    }

    static void configAudioPipe(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup, GstCaps* newAudioCaps)
    {
        GstState current_state, pending;
        GstPad *pTypfdSrcPad = NULL;
        GstPad *pTypfdSrcPeerPad = NULL;
        GstPad *pNewAudioDecoderSrcPad = NULL;
        GstElement* newAudioDecoder = NULL;
        GstElement* newQueue = NULL;
        gboolean linkRet = false;

        LOG_RGU("OTF -> configAudioPipe(): Switching Audio Codec to AAC for the first time");
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

        newAudioDecoder   = gst_element_factory_make ("omxaacdec", "omxaacdec");
        newQueue          = gst_element_factory_make ("queue", "aqueue");
        // Add new Decoder to Decodebin
        if (gst_bin_add(GST_BIN (pgstUtilsPlaybackGroup->curAudioDecodeBin), newAudioDecoder) == TRUE) {
            LOG_RGU("OTF -> Added New AudioDecoder = %p", newAudioDecoder);
        }

        // Add new Queue to Decodebin
        if (gst_bin_add(GST_BIN (pgstUtilsPlaybackGroup->curAudioDecodeBin), newQueue) == TRUE) {
            LOG_RGU("OTF -> Added New queue = %p", newQueue);
        }

        if ((pNewAudioDecoderSrcPad   = gst_element_get_static_pad(newAudioDecoder, "src")) != NULL)    // Unref the Pad
            LOG_RGU("OTF -> New AudioDecoder Src Pad = %p", pNewAudioDecoderSrcPad);

        //Connect decoder to ASINK
        if (gst_pad_link(pNewAudioDecoderSrcPad, pTypfdSrcPeerPad) != GST_PAD_LINK_OK)
            LOG_RGU("OTF -> New AudioDecoder Downstream Link Failed");

        linkRet = gst_element_link_many(newQueue, newAudioDecoder, NULL);
        if (!linkRet)
            LOG_RGU("OTF -> Downstream Link Failed for typefind, decoder");

        /* Force Caps */
        LOG_RGU("OTF -> Typefind Setting to READY");
        gst_element_set_state(pgstUtilsPlaybackGroup->curAudioTypefind, GST_STATE_READY);

        g_object_set(G_OBJECT (pgstUtilsPlaybackGroup->curAudioTypefind), "force-caps", newAudioCaps, NULL);

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

        gst_object_unref(pTypfdSrcPad);
        gst_object_unref(pTypfdSrcPeerPad);
        gst_object_unref(pNewAudioDecoderSrcPad);
    }

    static void reconfigAudioPipe(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup,GstCaps *newAudioCaps)
    {
        GstElement* newAudioDecoder = NULL;
        GstPad *newAudioDecoderSrcPad = NULL;
        GstPad *newAudioDecoderSinkPad = NULL;
        GstPad *audioDecSrcPad = NULL;
        GstPad *audioDecSinkPad = NULL;
        GstPad *audioDecSrcPeerPad = NULL;
        GstPad *audioDecSinkPeerPad = NULL;
        GstState current_state, pending;

        LOG_RGU("OTF -> reconfigAudioPipe(): Switching Audio Codec");

        // Get Current State of the Pipeline
        getGstPlayerState(pgstUtilsPlaybackGroup, &current_state, &pending, 0);
        if (current_state == GST_STATE_PLAYING)
          LOG_RGU("OTF -> Pipeline in the play state");
        else if (current_state == GST_STATE_PAUSED)
          LOG_RGU("OTF -> Pipeline in the paused state ");

        // Get AudioDecoder Src Pads
        if ((audioDecSrcPad = gst_element_get_static_pad(pgstUtilsPlaybackGroup->curAudioDecoder, "src")) != NULL) // Unref the Pad
            LOG_RGU("OTF -> Current AudioDecoder Src Pad = %p", audioDecSrcPad);

        // Get AudioDecoder Sink Pads
        if ((audioDecSinkPad = gst_element_get_static_pad(pgstUtilsPlaybackGroup->curAudioDecoder, "sink")) != NULL) // Unref the Pad
            LOG_RGU("OTF -> Current AudioDecoder Sink Pad = %p", audioDecSinkPad);

        // Get AudioDecoder Src Peer i.e. Downstream Element Pad
        if ((audioDecSrcPeerPad = gst_pad_get_peer(audioDecSrcPad)) != NULL) // Unref the Pad
            LOG_RGU("OTF -> Current AudioDecoder Src Downstream Element Pad = %p", audioDecSrcPeerPad);

        // Get AudioDecoder Sink Peer i.e. Upstream Element Pad
        if ((audioDecSinkPeerPad = gst_pad_get_peer(audioDecSinkPad)) != NULL) // Unref the Pad
            LOG_RGU("OTF -> Current AudioDecoder Sink Upstream Element Pad = %p", audioDecSinkPeerPad);

        // AudioDecoder Downstream Unlink
        if (gst_pad_unlink(audioDecSrcPad, audioDecSrcPeerPad) == FALSE)
            LOG_RGU("OTF -> AudioDecoder Downstream Unlink Failed");

        // AudioDecoder Upstream Unlink
        if (gst_pad_unlink(audioDecSinkPeerPad, audioDecSinkPad) == FALSE)
            LOG_RGU("OTF -> AudioDecoder Upstream Unlink Failed");

        // Current Audio Decoder NULL
        gst_element_set_state(pgstUtilsPlaybackGroup->curAudioDecoder, GST_STATE_NULL);
        gst_element_get_state(pgstUtilsPlaybackGroup->curAudioDecoder, &current_state, &pending, GST_CLOCK_TIME_NONE);
        if (current_state == GST_STATE_NULL)
            LOG_RGU("OTF -> Current AudioDecoder State = %d", current_state);

        // Remove Audio Decoder From Decodebin
        if (gst_bin_remove(GST_BIN (pgstUtilsPlaybackGroup->curAudioDecodeBin), pgstUtilsPlaybackGroup->curAudioDecoder) == TRUE) {
            LOG_RGU("OTF -> Removed AudioDecoder = %p", pgstUtilsPlaybackGroup->curAudioDecoder);
            pgstUtilsPlaybackGroup->curAudioDecoder = NULL;
        }

        // Create new Audio Decoder and Parser. The inverse of the current
        if (pgstUtilsPlaybackGroup->isAudioAAC) {
            LOG_RGU("OTF -> Switching from AAC to EAC3");
            newAudioDecoder   = gst_element_factory_make ("omxeac3dec", "omxeac3dec");
        }
        else {
            LOG_RGU("OTF -> Switching from EAC3 to AAC");
            newAudioDecoder   = gst_element_factory_make ("omxaacdec", "omxaacdec");
        }

        if(NULL != newAudioDecoder)
        {
          g_object_set(newAudioDecoder, "avsync-audio-skip", true, NULL);
        }
        else
        {
          LOG_RGU("OTF -> setAudioMasterSkip() newAudioDecoder is NULL.");
        }

        {
            GstPadLinkReturn gstPadLinkRet = GST_PAD_LINK_OK;
            GstElement*      audioDecoderUpstreamEl = NULL;
            // Add new Decoder to Decodebin
            if (gst_bin_add(GST_BIN (pgstUtilsPlaybackGroup->curAudioDecodeBin), newAudioDecoder) == TRUE) {
                LOG_RGU("OTF -> Added New AudioDecoder = %p", newAudioDecoder);
            }

            if ((newAudioDecoderSrcPad   = gst_element_get_static_pad(newAudioDecoder, "src")) != NULL)    // Unref the Pad
                LOG_RGU("OTF -> New AudioDecoder Src Pad = %p", newAudioDecoderSrcPad);

            if ((newAudioDecoderSinkPad  = gst_element_get_static_pad(newAudioDecoder, "sink")) != NULL)   // Unref the Pad
                LOG_RGU("OTF -> New AudioDecoder Sink Pad = %p", newAudioDecoderSinkPad);

            // Link New Decoder to Downstream followed by UpStream
            LOG_RGU("OTF -> audioDecSrcPeerPad's parent: %s", gst_element_get_name(gst_pad_get_parent(audioDecSrcPeerPad)));
            if ((gstPadLinkRet = gst_pad_link_full(newAudioDecoderSrcPad, audioDecSrcPeerPad,GST_PAD_LINK_CHECK_NOTHING)) != GST_PAD_LINK_OK)
                LOG_RGU("OTF -> New AudioDecoder Downstream Link Failed");

            LOG_RGU("OTF -> audioDecSinkPeerPad's parent: %s", gst_element_get_name(gst_pad_get_parent(audioDecSinkPeerPad)));
            if ((gstPadLinkRet = gst_pad_link_full(audioDecSinkPeerPad, newAudioDecoderSinkPad,GST_PAD_LINK_CHECK_NOTHING)) != GST_PAD_LINK_OK)
                LOG_RGU("OTF -> New AudioDecoder Upstream Link Failed - %d", gstPadLinkRet);

            LOG_RGU("OTF -> audioDecSinkPeerPad's parent: %s", gst_element_get_name(gst_pad_get_parent(audioDecSinkPeerPad)));
            if ((audioDecoderUpstreamEl = GST_ELEMENT_CAST (gst_pad_get_parent(audioDecSinkPeerPad))) == pgstUtilsPlaybackGroup->curAudioTypefind) {
                LOG_RGU("OTF -> Typefind Setting to READY");
                gst_element_set_state(audioDecoderUpstreamEl, GST_STATE_READY);
                g_object_set(G_OBJECT (audioDecoderUpstreamEl), "force-caps", newAudioCaps, NULL);
                LOG_RGU("OTF -> Typefind Syncing with Parent");
                gst_element_sync_state_with_parent(audioDecoderUpstreamEl);
                gst_element_get_state(audioDecoderUpstreamEl, &current_state, &pending, GST_CLOCK_TIME_NONE);
                LOG_RGU("OTF -> New Typefind State = %d Pending = %d", current_state, pending);
                 pgstUtilsPlaybackGroup->linkTypefindParser = true;
                gst_object_unref(audioDecoderUpstreamEl);
            }

            gst_object_unref(newAudioDecoderSrcPad);
            gst_object_unref(newAudioDecoderSinkPad);
        }

        gst_object_unref(audioDecSinkPeerPad);
        gst_object_unref(audioDecSrcPeerPad);
        gst_object_unref(audioDecSinkPad);
        gst_object_unref(audioDecSrcPad);

        LOG_RGU("OTF -> newAudioDecoder Syncing with Parent");
        gst_element_sync_state_with_parent(newAudioDecoder);
        LOG_RGU("OTF -> newAudioDecoder Checking newAudioDecoder");
        gst_element_get_state(newAudioDecoder, &current_state, &pending, GST_CLOCK_TIME_NONE);
        LOG_RGU("OTF -> New AudioDecoder State = %d Pending = %d", current_state, pending);

        pgstUtilsPlaybackGroup->curAudioDecoder = newAudioDecoder;
        LOG_RGU("OTF -> reconfigAudioPipe(): Switching Audio Codec Done");
    }

    static bool switchAudioCodec(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup,bool isAudioAAC, GstCaps* newAudioCaps)
    {
        bool ret = false;
        LOG_RGU("switchAudioCodec(): Current Audio Codec AAC = %d, Incoming audio Codec AAC = %d", pgstUtilsPlaybackGroup->isAudioAAC, isAudioAAC);
        if (pgstUtilsPlaybackGroup->isAudioAAC == isAudioAAC) {
            LOG_RGU("switchAudioCodec(): Current Audio Codec AAC = %d is same as Incoming audio Codec AAC = %d", pgstUtilsPlaybackGroup->isAudioAAC, isAudioAAC);
            return ret;
        }
        if ((pgstUtilsPlaybackGroup->curAudioDecoder == NULL) && (!pgstUtilsPlaybackGroup->isAudioAAC) && (isAudioAAC))
        {
            LOG_RGU("switchAudioCodec(): Current Audio Codec AAC = %d different from Incoming audio Codec AAC = %d => First time, calling configAudioPipe()", pgstUtilsPlaybackGroup->isAudioAAC, isAudioAAC);
            configAudioPipe(pgstUtilsPlaybackGroup, newAudioCaps);
        }
        else
        {
            LOG_RGU("switchAudioCodec(): Current Audio Codec AAC = %d different from Incoming audio Codec AAC = %d, calling reconfigAudioPipe()", pgstUtilsPlaybackGroup->isAudioAAC, isAudioAAC);
            reconfigAudioPipe(pgstUtilsPlaybackGroup, newAudioCaps);
        }
        pgstUtilsPlaybackGroup->isAudioAAC = isAudioAAC;
        LOG_RGU("switchAudioCodec() Done");
        return true;
    }


    bool performAudioTrackCodecChannelSwitch_soc(struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup, const void *pSampleAttr, AudioAttributes *pAudioAttr, uint32_t *pStatus, unsigned int *pui32Delay,
                                                 llong *pAudioChangeTargetPts, const llong *currentDispPts, unsigned int *audio_change_stage, GstCaps **appsrcCaps,
                                                 bool *audioaac, bool svpenabled, GstElement *aSrc, bool *ret)
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);

        LOG_RGU("OTF -> Audio change current disp pts %lld\n",
                                          *currentDispPts);

        if (*pStatus != 0 || pSampleAttr == nullptr || pAudioAttr == nullptr ) {
            LOG_RGU("OTF -> No audio data ready yet");
            *pui32Delay = 100; //delay of 100ms
            return false;
        }

        LOG_RGU("OTF -> Audio change current disp pts %lld\n",
                                      *currentDispPts);

        GstEvent* flush_start = NULL;
        GstEvent* flush_stop  = NULL;

        LOG_RGU("OTF -> Received first audio packet from Netflix after a flush\n ");
        LOG_RGU("OTF -> pSampleAttr = %p pAudioAttr= %p", pSampleAttr, pAudioAttr);
        if (pAudioAttr) {
            const char  *pCodecStr = pAudioAttr->mCodecParam.c_str();
            const char  *pCodecAcc = strstr(pCodecStr, "mp4a");
            bool isAudioAAC = (pCodecAcc) ? true : false ;
            bool isCodecSwitch = false;

            pgstUtilsPlaybackGroup->isAudioAAC = *audioaac;
            LOG_RGU(
                  "OTF -> Audio Atrtibue format %s  channel %d samp %d, bitrate %d blockAligment %d",
                  pCodecStr, pAudioAttr->mNumberOfChannels,
                  pAudioAttr->mSamplesPerSecond, pAudioAttr->mBitrate,
                  pAudioAttr->mBlockAlignment);

            *pAudioChangeTargetPts = *currentDispPts;
            *audio_change_stage = AUDCHG_ALIGN;


            LOG_RGU("OTF -> isAudioAAC = %d, *audioaac = %d", isAudioAAC, *audioaac);

            if (isAudioAAC != *audioaac)
                isCodecSwitch = true;

            LOG_RGU("OTF -> pgstUtilsPlaybackGroup->isAudioAAC = %d", pgstUtilsPlaybackGroup->isAudioAAC);
            if(NULL != pgstUtilsPlaybackGroup->curAudioDecoder)
            {
                g_object_set(pgstUtilsPlaybackGroup->curAudioDecoder, "otf-fade-out", true, NULL);
				LOG_RGU("OTF -> Do the OTF fade out done");
            }

            LOG_RGU("OTF -> Sending flush-start event");
            flush_start = gst_event_new_flush_start();
            *ret = gst_element_send_event(aSrc, flush_start);
            if (!*ret)
                LOG_RGU( "failed to send flush-start event");

            if (pgstUtilsPlaybackGroup->curAudioDecoder) {
              int next_codec = (isAudioAAC)?0:1;
              g_object_set(pgstUtilsPlaybackGroup->curAudioDecoder, "otf-next-codec", next_codec, NULL);
            }

            LOG_RGU("OTF -> Sending flush-stop event");
            flush_stop = gst_event_new_flush_stop(TRUE);
            *ret = gst_element_send_event(aSrc, flush_stop);
            if (!*ret)
                LOG_RGU("failed to send flush-stop event");

            setAudioMasterSkip(pgstUtilsPlaybackGroup);

            gint64 currentPts = currentPosition(pgstUtilsPlaybackGroup);
            if(GST_CLOCK_TIME_NONE != currentPts)
            {
              *pAudioChangeTargetPts = currentPts / (1000*1000);
              LOG_RGU("OTF -> *pAudioChangeTargetPts: %lld", *pAudioChangeTargetPts);
            }
            else
            {
              LOG_RGU("OTF -> *pAudioChangeTargetPts use *currentDispPts: %lld", *pAudioChangeTargetPts);
            }
        }
        else {
            LOG_RGU("####### OTF -> first audio after change no atrribute drop!");
             *pui32Delay = 0;
            return false;
        }

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

//OTF - Added
    void typeFoundCb (GstElement *typefind, guint probability, const GstCaps*  caps, gpointer  data)
    {
        gchar*          typefindCaps = NULL;
        if (caps == NULL) {
            LOG_RGU("onTypeFound(): Typefind SRC Pad Caps NULL");
            return;
        }
        rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup = (rdkGstreamerUtilsPlaybackGrp *) data;
        typefindCaps = gst_caps_to_string(caps);
        if (typefindCaps) {
           LOG_RGU( "onTypeFound(); Typefind SRC Pad Strm Parsed Caps %s", typefindCaps);
            if (g_strrstr(typefindCaps, "audio/")) {
                LOG_RGU("onTypeFound(): Typefind Audio Caps %s", typefindCaps);
                GstElement* typeFindParent = (GstElement *) gst_element_get_parent(typefind);
                if (typeFindParent) {
                    gchar* elementName = gst_element_get_name(typeFindParent);
                    LOG_RGU( "elementName %s", elementName);
                    if (elementName && g_strrstr(elementName, "decodebin")) {
                        LOG_RGU("onTypeFound(): pgstUtilsPlaybackGroup->curAudioDecodeBin %s", elementName);
                        pgstUtilsPlaybackGroup->curAudioDecodeBin = typeFindParent;
                        LOG_RGU("onTypeFound(): pgstUtilsPlaybackGroup->curAudioTypefind %s", gst_element_get_name(typefind));
                        pgstUtilsPlaybackGroup->curAudioTypefind = typefind;
                        if (pgstUtilsPlaybackGroup->linkTypefindParser == true) {
                            if(pgstUtilsPlaybackGroup->curAudioParse != NULL) {
                                if (gst_element_link(typefind, pgstUtilsPlaybackGroup->curAudioParse) == true) {
                                    pgstUtilsPlaybackGroup->linkTypefindParser = false;
                                    LOG_RGU("onTypeFound(): Typefind %p NewTypefind %p LINKED TO AudioParse %p", pgstUtilsPlaybackGroup->curAudioTypefind, typefind, pgstUtilsPlaybackGroup->curAudioParse);
                                }
                            }
                            else if (pgstUtilsPlaybackGroup->curAudioDecoder != NULL) {
                                if (gst_element_link(typefind, pgstUtilsPlaybackGroup->curAudioDecoder) == true) {
                                    pgstUtilsPlaybackGroup->linkTypefindParser = false;
                                    LOG_RGU("onTypeFound(): Typefind %p NewTypefind %p LINKED TO AudioDecoder %p", pgstUtilsPlaybackGroup->curAudioTypefind, typefind, pgstUtilsPlaybackGroup->curAudioDecoder);
                                }
                            }
                        }
                    }
                    g_free(elementName);
                }
            }
            g_free(typefindCaps);
        }
    }

    void deepElementAdded_soc (struct rdkGstreamerUtilsPlaybackGrp *pgstUtilsPlaybackGroup,GstBin* pipeline, GstBin* bin, GstElement* element)
    {
        pgstUtilsPlaybackGroup->gstPipeline = GST_ELEMENT(pipeline);

       LOG_RGU( "dElementAdded - Element = %p Bin = %p Pipeline = %p", element, bin, pipeline);
        if (GST_OBJECT_PARENT (element) == GST_OBJECT_CAST (bin)) {
            gchar* elementName = gst_element_get_name(element);
            if (elementName) {
               LOG_RGU( "dElementAdded - Element Name = %s", elementName);
                if (g_strrstr(elementName, "typefind")) {
                     pgstUtilsPlaybackGroup->curAudioTypefind = element;
                    LOG_RGU("Registering onTypeFoundCallback()");
                    g_signal_connect (G_OBJECT (element), "have-type", G_CALLBACK (typeFoundCb), pgstUtilsPlaybackGroup);
                }
                g_free(elementName);
            }
            if (GST_OBJECT_CAST (bin) == GST_OBJECT_CAST (pgstUtilsPlaybackGroup->curAudioDecodeBin)) {
                gchar* elementName = gst_element_get_name(element);
               LOG_RGU( "dElementAdded - Element Name = %s", elementName);
                if (elementName) {
                    if (g_strrstr(elementName, "parse")) {
                        LOG_RGU("dElementAdded - curAudioParse = %s", elementName);
                        pgstUtilsPlaybackGroup->curAudioParse = element;
                    }
                    else if (g_strrstr(elementName, "dec")) {
                        LOG_RGU("dElementAdded - curAudioDecoder = %s", elementName);
                        pgstUtilsPlaybackGroup->curAudioDecoder = element;
                    }
                    g_free(elementName);
                }
            }
            else {
                gchar* elementName = gst_element_get_name(element);
               LOG_RGU( "dElementAdded - Element Name = %s", elementName);
                if (elementName && g_strrstr(elementName, "audiosink")) {
                    mCurAudioSink = element;
                    /*Set audiosink into async false state to speedup the start of playback.*/
                    g_object_set(mCurAudioSink, "async", false, NULL);
                    GstElement* audioSinkParent = (GstElement *) gst_element_get_parent(element);
                    if (audioSinkParent) {
                        gchar* audioSinkParentName = gst_element_get_name(audioSinkParent);
                        LOG_RGU("dElementAdded - audioSinkParentName = %s", audioSinkParentName);
                        if (audioSinkParentName && g_strrstr(audioSinkParentName, "bin")) {
                            LOG_RGU("dElementAdded - curAudioPlaysinkBin = %s", audioSinkParentName);
                            pgstUtilsPlaybackGroup->curAudioPlaysinkBin = audioSinkParent;
                        }
                        g_free(audioSinkParentName);
                    }
                    g_free(elementName);
                }
            }
        }
        return;
    }

    /**
    *  Time            Size            Frames
    * ------------------------------------------
    *  48ms           9216 bytes       2304 frames
    */
    #define GST_FIFO_SIZE_MS (131)
    void audioMixerGetDeviceInfo_soc(uint32_t& preferredFrames, uint32_t& maximumFrames)
    {
        maximumFrames = GST_FIFO_SIZE_MS * 48;
        preferredFrames = maximumFrames / 4;

    }

    size_t audioMixerGetBufferDelay_soc(int64_t queuedBytes,int bufferDelayms)
    {
        return ((queuedBytes/256) * 64 +  (bufferDelayms * 48));
    }

    uint64_t audioMixerGetQueuedBytes_soc(uint64_t bytesPushed,uint64_t bytesPlayed)
    {
        return (bytesPushed-bytesPlayed-(GST_PRELOAD_TIME * 48 * 4));
    }

    void audioMixerConfigurePipeline_soc(GstElement *gstPipeline,GstElement *aSink,GstElement *aSrc,bool attenuateOutput)
    {
        const float AUDIO_VOLUME_SCALE_FACTOR=0.8;
        g_object_set(G_OBJECT(aSink), "volume", 1.0 * AUDIO_VOLUME_SCALE_FACTOR, NULL);
        g_object_set(G_OBJECT(aSink), "buffer-time",  ((100 * GST_MSECOND) / GST_USECOND), NULL);

        GstElement *convert = NULL;
        GstElement *resample = NULL;
        GstElement *volume = NULL;

        convert = gst_element_factory_make("audioconvert", NULL);
        resample = gst_element_factory_make("audioresample", NULL);
        volume = gst_element_factory_make("volume", NULL);

        gst_bin_add_many(GST_BIN(gstPipeline), aSrc, volume, convert, resample, aSink,  NULL);
        gst_element_link_many (aSrc, volume, convert, resample, aSink, NULL);
        if(attenuateOutput)
        {
           LOG_RGU("GstAudioMixerOutput: No Audio Equivalence, so attenuating mixer output");
           g_object_set(G_OBJECT(aSink), "volume", 1.0 * AUDIO_VOLUME_SCALE_FACTOR, NULL);
        }
        else
        {
           LOG_RGU("GstAudioMixerOutput: Audio Equivalence On, not attenuating mixer output");
        }
    }

    uint64_t audioMixerGetFifoSize_soc()
    {
        return (GST_FIFO_SIZE_MS * 48 * 4);
    }

    void setVideoSinkMode_soc(GstElement * videoSink)
    {
        return ;
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
            *capsString = g_strdup_printf("video/x-h265, alignment=(string)au, stream-format=(string)byte-stream,width=(int)%u, height=(int)%u, framerate=(fraction)%u/%u,enable-svp=(string)%s,enable-fastplayback=(string)true",
                                imageWidth,imageHeight,frameRateValue,frameRateScale,svpEnabled ? "true" : "false");
        }
        else if(vCodec.find(std::string("av1")) != std::string::npos)
        {
            LOG_RGU("####### Using AV1 codec\n");
            *capsString = g_strdup_printf("video/x-av1, alignment=(string)au, stream-format=(string)byte-stream,width=(int)%u, height=(int)%u, framerate=(fraction)%u/%u,enable-svp=(string)%s,enable-fastplayback=(string)true",
                                imageWidth,imageHeight,frameRateValue,frameRateScale,svpEnabled ? "true" : "false");
        }
        else
        {
            LOG_RGU("####### Using H264 codec\n");
            *capsString = g_strdup_printf("video/x-h264, alignment=(string)au, stream-format=(string)byte-stream,width=(int)%u, height=(int)%u, framerate=(fraction)%u/%u,enable-svp=(string)%s,enable-fastplayback=(string)true",
                                imageWidth,imageHeight,frameRateValue,frameRateScale,svpEnabled ? "true" : "false");
        }

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
