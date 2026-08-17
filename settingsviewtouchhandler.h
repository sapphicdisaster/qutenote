#ifndef SETTINGSVIEWTOUCHHANDLER_H
#define SETTINGSVIEWTOUCHHANDLER_H

#include "touchinteractionhandler.h"
#include "smartpointers.h"
#include <QScrollArea>

class QScroller;
class SettingsView;
class TouchInteraction;

class SettingsViewTouchHandler : public TouchInteractionHandler {
    Q_OBJECT

public:
    explicit SettingsViewTouchHandler(SettingsView* settingsView);
    ~SettingsViewTouchHandler() override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    
    // Access to touch interaction for SettingsView
    TouchInteraction* touchInteraction() const { return m_touchInteraction.get(); }
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
    QuteNote::OwnedPtr<TouchInteraction> m_touchInteraction;
    QScroller* m_scroller = nullptr;
};

#endif // SETTINGSVIEWTOUCHHANDLER_H
