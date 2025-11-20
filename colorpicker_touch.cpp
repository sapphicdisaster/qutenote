#include "colorpicker_touch.h"
#include <QtCore/qglobal.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QGestureEvent>
#include <QSettings>
#include <QtMath>
 #include <QPropertyAnimation>
 #include <QScreen>
 #include <QGuiApplication>
 #include <QTimer>
 #include <QEasingCurve>
 #include <functional>
 #include <QShowEvent>


TouchColorPicker::TouchColorPicker(QWidget *parent)
    : QDialog(parent)
    , m_touchInteraction(new TouchInteraction(this))
    , m_hueSatMap(new HueSatMap(this))
    , m_colorPreview(nullptr)
    , m_touchSlider(nullptr)
    , m_recentColorsWidget(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_scale(1.0)
    , m_color(Qt::white)
    , m_updatingControls(false)
    , m_compactMode(false)
    , m_fullscreenAnimated(false)
    , m_animDuration(300)
{
    #ifndef Q_OS_ANDROID
    setWindowTitle(tr("Color Picker"));
    #endif
    setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::PanGesture);
    // Backdrop for dimming
    m_backdrop = nullptr;
    
    // Load saved recent colors
    QSettings settings;
    const QStringList colorStrings = settings.value("recentColors").toStringList();
    for (const QString &colorStr : colorStrings) {
        QColor color(colorStr);
        if (color.isValid()) {
            m_recentColors.append(color);
        }
    }
    
    setupTouchInteraction();
    createLayout();
    updateColorControls();
}

void TouchColorPicker::setCompactMode(bool enabled)
{
    if (m_compactMode == enabled)
        return;
    m_compactMode = enabled;

    // Adjust layout for compact mode: tighten margins and hide non-essential widgets
    if (layout()) {
        QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
        if (mainLayout) {
            if (m_compactMode) {
                mainLayout->setContentsMargins(8, 8, 8, 8);
                mainLayout->setSpacing(12);
            } else {
                mainLayout->setContentsMargins(20, 20, 20, 20);
                mainLayout->setSpacing(20);
            }
        }
    }

    // Hide recent colors in very compact mode to simplify UI
    if (m_recentColorsWidget)
        m_recentColorsWidget->setVisible(!m_compactMode);

    // Increase preview size in compact mode
    if (m_colorPreview) {
        if (m_compactMode)
            m_colorPreview->setMinimumHeight(64);
        else
            m_colorPreview->setMinimumHeight(MIN_TOUCH_TARGET * 2);
    }
}

void TouchColorPicker::setFullscreenAnimated(bool enabled)
{
    m_fullscreenAnimated = enabled;
    if (m_fullscreenAnimated) {
        setWindowFlag(Qt::FramelessWindowHint, true);
    } else {
        setWindowFlag(Qt::FramelessWindowHint, false);
    }
}

void TouchColorPicker::setupTouchInteraction()
{
    connect(m_touchInteraction, &TouchInteraction::pinchScaleChanged,
            this, &TouchColorPicker::setScale);
            
    connect(m_hueSatMap, &HueSatMap::colorSelected,
            this, &TouchColorPicker::setColor);

    // Install pan/swipe gestures on this dialog for dismiss
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::PanGesture);
}

