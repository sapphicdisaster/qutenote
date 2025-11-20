#include "settingsviewtouchhandler.h"
#include "settingsview.h"
#include "touchinteraction.h"
#include <QScrollBar>
#include <QtMath>
#include <QApplication>

SettingsViewTouchHandler::SettingsViewTouchHandler(SettingsView* settingsView)
    : TouchInteractionHandler(settingsView)
    , m_settingsView(settingsView)
    , m_scrollArea(new QScrollArea(settingsView))
    , m_touchInteraction(new TouchInteraction(this))
    , m_scroller(nullptr)
{
    if (!m_settingsView) return;
    
    setupScrolling();
    
    // Configure touch interaction physics
    m_touchInteraction->setBouncePreset(TouchInteraction::Normal);
    
    // Connect touch interaction signals
    connect(m_touchInteraction, &TouchInteraction::overscrollAmountChanged,
            this, &SettingsViewTouchHandler::overscrollAmountChanged);
}

SettingsViewTouchHandler::~SettingsViewTouchHandler()
{
}

void SettingsViewTouchHandler::setupScrolling()
{
    if (!m_scrollArea) return;
    
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_scroller = QScroller::scroller(m_scrollArea);
    QScrollerProperties props = m_scroller->scrollerProperties();
    
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, 
                         QScrollerProperties::OvershootWhenScrollable);
    props.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.33);
    props.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0.33);
    
    m_scroller->setScrollerProperties(props);
    QScroller::grabGesture(m_scrollArea, QScroller::TouchGesture);
    
    // Enable touch events
    m_scrollArea->setAttribute(Qt::WA_AcceptTouchEvents);
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
