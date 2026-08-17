#include "panelmanager.h"
#include "thememanager.h"
#include <QSplitter>
#include <QToolButton>
#include <QVariantAnimation>
#include <QIcon>
#include <QSize>
#include <QtMath>

PanelManager::PanelManager(QSplitter *splitter, QWidget *leftPanel, QWidget *rightPanel,
                           QWidget *handleParent, QObject *parent)
    : QObject(parent)
    , m_splitter(splitter)
    , m_leftPanel(leftPanel)
    , m_rightPanel(rightPanel)
{
    createHandles(handleParent);
    createAnimation();

    connect(m_splitter, &QSplitter::splitterMoved, this, &PanelManager::positionHandles);
}

PanelManager::~PanelManager()
{
    if (m_animation) {
        m_animation->stop();
        m_animation->disconnect();
    }
}

// ── Public API ──────────────────────────────────────────────────────────────

bool PanelManager::isPanelOpen(PanelSide side) const
{
    return side == PanelSide::Left ? m_leftOpen : m_rightOpen;
}

void PanelManager::togglePanel(PanelSide side, bool open)
{
    if (side == PanelSide::Left && m_leftOpen == open) return;
    if (side == PanelSide::Right && m_rightOpen == open) return;
    if (side == PanelSide::Left)
        m_userToggledSidebar = true;

    if (side == PanelSide::Left) {
        m_leftOpen = open;
        if (open && m_rightOpen) m_rightOpen = false;
    } else {
        m_rightOpen = open;
        if (open && m_leftOpen) m_leftOpen = false;
    }

    if (m_leftOpen && m_leftPanel)  m_leftPanel->show();
    if (m_rightOpen && m_rightPanel) m_rightPanel->show();

    if (m_leftHandle) {
        m_leftHandle->setIcon(QIcon(m_leftOpen
            ? ":/resources/icons/custom/chevrons-left.svg"
            : ":/resources/icons/custom/chevrons-right.svg"));
    }
    if (m_rightHandle) {
        m_rightHandle->setIcon(QIcon(m_rightOpen
            ? ":/resources/icons/custom/chevrons-right.svg"
            : ":/resources/icons/custom/chevrons-left.svg"));
    }

    startAnimation();
}

// ── Handle creation ─────────────────────────────────────────────────────────

void PanelManager::createHandles(QWidget *handleParent)
{
    m_leftHandle = new QToolButton(handleParent);
    m_leftHandle->setFixedWidth(handleThumbWidth());
    m_leftHandle->setCursor(Qt::PointingHandCursor);
    m_leftHandle->setToolTip("File Browser");
    m_leftHandle->setIcon(QIcon(":/resources/icons/custom/chevrons-right.svg"));
    m_leftHandle->setIconSize(QSize(handleThumbWidth(),
        ThemeManager::instance()->currentTheme().metrics.iconSize));
    m_leftHandle->setVisible(true);
    m_leftHandle->raise();
    connect(m_leftHandle, &QToolButton::clicked, this, [this]() {
        togglePanel(PanelSide::Left, !m_leftOpen);
    });

    m_rightHandle = new QToolButton(handleParent);
    m_rightHandle->setFixedWidth(handleThumbWidth());
    m_rightHandle->setCursor(Qt::PointingHandCursor);
    m_rightHandle->setToolTip("Settings");
    m_rightHandle->setIcon(QIcon(":/resources/icons/custom/chevrons-left.svg"));
    m_rightHandle->setIconSize(QSize(handleThumbWidth(),
        ThemeManager::instance()->currentTheme().metrics.iconSize));
    m_rightHandle->setVisible(true);
    m_rightHandle->raise();
    connect(m_rightHandle, &QToolButton::clicked, this, [this]() {
        togglePanel(PanelSide::Right, !m_rightOpen);
    });
}

// ── Animation ───────────────────────────────────────────────────────────────

void PanelManager::createAnimation()
{
    m_animation = QuteNote::makeOwned<QVariantAnimation>();
    m_animation->setDuration(220);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_animation.get(), &QVariantAnimation::valueChanged,
            this, &PanelManager::onAnimationValueChanged);
    connect(m_animation.get(), &QVariantAnimation::finished, this, [this]() {
        if (!m_leftOpen && m_leftPanel) m_leftPanel->hide();
        if (!m_rightOpen && m_rightPanel) m_rightPanel->hide();
        positionHandles();
    });
}

