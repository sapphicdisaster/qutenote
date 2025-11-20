#include "filebrowser.h"
#include "filebrowserdividerdelegate.h"
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QStyle>
#include <QMainWindow>
#include <QStatusBar>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QFile>
#include <QSaveFile>
#include <QFrame>
#include <QEvent>
#include <QTimer>
#include <QRegularExpression>
#include <QDebug>
#include <QTextStream>
#include <algorithm>
#include "thememanager.h"

namespace {
constexpr const char *kOrderingFileName = ".qutenote_order.md";
}

FileBrowser::FileBrowser(QWidget *parent)
    : QuteNote::ComponentBase(parent)
    , m_rootDirectory(QDir::homePath())
    , m_currentDirectory(QDir::homePath())
{
    initializeComponent();
}

void FileBrowser::initializeComponent()
{
    // Create tree widget and main layout (Qt 6: install layout on this widget)
    m_treeWidget = QuteNote::makeOwned<FileBrowserTreeWidget>(this);
    m_layout = QuteNote::makeOwned<QVBoxLayout>(this);

    // Set initial root directory for drag-drop handling
    m_treeWidget->setRootDirectory(m_rootDirectory);

    // Drag-drop is now configured in FileBrowserTreeWidget constructor
    // with proper drop event handling to perform actual file moves
    
    // Make tree widget touch-friendly
    UIUtils::makeTouchFriendly(m_treeWidget.get());
    // Enable kinetic scrolling on the tree viewport for touch devices
    QScroller::grabGesture(m_treeWidget->viewport(), QScroller::TouchGesture);

    // Set up touch handling
    m_touchHandler = QuteNote::makeOwned<FileBrowserTouchHandler>(this);

    // Setup UI and load data
    setupUI();
    loadRecentFiles();
    setupConnections();

    // Move buffer timer for batching filesystem operations. Use a short delay
    // so rapid drag/drops are coalesced; flush will apply filesystem changes
    // and then refresh the view.
    m_moveBufferTimer = QuteNote::makeOwned<QTimer>(this);
    m_moveBufferTimer->setSingleShot(true);
    m_moveBufferTimer->setInterval(250); // ms
    connect(m_moveBufferTimer.get(), &QTimer::timeout, this, &FileBrowser::processMoveBuffer);

    // Tree widget is now styled by the global application stylesheet
}

void FileBrowser::navigateBack()
{
    // TODO: Implement folder navigation history
    QDir currentDir(m_currentDirectory);
    if (currentDir.cdUp()) {
        setRootDirectory(currentDir.absolutePath());
    }
}

void FileBrowser::navigateForward()
{
    // TODO: Implement folder navigation history
    if (m_treeWidget->selectedItems().count() == 1) {
        QTreeWidgetItem* item = m_treeWidget->selectedItems().first();
        QString path = item->data(0, Qt::UserRole).toString();
        if (QFileInfo(path).isDir()) {
            setRootDirectory(path);
        }
    }
    
    // Set up the recent files section
    QTreeWidgetItem* recentFilesRoot = createRecentFilesSection();
    m_recentFilesRoot.reset(recentFilesRoot);
    m_treeWidget->insertTopLevelItem(0, m_recentFilesRoot.get());
    m_recentFilesRoot->setExpanded(true);
}

FileBrowser::~FileBrowser()
{
    cleanupResources();
}

void FileBrowser::setupConnections()
{
    // Connect touch handler signals
    if (m_touchHandler) {
        connect(m_touchHandler.get(), &FileBrowserTouchHandler::overscrollAmountChanged,
                this, &FileBrowser::setOverscrollAmount);
        connect(m_touchHandler.get(), &FileBrowserTouchHandler::itemTapped,
                this, [this](QTreeWidgetItem* item) {
                    if (item) {
                        onItemDoubleClicked(item, 0);
                    }
                });
    }
            
    // Show directory changes in status bar
    connect(this, &FileBrowser::directoryChanged, this, [this](const QString &path) {
        QFileInfo info(path);
        const QString name = info.fileName().isEmpty() ? path : info.fileName();
        updateStatusBar(tr("Folder: %1").arg(name));
    });

    // Connect expansion/collapse once for lazy loading and icon updates
    if (m_treeWidget) {
        connect(m_treeWidget.get(), &QTreeWidget::itemExpanded, this,
                [this](QTreeWidgetItem *item) {
            QString path = item->data(0, Qt::UserRole).toString();
            if (!path.isEmpty() && item->childCount() == 1 &&
                item->child(0)->data(0, Qt::UserRole).toString().isEmpty()) {
                // Remove placeholder and load real children
                item->takeChild(0);
                loadDirectory(item, path);
            }
            // Update icon to expanded (minus) or empty folder icon if no children
            if (!path.isEmpty()) {
                if (item->childCount() > 0) {
                    item->setIcon(0, QIcon(":/resources/icons/custom/folder-minus.svg"));
                } else {
                    item->setIcon(0, QIcon(":/resources/icons/custom/folder.svg"));
                }
                // Track expanded directory
                QFileInfo info(path);
                if (info.isDir()) {
                    m_expandedDirs.insert(info.absoluteFilePath());
                }
            }
        });

        connect(m_treeWidget.get(), &QTreeWidget::itemCollapsed, this,
                [this](QTreeWidgetItem *item) {
            const QString path = item->data(0, Qt::UserRole).toString();
            if (path.isEmpty()) return;
            // On collapse, show contracted icon unless it's empty
            if (item->childCount() > 0) {
                item->setIcon(0, QIcon(":/resources/icons/custom/folder-plus.svg"));
            } else {
                item->setIcon(0, QIcon(":/resources/icons/custom/folder.svg"));
            }
            QFileInfo info(path);
            if (info.isDir()) {
                m_expandedDirs.remove(info.absoluteFilePath());
            }
        });
        connect(m_treeWidget.get(), &FileBrowserTreeWidget::itemOrderChanged,
                this, &FileBrowser::onItemOrderChanged);
    }

}

void FileBrowser::cleanupResources()
{
    // Save recent files before cleanup
    saveRecentFiles();

    // Clear tree widget to free memory
    if (m_treeWidget) {
        m_treeWidget->clear();
    }

    // Clear loaded paths cache
    m_loadedPaths.clear();

    // Reset navigation state
    m_currentDirectory = m_rootDirectory;

    // Clean up touch handler
    m_touchHandler = nullptr;
}

void FileBrowser::handleMemoryWarning()
{
    // Clear loaded paths cache to reduce memory usage
    m_loadedPaths.clear();

    // Collapse all items except current path
    if (m_treeWidget) {
        QTreeWidgetItemIterator it(m_treeWidget.get());
        while (*it) {
            if (!*it) break; // Safety check
            QString path = (*it)->data(0, Qt::UserRole).toString();
            if (!m_currentDirectory.startsWith(path)) {
                (*it)->setExpanded(false);
            }
            ++it;
        }
    }

    // Clear recent files if needed
    if (m_recentFiles.size() > MAX_RECENT_FILES / 2) {
        while (m_recentFiles.size() > MAX_RECENT_FILES / 2) {
            if (m_recentFiles.empty()) break; // Safety check
            m_recentFiles.remove(*m_recentFiles.begin());
        }
        saveRecentFiles();
    }
}

