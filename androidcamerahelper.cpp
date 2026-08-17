#include "androidcamerahelper.h"
#include <QDebug>
#ifdef Q_OS_ANDROID
#include <jni.h>
#include <qcoreapplication_platform.h>
#include <QJniObject>
#include <QJniEnvironment>
#include <QCoreApplication>

extern "C" {
    JNIEXPORT void JNICALL javaComQutenoteAppQuteNoteActivityOnImageReceived(JNIEnv *env, jobject  /*thiz*/, jstring path)
    {
        const char *nativeString = env->GetStringUTFChars(path, nullptr);
        QString qPath = QString::fromUtf8(nativeString);
        env->ReleaseStringUTFChars(path, nativeString);
        
        QMetaObject::invokeMethod(AndroidCameraHelper::instance(), [qPath]() {
            emit AndroidCameraHelper::instance()->imageReceived(qPath);
        });
    }
}
#endif

AndroidCameraHelper* AndroidCameraHelper::m_instance = nullptr;

AndroidCameraHelper* AndroidCameraHelper::instance()
{
    if (!m_instance) {
        m_instance = new AndroidCameraHelper();
    }
    return m_instance;
}

AndroidCameraHelper::AndroidCameraHelper(QObject *parent) : QObject(parent)
{
}

AndroidCameraHelper::~AndroidCameraHelper()
= default;

void AndroidCameraHelper::openCamera()
{
#ifdef Q_OS_ANDROID
    // Request permissions first if needed (simplified for now, ideally strictly check)
    // For now assuming permissions are handled or user grants them.
    
    // Call the static method in QuteNoteActivity to start the camera intent
    QJniObject::callStaticMethod<void>(
        "com/qutenote/app/QuteNoteActivity",
        "startCameraIntent"
    );
#else
    qDebug() << "Camera not supported on this platform";
#endif
}
