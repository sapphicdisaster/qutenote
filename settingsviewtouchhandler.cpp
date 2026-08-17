#include "settingsviewtouchhandler.h"
#include "settingsview.h"
#include "touchinteraction.h"
#include <QScroller>
#include <QScrollBar>
#include <QtMath>
#include <QApplication>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QAbstractButton>
#include <QSlider>
#include <QComboBox>
#include <QAbstractSpinBox>

SettingsViewTouchHandler::SettingsViewTouchHandler(SettingsView* settingsView)
    : TouchInteractionHandler(settingsView)
    , m_settingsView(settingsView)
    , m_scrollArea(new QScrollArea(settingsView))
    , m_touchInteraction(QuteNote::makeOwned<TouchInteraction>(this))
    , m_scroller(nullptr)
{
    if (!m_settingsView) return;
    
    setupScrolling();
    
    // Configure touch interaction physics
    m_touchInteraction->setBouncePreset(TouchInteraction::Normal);
    
    // Connect touch interaction signals
    connect(m_touchInteraction.get(), &TouchInteraction::overscrollAmountChanged,
            this, &SettingsViewTouchHandler::overscrollAmountChanged);
}

SettingsViewTouchHandler::~SettingsViewTouchHandler()
{
}

bool SettingsViewTouchHandler::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
        QWidget* widget = m_settingsView;
        if (widget) {
            QPoint globalPos;
            if (event->type() == QEvent::MouseButtonPress) {
                globalPos = static_cast<QMouseEvent*>(event)->globalPosition().toPoint();
            } else {
                QTouchEvent* touchEvent = static_cast<QTouchEvent*>(event);
                if (!touchEvent->points().isEmpty()) {
                    globalPos = touchEvent->points().first().globalPosition().toPoint();
                }
            }
            if (!globalPos.isNull()) {
                QPoint localPos = widget->mapFromGlobal(globalPos);
                QWidget* child = widget->childAt(localPos);
                bool isInteractive = false;
                while (child && child != widget) {
                    if (qobject_cast<QAbstractButton*>(child) || 
                        qobject_cast<QSlider*>(child) || 
                        qobject_cast<QComboBox*>(child) ||
                        qobject_cast<QAbstractSpinBox*>(child)) {
                        isInteractive = true;
                        break;
                    }
                    child = child->parentWidget();
                }
                if (isInteractive) {
                    if (m_scroller && m_scroller->state() != QScroller::Inactive) {
                        m_scroller->stop();
                    }
                }
            }
        }
    }
    return TouchInteractionHandler::eventFilter(watched, event);
}

void SettingsViewTouchHandler::setupScrolling()
{
    if (!m_scrollArea) return;
    
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_scroller = setupQScroller(m_scrollArea, 0.33, 0.33);
    
    // Enable touch events
    m_scrollArea->setAttribute(Qt::WA_AcceptTouchEvents);
    if (m_scrollArea->viewport()) {
        m_scrollArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
        m_scrollArea->viewport()->installEventFilter(this);
    }
}

void SettingsViewTouchHandler::handlePinchGesture(QPinchGesture* gesture)
{
    if (!gesture || !m_settingsView) return;
    // Handle pinch gesture for settings view - could be used for zooming content
}

void SettingsViewTouchHandler::handleSwipeGesture(QSwipeGesture* gesture)
{
    if (!gesture || !m_settingsView) return;
    // Handle swipe gesture for settings view - could be used for navigation
}

void SettingsViewTouchHandler::handlePanGesture(QPanGesture* gesture)
{
    // Pan gesture handling is managed by QScroller
}

bool SettingsViewTouchHandler::handleTouchBegin(QTouchEvent* event)
{
    bool handled = TouchInteractionHandler::handleTouchBegin(event);
    return handled;
}

bool SettingsViewTouchHandler::handleTouchUpdate(QTouchEvent* event)
{
    bool handled = TouchInteractionHandler::handleTouchUpdate(event);
    return handled;
}

bool SettingsViewTouchHandler::handleTouchEnd(QTouchEvent* event)
{
    bool handled = TouchInteractionHandler::handleTouchEnd(event);
    return handled;
}
