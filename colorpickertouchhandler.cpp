#include "colorpickertouchhandler.h"
#include "colorpicker.h"
#include <QTouchEvent>
#include <QGesture>
#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

ColorPickerTouchHandler::ColorPickerTouchHandler(ColorPicker *parent)
    : TouchInteractionHandler(parent)
    , m_colorPicker(parent)
    , m_isHueSatDragging(false)
    , m_isBrightnessDragging(false)
{
    if (m_colorPicker) {
        m_colorPicker->setAttribute(Qt::WA_AcceptTouchEvents);
        m_colorPicker->grabGesture(Qt::PinchGesture);
        m_colorPicker->grabGesture(Qt::SwipeGesture);
        m_colorPicker->grabGesture(Qt::PanGesture);
    }
}

bool ColorPickerTouchHandler::handleTouchBegin(QTouchEvent *event)
{
    const QList<QEventPoint> &touchPoints = event->points();
    if (touchPoints.isEmpty()) return false;

    const QPointF &touchPoint = touchPoints.first().position();
    
    if (isTouchInHueSatArea(touchPoint)) {
        m_isHueSatDragging = true;
        updateHueSatFromTouch(touchPoint);
    } else if (isTouchInBrightnessArea(touchPoint)) {
        m_isBrightnessDragging = true;
        updateBrightnessFromTouch(touchPoint);
    }
    return true;
}

bool ColorPickerTouchHandler::handleTouchUpdate(QTouchEvent *event)
{
    const QList<QEventPoint> &touchPoints = event->points();
    if (touchPoints.isEmpty()) return false;

    const QPointF &touchPoint = touchPoints.first().position();
    
    if (m_isHueSatDragging) {
        updateHueSatFromTouch(touchPoint);
    } else if (m_isBrightnessDragging) {
        updateBrightnessFromTouch(touchPoint);
    }
    return true;
}

bool ColorPickerTouchHandler::handleTouchEnd(QTouchEvent *event)
{
    m_isHueSatDragging = false;
    m_isBrightnessDragging = false;
    return true;
}

void ColorPickerTouchHandler::handlePinchGesture(QPinchGesture *gesture)
{
    // Not used in ColorPicker
}

void ColorPickerTouchHandler::handleSwipeGesture(QSwipeGesture *gesture)
{
    // Not used in ColorPicker
}

void ColorPickerTouchHandler::handlePanGesture(QPanGesture *gesture)
{
    // Pan gesture can be used for fine-tuning color selection
    if (gesture->state() == Qt::GestureStarted) {
        const QPointF pos = gesture->offset();
        if (isTouchInHueSatArea(pos)) {
            m_isHueSatDragging = true;
        } else if (isTouchInBrightnessArea(pos)) {
            m_isBrightnessDragging = true;
        }
    } else if (gesture->state() == Qt::GestureUpdated) {
        const QPointF delta = gesture->delta();
        if (m_isHueSatDragging || m_isBrightnessDragging) {
            const QPointF pos = gesture->offset();
            if (m_isHueSatDragging) {
                updateHueSatFromTouch(pos);
            } else {
                updateBrightnessFromTouch(pos);
            }
        }
    } else if (gesture->state() == Qt::GestureFinished) {
        m_isHueSatDragging = false;
        m_isBrightnessDragging = false;
    }
}

void ColorPickerTouchHandler::updateHueSatFromTouch(const QPointF &touchPoint)
{
    if (!m_colorPicker) return;
    QPointF clampedPoint = clampToHueSatArea(touchPoint);
    emit hueSatValueChanged(clampedPoint);
}

void ColorPickerTouchHandler::updateBrightnessFromTouch(const QPointF &touchPoint)
{
    if (!m_colorPicker) return;
    qreal value = clampToBrightnessRange(touchPoint.y());
    emit brightnessChanged(value);
}

bool ColorPickerTouchHandler::isTouchInHueSatArea(const QPointF &touchPoint) const
{
    if (!m_colorPicker) return false;
    
    // Check if touch is in hue/sat area but not on a button
    QRectF hueSatRect = m_colorPicker->hueSatMapRect();
    if (!hueSatRect.contains(touchPoint)) {
        return false;
    }
    
    // Additional check to ensure we're not on a button or other interactive element
    QWidget* child = m_colorPicker->childAt(touchPoint.toPoint());
    if (child && (qobject_cast<QPushButton*>(child) || 
                  qobject_cast<QSlider*>(child) || 
                  qobject_cast<QSpinBox*>(child))) {
        return false;
    }
    
    return true;
}

bool ColorPickerTouchHandler::isTouchInBrightnessArea(const QPointF &touchPoint) const
{
    if (!m_colorPicker) return false;
    
    // Check if touch is in brightness area but not on a button
    QRectF brightnessRect = m_colorPicker->brightnessSliderRect();
    if (!brightnessRect.contains(touchPoint)) {
        return false;
    }
    
    // Additional check to ensure we're not on a button or other interactive element
    QWidget* child = m_colorPicker->childAt(touchPoint.toPoint());
    if (child && (qobject_cast<QPushButton*>(child) || 
                  qobject_cast<QSlider*>(child) || 
                  qobject_cast<QSpinBox*>(child))) {
        return false;
    }
    
    return true;
}

QPointF ColorPickerTouchHandler::clampToHueSatArea(const QPointF &point) const
{
    if (!m_colorPicker) return point;
    const QRectF &bounds = m_colorPicker->hueSatMapRect();
    return QPointF(
        qBound(bounds.left(), point.x(), bounds.right()),
        qBound(bounds.top(), point.y(), bounds.bottom())
    );
}

qreal ColorPickerTouchHandler::clampToBrightnessRange(qreal value) const
{
    if (!m_colorPicker) return value;
    const QRectF &bounds = m_colorPicker->brightnessSliderRect();
    return qBound(bounds.top(), value, bounds.bottom());
}