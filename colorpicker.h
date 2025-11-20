#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <QDialog>
#include <QColor>
#include <QColorDialog>
#include <QRectF>
#include "colorpickertouchhandler.h"
#include "smartpointers.h"
#include "thememanager.h"

#if defined(COLORPICKER_LIBRARY)
#  define COLORPICKER_EXPORT Q_DECL_EXPORT
#else
#  define COLORPICKER_EXPORT Q_DECL_IMPORT
#endif

class COLORPICKER_EXPORT ColorPicker : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ColorPicker)
    
public:
    explicit ColorPicker(QWidget *parent = nullptr);
    static QColor getColor(const QColor &initial = Qt::white, QWidget *parent = nullptr);
    
    // Methods for theme settings page
    void setColor(const QColor &color);
    QColor currentColor() const;
    
    // Methods for touch handler
    QRectF hueSatMapRect() const;
    QRectF brightnessSliderRect() const;
    
Q_SIGNALS:
    void colorChanged(const QColor &color);
    
private:
    // Touch handler (owned)
    QuteNote::OwnedPtr<ColorPickerTouchHandler> m_touchHandler;
    
    // Color state
    QColor m_color;
};

#endif // COLORPICKER_H