void FileBrowser::setupUI()
{
    if (!m_treeWidget) return;
    
    // Configure tree widget for touch-friendly use
    // Hide the header (remove "Notes" header)
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Ensure tree widget expands to fill available space
    m_treeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Create actions
    m_createFolderAction = QuteNote::makeOwned<QAction>(tr("New Folder"), this);
    m_createFolderAction->setIcon(QIcon(":/resources/icons/custom/new-file.svg"));
        // Use custom delegate for divider items
        m_treeWidget->setItemDelegate(new FileBrowserDividerDelegate(m_treeWidget.get()));
    m_createNoteAction = QuteNote::makeOwned<QAction>(tr("New Note"), this);
    m_createNoteAction->setIcon(QIcon(":/resources/icons/custom/file-plus-2.svg"));
    m_createDividerAction = QuteNote::makeOwned<QAction>(tr("Add Divider"), this);
    m_removeAction = QuteNote::makeOwned<QAction>(tr("Remove"), this);
    m_removeAction->setIcon(QIcon(":/resources/icons/custom/close.svg"));
    m_renameAction = QuteNote::makeOwned<QAction>(tr("Rename"), this);

    // Create breadcrumb container
    m_breadcrumbContainer = QuteNote::makeOwned<QWidget>(this);
    m_breadcrumbContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_breadcrumbContainer->setMaximumHeight(30); // Limit height to prevent taking too much space
    m_breadcrumbLayout = QuteNote::makeOwned<QHBoxLayout>(m_breadcrumbContainer.get());
    m_breadcrumbLayout->setContentsMargins(0, 0, 0, 0);
    m_breadcrumbLayout->setSpacing(2);

    // Create context menu
    m_contextMenu = QuteNote::makeOwned<QMenu>(this);
    m_contextMenu->addAction(m_createFolderAction.get());
    m_contextMenu->addAction(m_createNoteAction.get());
    m_contextMenu->addAction(m_createDividerAction.get());
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_removeAction.get());
    m_contextMenu->addAction(m_renameAction.get());


    // Create a fixed top button bar (toolbar) above the breadcrumbs and tree widget
    m_buttonBar = QuteNote::makeOwned<QWidget>(this);
    // Match ThemeManager selector so the bar picks up themed background/colors
    m_buttonBar->setObjectName("FileBrowserButtonContainer");
    m_buttonBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_buttonBarLayout = QuteNote::makeOwned<QHBoxLayout>(m_buttonBar.get());
    m_buttonBarLayout->setContentsMargins(2, 0, 2, 0); // no vertical margin; keep small horizontal margin
    m_buttonBarLayout->setSpacing(4); // tighter spacing between buttons

    m_buttonBarLayout->addStretch();


    m_createFolderBtn = QuteNote::makeOwned<QPushButton>(m_buttonBar.get());
    m_createFolderBtn->setIcon(QIcon(":/resources/icons/custom/folder-plus-2.svg"));
    m_createFolderBtn->setToolTip(tr("New Folder"));
    // Narrow width (icon + padding), full height for touch
    m_createFolderBtn->setMinimumSize(42, 34);
    m_createFolderBtn->setObjectName("FileBrowserCreateFolderBtn");
    m_buttonBarLayout->addWidget(m_createFolderBtn.get());

    m_createNoteBtn = QuteNote::makeOwned<QPushButton>(m_buttonBar.get());
    m_createNoteBtn->setIcon(QIcon(":/resources/icons/custom/file.svg"));
    m_createNoteBtn->setToolTip(tr("New Note"));
    m_createNoteBtn->setMinimumSize(42, 34);
    m_createNoteBtn->setObjectName("FileBrowserCreateNoteBtn");
    m_buttonBarLayout->addWidget(m_createNoteBtn.get());

    m_createDividerBtn = QuteNote::makeOwned<QPushButton>(m_buttonBar.get());
    m_createDividerBtn->setIcon(QIcon(":/resources/icons/custom/file-minus-2.svg"));
    m_createDividerBtn->setToolTip(tr("Add Divider"));
    m_createDividerBtn->setMinimumSize(42, 34);
    m_createDividerBtn->setObjectName("FileBrowserCreateDividerBtn");
    m_buttonBarLayout->addWidget(m_createDividerBtn.get());

    m_removeBtn = QuteNote::makeOwned<QPushButton>(m_buttonBar.get());
    m_removeBtn->setIcon(QIcon(":/resources/icons/custom/trash.svg"));
    m_removeBtn->setToolTip(tr("Remove"));
    m_removeBtn->setMinimumSize(42, 34);
    m_removeBtn->setEnabled(false);
    m_removeBtn->setObjectName("FileBrowserRemoveBtn");
    m_buttonBarLayout->addWidget(m_removeBtn.get());

    m_buttonBarLayout->addStretch();

    // Connect to theme changes to update icon sizes and button heights
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](const Theme &theme) {
        const int iconSz = theme.metrics.iconSize;
        const int touchTarget = theme.metrics.touchTarget;
        // Buttons should be narrow (icon + padding) but full touchTarget height
        int btnWidth = iconSz + 18; // icon + padding
        m_createFolderBtn->setIconSize(QSize(iconSz, iconSz));
        m_createFolderBtn->setMinimumSize(btnWidth, touchTarget);
        m_createFolderBtn->setMaximumSize(btnWidth, touchTarget);
        m_createNoteBtn->setIconSize(QSize(iconSz, iconSz));
        m_createNoteBtn->setMinimumSize(btnWidth, touchTarget);
        m_createNoteBtn->setMaximumSize(btnWidth, touchTarget);
        m_createDividerBtn->setIconSize(QSize(iconSz, iconSz));
        m_createDividerBtn->setMinimumSize(btnWidth, touchTarget);
        m_createDividerBtn->setMaximumSize(btnWidth, touchTarget);
        m_removeBtn->setIconSize(QSize(iconSz, iconSz));
        m_removeBtn->setMinimumSize(btnWidth, touchTarget);
        m_removeBtn->setMaximumSize(btnWidth, touchTarget);
        
        // Update button bar height
        const int padding = 8;
        m_buttonBar->setFixedHeight(touchTarget + padding);
    });
    
    // Apply initial theme icon sizes and button heights
    Theme currentTheme = ThemeManager::instance()->currentTheme();
    const int iconSz = currentTheme.metrics.iconSize;
    const int touchTarget = currentTheme.metrics.touchTarget;
    // Buttons should be narrow (icon + padding) but full touchTarget height
    int btnWidth = iconSz + 18; // icon + padding
    m_createFolderBtn->setIconSize(QSize(iconSz, iconSz));
    m_createFolderBtn->setMinimumSize(btnWidth, touchTarget);
    m_createFolderBtn->setMaximumSize(btnWidth, touchTarget);
    m_createNoteBtn->setIconSize(QSize(iconSz, iconSz));
    m_createNoteBtn->setMinimumSize(btnWidth, touchTarget);
    m_createNoteBtn->setMaximumSize(btnWidth, touchTarget);
    m_createDividerBtn->setIconSize(QSize(iconSz, iconSz));
    m_createDividerBtn->setMinimumSize(btnWidth, touchTarget);
    m_createDividerBtn->setMaximumSize(btnWidth, touchTarget);
    m_removeBtn->setIconSize(QSize(iconSz, iconSz));
    m_removeBtn->setMinimumSize(btnWidth, touchTarget);
    m_removeBtn->setMaximumSize(btnWidth, touchTarget);
    
    // Set initial button bar height
    const int padding = 8;
    m_buttonBar->setFixedHeight(touchTarget + padding);

    // Ensure toolbar buttons don't stay visually pressed after interaction
    auto configureButton = [this](QPushButton* btn) {
        if (!btn) return;
        btn->setCheckable(false);
        btn->setChecked(false);
        btn->setAutoDefault(false);
        btn->setDefault(false);
        btn->setFocusPolicy(Qt::NoFocus);
        // Clear pressed state after click and repaint
        connect(btn, &QPushButton::clicked, this, [btn]() {
            btn->setDown(false);
            btn->clearFocus();
            btn->update();
        });
        // Avoid filtering MouseButtonRelease/TouchEnd on buttons to not interfere with clicked()
    };
    configureButton(m_createFolderBtn.get());
    configureButton(m_createNoteBtn.get());
    configureButton(m_createDividerBtn.get());
    configureButton(m_removeBtn.get());

    // Style tree and bar for a single bezelled look
    m_treeWidget->setFrameShape(QFrame::NoFrame);
    // m_treeWidget->setStyleSheet(
    //     "QTreeWidget {"
    //     "  background-color: palette(base);"
    //     "  border: 2px solid palette(highlight);"
    //     "  border-top-left-radius: 8px;"
    //     "  border-top-right-radius: 8px;"
    //     "  border-bottom-left-radius: 8px;"
    //     "  border-bottom-right-radius: 8px;"
    //     "}"
    // );

    // Let ThemeManager drive background and button styles via applyThemeToFileBrowser()
    // Do not set an empty widget-level stylesheet here — that prevents
    // the application-level stylesheet from being inherited by child widgets.

    // Align the top bar height with the TextEditor toolbar height; add ~2px to better match
    m_buttonBar->setFixedHeight(58);

    // Add top bar to main layout before breadcrumbs and tree
    m_layout->insertWidget(0, m_buttonBar.get());
    m_layout->insertWidget(1, m_breadcrumbContainer.get());
    m_layout->insertWidget(2, m_treeWidget.get(), /*stretch=*/1);

    // Explicitly set stretch by index for clarity in Qt 6
    // 0: button bar, 1: breadcrumbs, 2: tree
    m_layout->setStretch(0, 0);
    m_layout->setStretch(1, 0);
    m_layout->setStretch(2, 1);
    
    // Tree widget will be styled by ThemeManager

    // Remove margins from the FileBrowser; keep tight spacing
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    // Ensure the layout is installed (redundant if constructed with parent, but harmless)
    if (this->layout() != m_layout.get()) {
        this->setLayout(m_layout.get());
    }

    // Install event filter on viewport to address drag-leave artifacts
    m_treeWidget->viewport()->installEventFilter(this);

    // Basic signal connections only
    connect(m_treeWidget.get(), &QTreeWidget::itemClicked,
            this, &FileBrowser::onItemClicked);
    connect(m_treeWidget.get(), &QTreeWidget::itemDoubleClicked,
            this, &FileBrowser::onItemDoubleClicked);
    connect(m_treeWidget.get(), &QTreeWidget::itemSelectionChanged,
            this, &FileBrowser::onItemSelectionChanged);
    connect(m_treeWidget.get(), &QTreeWidget::customContextMenuRequested,
            this, &FileBrowser::onContextMenuRequested);
    
    // Connect toolbar buttons
    connect(m_createFolderBtn.get(), &QPushButton::clicked,
            this, &FileBrowser::onCreateFolder);
    connect(m_createNoteBtn.get(), &QPushButton::clicked,
            this, &FileBrowser::onCreateNote);
    connect(m_createDividerBtn.get(), &QPushButton::clicked,
            this, &FileBrowser::onCreateDivider);
    connect(m_removeBtn.get(), &QPushButton::clicked,
            this, &FileBrowser::onRemoveItem);
    
    // Initialize button states
    if (m_removeBtn) {
        m_removeBtn->setEnabled(false);
    }
}

