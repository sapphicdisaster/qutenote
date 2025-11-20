#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <QWidget>
#include <QSet>
#include <QHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QPropertyAnimation>
#include <QScroller>
#include <QPushButton>
#include <QTimer>
#include <QShowEvent>
#include "filebrowsertreewidget.h"
#include "filebrowsertouchhandler.h"
#include "uiutils.h"
#include "componentbase.h"
#include "smartpointers.h"
#include "touchinteraction.h"

// Forward declarations
class QVBoxLayout;
class QHBoxLayout;
class QMenu;
class QAction;
struct Theme;

struct RecentFile {
    QString path;
    QDateTime lastAccessed;
    bool operator<(const RecentFile &other) const {
        return lastAccessed > other.lastAccessed;
    }
    bool operator==(const RecentFile &other) const {
        return path == other.path;
    }
};

inline uint qHash(const RecentFile &file, uint seed = 0) {
    return qHash(file.path, seed);
}

class FileBrowser : public QuteNote::ComponentBase
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FileBrowser)

public:
    explicit FileBrowser(QWidget *parent = nullptr);
    ~FileBrowser() override;
    
    void setRootDirectory(const QString &path);
    QString currentDirectory() const { return m_currentDirectory; }
    QString selectedFile() const;
    void populateTree();
    
    // Touch-related methods
    FileBrowserTreeWidget* treeWidget() const { return m_treeWidget.get(); }
    void navigateBack();
    void navigateForward();
    void setJellyStrength(qreal strength) { if (m_touchHandler) m_touchHandler->touchInteraction()->setJellyStrength(strength); }
    void setFriction(qreal friction) { if (m_touchHandler) m_touchHandler->touchInteraction()->setFriction(friction); }
    void setOverscrollAmount(qreal amount);
    
    // Touch handler access
    FileBrowserTouchHandler* touchHandler() const { return m_touchHandler.get(); }
    
    // Sorting
    enum SortOrder { Name, Date, Size, Type };
    void setSortOrder(SortOrder order);
    
    // Navigation
    void navigateTo(const QString &path);
    
    // Performance
    void setLazyLoadingEnabled(bool enabled);
    
    // Signals
    Q_SIGNALS:
    void fileSelected(const QString &path);
    void directoryChanged(const QString &path);
    void fileCreated(const QString &path);
    void fileDeleted(const QString &path);
    void fileRenamed(const QString &oldPath, const QString &newPath);
    void overscrollAmountChanged(qreal amount);
    void scrollLimitReached(qreal position);

public:
    // File operations
    void createFolder();
    void createNote();
    void createDivider();
    void removeSelectedItem();

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onItemSelectionChanged();
    void onContextMenuRequested(const QPoint &pos);
    void onCreateFolder();
    void onCreateNote();
    void onCreateDivider();
    void onRemoveItem();
    void onRename();
    void onItemOrderChanged(const QString &sourcePath, const QString &oldParentPath, const QString &newParentPath, int newIndex);
    void processMoveBuffer();

