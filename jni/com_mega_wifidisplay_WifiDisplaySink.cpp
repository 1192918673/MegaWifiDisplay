//#define LOG_NDEBUG 0
#define LOG_TAG "WifiDisplaySinkCpp"
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <binder/ProcessState.h>
#include <gui/IGraphicBufferProducer.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/ANetworkSession.h>
#include <android_runtime/android_view_Surface.h>
#include <VideoFormats.h>
#include <sink/WifiDisplaySink.h>

#include <JNIHelp.h>

#include "com_mega_wifidisplay_WifiDisplaySink.h"

using namespace android;

static const char * curHost;
static int curPort;

static sp<IGraphicBufferProducer> bufferProducer;
static sp<VideoFormats::FormatConfig> videoConfig = new VideoFormats::FormatConfig;
static sp<ALooper> looper;
static sp<WifiDisplaySink> sink;

/*
 * Class:     com_mega_wifidisplay_WifiDisplaySink
 * Method:    setSurface
 * Signature: (Landroid/view/Surface;)V
 */
JNIEXPORT void JNICALL Java_com_mega_wifidisplay_WifiDisplaySink_setSurface
  (JNIEnv * env, jclass clazz, jobject jSurface) {
    sp<Surface> surface(android_view_Surface_getSurface(env, jSurface));

    if (surface != NULL) {
        bufferProducer = surface->getIGraphicBufferProducer();
        if (bufferProducer == NULL) {
            jniThrowException(env, "java/lang/IllegalArgumentException",
                "The surface does not have a binding SurfaceTexture!");
        }
    } else {
        jniThrowException(env, "java/lang/IllegalArgumentException",
                "The surface has been released");
    }
}

/*
 * Class:     com_mega_wifidisplay_WifiDisplaySink
 * Method:    setVideoConfig
 * Signature: (IIIII)V
 */
JNIEXPORT void JNICALL Java_com_mega_wifidisplay_WifiDisplaySink_setVideoConfig
  (JNIEnv *, jclass, jint width, jint height, jint fps, jint profile, jint level) {
    videoConfig->width = width;
    videoConfig->height = height;
    videoConfig->framesPerSecond = fps;
    videoConfig->profileType = (VideoFormats::ProfileType) profile;
    videoConfig->levelType = (VideoFormats::LevelType) level;
}

/*
 * Class:     com_mega_wifidisplay_WifiDisplaySink
 * Method:    setServer
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_mega_wifidisplay_WifiDisplaySink_setServer
  (JNIEnv * env, jclass clazz, jstring host, jint port) {
    curHost = env->GetStringUTFChars(host, NULL);
    curPort = (int)port;
}

/*
 * Class:     com_mega_wifidisplay_WifiDisplaySink
 * Method:    start
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_mega_wifidisplay_WifiDisplaySink_start
  (JNIEnv * env, jclass clazz) {
    ProcessState::self()->startThreadPool();

    sp<ANetworkSession> session = new ANetworkSession;
    session->start();

    sink = new WifiDisplaySink(0, session, bufferProducer, NULL, videoConfig);

    looper = new ALooper;
    looper->registerHandler(sink);

    sink->start(curHost, curPort);

    looper->start(true /* runOnCallingThread */);
}

/*
 * Class:     com_mega_wifidisplay_WifiDisplaySink
 * Method:    stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_mega_wifidisplay_WifiDisplaySink_stop
  (JNIEnv * env, jclass clazz) {
    looper->stop();
}

/*
 * Class:     com_mega_wifidisplay_WifiDisplaySink
 * Method:    stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_mega_wifidisplay_WifiDisplaySink_stop
  (JNIEnv * env, jclass clazz) {
    looper->stop();
}
}