void FileBrowser::showEvent(QShowEvent *event)
{
    QuteNote::ComponentBase::showEvent(event);
}

void FileBrowser::setupContextMenu()
{
    m_contextMenu.reset(new QMenu(QWidget::parentWidget()));
    UIUtils::makeTouchFriendly(m_contextMenu.get());
    
    // Menu will be styled by ThemeManager theme

    // Create actions with icons
    m_createFolderAction.reset(m_contextMenu->addAction(QIcon::fromTheme("folder-new"), "New Folder"));
    m_createNoteAction.reset(m_contextMenu->addAction(QIcon::fromTheme("document-new"), "New Note"));
    m_createDividerAction.reset(m_contextMenu->addAction(QIcon::fromTheme("view-more"), "New Divider"));
    m_renameAction.reset(m_contextMenu->addAction(QIcon::fromTheme("edit-rename"), "Rename"));
    m_contextMenu->addSeparator();
    m_removeAction.reset(m_contextMenu->addAction(QIcon::fromTheme("edit-delete"), "Remove"));

    // Connect signals
    connect(m_createFolderAction.get(), &QAction::triggered,
            this, &FileBrowser::onCreateFolder);
    connect(m_createNoteAction.get(), &QAction::triggered,
            this, &FileBrowser::onCreateNote);
    connect(m_createDividerAction.get(), &QAction::triggered,
            this, &FileBrowser::onCreateDivider);
    connect(m_renameAction.get(), &QAction::triggered,
            this, &FileBrowser::onRename);
    connect(m_removeAction.get(), &QAction::triggered,
            this, &FileBrowser::onRemoveItem);
}

void FileBrowser::populateTree()
{
    if (!m_treeWidget) return;

    // Remember expanded directories before rebuilding
    captureExpandedPaths();

    m_treeWidget->clear();

    // Populate current root directory entries directly as top-level items (no explicit root item)
    const QList<QFileInfo> entries = orderedEntriesForDirectory(m_rootDirectory);
    for (const QFileInfo &entry : entries) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_treeWidget.get());
        item->setData(0, Qt::UserRole, entry.absoluteFilePath());
        if (entry.isDir()) {
            item->setText(0, displayNameForEntry(entry));
            QDir d(entry.absoluteFilePath());
            const bool empty = d.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot).isEmpty();
            if (empty) {
                item->setIcon(0, QIcon(":/resources/icons/custom/folder.svg"));
            } else {
                item->setIcon(0, QIcon(":/resources/icons/custom/folder-plus.svg"));
            }
            if (m_lazyLoading) {
                item->addChild(new QTreeWidgetItem()); // placeholder for expansion
            }
        } else {
            // Custom display for divider files
            if (entry.suffix().compare("divider", Qt::CaseInsensitive) == 0) {
                const QString base = entry.completeBaseName();
                item->setText(0, base); // Just the title, delegate will handle drawing
                // No icon for dividers, delegate draws the line
            } else {
                item->setText(0, displayNameForEntry(entry));
                // Use custom file icon for all other files
                QIcon fileIcon(":/resources/icons/custom/file.svg");
                if (fileIcon.isNull()) {
                    fileIcon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
                }
                item->setIcon(0, fileIcon);
            }
        }
    }

    // Restore previously expanded directories
    restoreExpandedPaths();

    // Ensure something is selected by default (first item)
    if (m_treeWidget->topLevelItemCount() > 0) {
        m_treeWidget->setCurrentItem(m_treeWidget->topLevelItem(0));
    }
}

void FileBrowser::setRootDirectory(const QString &path)
{
    m_rootDirectory = path;
    m_currentDirectory = path;
    
    // Update the tree widget's root directory for proper drop handling
    if (m_treeWidget) {
        m_treeWidget->setRootDirectory(path);
    }
    
    populateTree();
    emit directoryChanged(m_currentDirectory);
}

