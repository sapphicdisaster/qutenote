#include "filebrowser.h"
#include "thememanager.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTouchEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QFileInfo>
#include <QScroller>
#include <QScrollBar>
#include <QGraphicsEffect>
#include <QTimer>
#include <QList>
#include <QDebug>
#include <algorithm>

void FileBrowser::setupTouchFeedback()
{
    // Touch feedback is now handled by FileBrowserTouchHandler
    // This method is kept for compatibility but does nothing
    // The touch handler is initialized in FileBrowser::initializeComponent()
}

void FileBrowser::setupOverscrollAnimation()
{
    if (!m_overscrollAnimation) {
        m_overscrollAnimation = QuteNote::makeOwned<QPropertyAnimation>(this, "overscrollAmount", this);
        m_overscrollAnimation->setEasingCurve(QEasingCurve::OutBack);
        m_overscrollAnimation->setDuration(350);
    }
}

void FileBrowser::setOverscrollAmount(qreal amount)
{
    QScrollBar *vScrollBar = m_treeWidget->verticalScrollBar();
    if (!vScrollBar)
        return;
    
    // Apply overscroll with visual feedback
    const int maxOverscroll = m_treeWidget->height() / 3;
    const qreal clampedAmount = qBound(-maxOverscroll, static_cast<int>(amount), maxOverscroll);
    
    // Apply a scale transform for the "squish" effect
    qreal scale = 1.0;
    if (clampedAmount != 0) {
        const qreal factor = qAbs(clampedAmount) / maxOverscroll;
        scale = 1.0 - (factor * 0.1); // Squish by up to 10%
    }
    
    // Apply transforms - need option for qtreewidget, doesnt support settransform
    
    // Adjust content position
    vScrollBar->setValue(vScrollBar->value() + clampedAmount);
    
    emit overscrollAmountChanged(amount);
}

