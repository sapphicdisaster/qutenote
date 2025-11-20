#include "filebrowsertreewidget.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QMimeData>
#include <QUrl>
#include <QPixmap>
#include <QPainter>
#include <QDrag>
#include <memory>

FileBrowserTreeWidget::FileBrowserTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragEnabled(true);
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(true);
}

void FileBrowserTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    QTreeWidget::dragEnterEvent(event); // Let base class handle it
}

void FileBrowserTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event); // Let base class handle it
}

void FileBrowserTreeWidget::dropEvent(QDropEvent *event)
{
    if (!event) {
        QTreeWidget::dropEvent(event);
        return;
    }

    QStringList sourcePaths = extractPathsFromMime(event->mimeData());
    if (sourcePaths.isEmpty()) {
        sourcePaths = currentSelectionPaths();
    }

    if (sourcePaths.isEmpty()) {
        QTreeWidget::dropEvent(event);
        return;
    }

    QStringList oldParentPaths;
    oldParentPaths.reserve(sourcePaths.size());
    for (const QString &path : sourcePaths) {
        oldParentPaths << QFileInfo(path).absolutePath();
    }

    const QPoint dropPos = event->position().toPoint();
    QTreeWidgetItem *targetItem = itemAt(dropPos);
    QTreeWidgetItem *resolvedParent = nullptr;
    int insertIndex = -1;

    auto indicator = dropIndicatorPosition();
    if (targetItem && indicator != QAbstractItemView::OnItem) {
        const QFileInfo targetInfo(getItemPath(targetItem));
        if (targetInfo.isDir()) {
            const QRect targetRect = visualItemRect(targetItem);
            if (targetRect.contains(dropPos) && targetRect.height() > 0) {
                const int relativeY = dropPos.y() - targetRect.top();
                const double ratio = static_cast<double>(relativeY) / targetRect.height();
                if (ratio >= 0.25 && ratio <= 0.75) {
                    indicator = QAbstractItemView::OnItem;
                }
            }
        }
    }

    switch (indicator) {
    case QAbstractItemView::OnItem:
        resolvedParent = targetItem;
        insertIndex = resolvedParent ? resolvedParent->childCount() : -1;
        break;
    case QAbstractItemView::AboveItem:
    case QAbstractItemView::BelowItem:
        if (targetItem) {
            resolvedParent = targetItem->parent();
            const int siblingIndex = resolvedParent ? resolvedParent->indexOfChild(targetItem)
                                                    : indexOfTopLevelItem(targetItem);
            insertIndex = siblingIndex;
            if (indicator == QAbstractItemView::BelowItem) {
                ++insertIndex;
            }
        } else {
            resolvedParent = nullptr;
            insertIndex = topLevelItemCount();
        }
        break;
    case QAbstractItemView::OnViewport:
    default:
        resolvedParent = nullptr;
        insertIndex = topLevelItemCount();
        break;
    }

    if (indicator == QAbstractItemView::OnItem && resolvedParent) {
        const QFileInfo info(getItemPath(resolvedParent));
        if (!info.isDir()) {
            QTreeWidgetItem *parentOfTarget = resolvedParent->parent();
            if (parentOfTarget) {
                insertIndex = parentOfTarget->indexOfChild(resolvedParent) + 1;
            } else {
                insertIndex = indexOfTopLevelItem(resolvedParent) + 1;
            }
            resolvedParent = parentOfTarget;
        }
    }

    QString parentPath = resolvedParent ? getItemPath(resolvedParent) : m_rootDirectory;

    QTreeWidget::dropEvent(event);

    if (parentPath.isEmpty()) {
        parentPath = m_rootDirectory;
    }

    for (int i = 0; i < sourcePaths.size(); ++i) {
        emit itemOrderChanged(sourcePaths.at(i),
                              oldParentPaths.value(i),
                              parentPath,
                              insertIndex < 0 ? -1 : insertIndex + i);
    }
}