void FileBrowser::captureExpandedPaths()
{
    m_expandedDirs.clear();
    if (!m_treeWidget) return;

    const int topCount = m_treeWidget->topLevelItemCount();
    for (int i = 0; i < topCount; ++i) {
        QTreeWidgetItem *top = m_treeWidget->topLevelItem(i);
        if (!top) continue;
        QString path = top->data(0, Qt::UserRole).toString();
        if (top->isExpanded() && !path.isEmpty()) {
            QFileInfo info(path);
            if (info.isDir()) m_expandedDirs.insert(info.absoluteFilePath());
        }
        // Walk children (limited depth to avoid heavy traversal)
        QList<QTreeWidgetItem*> stack;
        stack.append(top);
        while (!stack.isEmpty()) {
            QTreeWidgetItem *it = stack.takeLast();
            const int cc = it->childCount();
            for (int c = 0; c < cc; ++c) {
                QTreeWidgetItem *child = it->child(c);
                if (!child) continue;
                QString cpath = child->data(0, Qt::UserRole).toString();
                if (child->isExpanded() && !cpath.isEmpty()) {
                    QFileInfo cinfo(cpath);
                    if (cinfo.isDir()) m_expandedDirs.insert(cinfo.absoluteFilePath());
                }
                stack.append(child);
            }
        }
    }
}

bool FileBrowser::expandPath(const QString &absPath, bool selectEnd)
{
    if (!m_treeWidget) return false;
    if (absPath.isEmpty()) return false;

    QFileInfo targetInfo(absPath);
    const QString dirPath = targetInfo.isDir() ? targetInfo.absoluteFilePath()
                                               : targetInfo.absolutePath();

    // Special case: item lives directly under root directory
    if (QDir::cleanPath(dirPath) == QDir::cleanPath(m_rootDirectory)) {
        // For files, select the top-level file item; for directories, select the dir item
        const int topCount = m_treeWidget->topLevelItemCount();
        for (int i = 0; i < topCount; ++i) {
            QTreeWidgetItem *it = m_treeWidget->topLevelItem(i);
            if (!it) continue;
            const QString ipath = it->data(0, Qt::UserRole).toString();
            if (ipath == absPath || ipath == dirPath) {
                if (targetInfo.isDir()) it->setExpanded(true);
                if (selectEnd) {
                    m_treeWidget->setCurrentItem(it);
                    m_treeWidget->scrollToItem(it);
                }
                return true;
            }
        }
        return false;
    }

    // Compute relative path from root to the directory containing the target
    QString rel = QDir(m_rootDirectory).relativeFilePath(dirPath);
    QString topComponent = rel.section('/', 0, 0);
    QTreeWidgetItem *current = nullptr;

    // Find top-level item
    const int count = m_treeWidget->topLevelItemCount();
    for (int i = 0; i < count; ++i) {
        QTreeWidgetItem *it = m_treeWidget->topLevelItem(i);
        if (!it) continue;
        if (it->data(0, Qt::UserRole).toString() == QDir(m_rootDirectory).absoluteFilePath(topComponent)) {
            current = it;
            break;
        }
    }
    if (!current) return false;

    // Expand through the remaining components
    QStringList components = rel.split('/', Qt::SkipEmptyParts);
    for (int idx = 0; idx < components.size(); ++idx) {
        if (idx == 0) {
            // already at topComponent
        } else {
            // Ensure children are loaded
            if (current->childCount() == 1 && current->child(0)->data(0, Qt::UserRole).toString().isEmpty()) {
                // Placeholder present, load children
                QString cpath = current->data(0, Qt::UserRole).toString();
                current->takeChild(0);
                loadDirectory(current, cpath);
            }
            // Find next component under current
            const QString nextName = components.at(idx);
            QTreeWidgetItem *nextItem = nullptr;
            const int cc = current->childCount();
            for (int c = 0; c < cc; ++c) {
                QTreeWidgetItem *child = current->child(c);
                if (!child) continue;
                QFileInfo chInfo(child->data(0, Qt::UserRole).toString());
                if (chInfo.fileName() == nextName) { nextItem = child; break; }
            }
            if (!nextItem) break; // path component not found
            current = nextItem;
        }
        current->setExpanded(true);
    }

    // Optionally select the end target
    if (selectEnd) {
        QString selectPath = targetInfo.isDir() ? targetInfo.absoluteFilePath()
                                               : targetInfo.absoluteFilePath();
        // If selecting a file, find it under its parent dir
        if (!targetInfo.isDir()) {
            // Ensure children loaded
            if (current->childCount() == 1 && current->child(0)->data(0, Qt::UserRole).toString().isEmpty()) {
                QString cpath = current->data(0, Qt::UserRole).toString();
                current->takeChild(0);
                loadDirectory(current, cpath);
            }
            const int cc = current->childCount();
            for (int c = 0; c < cc; ++c) {
                QTreeWidgetItem *child = current->child(c);
                if (!child) continue;
                if (child->data(0, Qt::UserRole).toString() == selectPath) {
                    m_treeWidget->setCurrentItem(child);
                    m_treeWidget->scrollToItem(child);
                    return true;
                }
            }
        } else {
            m_treeWidget->setCurrentItem(current);
            m_treeWidget->scrollToItem(current);
            return true;
        }
    }
    return true;
}

void FileBrowser::restoreExpandedPaths()
{
    if (m_expandedDirs.isEmpty() || !m_treeWidget) return;
    // Expand each remembered directory path
    for (const QString &path : std::as_const(m_expandedDirs)) {
        expandPath(path, false);
    }
}


void FileBrowser::addDirectoryToTree(QTreeWidgetItem *parentItem, const QDir &directory)
{
    const QList<QFileInfo> entries = orderedEntriesForDirectory(directory.absolutePath());

    for (const QFileInfo &entry : entries) {
        QTreeWidgetItem *item = addFileItem(parentItem, entry);
        if (entry.isDir()) {
            QDir subDir(entry.absoluteFilePath());
            addDirectoryToTree(item, subDir);
        }
    }
}

QString FileBrowser::stripOrderingPrefix(const QString &text) const
{
    static const QRegularExpression prefixRx("^\\d{3}_(.+)$");
    const QRegularExpressionMatch match = prefixRx.match(text);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return text;
}

QString FileBrowser::displayNameForEntry(const QFileInfo &info) const
{
    return stripOrderingPrefix(info.fileName());
}

QString FileBrowser::orderingMetadataPath(const QString &directoryPath) const
{
    const QString baseDir = directoryPath.isEmpty() ? m_rootDirectory : directoryPath;
    return QDir(baseDir).filePath(QString::fromLatin1(kOrderingFileName));
}

QStringList FileBrowser::loadOrderingMetadata(const QString &directoryPath) const
{
    const QString normalizedDir = QDir::cleanPath(directoryPath.isEmpty() ? m_rootDirectory : directoryPath);
    if (m_cachedOrdering.contains(normalizedDir)) {
        return m_cachedOrdering.value(normalizedDir);
    }

    QStringList names;
    QFile file(orderingMetadataPath(normalizedDir));
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                const QString line = stream.readLine().trimmed();
                if (line.startsWith(QStringLiteral("- "))) {
                    const QString name = line.mid(2).trimmed();
                    if (!name.isEmpty()) {
                        names << name;
                    }
                } else if (line.startsWith(QStringLiteral("* "))) {
                    const QString name = line.mid(2).trimmed();
                    if (!name.isEmpty()) {
                        names << name;
                    }
                }
            }
        } else {
            qWarning() << "Failed to read ordering metadata for" << normalizedDir << file.errorString();
        }
    }

    m_cachedOrdering.insert(normalizedDir, names);
    return names;
}

