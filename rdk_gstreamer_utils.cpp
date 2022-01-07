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


#include "rdk_gstreamer_utils.h"
#include "rdk_gstreamer_utils_soc.h"
#include <gst/gst.h>
#include <glib.h>
#include <string.h>


using namespace std;

namespace rdk_gstreamer_utils {

    GstElement* retrieveGstElementByName(GstElement *element, const char *targetName)
    {
        GstElement *re = NULL;
        if (GST_IS_BIN(element)) {
            GstIterator* it = gst_bin_iterate_elements(GST_BIN(element));
            GValue item = G_VALUE_INIT;
            bool done = false;
            while(!done) {
                switch (gst_iterator_next(it, &item)) {
                    case GST_ITERATOR_OK:
                    {
                        GstElement *next = GST_ELEMENT(g_value_get_object(&item));
                        done = (re = retrieveGstElementByName(next, targetName)) != NULL;
                        g_value_reset (&item);
                        break;
                    }
                    case GST_ITERATOR_RESYNC:
                        gst_iterator_resync (it);
                        break;
                    case GST_ITERATOR_ERROR:
                    case GST_ITERATOR_DONE:
                        done = true;
                        break;
                }
            }
            g_value_unset (&item);
            gst_iterator_free(it);
        } else {
            gchar* elemName = gst_element_get_name(element);
            if(elemName != NULL) {
                if (strstr(elemName, targetName)) {
                    re = element;
                }
                g_free(elemName);
            }

        }
        return re;
    }

    void initVirtualDisplayHeightandWidthFromPlatform(unsigned int* mVirtualDisplayHeight, unsigned int* mVirtualDisplayWidth)
    {
        initVirtualDisplayHeightandWidthFromPlatform_soc(mVirtualDisplayHeight, mVirtualDisplayWidth);
    }

    bool installUnderflowCallbackFromPlatform(GstElement *pipeline, GCallback underflowVideoCallback, GCallback underflowAudioCallback, gpointer data)
    {
        const char* audiodecodername = getAudioDecoderName_soc();
        GstElement* audiodecoder = retrieveGstElementByName(pipeline, audiodecodername);
        GstElement* videodecoder = retrieveGstElementByName(pipeline, "westerossink"); //default on RDK platforms
        const char* AudioUnderflowSignal = getAudioUnderflowSignalName_soc();
        const char* VideoUnderflowSignal = getVideoUnderflowSignalName_soc();

        gulong id_audio = g_signal_connect(audiodecoder, AudioUnderflowSignal, underflowAudioCallback, data);
        gulong id_video = g_signal_connect(videodecoder, VideoUnderflowSignal, underflowVideoCallback, data);

	return id_audio > 0 && id_video > 0;
    }

    bool isSocAudioFadeSupported()
    {
        return IsAudioFadeSupported_soc();
    }

    void doAudioEasingonSoc(double target, uint32_t duration, rgu_Ease ease)
    {
        EaseAudio_soc(target, duration, ease);
    }


    void setVideoProperty(GstElement *pipeline)
    {
        setVideoProperty_soc(pipeline);
    }

    void processAudioGap(GstElement *pipeline,gint64 gapstartpts,gint32 gapduration)
    {
        processAudioGap_soc(pipeline,gapstartpts,gapduration);
    }

    void enableAudioSwitch(GstElement *pipeline)
    {
        enableAudioSwitch_soc(pipeline);
    }

    GstElement * configureUIAudioSink(bool TTSenabled)
    {
        return configureUIAudioSink_soc(TTSenabled);
    }

    GstElement * getAudioSinkPlaysinkBin(GstElement *element)
    {
       return getAudioSinkPlaysinkBin_soc(element);
    }

    rgu_gstelement getAudioElement(GstElement *element)
    {
        gchar* elementName = gst_element_get_name(element);
        rgu_gstelement audelement = GSTELEMENTNULL;
        if (elementName) {
            if (g_strrstr(elementName, "parse")) {
                audelement = GSTAUDIOPARSER;
            }
            else if (g_strrstr(elementName, "dec")) {
                audelement = GSTAUDIODECODER;
            }
            g_free(elementName);
        }
        return audelement;
    }

    bool foundAudioDecodeBin(GstElement * typeFindParent)
    {
        bool found=false;
        gchar* elementName = gst_element_get_name(typeFindParent);
        if (elementName && g_strrstr(elementName, "decodebin"))
        {
           found=true;
        }
        g_free(elementName);
        return found;
    }

    bool isUIAudioVGAudioMixSupported()
    {
        return isUIAudioVGAudioMixSupported_soc();
    }

    std::map<rgu_gstelement,GstElement *> createNewAudioElements(bool isAudioAAC,bool createqueue)
    {
        return createNewAudioElements_soc(isAudioAAC,createqueue);
    }

    unsigned getNativeAudioFlag()
    {
        return getNativeAudioFlag_soc();
    }
    
    bool isPtsOffsetAdjustmentSupported()
    {
        return isPtsOffsetAdjustmentSupported_soc();
    }
    
    int getPtsOffsetAdjustment(const std::string& audioCodecString)
    {
        if (isPtsOffsetAdjustmentSupported_soc()) {
            return getPtsOffsetAdjustment_soc(audioCodecString);
        } else {
            return 0;
        }
    }

} // namespace rdk_gstreamer_utils
