#ifndef HUESATMAP_H
#define HUESATMAP_H

#include <QWidget>
#include <QImage>
#include <QColor>
#include "touchinteraction.h"
#include "huesatmapcache.h"
#include "smartpointers.h"

class HueSatMap : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(HueSatMap)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(qreal targetRadius READ targetRadius WRITE setTargetRadius NOTIFY targetRadiusChanged)

public:
    explicit HueSatMap(QWidget *parent = nullptr);
    
    void setHueSatMap(const QImage &image);
    void setIndicatorPos(const QPointF &pos);
    void setIndicatorColor(const QColor &color);
    
    qreal scale() const { return m_scale; }
    void setScale(qreal scale);
    
    qreal targetRadius() const { return m_targetRadius; }
    void setTargetRadius(qreal radius);
    
    QSize sizeHint() const override { return QSize(300, 300); }
    
Q_SIGNALS:
    void scaleChanged(qreal scale);
    void targetRadiusChanged(qreal radius);
    void colorSelected(const QColor &color);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;
    
private:
    // Component functionality
    void initializeComponent();
    void setupConnections();
    void cleanupResources();
    void handleMemoryWarning();

    // Private methods
    void updateGradient();
    void resetGradient(int width, int height);
    void setupTouchInteraction();
    void drawTargetIndicator(QPainter *painter);
    QImage getCachedGradient(const QSize &size);
    
    // Member variables
    QuteNote::OwnedPtr<TouchInteraction> m_touchInteraction;
    HueSatMapCache *m_gradientCache;
    QImage m_hueSatMap;
    QPointF m_indicatorPos;
    QColor m_indicatorColor;
    qreal m_scale;
    qreal m_targetRadius;
    bool m_isDragging;
    static const int MIN_TOUCH_TARGET = 48; // Minimum touch target size in pixels
};

#endif // HUESATMAP_H