void FileBrowser::saveOrderingMetadata(const QString &directoryPath, const QStringList &orderedNames) const
{
    const QString normalizedDir = QDir::cleanPath(directoryPath.isEmpty() ? m_rootDirectory : directoryPath);
    const QString metadataPath = orderingMetadataPath(normalizedDir);

    QDir dir(normalizedDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QSaveFile file(metadataPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Unable to save ordering metadata for" << normalizedDir << file.errorString();
        return;
    }

    QTextStream stream(&file);
    stream << "<!-- QuteNote ordering metadata -->\n";
    for (const QString &name : orderedNames) {
        stream << "- " << name << '\n';
    }

    if (!file.commit()) {
        qWarning() << "Failed to commit ordering metadata for" << normalizedDir;
    }

    m_cachedOrdering.insert(normalizedDir, orderedNames);
}

QStringList FileBrowser::ensureOrderingMetadata(const QString &directoryPath, const QFileInfoList &entries)
{
    const QString normalizedDir = QDir::cleanPath(directoryPath.isEmpty() ? m_rootDirectory : directoryPath);
    QStringList actualNames;
    actualNames.reserve(entries.size());
    for (const QFileInfo &info : entries) {
        const QString fileName = info.fileName();
        if (fileName == QString::fromLatin1(kOrderingFileName)) {
            continue;
        }
        actualNames << fileName;
    }

    QStringList existingOrder = loadOrderingMetadata(normalizedDir);
    QStringList finalOrder;
    finalOrder.reserve(actualNames.size());
    QSet<QString> seen;

    for (const QString &name : existingOrder) {
        if (actualNames.contains(name) && !seen.contains(name)) {
            finalOrder << name;
            seen.insert(name);
        }
    }

    QStringList leftovers;
    for (const QString &name : actualNames) {
        if (!seen.contains(name)) {
            leftovers << name;
        }
    }

    std::sort(leftovers.begin(), leftovers.end(), [](const QString &a, const QString &b) {
        return a.localeAwareCompare(b) < 0;
    });

    finalOrder.append(leftovers);

    if (finalOrder != existingOrder) {
        saveOrderingMetadata(normalizedDir, finalOrder);
    }

    return finalOrder;
}

QFileInfo FileBrowser::maybeMigratePrefixedEntry(const QFileInfo &info)
{
    static const QRegularExpression prefixRx(QStringLiteral("^(\\d{3})_(.+)$"));
    const QRegularExpressionMatch match = prefixRx.match(info.fileName());
    if (!match.hasMatch()) {
        return info;
    }

    const QString sanitizedName = match.captured(2);
    const QString baseDir = info.absolutePath();
    const QString targetPath = QDir(baseDir).filePath(sanitizedName);

    if (QFileInfo::exists(targetPath)) {
        qWarning() << "Skipping prefix migration, target exists" << targetPath;
        return info;
    }

    bool ok = false;
    if (info.isDir()) {
        QDir dir;
        ok = dir.rename(info.absoluteFilePath(), targetPath);
    } else {
        ok = QFile::rename(info.absoluteFilePath(), targetPath);
    }

    if (!ok) {
        qWarning() << "Failed to migrate prefixed entry" << info.absoluteFilePath();
        return info;
    }

    const QFileInfo migratedInfo(targetPath);
    renameEntryInOrdering(baseDir, info.fileName(), migratedInfo.fileName());
    emit fileRenamed(info.absoluteFilePath(), targetPath);
    return migratedInfo;
}

QFileInfoList FileBrowser::listDirectoryEntries(const QString &directoryPath)
{
    QDir dir(directoryPath);
    const QFileInfoList rawEntries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                                       QDir::Name | QDir::DirsFirst);

    QFileInfoList cleaned;
    cleaned.reserve(rawEntries.size());
    for (const QFileInfo &entry : rawEntries) {
        if (entry.fileName() == QString::fromLatin1(kOrderingFileName)) {
            continue;
        }
        cleaned << maybeMigratePrefixedEntry(entry);
    }
    return cleaned;
}

QList<QFileInfo> FileBrowser::orderedEntriesForDirectory(const QString &directoryPath)
{
    const QFileInfoList entries = listDirectoryEntries(directoryPath);
    const QStringList order = ensureOrderingMetadata(directoryPath, entries);

    QHash<QString, QFileInfo> infoByName;
    for (const QFileInfo &info : entries) {
        infoByName.insert(info.fileName(), info);
    }

    QList<QFileInfo> ordered;
    ordered.reserve(order.size());
    for (const QString &name : order) {
        if (infoByName.contains(name)) {
            ordered << infoByName.value(name);
        }
    }
    return ordered;
}

void FileBrowser::recordOrderingFromTree(const QString &directoryPath)
{
    if (!m_treeWidget) {
        return;
    }

    const QString normalizedDir = QDir::cleanPath(directoryPath.isEmpty() ? m_rootDirectory : directoryPath);
    QStringList orderedNames;

    auto gather = [&](QTreeWidgetItem *parent) {
        const int count = parent ? parent->childCount() : m_treeWidget->topLevelItemCount();
        for (int i = 0; i < count; ++i) {
            QTreeWidgetItem *item = parent ? parent->child(i) : m_treeWidget->topLevelItem(i);
            if (!item) continue;
            if (!parent && item == m_recentFilesRoot.get()) {
                continue;
            }
            const QString path = item->data(0, Qt::UserRole).toString();
            if (path.isEmpty()) {
                continue;
            }
            const QFileInfo info(path);
            if (info.fileName() == QString::fromLatin1(kOrderingFileName)) {
                continue;
            }
            if (QDir::cleanPath(info.absolutePath()) == normalizedDir) {
                orderedNames << info.fileName();
            }
        }
    };

    if (QDir::cleanPath(normalizedDir) == QDir::cleanPath(m_rootDirectory)) {
        gather(nullptr);
    } else {
        QTreeWidgetItem *parentItem = findTreeItemForPath(normalizedDir);
        if (!parentItem) {
            return;
        }
        gather(parentItem);
    }

    if (orderedNames.isEmpty()) {
        return;
    }

    saveOrderingMetadata(normalizedDir, orderedNames);
    m_cachedOrdering.insert(normalizedDir, orderedNames);
}

void FileBrowser::upsertNameInOrdering(const QString &directoryPath, const QString &name, int index)
{
    if (name.isEmpty()) {
        return;
    }

    QStringList order = loadOrderingMetadata(directoryPath);
    if (order.isEmpty()) {
        const QFileInfoList entries = listDirectoryEntries(directoryPath);
        order = ensureOrderingMetadata(directoryPath, entries);
    }

    order.removeAll(name);
    if (index < 0 || index > order.size()) {
        order << name;
    } else {
        order.insert(index, name);
    }

    saveOrderingMetadata(directoryPath, order);
}

void FileBrowser::removeNameFromOrdering(const QString &directoryPath, const QString &name)
{
    if (name.isEmpty()) {
        return;
    }
    QStringList order = loadOrderingMetadata(directoryPath);
    if (order.isEmpty()) {
        return;
    }
    const int removed = order.removeAll(name);
    if (removed > 0) {
        saveOrderingMetadata(directoryPath, order);
    }
}

