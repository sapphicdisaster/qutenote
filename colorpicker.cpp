#include "colorpicker.h"
using QuteNote::OwnedPtr;
#include <QApplication>

ColorPicker::ColorPicker(QWidget *parent)
    : QDialog(parent)
    , m_touchHandler(QuteNote::makeOwned<ColorPickerTouchHandler>(this))
    , m_color(Qt::white)
{
    // Install the touch handler as an event filter
    if (m_touchHandler) {
        m_touchHandler->enableGestureHandling(this);
    }
    
    // Ensure the dialog can receive touch events
    setAttribute(Qt::WA_AcceptTouchEvents);
}

QColor ColorPicker::getColor(const QColor &initial, QWidget *parent)
{
    // Create a QColorDialog with touch-friendly options
    QColorDialog dialog(initial, parent);
    
    // Configure dialog options for better touch experience
    dialog.setOptions(QColorDialog::DontUseNativeDialog | 
                      QColorDialog::ShowAlphaChannel);
    
    // Apply theme from ThemeManager
    ThemeManager* themeManager = ThemeManager::instance();
    if (themeManager) {
        Theme currentTheme = themeManager->currentTheme();
        QString styleSheet = QString("QDialog { background-color: %1; }"
                                   "QPushButton { min-height: %2px; min-width: %2px; }"
                                   "QSlider { min-height: %2px; }"
                                   "QSpinBox { min-height: %2px; }")
                            .arg(currentTheme.colors.background.name())
                            .arg(40); // TOUCH_TARGET_SIZE
        dialog.setStyleSheet(styleSheet);
    }
    
    // Show the dialog
    if (dialog.exec() == QDialog::Accepted) {
        return dialog.selectedColor();
    }
    return initial;
}

void ColorPicker::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        emit colorChanged(m_color);
    }
}

QColor ColorPicker::currentColor() const
{
    return m_color;
}

QRectF ColorPicker::hueSatMapRect() const
{
    // Return a default rectangle, as the actual implementation would depend on the UI structure
    return QRectF(0, 0, 200, 200);
}

QRectF ColorPicker::brightnessSliderRect() const
{
    // Return a default rectangle, as the actual implementation would depend on the UI structure
    return QRectF(210, 0, 20, 200);
}
