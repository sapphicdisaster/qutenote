#ifndef FILEBROWSERTREEWIDGET_H
#define FILEBROWSERTREEWIDGET_H

#include <QTreeWidget>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMimeData>
#include <QDrag>

class FileBrowserTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit FileBrowserTreeWidget(QWidget *parent = nullptr);
    ~FileBrowserTreeWidget() override = default;

    // Set the root directory for proper drop handling on empty space
    void setRootDirectory(const QString &rootDir) { m_rootDirectory = rootDir; }
    QString rootDirectory() const { return m_rootDirectory; }

    // Public slot to initiate drag operation (overrides QAbstractItemView)
    void startDrag(Qt::DropActions supportedActions) override;

    // New public slot to initiate drag with a start position
    void initiateDrag(Qt::DropActions supportedActions, const QPoint &startPos);

signals:
    // Emitted after an internal move. We pass string paths instead of pointers so
    // callers can safely act on the filesystem without relying on item pointer validity.
    void itemOrderChanged(const QString &sourcePath, const QString &oldParentPath, const QString &newParentPath, int newIndex);

protected:
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    
    // Override to prevent Qt's default drag-drop behavior
    QMimeData* mimeData(const QList<QTreeWidgetItem*> &items) const override;

private:
    QString getItemPath(QTreeWidgetItem *item) const;
    QStringList extractPathsFromMime(const QMimeData *data) const;
    QStringList currentSelectionPaths() const;
    
    QString m_rootDirectory;
    QPoint m_dragStartPos; // Store the initial drag position
};

#endif // FILEBROWSERTREEWIDGET_H