void FileBrowser::renameEntryInOrdering(const QString &directoryPath, const QString &oldName, const QString &newName)
{
    if (oldName == newName || newName.isEmpty()) {
        return;
    }

    QStringList order = loadOrderingMetadata(directoryPath);
    if (order.isEmpty()) {
        return;
    }

    bool changed = false;
    for (QString &name : order) {
        if (name == oldName) {
            name = newName;
            changed = true;
        }
    }

    if (changed) {
        saveOrderingMetadata(directoryPath, order);
    }
}

QTreeWidgetItem* FileBrowser::findTreeItemForPath(const QString &path) const
{
    if (!m_treeWidget) {
        return nullptr;
    }
    const QString normalized = QDir::cleanPath(path);
    QTreeWidgetItemIterator it(m_treeWidget.get());
    while (*it) {
        QTreeWidgetItem *item = *it;
        const QString itemPath = QDir::cleanPath(item->data(0, Qt::UserRole).toString());
        if (!itemPath.isEmpty() && itemPath == normalized) {
            return item;
        }
        ++it;
    }
    return nullptr;
}

QTreeWidgetItem* FileBrowser::addFileItem(QTreeWidgetItem *parentItem, const QFileInfo &fileInfo)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parentItem);
    item->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());

    if (fileInfo.isDir()) {
        item->setText(0, displayNameForEntry(fileInfo));
        // Set initial icon based on whether the directory is empty
        QDir d(fileInfo.absoluteFilePath());
        const bool empty = d.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot).isEmpty();
        if (empty) {
            item->setIcon(0, QIcon(":/resources/icons/custom/folder.svg"));
        } else {
            item->setIcon(0, QIcon(":/resources/icons/custom/folder-plus.svg"));
        }
    } else {
        // Custom display for divider files
        if (fileInfo.suffix().compare("divider", Qt::CaseInsensitive) == 0) {
            const QString base = stripOrderingPrefix(fileInfo.completeBaseName());
            item->setText(0, base); // Just the title, delegate will handle drawing
            // No icon for dividers, delegate draws the line
        } else {
            item->setText(0, displayNameForEntry(fileInfo));
            QIcon fileIcon(":/resources/icons/custom/file.svg");
            if (fileIcon.isNull()) {
                fileIcon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
            }
            item->setIcon(0, fileIcon);
            }
        }
    return item;
}

void FileBrowser::onItemClicked(QTreeWidgetItem *item, int /*column*/)
{
    if (!m_treeWidget || !item) {
        return;
    }

    if (m_treeWidget->currentItem() != item) {
        m_treeWidget->setCurrentItem(item);
    }

    // Handle recent files section
    if (item->parent() == m_recentFilesRoot.get()) {
        QString filePath = item->data(0, Qt::UserRole).toString();
        if (filePath.isEmpty()) {
            filePath = item->toolTip(0);
        }

        QFileInfo fileInfo(filePath);
        const QString resolvedPath = fileInfo.absoluteFilePath();

        if (resolvedPath.isEmpty()) {
            updateStatusBar(tr("Unable to open recent item: missing file path"), 4000);
            return;
        }

        if (!fileInfo.exists()) {
            // Drop the stale entry and persist the change
            m_recentFiles.remove(RecentFile{resolvedPath, QDateTime()});
            saveRecentFiles();

            if (auto *parent = item->parent()) {
                parent->removeChild(item);
            }
            delete item;

            const QFileInfo staleInfo(resolvedPath);
            updateStatusBar(tr("File no longer exists: %1").arg(displayNameForEntry(staleInfo)), 4000);
            return;
        }

        emit fileSelected(resolvedPath);
        updateRecentFiles(resolvedPath);
        updateButtonStates();
        return;
    }

    const QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) {
        updateButtonStates();
        return; // Virtual items like "Recent Files" header - ignore
    }

    QFileInfo info(path);

    if (info.isDir()) {
        // Folder - toggle expansion and treat as current directory
        if (item->isExpanded()) {
            m_treeWidget->collapseItem(item);
        } else {
            m_treeWidget->expandItem(item);
        }
        m_currentDirectory = info.absoluteFilePath();
        emit directoryChanged(m_currentDirectory);
    } else {
        emit fileSelected(path);
        updateRecentFiles(path);
    }

    updateButtonStates();
}

void FileBrowser::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    onItemClicked(item, column);
}

void FileBrowser::onItemSelectionChanged()
{
    // Update button states based on selection
    if (!m_treeWidget) return;
    
    bool hasSelection = !m_treeWidget->selectedItems().isEmpty();
    
    // Enable/disable remove button based on selection
    if (m_removeBtn) {
        m_removeBtn->setEnabled(hasSelection);
    }
    
    // Enable/disable remove action as well
    if (m_removeAction) {
        m_removeAction->setEnabled(hasSelection);
    }
}

void FileBrowser::onContextMenuRequested(const QPoint &pos)
{
    if (!m_treeWidget) return;
    
    // Simplified - no context menu for Android compatibility
    QTreeWidgetItem *item = m_treeWidget->itemAt(pos);
    if (item) {
        // Just select the item without showing context menu
        m_treeWidget->setCurrentItem(item);
    }
}

void FileBrowser::onCreateFolder()
{
    // Reset button state after dialog interaction
    auto resetButtonState = [this]() {
        if (m_createFolderBtn) {
            m_createFolderBtn->setDown(false);
            m_createFolderBtn->clearFocus();
            m_createFolderBtn->update();
        }
    };
    
    // Use existing prompt UI, but create inside the current directory context
    bool ok = false;
    QString folderName = QInputDialog::getText(this, tr("New Folder"),
                                               tr("Folder name:"), QLineEdit::Normal,
                                               QString(), &ok);

    // Reset button state after dialog closes
    QTimer::singleShot(0, this, resetButtonState);
    // Force repaint to clear any visual artifacts left by the dialog (esp. when closed via Enter)
    QTimer::singleShot(0, this, &FileBrowser::forceUiRefreshAfterDialog);

    if (!ok) {
        return; // cancelled
    }

    folderName = folderName.trimmed();
    if (folderName.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Name"), tr("Folder name can't be empty."));
        updateStatusBar(tr("Create folder cancelled: empty name"));
        return;
    }
    if (folderName.contains('/') || folderName.contains('\u005C')) { // disallow path separators
        QMessageBox::warning(this, tr("Invalid Name"), tr("Folder name can't contain '/' or '\\'."));
        updateStatusBar(tr("Create folder cancelled: invalid name"));
        return;
    }

    // Determine target directory: selected folder, selected file's parent, or root/current
    QString baseDir = !m_currentDirectory.isEmpty() ? m_currentDirectory : m_rootDirectory;
    if (m_treeWidget && m_treeWidget->currentItem()) {
        const QString selPath = m_treeWidget->currentItem()->data(0, Qt::UserRole).toString();
        if (!selPath.isEmpty()) {
            QFileInfo selInfo(selPath);
            baseDir = selInfo.isDir() ? selInfo.absoluteFilePath() : selInfo.absolutePath();
        }
    } else if (!m_currentDirectory.isEmpty()) {
        baseDir = m_currentDirectory;
    }

    QDir dir(baseDir);
    const QString folderPath = dir.filePath(folderName);

    if (QFileInfo::exists(folderPath)) {
        QMessageBox::warning(this, tr("Already Exists"), tr("A file or folder with that name already exists."));
        updateStatusBar(tr("Folder already exists: %1").arg(folderName));
        return;
    }

    if (!dir.mkpath(folderPath)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not create folder: %1").arg(folderPath));
        updateStatusBar(tr("Create folder failed: %1").arg(folderName));
        return;
    }

    upsertNameInOrdering(baseDir, QFileInfo(folderPath).fileName());

    updateStatusBar(tr("Folder created: %1").arg(folderName));

    // Try to reflect the new folder in the visible tree without a full reload
    if (m_treeWidget) {
        // Find the parent item representing baseDir
        QTreeWidgetItem* parentItem = nullptr;
        for (QTreeWidgetItemIterator it(m_treeWidget.get()); *it; ++it) {
            QTreeWidgetItem* candidate = *it;
            const QString candidatePath = candidate->data(0, Qt::UserRole).toString();
            if (!candidatePath.isEmpty() && QFileInfo(candidatePath).absoluteFilePath() == QFileInfo(baseDir).absoluteFilePath()) {
                parentItem = candidate;
                break;
            }
        }
        if (parentItem) {
            QFileInfo newInfo(folderPath);
            QTreeWidgetItem* newItem = addFileItem(parentItem, newInfo);
            if (m_lazyLoading) {
                newItem->addChild(new QTreeWidgetItem()); // placeholder for expansion
            }
            parentItem->setExpanded(true);
        }
    }

    // Refresh the tree to reflect the new folder
    populateTree();
    emit directoryChanged(baseDir);
    updateButtonStates();
}

