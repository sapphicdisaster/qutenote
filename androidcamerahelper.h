#ifndef ANDROIDCAMERAHELPER_H
#define ANDROIDCAMERAHELPER_H

#include <QObject>
#include <QString>

class AndroidCameraHelper : public QObject
{
    Q_OBJECT
public:
    static AndroidCameraHelper* instance();
    
    // Open the camera to take a picture
    static void openCamera();

signals:
    // Emitted when an image is successfully captured
    void imageReceived(const QString &path);

private:
    explicit AndroidCameraHelper(QObject *parent = nullptr);
    ~AndroidCameraHelper();
    
    static AndroidCameraHelper* m_instance;
};

#endif // ANDROIDCAMERAHELPER_H
