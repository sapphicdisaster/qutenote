#ifndef TEXTEDITORTOUCHHANDLER_H
#define TEXTEDITORTOUCHHANDLER_H

#include "touchinteractionhandler.h"
#include <QScroller>
#include <QPinchGesture>
#include <QSwipeGesture>
#include <QPanGesture>
#include <QTouchEvent>

// Forward declarations to avoid circular dependencies
class TextEditor;
class TouchInteraction;

class TextEditorTouchHandler : public TouchInteractionHandler {
    Q_OBJECT

public:
    explicit TextEditorTouchHandler(TextEditor* textEditor);
    ~TextEditorTouchHandler() override;

Q_SIGNALS:
    void overscrollAmountChanged(qreal amount);
    void pinchScaleChanged(qreal scale);

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

    TextEditor* m_textEditor;
    QScroller* m_scroller;
    TouchInteraction* m_touchInteraction;
    qreal m_currentScale{1.0};
    qreal m_lastOverscrollAmount{0.0};
    
    static constexpr qreal MIN_SCALE = 0.5;
    static constexpr qreal MAX_SCALE = 2.0;
};

#endif // TEXTEDITORTOUCHHANDLER_H