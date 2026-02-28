#include "imagesettingsdialog.h"
#include <QIcon>
#include "uiutils.h"
#include "thememanager.h"
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QTouchEvent>
#include <qframe.h>
#include <qnamespace.h>
#include <qcoreapplication.h>
#include <qtmetamacros.h>
#include <qwidget.h>
#include <qpoint.h>
#include <qpaintdevice.h>
#include <qboxlayout.h>
#include <qobject.h>
#include <qfont.h>
#include <qslider.h>
#include <qbuttongroup.h>
#include <qvariant.h>
#include <qabstractbutton.h>

ImageSettingsDialog::ImageSettingsDialog(QWidget *parent)
    : QFrame(parent)
    , m_currentWidthPct(100)
    , m_currentAlignment(Qt::AlignLeft)
{
    // Make sure it looks like a nice floating panel
    setAutoFillBackground(true);
    // Note: We don't set WindowFlags so it remains a child widget of TextEditor
    
    setupUI();
}

ImageSettingsDialog::~ImageSettingsDialog()
= default;

void ImageSettingsDialog::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->installEventFilter(this);
    }
}

void ImageSettingsDialog::hideEvent(QHideEvent *event)
{
    QFrame::hideEvent(event);
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->removeEventFilter(this);
    }
    emit dialogHidden();
}

bool ImageSettingsDialog::eventFilter(QObject *obj, QEvent *event)
{
    // Ignore events sent to our own children to prevent accidental self-dismissal
    if (obj == this || this->isAncestorOf(qobject_cast<QWidget*>(obj))) {
        return QFrame::eventFilter(obj, event);
    }

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
        QPoint pos;
        if (event->type() == QEvent::MouseButtonPress) {
            pos = dynamic_cast<QMouseEvent*>(event)->globalPosition().toPoint();
        } else {
            pos = dynamic_cast<QTouchEvent*>(event)->points().first().globalPosition().toPoint();
        }
        
        QRect globalRect(mapToGlobal(QPoint(0,0)), size());
        if (!globalRect.contains(pos)) {
            // Un-select the image in the editor so tapping it again will trigger the detection logic
            hide();
            // Let the event propagate so the target widget can process the tap (e.g., keyboard opening)
        }
    }
    return QFrame::eventFilter(obj, event);
}

void ImageSettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // Reduced margins and spacing even more as requested (compact UI)
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(4);

    // Style the dialog
    Theme theme = ThemeManager::instance()->currentTheme();
    QString style = QString(
        "QFrame {"
        "   background-color: %1;"
        "   border: 1px solid %2;"
        "   border-radius: 16px;" // More rounded corners
        "}"
        "QLabel { color: %4; font-weight: bold; }" 
    ).arg(theme.colors.background.name())
     .arg(theme.colors.border.name())
     //.arg(theme.metrics.borderRadius) // Use hardcoded 16px as requested "round corners... even more"
     .arg(theme.colors.text.name());
     
    // Append specific styles for sliders and buttons
    style += QString(
        "QSlider::groove:horizontal { border: 1px solid %1; height: 8px; background: %2; border-radius: 4px; }"
        "QSlider::handle:horizontal { background: %3; border: 1px solid %1; width: 22px; height: 22px; margin: -7px 0; border-radius: 11px; }"
        "QPushButton { background-color: %4; border: 1px solid %1; border-radius: 8px; padding: 4px; }" // rounded buttons
        "QPushButton:checked { background-color: %6; border: 1px solid %7; }"
    ).arg(theme.colors.border.name())
     .arg(theme.colors.surface.name())
     .arg(theme.colors.accent.name())
     .arg(theme.colors.surface.name())
     //.arg(theme.metrics.borderRadius)
     .arg(theme.colors.accent.name())
     .arg(theme.colors.primary.name());

    setStyleSheet(style);

    // Font for labels - slightly larger
    QFont labelFont = theme.defaultFont;
    labelFont.setPointSizeF(labelFont.pointSizeF() * 1.2);

    // --- Resize Section ---
    QLabel *resizeLabel = new QLabel(tr("Size"), this);
    resizeLabel->setFont(labelFont);
    mainLayout->addWidget(resizeLabel);

    QHBoxLayout *sliderLayout = new QHBoxLayout();
    m_widthSlider = new QSlider(Qt::Horizontal, this);
    m_widthSlider->setRange(10, 100);
    m_widthSlider->setValue(100);
    // Touch friendly slider
    m_widthSlider->setFixedHeight(theme.metrics.touchTarget);
    
    m_widthLabel = new QLabel("100%", this);
    m_widthLabel->setMinimumWidth(40);
    m_widthLabel->setFont(theme.defaultFont); // Keep value label normal size
    m_widthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    sliderLayout->addWidget(m_widthSlider);
    sliderLayout->addWidget(m_widthLabel);
    mainLayout->addLayout(sliderLayout);

    connect(m_widthSlider, &QSlider::valueChanged, this, &ImageSettingsDialog::onSliderValueChanged);

    // --- Alignment Section ---
    QLabel *alignLabel = new QLabel(tr("Alignment"), this);
    alignLabel->setFont(labelFont);
    mainLayout->addWidget(alignLabel);

    QHBoxLayout *alignLayout = new QHBoxLayout();
    m_alignGroup = new QButtonGroup(this);
    m_alignGroup->setExclusive(true);

    auto createAlignBtn = [&](const QString &icon, Qt::Alignment align) -> QPushButton* {
        QPushButton *btn = new QPushButton(this);
        btn->setIcon(QIcon(icon));
        btn->setCheckable(true);
        btn->setFixedSize(theme.metrics.touchTarget, theme.metrics.touchTarget);
        btn->setProperty("alignment", QVariant::fromValue(align));
        UIUtils::makeTouchFriendly(btn);
        m_alignGroup->addButton(btn);
        alignLayout->addWidget(btn);
        return btn;
    };

    m_btnLeft = createAlignBtn(":/resources/icons/custom/align-left.svg", Qt::AlignLeft);
    m_btnCenter = createAlignBtn(":/resources/icons/custom/align-center.svg", Qt::AlignCenter);
    m_btnRight = createAlignBtn(":/resources/icons/custom/align-right.svg", Qt::AlignRight);

    alignLayout->addStretch();
    mainLayout->addLayout(alignLayout);

    connect(m_alignGroup, &QButtonGroup::buttonClicked, this, &ImageSettingsDialog::onAlignmentChanged);
}

void ImageSettingsDialog::setImageProperties(int widthPct, Qt::Alignment alignment)
{
    m_currentWidthPct = widthPct;
    m_widthSlider->blockSignals(true);
    m_widthSlider->setValue(widthPct);
    m_widthSlider->blockSignals(false);
    updateSliderLabel(widthPct);

    m_currentAlignment = alignment;
    m_alignGroup->blockSignals(true);
    if (alignment & Qt::AlignLeft) { m_btnLeft->setChecked(true);
    } else if (alignment & Qt::AlignHCenter) { m_btnCenter->setChecked(true);
    } else if (alignment & Qt::AlignRight) { m_btnRight->setChecked(true);
    } else { m_btnLeft->setChecked(true); // Default
}
    m_alignGroup->blockSignals(false);
}

void ImageSettingsDialog::onSliderValueChanged(int value)
{
    m_currentWidthPct = value;
    updateSliderLabel(value);
    emit settingsChanged(m_currentWidthPct, m_currentAlignment);
}

void ImageSettingsDialog::updateSliderLabel(int value)
{
    m_widthLabel->setText(QString("%1%").arg(value));
}

void ImageSettingsDialog::onAlignmentChanged(QAbstractButton *button)
{
    m_currentAlignment = button->property("alignment").value<Qt::Alignment>();
    emit settingsChanged(m_currentWidthPct, m_currentAlignment);
}

int ImageSettingsDialog::widthPercentage() const
{
    return m_currentWidthPct;
}

Qt::Alignment ImageSettingsDialog::alignment() const
{
    return m_currentAlignment;
}
