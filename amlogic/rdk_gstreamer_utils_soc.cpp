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
#include <string>
#include <audio_if.h>
#include <math.h>

namespace rdk_gstreamer_utils
{
    const uint32_t MIN_AUDIO_GAP_SUPPORTED = 35;
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

    void processAudioGap_soc(GstElement *pipeline,gint64 gapstartpts,gint32 gapduration)
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

    GstElement * getAudioSinkPlaysinkBin_soc(GstElement *element)
    {
        gchar* elementName = gst_element_get_name(element);
        GstElement* audioSinkParent = NULL;
        if (elementName && g_strrstr(elementName, "asink")) {
            audioSinkParent = (GstElement *) gst_element_get_parent(element);
            if (audioSinkParent) {
                gchar* audiSinkParentName = gst_element_get_name(audioSinkParent);
                if (!(audiSinkParentName && g_strrstr(audiSinkParentName, "bin"))) {
                    audioSinkParent = NULL;
                }
                g_free(audiSinkParentName);
            }
            g_free(elementName);
        }
        return audioSinkParent;
    }

    
    
    bool isUIAudioVGAudioMixSupported_soc()
    {
        return false;
    }

    std::map<rgu_gstelement,GstElement *> createNewAudioElements_soc(bool isAudioAAC,bool createqueue)
    {
        std::map<rgu_gstelement,GstElement *> newAudElements;
        GstElement * newAudioParse;
        GstElement * newAudioDecoder;
        GstElement * newAudioQueue;
        if (isAudioAAC){
            newAudioParse     = gst_element_factory_make ("ac3parse", "ac3parse");
            newAudioDecoder   = gst_element_factory_make ("identity", "fake_aud_ac3dec");   
        }
        else {
            newAudioParse     = gst_element_factory_make ("aacparse", "aacparse");
            newAudioDecoder   = gst_element_factory_make ("avdec_aac", "avdec_aac");
        }
        newAudElements.insert({GSTAUDIODECODER,newAudioDecoder});
        newAudElements.insert({GSTAUDIOPARSER,newAudioParse});
        if (createqueue){
            newAudioQueue = gst_element_factory_make ("queue", "aqueue");
            newAudElements.insert({GSTAUDIOQUEUE,newAudioQueue});
        }
        return newAudElements;
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
} //namespace rdk_gstreamer_utils_soc.cpp