QMimeData* FileBrowserTreeWidget::mimeData(const QList<QTreeWidgetItem*> &items) const
{
    // Keep Qt's default MIME payload so the base class can reinsert items, then
    // augment it with our filesystem-specific data.
    QMimeData *mimeData = QTreeWidget::mimeData(items);
    if (!mimeData) {
        mimeData = new QMimeData();
    }

    if (!items.isEmpty()) {
        QStringList paths;
        for (QTreeWidgetItem *it : items) {
            if (!it) continue;
            const QVariant d = it->data(0, Qt::UserRole);
            if (d.isValid()) paths << d.toString();
        }

        if (!paths.isEmpty()) {
            mimeData->setData("application/x-qutenote-paths", paths.join('\n').toUtf8());

            QList<QUrl> urls = mimeData->urls();
            for (const QString &p : paths) {
                urls << QUrl::fromLocalFile(p);
            }
            mimeData->setUrls(urls);
        }
    }

    return mimeData;
}

QString FileBrowserTreeWidget::getItemPath(QTreeWidgetItem *item) const
{
    if (!item) return QString();
    const QVariant d = item->data(0, Qt::UserRole);
    if (d.isValid() && d.canConvert<QString>()) return d.toString();

    // Fallback: try to reconstruct a path by walking parents and joining texts.
    // This is a conservative fallback to avoid returning empty strings in some
    // legacy list setups; callers should prefer UserRole data when available.
    QStringList parts;
    QTreeWidgetItem *cur = item;
    while (cur) {
        parts.prepend(cur->text(0));
        cur = cur->parent();
    }
    if (!parts.isEmpty()) return parts.join('/');
    return QString();
}

void FileBrowserTreeWidget::startDrag(Qt::DropActions supportedActions)
{
    const QList<QTreeWidgetItem*> items = selectedItems();
    std::unique_ptr<QMimeData> md(mimeData(items));
    if (!md) return;

    QDrag *drag = new QDrag(this);
    drag->setMimeData(md.release());

    // Create a simple drag pixmap using the first selected item's text/icon
    if (!items.isEmpty()) {
        QPixmap pix(200, 24);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setPen(palette().color(QPalette::Text));
        p.drawText(4, 16, items.first()->text(0));
        p.end();
        drag->setPixmap(pix);
    }

    // Execute the drag; allow moving by default
    drag->exec(supportedActions ? supportedActions : Qt::MoveAction);
}

QStringList FileBrowserTreeWidget::extractPathsFromMime(const QMimeData *data) const
{
    QStringList sourcePaths;
    if (!data) {
        return sourcePaths;
    }

    if (data->hasFormat("application/x-qutenote-paths")) {
        const QByteArray ba = data->data("application/x-qutenote-paths");
        sourcePaths = QString::fromUtf8(ba).split('\n', Qt::SkipEmptyParts);
    } else if (!data->urls().isEmpty()) {
        for (const QUrl &u : data->urls()) {
            if (u.isLocalFile()) {
                sourcePaths << u.toLocalFile();
            }
        }
    }

    return sourcePaths;
}

QStringList FileBrowserTreeWidget::currentSelectionPaths() const
{
    QStringList paths;
    const QList<QTreeWidgetItem*> items = selectedItems();
    for (QTreeWidgetItem *item : items) {
        if (!item) {
            continue;
        }
        const QVariant d = item->data(0, Qt::UserRole);
        if (d.isValid()) {
            paths << d.toString();
        }
    }
    return paths;
}

void FileBrowserTreeWidget::initiateDrag(Qt::DropActions supportedActions, const QPoint &startPos)
{
    // Store the start position (unused by current implementation but useful
    // for future enhancements where the visual drag start may differ)
    m_dragStartPos = startPos;

    // Ensure the widget has focus so selection is respected
    setFocus(Qt::MouseFocusReason);

    // Start the drag immediately
    startDrag(supportedActions);
}