void PanelManager::startAnimation()
{
    if (!m_splitter || !m_animation) return;

    QList<int> sizes = m_splitter->sizes();
    if (sizes.size() < 3) return;

    m_animStartLeft  = sizes[0];
    m_animStartRight = sizes[2];

    m_animEndLeft = 0;
    if (m_leftOpen) {
        if (m_leftWidth <= 0) m_leftWidth = defaultWidth(PanelSide::Left);
        m_animEndLeft = m_leftWidth;
    }
    if (!m_leftOpen && m_animStartLeft > 0)
        m_leftWidth = m_animStartLeft;

    m_animEndRight = 0;
    if (m_rightOpen) {
        if (m_rightWidth <= 0) m_rightWidth = defaultWidth(PanelSide::Right);
        m_animEndRight = m_rightWidth;
    }
    if (!m_rightOpen && m_animStartRight > 0)
        m_rightWidth = m_animStartRight;

    m_animation->stop();
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->start();
}

void PanelManager::onAnimationValueChanged(const QVariant &value)
{
    if (!m_splitter) return;

    double t = value.toDouble();
    int leftW  = qRound(m_animStartLeft  + (m_animEndLeft  - m_animStartLeft)  * t);
    int rightW = qRound(m_animStartRight + (m_animEndRight - m_animStartRight) * t);

    int totalWidth = m_splitter->width();
    int editorWidth = totalWidth - leftW - rightW;
    if (editorWidth < kEditorMinWidth) {
        editorWidth = kEditorMinWidth;
        int excess = totalWidth - editorWidth - leftW - rightW;
        if (excess < 0) {
            int share = leftW + rightW;
            if (share > 0) {
                leftW  = leftW  * (totalWidth - editorWidth) / share;
                rightW = rightW * (totalWidth - editorWidth) / share;
            }
        }
        editorWidth = totalWidth - leftW - rightW;
        if (editorWidth < kEditorMinWidth) editorWidth = kEditorMinWidth;
    }

    m_splitter->setSizes({leftW, editorWidth, rightW});
    positionHandles();
}

// ── Handle styling ──────────────────────────────────────────────────────────

void PanelManager::applyHandleStyle(QToolButton *handle, PanelSide side)
{
    if (!handle) return;
    Q_UNUSED(side);
    Theme theme = ThemeManager::instance()->currentTheme();

    QColor bg = theme.colors.menuBackground.isValid()
        ? theme.colors.menuBackground
        : theme.colors.background.darker(120);
    QColor textCol = theme.colors.toolbarTextIcon.isValid()
        ? theme.colors.toolbarTextIcon
        : theme.colors.text;
    int radius = theme.metrics.borderRadius;

    handle->setStyleSheet(QString(
        "QToolButton {"
        "  background: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: %3px;"
        "  padding: 0px;"
        "}"
        "QToolButton:hover {"
        "  background: %4;"
        "}"
    ).arg(bg.name())
     .arg(textCol.name())
     .arg(radius)
     .arg(bg.lighter(120).name()));

    handle->setIconSize(QSize(16, ThemeManager::instance()->currentTheme().metrics.iconSize));
}

// ── Dimension helpers ───────────────────────────────────────────────────────

int PanelManager::handleThumbWidth() const
{
    Theme theme = ThemeManager::instance()->currentTheme();
    return qMax(12, theme.metrics.touchTarget / 4);
}

int PanelManager::handleThumbHeight() const
{
    Theme theme = ThemeManager::instance()->currentTheme();
    return qMin(kHandleMaxHeight, theme.metrics.touchTarget * 2);
}

int PanelManager::defaultWidth(PanelSide side) const
{
    Q_UNUSED(side);
    int totalWidth = m_splitter ? m_splitter->width() : 800;
    if (totalWidth <= 0) totalWidth = 800;

    if (totalWidth <= 320)
        return totalWidth;
    if (totalWidth <= 768)
        return totalWidth / 2;
    return qMin(320, totalWidth / 2);
}

void PanelManager::positionHandles()
{
    if (!m_splitter) return;

    QRect splitterRect = m_splitter->geometry();
    QList<int> sizes = m_splitter->sizes();
    int handleH = qMin(splitterRect.height(), handleThumbHeight());
    int handleY = (splitterRect.height() - handleH) / 2;

    int leftPanelW = (sizes.size() >= 1) ? sizes[0] : 0;
    int rightPanelW = (sizes.size() >= 3) ? sizes[2] : 0;

    if (m_leftHandle) {
        m_leftHandle->setFixedHeight(handleH);
        int xPos = splitterRect.left() + (m_leftOpen ? leftPanelW : 0);
        m_leftHandle->move(xPos, splitterRect.top() + handleY);
        m_leftHandle->raise();
    }

    if (m_rightHandle) {
        m_rightHandle->setFixedHeight(handleH);
        int xPos = splitterRect.right() - m_rightHandle->width()
                   - (m_rightOpen ? rightPanelW : 0);
        m_rightHandle->move(xPos, splitterRect.top() + handleY);
        m_rightHandle->raise();
    }
}
