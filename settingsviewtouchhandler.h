#ifndef SETTINGSVIEWTOUCHHANDLER_H
#define SETTINGSVIEWTOUCHHANDLER_H

#include "touchinteractionhandler.h"
#include <QScroller>
#include <QPinchGesture>
#include <QSwipeGesture>
#include <QScrollArea>
#include <QTouchEvent>

// Forward declarations
class SettingsView;
class TouchInteraction;

class SettingsViewTouchHandler : public TouchInteractionHandler {
    Q_OBJECT

public:
    explicit SettingsViewTouchHandler(SettingsView* settingsView);
    ~SettingsViewTouchHandler() override;
    
    // Access to touch interaction for SettingsView
    TouchInteraction* touchInteraction() const { return m_touchInteraction; }
    QScrollArea* scrollArea() const { return m_scrollArea; }

Q_SIGNALS:
    void overscrollAmountChanged(qreal amount);

protected:
    void handlePinchGesture(QPinchGesture* gesture) override;
    void handleSwipeGesture(QSwipeGesture* gesture) override;
    void handlePanGesture(QPanGesture* gesture) override;
    bool handleTouchBegin(QTouchEvent* event) override;
    bool handleTouchUpdate(QTouchEvent* event) override;
    bool handleTouchEnd(QTouchEvent* event) override;

private:
    void setupScrolling();
    
    SettingsView* m_settingsView;
    QScrollArea* m_scrollArea;
    TouchInteraction* m_touchInteraction;
    QScroller* m_scroller;
};

#endif // SETTINGSVIEWTOUCHHANDLER_H