void TouchColorPicker::createLayout()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Color preview at the top
    m_colorPreview = new QWidget(this);
    m_colorPreview->setMinimumHeight(MIN_TOUCH_TARGET * 2);
    m_colorPreview->setAutoFillBackground(true);
    mainLayout->addWidget(m_colorPreview);

    // Hue/Saturation map
    m_hueSatMap->setMinimumSize(MIN_TOUCH_TARGET * 4, MIN_TOUCH_TARGET * 4);
    mainLayout->addWidget(m_hueSatMap, 1);

    // Recent colors grid
    m_recentColorsWidget = new QWidget(this);
    QGridLayout *gridLayout = new QGridLayout(m_recentColorsWidget);
    gridLayout->setSpacing(8);
    
    const int rows = MAX_RECENT_COLORS / COLOR_GRID_COLUMNS;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < COLOR_GRID_COLUMNS; ++j) {
            QPushButton *btn = new QPushButton(m_recentColorsWidget);
            btn->setFixedSize(MIN_TOUCH_TARGET, MIN_TOUCH_TARGET);
            btn->setFlat(true);
            gridLayout->addWidget(btn, i, j);
            
            const int index = i * COLOR_GRID_COLUMNS + j;
            if (index < m_recentColors.size()) {
                const QColor &color = m_recentColors.at(index);
                btn->setStyleSheet(QString(
                    "QPushButton {"
                    "  background-color: %1;"
                    "  border: 2px solid %2;"
                    "  border-radius: %3px;"
                    "}"
                ).arg(color.name())
                 .arg(color.lightness() < 128 ? "white" : "black")
                 .arg(MIN_TOUCH_TARGET / 4));
                
                connect(btn, &QPushButton::clicked, this, [this, color]() {
                    setColor(color);
                });
            }
        }
    }
    
    mainLayout->addWidget(m_recentColorsWidget);

    // Action buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(12);
    
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_okButton = new QPushButton(tr("OK"), this);
    
    m_cancelButton->setMinimumSize(MIN_TOUCH_TARGET * 2, MIN_TOUCH_TARGET);
    m_okButton->setMinimumSize(MIN_TOUCH_TARGET * 2, MIN_TOUCH_TARGET);
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);
    
    mainLayout->addLayout(buttonLayout);

    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void TouchColorPicker::showEvent(QShowEvent *ev)
{
    QDialog::showEvent(ev);
    if (m_fullscreenAnimated && isFullScreen()) {
        // Create and show a dim backdrop that fades in
        if (!m_backdrop) {
            QWidget *parentWindow = parentWidget() ? parentWidget()->window() : nullptr;
            Qt::WindowFlags f = Qt::Window | Qt::FramelessWindowHint;
            m_backdrop = new QWidget(parentWindow);
            m_backdrop->setWindowFlags(f);
            m_backdrop->setAttribute(Qt::WA_TransparentForMouseEvents, false);
            m_backdrop->setStyleSheet("background-color: rgba(0,0,0,180);");
            QRect screenGeom = QGuiApplication::primaryScreen()->availableGeometry();
            m_backdrop->setGeometry(screenGeom);
            m_backdrop->setWindowOpacity(0.0);
            m_backdrop->show();
            m_backdrop->raise();
            m_backdrop->installEventFilter(this);

            QPropertyAnimation *fade = new QPropertyAnimation(m_backdrop, "windowOpacity");
            fade->setDuration(m_animDuration);
            fade->setStartValue(0.0);
            fade->setEndValue(0.45);
            fade->setEasingCurve(QEasingCurve::OutCubic);
            fade->start(QAbstractAnimation::DeleteWhenStopped);
        }

        // animate dialog from bottom into the screen
        QRect screenGeom = QGuiApplication::primaryScreen()->availableGeometry();
        QRect startGeom = QRect(screenGeom.x(), screenGeom.y() + screenGeom.height(),
                                 screenGeom.width(), screenGeom.height());
        QRect endGeom = screenGeom;

        setGeometry(startGeom);
        QPropertyAnimation *anim = new QPropertyAnimation(this, "geometry");
        anim->setDuration(m_animDuration);
        anim->setStartValue(startGeom);
        anim->setEndValue(endGeom);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void TouchColorPicker::animateOutAndClose(std::function<void()> onFinished)
{
    if (!m_fullscreenAnimated || !isFullScreen()) {
        if (onFinished) onFinished();
        return;
    }

    QRect screenGeom = QGuiApplication::primaryScreen()->availableGeometry();
    QRect startGeom = geometry();
    QRect endGeom = QRect(screenGeom.x(), screenGeom.y() + screenGeom.height(),
                          screenGeom.width(), screenGeom.height());
    // animate backdrop fade-out if present
    if (m_backdrop) {
        QPropertyAnimation *fade = new QPropertyAnimation(m_backdrop, "windowOpacity");
        fade->setDuration(m_animDuration);
        fade->setStartValue(m_backdrop->windowOpacity());
        fade->setEndValue(0.0);
        fade->setEasingCurve(QEasingCurve::InCubic);
        connect(fade, &QPropertyAnimation::finished, m_backdrop, [this]() {
            if (m_backdrop) {
                m_backdrop->removeEventFilter(this);
                m_backdrop->hide();
                delete m_backdrop;
                m_backdrop = nullptr;
            }
        });
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QPropertyAnimation *anim = new QPropertyAnimation(this, "geometry");
    anim->setDuration(m_animDuration);
    anim->setStartValue(startGeom);
    anim->setEndValue(endGeom);
    anim->setEasingCurve(QEasingCurve::InCubic);
    connect(anim, &QPropertyAnimation::finished, this, [onFinished]() {
        if (onFinished) onFinished();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

bool TouchColorPicker::eventFilter(QObject *watched, QEvent *event)
{
    // If backdrop is clicked/touched, dismiss the dialog with animation
    if (watched == m_backdrop) {
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::TouchBegin) {
            // Dismiss with animation
            reject();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void TouchColorPicker::accept()
{
    // animate out then accept
    animateOutAndClose([this]() {
        QDialog::accept();
    });
}

void TouchColorPicker::reject()
{
    animateOutAndClose([this]() {
        QDialog::reject();
    });
}

void TouchColorPicker::setColor(const QColor &color)
{
    if (m_color == color)
        return;

    m_color = color;
    
    if (!m_updatingControls) {
        m_updatingControls = true;
        updateColorControls();
        m_updatingControls = false;
    }

    m_colorPreview->setStyleSheet(QString(
        "QWidget {"
        "  background-color: %1;"
        "  border: 2px solid %2;"
        "  border-radius: %3px;"
        "}"
    ).arg(color.name())
     .arg(color.lightness() < 128 ? "white" : "black")
     .arg(MIN_TOUCH_TARGET / 4));

    emit colorChanged(color);
}

void TouchColorPicker::saveRecentColor(const QColor &color)
{
    // Remove if already exists
    m_recentColors.removeAll(color);
    
    // Add to front
    m_recentColors.prepend(color);
    
    // Keep only the most recent colors
    while (m_recentColors.size() > MAX_RECENT_COLORS) {
        m_recentColors.removeLast();
    }
    
    // Save to settings
    QStringList colorStrings;
    for (const QColor &c : m_recentColors) {
        colorStrings.append(c.name());
    }
    QSettings().setValue("recentColors", colorStrings);
    
    updateRecentColors();
}

void TouchColorPicker::onHsvChanged()
{
    if (m_updatingControls) return;
    
    m_updatingControls = true;
    
    // Get current HSV values from UI controls
    qreal hue = m_colorValues.hue;
    qreal saturation = m_colorValues.saturation;
    qreal value = m_colorValues.value;
    
    // Update color
    QColor color;
    color.setHsv(static_cast<int>(hue), static_cast<int>(saturation), static_cast<int>(value));
    setColor(color);
    
    m_updatingControls = false;
}

void TouchColorPicker::onRgbChanged()
{
    if (m_updatingControls) return;
    
    m_updatingControls = true;
    
    // Get current RGB values from UI controls
    int red = m_colorValues.red;
    int green = m_colorValues.green;
    int blue = m_colorValues.blue;
    
    // Update color
    QColor color(red, green, blue);
    setColor(color);
    
    m_updatingControls = false;
}

void TouchColorPicker::updateColor()
{
    // This method could be used to update the color based on UI controls
    // For now, we'll just emit the colorChanged signal
    emit colorChanged(m_color);
}

void TouchColorPicker::setScale(qreal scale)
{
    if (qFuzzyCompare(m_scale, scale))
        return;
    
    m_scale = qBound(1.0, scale, 3.0);
    emit scaleChanged(m_scale);
    
    if (m_hueSatMap) {
        m_hueSatMap->setScale(m_scale);
    }
}

bool TouchColorPicker::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture) {
        QGestureEvent *ge = static_cast<QGestureEvent*>(event);
        if (QPinchGesture *pinch = static_cast<QPinchGesture*>(ge->gesture(Qt::PinchGesture))) {
            onPinchGesture(pinch);
            return true;
        }
        if (QSwipeGesture *sw = static_cast<QSwipeGesture*>(ge->gesture(Qt::SwipeGesture))) {
            // Swipe down to dismiss
            if (sw->horizontalDirection() == QSwipeGesture::NoDirection &&
                sw->verticalDirection() == QSwipeGesture::Down) {
                reject();
                return true;
            }
        }
    }
    return QDialog::event(event);
}

void TouchColorPicker::resizeEvent(QResizeEvent *event)
{
    // Call base implementation
    QDialog::resizeEvent(event);
    
    // Update UI to fit new size if needed
    // For now, we'll just update the color controls
    updateColorControls();
}

void TouchColorPicker::onPinchGesture(QPinchGesture *gesture)
{
    // The gesture event should be handled in the main event handler
    // This function is just for specific pinch gesture processing
    if (gesture->state() == Qt::GestureFinished) {
        // Animate back to normal scale if needed
        if (m_scale != 1.0) {
            m_touchInteraction->startBounceAnimation(1.0);
        }
    }
}

QColor TouchColorPicker::currentColor() const
{
    return m_color;
}

void TouchColorPicker::updateColorControls()
{
    // This method would typically update UI controls to reflect the current color
    // Since this is a touch color picker, we might not need complex controls
    // but we'll implement a basic version for completeness
    
    if (m_colorPreview) {
        m_colorPreview->setStyleSheet(QString(
            "QWidget {"
            "  background-color: %1;"
            "  border: 2px solid %2;"
            "  border-radius: %3px;"
            "}"
        ).arg(m_color.name())
         .arg(m_color.lightness() < 128 ? "white" : "black")
         .arg(MIN_TOUCH_TARGET / 4));
    }
    
    if (m_hueSatMap) {
        m_hueSatMap->setIndicatorColor(m_color);
        
        // Update indicator position based on the color
        // HueSatMap uses normalized coordinates (0.0 to 1.0)
        qreal hue = m_color.hueF();  // 0.0 to 1.0
        qreal saturation = m_color.saturationF();  // 0.0 to 1.0
        
        // Invert saturation for the Y coordinate (0 at top, 1 at bottom)
        QPointF pos(hue, 1.0 - saturation);
        m_hueSatMap->setIndicatorPos(pos);
    }
}

void TouchColorPicker::updateRecentColors()
{
    // Update the recent colors display
    // This would typically update the UI elements showing recent colors
    // For now, we'll just emit a signal to indicate the recent colors changed
    emit colorChanged(m_color);
}

QColor TouchColorPicker::getColor(const QColor &initial, QWidget *parent)
{
    // Create a per-call instance for predictable lifecycle
    TouchColorPicker picker(parent);
    picker.setColor(initial);
    // Decide whether to use a compact fullscreen variant for small devices
    QScreen *screen = QGuiApplication::primaryScreen();
    bool useCompact = false;
    if (screen) {
        const QSize sz = screen->availableGeometry().size();
        // Heuristic: treat screens under 700px in either dimension as small
        useCompact = (sz.width() < 700 || sz.height() < 700);
    }
    picker.setCompactMode(useCompact);
    picker.setFullscreenAnimated(useCompact);
    if (useCompact) {
        // request fullscreen for a simpler touch-first experience
        picker.setWindowState(Qt::WindowFullScreen);
    } else {
        picker.setWindowState(Qt::WindowNoState);
    }

    QColor result = initial;
    if (picker.exec() == QDialog::Accepted) {
        result = picker.currentColor();
        picker.saveRecentColor(result);
    }
    return result;
}


TouchColorPicker::~TouchColorPicker()
{
    if (m_backdrop) {
        m_backdrop->removeEventFilter(this);
        m_backdrop->hide();
        delete m_backdrop;
        m_backdrop = nullptr;
    }
}