#ifndef COLORPICKERTOUCHHANDLER_H
#define COLORPICKERTOUCHHANDLER_H

#include "touchinteractionhandler.h"
#include <QWidget>
#include <QPointF>

class ColorPicker;

class ColorPickerTouchHandler : public TouchInteractionHandler
{
    Q_OBJECT

public:
    explicit ColorPickerTouchHandler(ColorPicker *parent = nullptr);

Q_SIGNALS:
    void hueSatValueChanged(const QPointF &value);
    void brightnessChanged(qreal value);

protected:
    bool handleTouchBegin(QTouchEvent *event) override;
    bool handleTouchUpdate(QTouchEvent *event) override;
    bool handleTouchEnd(QTouchEvent *event) override;
    void handlePinchGesture(QPinchGesture *gesture) override;
    void handleSwipeGesture(QSwipeGesture *gesture) override;
    void handlePanGesture(QPanGesture *gesture) override;

private:
    void updateHueSatFromTouch(const QPointF &touchPoint);
    void updateBrightnessFromTouch(const QPointF &touchPoint);
    bool isTouchInHueSatArea(const QPointF &touchPoint) const;
    bool isTouchInBrightnessArea(const QPointF &touchPoint) const;
    QPointF clampToHueSatArea(const QPointF &point) const;
    qreal clampToBrightnessRange(qreal value) const;

    ColorPicker *m_colorPicker;
    bool m_isHueSatDragging;
    bool m_isBrightnessDragging;
};

#endif // COLORPICKERTOUCHHANDLER_H