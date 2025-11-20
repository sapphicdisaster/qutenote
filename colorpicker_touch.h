#ifndef COLORPICKER_TOUCH_H
#define COLORPICKER_TOUCH_H

#include <QDialog>
#include <QColor>
#include <QCache>
#include <QShowEvent>
#include "touchinteraction.h"
#include "huesatmap.h"
#include <functional>
class QPushButton;

// Export/import macros
#if defined(COLORPICKER_LIBRARY)
#  define COLORPICKER_EXPORT Q_DECL_EXPORT
#else
#  define COLORPICKER_EXPORT Q_DECL_IMPORT
#endif

class COLORPICKER_EXPORT TouchColorPicker : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TouchColorPicker)
    Q_PROPERTY(QColor color READ currentColor WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
    
public:
    static TouchColorPicker* instance(QWidget *parent = nullptr);
    ~TouchColorPicker();

    static QColor getColor(const QColor &initial = Qt::white, QWidget *parent = nullptr);
    QColor currentColor() const;
    void setCompactMode(bool enabled);
    void setFullscreenAnimated(bool enabled);
    
    qreal scale() const { return m_scale; }
    void setScale(qreal scale);

Q_SIGNALS:
    void colorSelected(const QColor &color);
    void colorChanged(const QColor &color);
    void scaleChanged(qreal scale);

protected:
    explicit TouchColorPicker(QWidget *parent = nullptr);
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void accept() override;
    void reject() override;

private slots:
    void onHsvChanged();
    void onRgbChanged();
    void updateColor();
    void onPinchGesture(QPinchGesture *gesture);

private:
    void createLayout();
    void setupTouchInteraction();
    void updateColorControls();
    void setColor(const QColor &color);
    void saveRecentColor(const QColor &color);
    void updateRecentColors();

    void animateOutAndClose(std::function<void()> onFinished);

    bool eventFilter(QObject *watched, QEvent *event) override;

    static const int MIN_TOUCH_TARGET = 48; // Minimum touch target size in dp
    static const int COLOR_GRID_COLUMNS = 4;
    static const int MAX_RECENT_COLORS = 16;
    
    TouchInteraction *m_touchInteraction;
    HueSatMap *m_hueSatMap;
    QWidget *m_colorPreview;
    QWidget *m_touchSlider;
    QWidget *m_recentColorsWidget;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    QWidget *m_backdrop;
    qreal m_scale;
    QColor m_color;
    QList<QColor> m_recentColors;
    bool m_compactMode;
    bool m_fullscreenAnimated;
    int m_animDuration;
    
    bool m_updatingControls;
    // no longer a singleton: per-call instances preferred

    struct {
        int hue;
        int saturation;
        int value;
        int red;
        int green;
        int blue;
    } m_colorValues;
};

#endif // COLORPICKER_TOUCH_H