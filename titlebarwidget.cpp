#include "titlebarwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFileInfo>
#include <QStyleOption>
#include <QInputDialog>

TitleBarWidget::TitleBarWidget(QWidget *parent)
    : QWidget(parent), m_edit(new QLineEdit(this)), m_label(new QLabel(this)), m_saveButton(new QPushButton("Save", this)), m_layout(new QHBoxLayout(this)), m_themeManager(nullptr)
{
    // Allow application stylesheet to paint the background/border for this widget
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    // Add small margins to prevent text from clipping border
    m_layout->setContentsMargins(4, 0, 4, 0);
    m_layout->addWidget(m_label);
    m_layout->addWidget(m_edit);
    m_layout->addWidget(m_saveButton);
    m_edit->hide();
    m_saveButton->hide();
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // Align the label text to the left
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setLayout(m_layout);
    connect(m_edit, &QLineEdit::editingFinished, this, &TitleBarWidget::finishEditing);
    connect(m_saveButton, &QPushButton::clicked, this, &TitleBarWidget::onSaveClicked);
    m_label->setCursor(Qt::IBeamCursor);

    // Ensure the titlebar has a sensible width based on the theme's touch target
    // so it remains touch-friendly and visually consistent. Use 3x the touch
    // target.
    if (ThemeManager::instance()) {
        int touch = ThemeManager::instance()->currentTheme().metrics.touchTarget;
        if (touch > 0) {
            const int targetW = touch * 3;
            setMinimumWidth(targetW);
            setMaximumWidth(targetW);
        }
    }
}

void TitleBarWidget::setFilename(const QString &filename) {
    m_filename = filename;
    updateDisplay();
}

void TitleBarWidget::setThemeManager(ThemeManager *themeManager) {
    m_themeManager = themeManager;
    // Update appearance immediately when the theme manager is provided
    if (m_themeManager) {
        // Apply initial theme styling
        onThemeChanged(m_themeManager->currentTheme());
        // Use toolbar/menu background for consistency with MainView
        QColor textCol = m_themeManager->currentTheme().colors.toolbarTextIcon;
        this->setStyleSheet(QString("color: %1;").arg(textCol.name()));
    }
    // Ensure we repaint to pick up any background/menu color used in paintEvent
    update();

    // Connect to theme changes so the titlebar repaints and updates its text color
    // when the active theme changes. Disconnect any previous connection first
    // to avoid duplicate connections if setThemeManager is called more than once.
    if (m_themeManager) {
        // Use a queued connection to ensure ThemeManager's internal state is
        // fully updated before we read it in the slot.
        QObject::disconnect(m_themeManager, nullptr, this, nullptr);
        connect(m_themeManager, &ThemeManager::themeChanged, this, &TitleBarWidget::onThemeChanged, Qt::QueuedConnection);
    }
}

void TitleBarWidget::onThemeChanged(const Theme &newTheme)
{
    Q_UNUSED(newTheme);
    // Update text color and repaint so paintEvent reads the new ThemeManager state
    if (m_themeManager) {
        QColor textCol = m_themeManager->currentTheme().colors.toolbarTextIcon;
        this->setStyleSheet(QString("color: %1;").arg(textCol.name()));
        
        // Apply headerFont to label
        if (m_label) {
            m_label->setFont(m_themeManager->currentTheme().headerFont);
        }
        
        // Apply theme colors and font to edit box
        if (m_edit) {
            m_edit->setFont(m_themeManager->currentTheme().headerFont);
            QColor bg = m_themeManager->currentTheme().colors.background;
            QColor text = m_themeManager->currentTheme().colors.text;
            m_edit->setStyleSheet(QString("QLineEdit { background-color: %1; color: %2; font-size: %3pt; }")
                .arg(bg.name())
                .arg(text.name())
                .arg(m_themeManager->currentTheme().headerFont.pointSize()));
        }
    }
    // Recompute width and height based on the theme's touch target
    if (m_themeManager) {
        int touch = m_themeManager->currentTheme().metrics.touchTarget;
        if (touch > 0) {
            const int targetW = touch * 3;
            setMinimumWidth(targetW);
            setMaximumWidth(targetW);
            // Update height to match touchTarget for proper vertical scaling
            setFixedHeight(touch);
        }
    }

    // Ensure layout/geometry updates propagate to parent containers
    updateGeometry();
    if (parentWidget() && parentWidget()->layout()) {
        parentWidget()->layout()->activate();
    }
    update();
}

QString TitleBarWidget::filename() const {
    return m_filename;
}

void TitleBarWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_label->underMouse()) {
        startEditing();
    }
    QWidget::mousePressEvent(event);
}

void TitleBarWidget::paintEvent(QPaintEvent *event) {
    // Defer to QWidget's paint handling so stylesheets (ThemeManager) control
    // the background and border. Avoid custom painting here which can get out
    // of sync with stylesheet-driven theming.
    QWidget::paintEvent(event);
}

void TitleBarWidget::startEditing() {
    m_label->hide();
    m_edit->setText(nameWithoutExtension(m_filename));
    m_edit->selectAll();
    m_edit->show();
    // Show save button only if there's content or we're editing an existing file
    if (!m_filename.isEmpty() || !m_edit->text().isEmpty()) {
        m_saveButton->show();
    }
    m_edit->setFocus();
    emit filenameEditRequested();
}

void TitleBarWidget::finishEditing() {
    QString newName = m_edit->text().trimmed();
    if (!newName.isEmpty() && newName != nameWithoutExtension(m_filename)) {
        QString ext = extension(m_filename);
        m_filename = newName + ext;
        emit filenameChanged(m_filename);
    }
    m_edit->hide();
    m_saveButton->hide();
    updateDisplay();
}

void TitleBarWidget::updateDisplay() {
    m_label->setText(nameWithoutExtension(m_filename));
    m_label->show();
    m_edit->hide();
    m_saveButton->hide();
}

void TitleBarWidget::onSaveClicked() {
    QString newName = m_edit->text().trimmed();
    if (newName.isEmpty()) {
        // Prompt user for a filename
        newName = QInputDialog::getText(this, tr("Save File"), tr("Enter filename:"));
        if (newName.isEmpty()) {
            return; // User cancelled
        }
    }
    
    // If we don't have a filename yet (new file), just emit filenameChanged
    // The main view will handle saving to a new file
    if (m_filename.isEmpty()) {
        m_filename = newName + ".txt"; // Default extension
        emit filenameChanged(m_filename);
    } else if (newName != nameWithoutExtension(m_filename)) {
        QString ext = extension(m_filename);
        m_filename = newName + ext;
        emit filenameChanged(m_filename);
    }
    
    m_edit->hide();
    m_saveButton->hide();
    updateDisplay();
    emit saveRequested();
}

QString TitleBarWidget::nameWithoutExtension(const QString &filename) const {
    QFileInfo fi(filename);
    return fi.completeBaseName();
}

QString TitleBarWidget::extension(const QString &filename) const {
    QFileInfo fi(filename);
    QString ext = fi.suffix();
    return ext.isEmpty() ? QString() : "." + ext;
}
