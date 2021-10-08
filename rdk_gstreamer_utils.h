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

#ifndef __RDK_GSTREAMER_UTILS_H___
#define __RDK_GSTREAMER_UTILS_H___
#include <gst/gst.h>
#include <glib.h>
#include <stdint.h>
#include <stdio.h>

namespace rdk_gstreamer_utils {

    #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    #define LOG_RGU(fmt, ...) do { fprintf(stderr, "[RGU:%s:%d]: " fmt "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); fflush(stderr); } while (0)

    enum rgu_Ease
    {
        EaseLinear = 0,
        EaseInCubic,
        EaseOutCubic,
        EaseCount
    };

    bool installUnderflowCallbackFromPlatform(GstElement *pipeline,
        GCallback underflowVideoCallback,
        GCallback underflowAudioCallback,
        gpointer data);

    void initVirtualDisplayHeightandWidthFromPlatform(unsigned int* mVirtualDisplayHeight, unsigned int* mVirtualDisplayWidth);

    bool isSocAudioFadeSupported();
    void doAudioEasingonSoc(double target, uint32_t duration, rgu_Ease ease);
} // namespace rdk_gstreamer_utils
#endif /* __RDK_GSTREAMER_UTILS_H___ */
