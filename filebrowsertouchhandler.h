#ifndef FILEBROWSERTOUCHHANDLER_H
#define FILEBROWSERTOUCHHANDLER_H

#include "touchinteractionhandler.h"
#include <QScroller>
#include <QTreeWidgetItem>
#include <QPinchGesture>
#include <QSwipeGesture>
#include <QPanGesture>
#include <QTouchEvent>
#include <QTimer>

// Forward declarations to avoid circular dependencies
class FileBrowser;
class TouchInteraction;

class FileBrowserTouchHandler : public TouchInteractionHandler {
    Q_OBJECT

public:
    explicit FileBrowserTouchHandler(FileBrowser* fileBrowser);
    ~FileBrowserTouchHandler() override;
    // Access to touch interaction for FileBrowser
    TouchInteraction* touchInteraction() const { return m_touchInteraction; }

Q_SIGNALS:
    void itemExpansionRequested(QTreeWidgetItem* item);
    void overscrollAmountChanged(qreal amount);
    void itemTapped(QTreeWidgetItem* item);
    void enableGestureHandling(QWidget* widget);
    void disableGestureHandling(QWidget* widget);

protected:
    void handlePinchGesture(QPinchGesture* gesture) override;
    void handleSwipeGesture(QSwipeGesture* gesture) override;
    void handlePanGesture(QPanGesture* gesture) override;
    bool handleTouchBegin(QTouchEvent* event) override;
    bool handleTouchUpdate(QTouchEvent* event) override;
    bool handleTouchEnd(QTouchEvent* event) override;

private:
    void setupScrolling();
    void updateScrollLimits();
    QTreeWidgetItem* itemAtPoint(const QPoint& point) const;
    void handleItemTap(QTreeWidgetItem* item);

    FileBrowser* m_fileBrowser;
    QScroller* m_scroller;
    TouchInteraction* m_touchInteraction;
    QPoint m_touchStartPos;
    QTreeWidgetItem* m_lastTouchedItem;
    bool m_isItemDrag{false};
    QTimer* m_longPressTimer{nullptr};
    QPoint m_lastTouchPos;
    bool m_longPressTriggered{false};
    
    static constexpr int DRAG_THRESHOLD = 10; // pixels
    static constexpr int TAP_TIMEOUT = 300;   // milliseconds
    static constexpr int LONG_PRESS_TIMEOUT = 500; // milliseconds
};

#endif // FILEBROWSERTOUCHHANDLER_H