void FileBrowser::onCreateNote()
{
    // Reset button state after dialog interaction
    auto resetButtonState = [this]() {
        if (m_createNoteBtn) {
            m_createNoteBtn->setDown(false);
            m_createNoteBtn->clearFocus();
            m_createNoteBtn->update();
        }
    };
    
    // Use existing prompt UI, but create inside the current directory context
    bool ok = false;
    QString noteName = QInputDialog::getText(this, tr("New Note"),
                                             tr("Note name:"), QLineEdit::Normal,
                                             QString(), &ok);

    // Reset button state after dialog closes
    QTimer::singleShot(0, this, resetButtonState);
    // Force repaint to clear any visual artifacts left by the dialog (esp. when closed via Enter)
    QTimer::singleShot(0, this, &FileBrowser::forceUiRefreshAfterDialog);

    if (!ok) {
        return; // cancelled
    }

    noteName = noteName.trimmed();
    if (noteName.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Name"), tr("Note name can't be empty."));
        updateStatusBar(tr("Create note cancelled: empty name"));
        return;
    }
    if (noteName.contains('/') || noteName.contains('\u005C')) { // disallow path separators
        QMessageBox::warning(this, tr("Invalid Name"), tr("Note name can't contain '/' or '\\'."));
        updateStatusBar(tr("Create note cancelled: invalid name"));
        return;
    }

    // Ensure .txt extension (keep simple behavior consistent with previous code)
    if (!noteName.endsWith(".txt", Qt::CaseInsensitive)) {
        noteName += ".txt";
    }

    // Determine target directory: selected folder, selected file's parent, or root/current
    QString baseDir = !m_currentDirectory.isEmpty() ? m_currentDirectory : m_rootDirectory;
    if (m_treeWidget && m_treeWidget->currentItem()) {
        const QString selPath = m_treeWidget->currentItem()->data(0, Qt::UserRole).toString();
        if (!selPath.isEmpty()) {
            QFileInfo selInfo(selPath);
            baseDir = selInfo.isDir() ? selInfo.absoluteFilePath() : selInfo.absolutePath();
        }
    } else if (!m_currentDirectory.isEmpty()) {
        baseDir = m_currentDirectory;
    }

    const QString notePath = QDir(baseDir).filePath(noteName);

    if (QFileInfo::exists(notePath)) {
        QMessageBox::warning(this, tr("Already Exists"), tr("A file with that name already exists."));
        updateStatusBar(tr("Note already exists: %1").arg(QFileInfo(notePath).fileName()));
        return;
    }

    QFile file(notePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not create note: %1").arg(file.errorString()));
        updateStatusBar(tr("Create note failed: %1").arg(QFileInfo(notePath).fileName()));
        return;
    }
    file.close();

    upsertNameInOrdering(baseDir, QFileInfo(notePath).fileName());

    updateStatusBar(tr("Note created: %1").arg(QFileInfo(notePath).fileName()));

    // Repopulate to ensure the new file is shown
    populateTree();
    emit fileCreated(notePath);
    emit directoryChanged(baseDir);
    updateButtonStates();
}

void FileBrowser::onCreateDivider()
{
    // Reset button state after dialog interaction
    auto resetButtonState = [this]() {
        if (m_createDividerBtn) {
            m_createDividerBtn->setDown(false);
            m_createDividerBtn->clearFocus();
            m_createDividerBtn->update();
        }
    };
    
    bool ok;
    QString dividerName = QInputDialog::getText(this, "New Divider",
                                                "Divider name:", QLineEdit::Normal,
                                                "", &ok);

    // Reset button state after dialog closes
    QTimer::singleShot(0, this, resetButtonState);

    if (ok && !dividerName.isEmpty()) {
        // Use current directory for divider as well
        const QString baseDir = !m_currentDirectory.isEmpty() ? m_currentDirectory : m_rootDirectory;
        QString dividerPath = QDir(baseDir).filePath(dividerName + ".divider");
        QFile file(dividerPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            upsertNameInOrdering(baseDir, QFileInfo(dividerPath).fileName());
            // Repopulate to show divider
            populateTree();
            updateStatusBar(tr("Divider created: %1").arg(dividerName));
        } else {
            QMessageBox::warning(this, "Error", "Could not create divider.");
            updateStatusBar(tr("Create divider failed: %1").arg(dividerName));
        }
    }
    
    // Update button states after creating divider
    updateButtonStates();
}

void FileBrowser::onRename()
{
    if (!m_treeWidget) return;
    
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (!currentItem) return;

    QString oldPath = currentItem->data(0, Qt::UserRole).toString();
    if (oldPath.isEmpty()) return; // Cannot rename virtual items like "Recent"

    QFileInfo info(oldPath);
    const QString currentName = info.fileName();

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename",
                                          "New name:", QLineEdit::Normal,
                                          currentName, &ok);
    // Force repaint to clear any visual artifacts left by the dialog (esp. when closed via Enter)
    QTimer::singleShot(0, this, &FileBrowser::forceUiRefreshAfterDialog);
    if (!ok || newName.isEmpty()) return;

    QString newPath = QDir(info.absolutePath()).filePath(newName);

    // Attempt rename
    bool success = false;
    if (info.isDir()) {
        QDir dir;
        success = dir.rename(oldPath, newPath);
    } else {
        success = QFile::rename(oldPath, newPath);
    }

    if (success) {
        currentItem->setText(0, newName);
        currentItem->setData(0, Qt::UserRole, newPath);
        renameEntryInOrdering(info.absolutePath(), info.fileName(), QFileInfo(newPath).fileName());
        // Repopulate to ensure consistency and resorting
        populateTree();
        updateStatusBar(tr("Renamed to: %1").arg(newName));
    } else {
        QMessageBox::warning(this, "Rename Failed", 
            QString("Could not rename %1").arg(info.isDir() ? "folder" : "file"));
        updateStatusBar(tr("Rename failed: %1").arg(currentItem->text(0)));
    }
}