private:
    // Recent files
    void updateRecentFiles(const QString &filePath);
    void loadRecentFiles();
    void saveRecentFiles();
    QTreeWidgetItem* createRecentFilesSection();
    void rebuildRecentFilesSection(QTreeWidgetItem *rootItem);
    
    // Animations
    void animateItemExpansion(QTreeWidgetItem *item);
    void animateItemCollapse(QTreeWidgetItem *item);
    QPropertyAnimation* createFadeAnimation(QWidget *target, qreal startValue, qreal endValue);
    void setupItemAnimation(QTreeWidgetItem *item);
    void setupOverscrollAnimation();
    
    // Touch feedback
    void setupTouchFeedback();
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void highlightItem(QTreeWidgetItem *item, bool highlight);
    
    // UI Setup
    void setupUI();
    void setupContextMenu();
    void setupBreadcrumbs();
    void updateBreadcrumbs();
    void clearBreadcrumbs();
    void updateOverlayButtonPositions();
    void forceUiRefreshAfterDialog();
    void updateStatusBar(const QString &message, int timeoutMs = 10000);
    
    // File operations
    void addDirectoryToTree(QTreeWidgetItem *parentItem, const QDir &directory);
    void loadDirectory(QTreeWidgetItem *parentItem, const QString &path);
    QTreeWidgetItem* addFileItem(QTreeWidgetItem *parentItem, const QFileInfo &fileInfo);
    QTreeWidgetItem* findOrCreateParentItem(const QString &path);
    void captureExpandedPaths();
    void restoreExpandedPaths();
    bool expandPath(const QString &absPath, bool selectEnd = false);
    void updateButtonStates();
    QString displayNameForEntry(const QFileInfo &info) const;
    QString stripOrderingPrefix(const QString &text) const;
    QFileInfoList listDirectoryEntries(const QString &directoryPath);
    QList<QFileInfo> orderedEntriesForDirectory(const QString &directoryPath);
    QFileInfo maybeMigratePrefixedEntry(const QFileInfo &info);
    QString orderingMetadataPath(const QString &directoryPath) const;
    QStringList loadOrderingMetadata(const QString &directoryPath) const;
    void saveOrderingMetadata(const QString &directoryPath, const QStringList &orderedNames) const;
    QStringList ensureOrderingMetadata(const QString &directoryPath, const QFileInfoList &entries);
    void recordOrderingFromTree(const QString &directoryPath);
    void upsertNameInOrdering(const QString &directoryPath, const QString &name, int index = -1);
    void removeNameFromOrdering(const QString &directoryPath, const QString &name);
    void renameEntryInOrdering(const QString &directoryPath, const QString &oldName, const QString &newName);
    QTreeWidgetItem* findTreeItemForPath(const QString &path) const;
    
    // Sorting
    void sortItems();
    static bool compareItems(QTreeWidgetItem *a, QTreeWidgetItem *b);
    
    // Component functionality
    void initializeComponent() override;
    void setupConnections() override;
    void cleanupResources() override;
    void handleMemoryWarning() override;

    // Performance
    bool m_lazyLoading = true;
    QSet<QString> m_loadedPaths;
    QuteNote::OwnedPtr<QTimer> m_refreshTimer;
    
    // Buffered file move operations to avoid partial/missing references while
    // the view is updated. Moves are accumulated and flushed by a timer.
    struct FileMove {
        QString sourcePath;
        QString oldParentPath;
        QString newParentPath;
        int newIndex = -1;
    };
    QList<FileMove> m_moveBuffer;
    QuteNote::OwnedPtr<QTimer> m_moveBufferTimer;
    
    // Touch interaction
    QuteNote::OwnedPtr<FileBrowserTouchHandler> m_touchHandler;
    QuteNote::OwnedPtr<QPropertyAnimation> m_overscrollAnimation;
    
    // UI Components
    QuteNote::OwnedPtr<QHBoxLayout> m_breadcrumbLayout;
    QuteNote::OwnedPtr<QWidget> m_breadcrumbContainer;
    SortOrder m_sortOrder = Name;

    QString m_rootDirectory;
    QString m_currentDirectory;
    QuteNote::OwnedPtr<QAction> m_removeAction;
    QuteNote::OwnedPtr<QAction> m_renameAction;
    QSet<RecentFile> m_recentFiles;
    QuteNote::OwnedPtr<QTreeWidgetItem> m_recentFilesRoot;
    static const int MAX_RECENT_FILES = 10;
    QuteNote::OwnedPtr<FileBrowserTreeWidget> m_treeWidget;
    QuteNote::OwnedPtr<QVBoxLayout> m_layout;
    
    // Bottom button bar (fixed at the bottom under the tree)
    QuteNote::OwnedPtr<QWidget> m_buttonBar;
    QuteNote::OwnedPtr<QHBoxLayout> m_buttonBarLayout;
    QuteNote::OwnedPtr<QPushButton> m_createFolderBtn;
    QuteNote::OwnedPtr<QPushButton> m_createNoteBtn;
    QuteNote::OwnedPtr<QPushButton> m_createDividerBtn;
    QuteNote::OwnedPtr<QPushButton> m_removeBtn;

    QuteNote::OwnedPtr<QMenu> m_contextMenu;
    QuteNote::OwnedPtr<QAction> m_createFolderAction;
    QuteNote::OwnedPtr<QAction> m_createNoteAction;
    QuteNote::OwnedPtr<QAction> m_createDividerAction;

    // Remember which directories are expanded by absolute path
    QSet<QString> m_expandedDirs;
    mutable QHash<QString, QStringList> m_cachedOrdering;
};

#endif // FILEBROWSER_H
