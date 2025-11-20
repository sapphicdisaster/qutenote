#include "uiutils.h"
#include "thememanager.h"
#include <QToolBar>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QApplication>
#include <QStyle>
#include <QScreen>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QScroller>
#include <QAbstractButton>

namespace UIUtils {

QWidget* createCollapsibleSection(const QString& title, QWidget* content, bool isExpanded, QWidget* parent)
{
    QWidget* container = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header
    QPushButton* header = new QPushButton(title, container);
    header->setCheckable(true);
    header->setChecked(isExpanded);
    header->setStyleSheet(
        "QPushButton {"
        "    text-align: left;"
        "    padding: 12px;"
        "    border: none;"
        "    background: transparent;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(128, 128, 128, 0.1);"
        "}"
    );
    makeTouchFriendly(header);

    // Content
    content->setVisible(isExpanded);
    
    // Add expand/collapse arrow
    QLabel* arrow = new QLabel(container);
    arrow->setText(isExpanded ? "▼" : "▶");
    arrow->setStyleSheet("QLabel { padding: 0 8px; }");
    
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->addWidget(arrow);
    headerLayout->addWidget(header, 1);
    
    layout->addLayout(headerLayout);
    layout->addWidget(content);

    // Setup animation
    QParallelAnimationGroup* animations = new QParallelAnimationGroup(container);
    
    QPropertyAnimation* heightAnimation = new QPropertyAnimation(content, "maximumHeight");
    heightAnimation->setDuration(200);
    
    QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect(content);
    content->setGraphicsEffect(opacity);
    QPropertyAnimation* opacityAnimation = new QPropertyAnimation(opacity, "opacity");
    opacityAnimation->setDuration(200);
    
    animations->addAnimation(heightAnimation);
    animations->addAnimation(opacityAnimation);

    // Connect toggle behavior
    QObject::connect(header, &QPushButton::toggled, [=](bool checked) {
        arrow->setText(checked ? "▼" : "▶");
        
        if (checked) {
            content->setVisible(true);
            heightAnimation->setStartValue(0);
            heightAnimation->setEndValue(content->sizeHint().height());
            opacityAnimation->setStartValue(0.0);
            opacityAnimation->setEndValue(1.0);
        } else {
            heightAnimation->setStartValue(content->height());
            heightAnimation->setEndValue(0);
            opacityAnimation->setStartValue(1.0);
            opacityAnimation->setEndValue(0.0);
        }
        
        animations->start();
    });

    // Clean up animations when done
    QObject::connect(animations, &QParallelAnimationGroup::finished, [=]() {
        if (!header->isChecked()) {
            content->setVisible(false);
        }
    });

    return container;
}

QWidget* createToolbarSection(const QString& title, const QList<QWidget*>& items,
                            bool isVertical, QWidget* parent)
{
    QWidget* container = new QWidget(parent);
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(TouchMetrics::MARGIN, TouchMetrics::MARGIN,
                                 TouchMetrics::MARGIN, TouchMetrics::MARGIN);
    mainLayout->setSpacing(TouchMetrics::SPACING);

    if (!title.isEmpty()) {
        QLabel* titleLabel = new QLabel(title, container);
        titleLabel->setStyleSheet("QLabel { font-weight: bold; }");
        mainLayout->addWidget(titleLabel);
    }

    QBoxLayout* itemsLayout = isVertical 
        ? static_cast<QBoxLayout*>(new QVBoxLayout())
        : static_cast<QBoxLayout*>(new QHBoxLayout());
    itemsLayout->setContentsMargins(0, 0, 0, 0);
    itemsLayout->setSpacing(TouchMetrics::SPACING);

    for (QWidget* item : items) {
        // Ensure minimum size for better touch targets
        if (item) {
            // For Android, use larger touch targets
#ifdef Q_OS_ANDROID
            item->setMinimumSize(60, 60); // Larger touch targets for Android
#else
            item->setMinimumSize(55, 55); // Standard touch targets
#endif
            // If it's a button, also set icon size
            if (QAbstractButton* button = qobject_cast<QAbstractButton*>(item)) {
                button->setIconSize(QSize(32, 32));
            }
            makeTouchFriendly(item);
        }
        itemsLayout->addWidget(item);
    }

    if (!isVertical) {
        itemsLayout->addStretch();
    }

    mainLayout->addLayout(itemsLayout);
    return container;
}

QToolBar* createTouchFriendlyToolbar(bool isVertical, QWidget* parent)
{
    QToolBar* toolbar = new QToolBar(parent);
    toolbar->setOrientation(isVertical ? Qt::Vertical : Qt::Horizontal);
    toolbar->setIconSize(QSize(32, 32)); // Larger icon size for better visibility
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    
    // Make toolbar touch-friendly with better Android support
#ifdef Q_OS_ANDROID
    // Use larger touch targets for Android
    toolbar->setStyleSheet(
        "QToolBar {"
        "    spacing: " + QString::number(TouchMetrics::SPACING) + "px;"
        "    padding: " + QString::number(TouchMetrics::MARGIN) + "px;"
        "}"
        "QToolButton {"
        "    min-width: 60px;"  // Larger touch targets for Android
        "    min-height: 60px;" // Larger touch targets for Android
        "    padding: 6px;"
        "}"
    );
#else
    toolbar->setStyleSheet(
        "QToolBar {"
        "    spacing: " + QString::number(TouchMetrics::SPACING) + "px;"
        "    padding: " + QString::number(TouchMetrics::MARGIN) + "px;"
        "}"
        "QToolButton {"
        "    min-width: " + QString::number(TouchMetrics::MINIMUM_TOUCH_TARGET) + "px;"
        "    min-height: " + QString::number(TouchMetrics::MINIMUM_TOUCH_TARGET) + "px;"
        "    padding: 4px;"
        "}"
    );
#endif
    // Avoid setting a widget-level stylesheet so the application stylesheet from
    // ThemeManager can fully control colors and outlines. Use margins as a
    // non-stylistic hint for layout where needed.
    toolbar->setContentsMargins(TouchMetrics::MARGIN, TouchMetrics::MARGIN,
                                TouchMetrics::MARGIN, TouchMetrics::MARGIN);

    return toolbar;
}

void setupTouchFeedback(QWidget* widget)
{
    if (!widget) return;
    
    widget->setAttribute(Qt::WA_AcceptTouchEvents);
    widget->setProperty("touch-friendly", true);
    
    // Add touch feedback style
    // Rely on ThemeManager's application stylesheet to style the
    // `touch-friendly` property (e.g., pressed state). Do not set a
    // widget-local stylesheet here.
}

void makeTouchFriendly(QWidget* widget, bool scaleContents)
{
    if (!widget) return;
    
    setupTouchFeedback(widget);
    
    // Use larger touch targets for Android
#ifdef Q_OS_ANDROID
    QSize touchSize = getTouchFriendlySize(true); // Use large touch targets for Android
#else
    QSize touchSize = getTouchFriendlySize();
#endif
    widget->setMinimumSize(touchSize);
    
    if (scaleContents) {
        // Only set icon size for widgets that support it (e.g., buttons)
        if (QAbstractButton* button = qobject_cast<QAbstractButton*>(widget)) {
            // Use larger icon size for better visibility on mobile
            button->setIconSize(QSize(32, 32));
        }
    }
}

void setupScrollArea(QWidget* area, bool enableTouch)
{
    if (!area) return;
    
    if (QScrollArea* scrollArea = qobject_cast<QScrollArea*>(area)) {
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setWidgetResizable(true);
        
        if (enableTouch) {
            scrollArea->setProperty("touch-friendly", true);
            scrollArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
            
            // Enable kinetic scrolling
            QScroller::grabGesture(scrollArea->viewport(),
                                 QScroller::TouchGesture);
        }
    }
}

QPushButton* createActionButton(const QString& text, const QString& iconName, QWidget* parent)
{
    QPushButton* button = new QPushButton(text, parent);
    
    if (!iconName.isEmpty()) {
        button->setIcon(QIcon::fromTheme(iconName));
    }
    
    makeTouchFriendly(button, true);
    return button;
}

QToolButton* createToolButton(const QString& text, const QString& iconName, QWidget* parent)
{
    QToolButton* button = new QToolButton(parent);
    button->setText(text);
    
    if (!iconName.isEmpty()) {
        button->setIcon(QIcon::fromTheme(iconName));
    }
    
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    makeTouchFriendly(button, true);
    return button;
}

QMargins getTouchFriendlyMargins()
{
    return QMargins(TouchMetrics::MARGIN, TouchMetrics::MARGIN,
                   TouchMetrics::MARGIN, TouchMetrics::MARGIN);
}

int getTouchFriendlySpacing()
{
    return TouchMetrics::SPACING;
}

QSize getTouchFriendlySize(bool isLarge)
{
    int size = isLarge ? TouchMetrics::LARGE_TOUCH_TARGET 
                      : TouchMetrics::PREFERRED_TOUCH_TARGET;
    return QSize(size, size);
}

QLabel* createStatusLabel(const QString& text, QWidget* parent)
{
    QLabel* label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

void updateStatusLabel(QLabel* label, const QString& text, bool isError)
{
    if (!label) return;
    
    label->setText(text);
    // Use ThemeManager colors for status labels when available so themes can control
    // error/success colors. Fall back to previous hard-coded colors if theme values
    // are not valid.
    Theme theme = ThemeManager::instance()->currentTheme();
    QColor c = isError
        ? (theme.colors.error.isValid() ? theme.colors.error : QColor("#d32f2f"))
        : (theme.colors.success.isValid() ? theme.colors.success : QColor("#2e7d32"));
    label->setStyleSheet(QString("QLabel { color: %1; }").arg(c.name()));
}

bool isMobileDevice()
{
#ifdef Q_OS_ANDROID
    return true;
#elif defined(Q_OS_IOS)
    return true;
#else
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) return false;
    
    // Consider device mobile if physical DPI is high or resolution is typical for mobile
    return screen->physicalDotsPerInch() > 200 || 
           (screen->size().width() <= 1080 && screen->size().height() <= 1920);
#endif
}

void adaptForMobile(QWidget* widget)
{
    if (!widget || !isMobileDevice()) return;
    
    // Increase touch targets
    makeTouchFriendly(widget);
    
    // Adjust margins and spacing
    if (QLayout* layout = widget->layout()) {
        layout->setContentsMargins(getTouchFriendlyMargins());
        layout->setSpacing(getTouchFriendlySpacing());
    }
    
    // Process child widgets
    for (QObject* child : widget->children()) {
        if (QWidget* childWidget = qobject_cast<QWidget*>(child)) {
            adaptForMobile(childWidget);
        }
    }
}

void adaptForTouch(QWidget* widget)
{
    if (!widget) return;
    
    // Enable touch and gesture handling
    widget->setAttribute(Qt::WA_AcceptTouchEvents);
    widget->grabGesture(Qt::SwipeGesture);
    widget->grabGesture(Qt::PanGesture);
    widget->grabGesture(Qt::PinchGesture);
    
    // Make the widget touch-friendly
    makeTouchFriendly(widget);
    
    // Process child widgets
    for (QObject* child : widget->children()) {
        if (QWidget* childWidget = qobject_cast<QWidget*>(child)) {
            adaptForTouch(childWidget);
        }
    }
}

} // namespace UIUtils