void FileBrowser::onItemOrderChanged(const QString &sourcePath, const QString &oldParentPath, const QString &newParentPath, int newIndex)
{
    qWarning() << "onItemOrderChanged (buffered) called. Source:" << sourcePath
               << "Old Parent:" << oldParentPath << "New Parent:" << newParentPath << "Index:" << newIndex;

    if (sourcePath.isEmpty()) {
        qWarning() << "Empty sourcePath, ignoring.";
        return;
    }

    // Enqueue the move and start/reset the buffer timer. The timer will
    // call processMoveBuffer() which does the actual filesystem operations
    // and refreshes the view once all queued moves are applied.
    FileMove fm;
    fm.sourcePath = sourcePath;
    fm.oldParentPath = oldParentPath.isEmpty() ? m_rootDirectory : oldParentPath;
    fm.newParentPath = newParentPath.isEmpty() ? m_rootDirectory : newParentPath;
    fm.newIndex = newIndex;
    m_moveBuffer.append(fm);

    recordOrderingFromTree(fm.newParentPath);
    if (QDir::cleanPath(fm.oldParentPath) != QDir::cleanPath(fm.newParentPath)) {
        recordOrderingFromTree(fm.oldParentPath);
    }

    if (m_moveBufferTimer) {
        m_moveBufferTimer->start(); // restart coalescing timer
    }

    updateStatusBar(tr("Queued move: %1").arg(QFileInfo(sourcePath).fileName()), 2000);
}


void FileBrowser::processMoveBuffer()
{
    if (m_moveBuffer.isEmpty()) return;

    // Copy and clear buffer so new moves can be queued while processing
    const QList<FileMove> toProcess = m_moveBuffer;
    m_moveBuffer.clear();

    for (const FileMove &fm : toProcess) {
        const QString src = fm.sourcePath;
        if (src.isEmpty()) continue;

        QFileInfo sfi(src);
        const QString fileName = sfi.fileName();
        const QString dstParent = fm.newParentPath.isEmpty() ? m_rootDirectory : fm.newParentPath;
        QDir dstDir(dstParent);
        const QString dstPath = dstDir.filePath(fileName);

        const QString normalizedSrc = QDir::cleanPath(src);
        const QString normalizedDst = QDir::cleanPath(dstPath);

        // Pure reordering within the same directory: skip the physical move but
        // ensure metadata stays up to date.
        if (normalizedSrc == normalizedDst) {
            continue;
        }

        if (QFileInfo::exists(dstPath)) {
            updateStatusBar(tr("Move failed, target exists: %1").arg(fileName), 5000);
            qWarning() << "Move failed, target exists:" << dstPath;
            continue;
        }

        bool ok = false;
        if (sfi.isDir()) {
            QDir dir;
            ok = dir.rename(src, dstPath);
        } else {
            ok = QFile::rename(src, dstPath);
        }

        if (ok) {
            qWarning() << "Moved" << src << "->" << dstPath;
            emit fileRenamed(src, dstPath);
        } else {
            updateStatusBar(tr("Failed to move %1").arg(fileName), 5000);
            qWarning() << "Failed to move" << src << "->" << dstPath;
        }
    }

    // Refresh tree once after applying all changes
    populateTree();
    updateStatusBar(tr("Finished moving items."), 3000);
}

void FileBrowser::forceUiRefreshAfterDialog()
{
    // Aggressively refresh the file browser viewport and the window to clear stale paints
    if (m_treeWidget) {
        if (auto *vp = m_treeWidget->viewport()) {
            vp->update();
            vp->repaint();
        }
        m_treeWidget->update();
        m_treeWidget->repaint();
    }
    if (QWidget *w = window()) {
        w->update();
        w->repaint();
    }
    // Process pending events briefly to flush paint events
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
}

void FileBrowser::updateStatusBar(const QString &message, int timeoutMs)
{
    if (QWidget *win = window()) {
        if (auto *mw = qobject_cast<QMainWindow*>(win)) {
            if (mw->statusBar()) {
                mw->statusBar()->showMessage(message, timeoutMs);
            }
        }
    }
}

void FileBrowser::onRemoveItem()
{
    // Reset button state after dialog interaction
    auto resetButtonState = [this]() {
        if (m_removeBtn) {
            m_removeBtn->setDown(false);
            m_removeBtn->clearFocus();
            m_removeBtn->update();
        }
    };
    
    if (!m_treeWidget) return;
    
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (!currentItem) {
        return;
    }

    QString path = currentItem->data(0, Qt::UserRole).toString();
    QString itemName = currentItem->text(0); // Store item name before potential deletion
    QFileInfo info(path);

    QString itemType;
    QString message;
    if (info.suffix().compare("divider", Qt::CaseInsensitive) == 0) {
        itemType = "divider";
        message = QString("Are you sure you want to delete the divider '%1'?")
                         .arg(itemName);
    } else {
        itemType = info.isDir() ? "folder" : "file";
        message = QString("Are you sure you want to remove the %1 '%2'?")
                         .arg(itemType, itemName);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Remove", message);
    
    // Reset button state after dialog closes
    QTimer::singleShot(0, this, resetButtonState);
    
    if (reply != QMessageBox::Yes) {
        return;
    }

    updateStatusBar(tr("Removing %1: %2").arg(itemType, itemName));
    bool success = false;
    if (info.isDir()) {
        QDir dir(path);
        success = dir.removeRecursively();
    } else {
        success = QFile::remove(path);
    }

    if (success) {
        removeNameFromOrdering(info.absolutePath(), info.fileName());
        // Explicitly remove the item from the tree widget
        if (currentItem->parent()) {
            currentItem->parent()->removeChild(currentItem);
        } else {
            m_treeWidget->takeTopLevelItem(m_treeWidget->indexOfTopLevelItem(currentItem));
        }
        delete currentItem; // Delete the item after removing it from the tree

        // Repopulate to reflect removal
        populateTree();
        updateStatusBar(tr("Removed %1: %2").arg(itemType, itemName));
    } else {
        QMessageBox::warning(this, "Error", "Could not remove item.");
        updateStatusBar(tr("Remove failed: %1").arg(currentItem->text(0)));
    }
    
    // Update button states after removing item
    updateButtonStates();
}

void FileBrowser::updateButtonStates()
{
    // Update button states
    if (!m_treeWidget) return;
    
    bool hasSelection = !m_treeWidget->selectedItems().isEmpty();
    
    // Enable/disable remove button based on selection
    if (m_removeBtn) {
        m_removeBtn->setEnabled(hasSelection);
    }
    
    // Enable/disable remove action as well
    if (m_removeAction) {
        m_removeAction->setEnabled(hasSelection);
    }
    
    // Bottom bar buttons are always enabled
    if (m_createFolderBtn) {
        m_createFolderBtn->setEnabled(true);
    }
    if (m_createNoteBtn) {
        m_createNoteBtn->setEnabled(true);
    }
    if (m_createDividerBtn) {
        m_createDividerBtn->setEnabled(true);
    }
}

void FileBrowser::loadDirectory(QTreeWidgetItem *parentItem, const QString &path)
{
    if (!parentItem) return;
    
    const QList<QFileInfo> entries = orderedEntriesForDirectory(path);
    for (const QFileInfo &entry : entries) {
        QTreeWidgetItem *item = addFileItem(parentItem, entry);
        if (entry.isDir() && m_lazyLoading) {
            item->addChild(new QTreeWidgetItem());
        }
    }
    m_currentDirectory = path;
    emit directoryChanged(path);
}

QString FileBrowser::selectedFile() const
{
    if (!m_treeWidget) return QString();
    
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (currentItem) {
        QString path = currentItem->data(0, Qt::UserRole).toString();
        QFileInfo info(path);
        if (!info.isDir()) {
            return path;
        }
    }
    return QString();
}
