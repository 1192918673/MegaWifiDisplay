//#define LOG_NDEBUG 0
#define LOG_TAG "WifiDisplaySinkCpp"
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/SurfaceComposerClient.h>
#include <media/IMediaPlayerService.h>
#include <media/IRemoteDisplay.h>
#include <media/IRemoteDisplayClient.h>
#include <media/stagefright/foundation/AString.h>

#include "com_mega_wifidisplay_WifiDisplaySource.h"

using namespace android;

static const char * curHost;
static int curPort;
static int curWidth;
static int curHeight;

class RemoteDisplayClient;

static sp<RemoteDisplayClient> curClient;

class RemoteDisplayClient : public BnRemoteDisplayClient {
private:
    Mutex mLock;
    Condition mCondition;

    bool mDone;

    sp<SurfaceComposerClient> mComposerClient;
    sp<IGraphicBufferProducer> mBufferProducer;
    sp<IBinder> mDisplayBinder;

public:
    RemoteDisplayClient() : mDone(false) {
        mComposerClient = new SurfaceComposerClient;
        mComposerClient->initCheck();
    }

    void onDisplayConnected(
            const sp<IGraphicBufferProducer> &bufferProducer,
            uint32_t width,
            uint32_t height,
            uint32_t flags,
            uint32_t session) {
        ALOGD("onDisplayConnected width=%u, height=%u, flags = 0x%08x, session = %d",
              width, height, flags, session);

        if (bufferProducer != NULL) {
            mBufferProducer = bufferProducer;
            mDisplayBinder = mComposerClient->createDisplay(String8("wfd"), false /* secure */);

            SurfaceComposerClient::openGlobalTransaction();
            mComposerClient->setDisplaySurface(mDisplayBinder, mBufferProducer);

            Rect layerStackRect(curWidth, curHeight);  // XXX fix this.
            Rect displayRect(curWidth, curHeight);

            mComposerClient->setDisplayProjection(
                    mDisplayBinder,
                    0 /* 0 degree rotation */,
                    layerStackRect,
                    displayRect);
            SurfaceComposerClient::setDisplayLayerStack(mDisplayBinder, 0);
            SurfaceComposerClient::closeGlobalTransaction();
        }
    }

    void onDisplayDisconnected() {
        ALOGD("onDisplayDisconnected");

        Mutex::Autolock autoLock(mLock);
        mDone = true;
        mCondition.broadcast();
    }

    void onDisplayError(int32_t error) {
        ALOGD("onDisplayError error=%d", error);

        Mutex::Autolock autoLock(mLock);
        mDone = true;
        mCondition.broadcast();
    }

    void waitUntilDone() {
        Mutex::Autolock autoLock(mLock);
        while (!mDone) {
            mCondition.wait(mLock);
        }
    }

    void stop() {
        ALOGD("RemoteDisplayClient stop");

        Mutex::Autolock autoLock(mLock);
        mDone = true;
        mCondition.broadcast();
    }

protected:
    ~RemoteDisplayClient() {
    }
};

/*
 * Class:     com_mega_wifidisplay_WifiDisplaySource
 * Method:    setServer
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_mega_wifidisplay_WifiDisplaySource_setServer
  (JNIEnv * env, jclass clazz, jstring host, jint port) {
    curHost = env->GetStringUTFChars(host, NULL);
    curPort = (int)port;
}

/*
 * Class:     com_mega_wifidisplay_WifiDisplaySource
 * Method:    setSize
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_mega_wifidisplay_WifiDisplaySource_setSize
  (JNIEnv * env, jclass clazz, jint width, jint height) {
    curWidth = (int)width;
    curHeight = (int)height;
}

/*
 * Class:     com_mega_wifidisplay_WifiDisplaySource
 * Method:    start
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_mega_wifidisplay_WifiDisplaySource_start
  (JNIEnv * env, jclass clazz) {
    ProcessState::self()->startThreadPool();

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16("media.player"));
    sp<IMediaPlayerService> service = interface_cast<IMediaPlayerService>(binder);

    String8 iface;
    iface.append(curHost);
    iface.append(AStringPrintf(":%d", curPort).c_str());

    curClient = new RemoteDisplayClient;
    sp<IRemoteDisplay> display =
        service->listenForRemoteDisplay(String16("android"), curClient, iface);

    curClient->waitUntilDone();

    display->dispose();
    display.clear();
}

/*
 * Class:     com_mega_wifidisplay_WifiDisplaySource
 * Method:    stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_mega_wifidisplay_WifiDisplaySource_stop
  (JNIEnv * env, jclass clazz) {
    curClient->stop();
}