bool FileBrowser::eventFilter(QObject *watched, QEvent *event)
{
    // Repaint viewport to clear drag artifacts when leaving/ending outside
    if (m_treeWidget && watched == m_treeWidget->viewport()) {
        switch (event->type()) {
        case QEvent::DragLeave:
        case QEvent::Drop:
        case QEvent::Leave:
        case QEvent::MouseButtonRelease:
        case QEvent::TouchEnd:
        case QEvent::TouchCancel:
            // Force immediate viewport update to clear any drag artifacts
            m_treeWidget->viewport()->update();
            // Also schedule a delayed update to catch any lingering artifacts
            QTimer::singleShot(0, this, [this]() {
                if (m_treeWidget && m_treeWidget->viewport()) {
                    m_treeWidget->viewport()->update();
                }
            });
            // Additional cleanup for drop events
            if (event->type() == QEvent::Drop) {
                QTimer::singleShot(50, this, [this]() {
                    if (m_treeWidget) {
                        m_treeWidget->viewport()->update();
                        // Clear any lingering selection artifacts
                        m_treeWidget->clearFocus();
                        m_treeWidget->setFocus();
                    }
                });
            }
            break;
        case QEvent::MouseButtonPress: {
            // If the user taps/clicks outside any item, select the parent directory
            // of the current selection. For top-level, clear selection to target root.
            if (auto *me = static_cast<QMouseEvent*>(event)) {
                QTreeWidgetItem *hit = m_treeWidget->itemAt(me->pos());
                if (!hit) {
                    QTreeWidgetItem *current = m_treeWidget->currentItem();
                    if (current) {
                        if (QTreeWidgetItem *parent = current->parent()) {
                            m_treeWidget->setCurrentItem(parent);
                        } else {
                            // No parent (top-level) -> clear selection and target root/current dir
                            m_treeWidget->clearSelection();
                            m_treeWidget->setCurrentItem(nullptr);
                            if (!m_rootDirectory.isEmpty()) {
                                m_currentDirectory = m_rootDirectory;
                                emit directoryChanged(m_currentDirectory);
                            }
                        }
                        updateButtonStates();
                        return true; // consume to avoid unintended behaviors
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }

    // Do not intercept button MouseButtonRelease/TouchEnd here; clicked() handles cleanup

    return QWidget::eventFilter(watched, event);
}

void FileBrowser::highlightItem(QTreeWidgetItem *item, bool highlight)
{
    QColor highlightColor = ThemeManager::instance()->currentTheme().colors.accent;
    highlightColor.setAlpha(highlight ? 128 : 0);
    item->setBackground(0, highlightColor);
}

QPropertyAnimation* FileBrowser::createFadeAnimation(QWidget *target, qreal startValue, qreal endValue)
{
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(target);
    target->setGraphicsEffect(effect);
    
    QPropertyAnimation *animation = new QPropertyAnimation(effect, "opacity", this);
    animation->setDuration(150);
    animation->setStartValue(startValue);
    animation->setEndValue(endValue);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    
    return animation;
}

void FileBrowser::animateItemExpansion(QTreeWidgetItem *item)
{
    // Create a widget to animate
    QWidget *itemWidget = m_treeWidget->itemWidget(item, 0);
    if (!itemWidget) {
        itemWidget = new QWidget(m_treeWidget.get());
        m_treeWidget->setItemWidget(item, 0, itemWidget);
    }
    
    // Create fade in animation
    QPropertyAnimation *fadeIn = createFadeAnimation(itemWidget, 0.0, 1.0);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
}

void FileBrowser::animateItemCollapse(QTreeWidgetItem *item)
{
    QWidget *itemWidget = m_treeWidget->itemWidget(item, 0);
    if (itemWidget) {
        QPropertyAnimation *fadeOut = createFadeAnimation(itemWidget, 1.0, 0.0);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void FileBrowser::rebuildRecentFilesSection(QTreeWidgetItem *rootItem)
{
    if (!rootItem) {
        return;
    }

    rootItem->takeChildren();
    QList<RecentFile> staleEntries;

    for (const RecentFile &file : m_recentFiles) {
        QFileInfo info(file.path);
        const QString absolutePath = info.absoluteFilePath();
        if (absolutePath.isEmpty() || !info.exists()) {
            staleEntries.append(file);
            continue;
        }

        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, displayNameForEntry(info));
        item->setData(0, Qt::UserRole, absolutePath);
        item->setToolTip(0, absolutePath);
        item->setIcon(0, QIcon::fromTheme("text-x-generic"));
        rootItem->addChild(item);
    }

    for (const RecentFile &stale : staleEntries) {
        m_recentFiles.remove(stale);
    }
}

QTreeWidgetItem* FileBrowser::createRecentFilesSection()
{
    QTreeWidgetItem *recentFiles = new QTreeWidgetItem(QStringList() << "Recent Files");
    recentFiles->setIcon(0, QIcon::fromTheme("document-open-recent"));
    rebuildRecentFilesSection(recentFiles);
    return recentFiles;
}

void FileBrowser::updateRecentFiles(const QString &filePath)
{
    QFileInfo info(filePath);
    const QString absolutePath = info.absoluteFilePath();

    if (absolutePath.isEmpty()) {
        qWarning() << "Skipping recent file update for empty path" << filePath;
        return;
    }

    if (!info.exists()) {
        m_recentFiles.remove(RecentFile{absolutePath, QDateTime()});
        rebuildRecentFilesSection(m_recentFilesRoot.get());
        saveRecentFiles();
        return;
    }

    RecentFile newFile{absolutePath, QDateTime::currentDateTime()};
    m_recentFiles.remove(newFile);
    m_recentFiles.insert(newFile);

    while (m_recentFiles.size() > MAX_RECENT_FILES) {
        auto it = std::min_element(m_recentFiles.begin(), m_recentFiles.end());
        if (it == m_recentFiles.end()) {
            break;
        }
        m_recentFiles.remove(*it);
    }

    rebuildRecentFilesSection(m_recentFilesRoot.get());
    saveRecentFiles();
}

void FileBrowser::loadRecentFiles()
{
    QSettings settings;
    const QByteArray raw = settings.value("recentFiles").toByteArray();
    if (raw.isEmpty()) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    const QJsonArray array = doc.array();

    for (const QJsonValue &val : array) {
        const QJsonObject obj = val.toObject();
        QFileInfo info(obj.value("path").toString());
        const QString absolutePath = info.absoluteFilePath();
        if (absolutePath.isEmpty() || !info.exists()) {
            continue;
        }

        QDateTime lastAccessed = QDateTime::fromString(obj.value("lastAccessed").toString(), Qt::ISODate);
        if (!lastAccessed.isValid()) {
            lastAccessed = QDateTime::currentDateTime();
        }

        m_recentFiles.insert(RecentFile{absolutePath, lastAccessed});
    }

    rebuildRecentFilesSection(m_recentFilesRoot.get());
    saveRecentFiles();
}

void FileBrowser::saveRecentFiles()
{
    QJsonArray array;
    for (const RecentFile &file : m_recentFiles) {
        QJsonObject obj;
        obj["path"] = file.path;
        obj["lastAccessed"] = file.lastAccessed.toString(Qt::ISODate);
        array.append(obj);
    }
    
    QSettings settings;
    settings.setValue("recentFiles", QJsonDocument(array).toJson());
}