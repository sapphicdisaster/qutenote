#include "documentlist.h"
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>
#include <QContextMenuEvent>
#include <QMouseEvent>

DocumentList::DocumentList(QWidget *parent)
    : QTreeView(parent)
    , m_model(new DocumentModel(this))
    , m_fileWatcher(new FileWatcherGuard(this))
{
    // Setup model and view
    setModel(m_model);
    setHeaderHidden(true);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setIndentation(15);
    
    // Setup file watcher
    connect(m_fileWatcher, &FileWatcherGuard::fileChanged, 
            this, &DocumentList::onFileChanged);
    connect(m_fileWatcher, &FileWatcherGuard::watcherError,
            this, [this](const QString &path, const QString &error) {
        qWarning() << "File watcher error for" << path << ":" << error;
    });
    
    // Setup actions
    setupActions();
    
    // Connect signals
    connect(this, &DocumentList::clicked, this, &DocumentList::onItemClicked);
    
    // Set documents path
    m_documentsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(m_documentsPath);
}

DocumentList::~DocumentList()
{
    saveDocuments();
}

bool DocumentList::loadDocuments(const QString &filePath)
{
    // Stop watching previous file if any
    if (!m_fileWatcher->files().isEmpty()) {
        m_fileWatcher->removePaths(m_fileWatcher->files());
    }
    
    // Load new file
    if (m_model->loadFromFile(filePath)) {
        m_fileWatcher->addPath(filePath);
        expandAll();
        return true;
    }
    
    return false;
}

bool DocumentList::saveDocuments() const
{
    return m_model->saveToFile(m_fileWatcher->files().value(0));
}

void DocumentList::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    setCurrentIndex(index);
    
    QMenu menu(this);
    
    // Always available actions
    menu.addAction(m_newDocumentAction);
    menu.addAction(m_newFolderAction);
    
    // Item-specific actions
    if (index.isValid()) {
        menu.addSeparator();
        menu.addAction(m_renameAction);
        menu.addAction(m_deleteAction);
    }
    
    menu.exec(event->globalPos());
}

void DocumentList::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        bool isFolder = m_model->data(index, Qt::UserRole + 1).toBool();
        if (isFolder) {
            // Toggle expanded state
            bool expanded = m_model->data(index, Qt::UserRole + 2).toBool();
            m_model->setData(index, !expanded, Qt::UserRole + 2);
            
            if (!expanded) {
                expand(index);
            } else {
                collapse(index);
            }
        } else {
            // Emit signal for document selection
            QString filePath = m_model->data(index, Qt::UserRole).toString();
            emit documentSelected(QDir(m_documentsPath).filePath(filePath));
        }
    }
    
    QTreeView::mouseDoubleClickEvent(event);
}

void DocumentList::onItemClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    
    bool isFolder = m_model->data(index, Qt::UserRole + 1).toBool();
    if (!isFolder) {
        QString filePath = m_model->data(index, Qt::UserRole).toString();
        emit documentSelected(QDir(m_documentsPath).filePath(filePath));
    }
}

void DocumentList::createNewDocument()
{
    QModelIndex parent = currentIndex();
    
    // If the selected item is not a folder, use its parent
    if (parent.isValid() && !m_model->data(parent, Qt::UserRole + 1).toBool()) {
        parent = parent.parent();
    }
    
    QString docName = getNewDocumentName();
    QModelIndex index = m_model->addDocument(docName, parent);
    
    if (index.isValid()) {
        scrollTo(index);
        edit(index);
        
        // Create the actual file
        QString filePath = QDir(m_documentsPath).filePath(
            m_model->data(index, Qt::UserRole).toString());
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            emit documentCreated(filePath);
        }
    }
}

void DocumentList::createNewFolder()
{
    QModelIndex parent = currentIndex();
    
    // If the selected item is not a folder, use its parent
    if (parent.isValid() && !m_model->data(parent, Qt::UserRole + 1).toBool()) {
        parent = parent.parent();
    }
    
    QString folderName = getNewFolderName();
    QModelIndex index = m_model->addFolder(folderName, parent);
    
    if (index.isValid()) {
        scrollTo(index);
        edit(index);
    }
}

void DocumentList::deleteItem()
{
    QModelIndex index = currentIndex();
    if (!index.isValid()) return;
    
    bool isFolder = m_model->data(index, Qt::UserRole + 1).toBool();
    QString name = m_model->data(index, Qt::DisplayRole).toString();
    
    QString message = tr("Are you sure you want to delete \"%1\"%2?")
        .arg(name)
        .arg(isFolder ? tr(" and all its contents") : "");
    
    if (QMessageBox::question(this, tr("Delete Item"), message) == QMessageBox::Yes) {
        if (!isFolder) {
            // Delete the actual file
            QString filePath = QDir(m_documentsPath).filePath(
                m_model->data(index, Qt::UserRole).toString());
            QFile::remove(filePath);
        }
        
        m_model->removeItem(index);
    }
}

void DocumentList::renameItem()
{
    QModelIndex index = currentIndex();
    if (index.isValid()) {
        edit(index);
    }
}

void DocumentList::onFileChanged(const QString &path)
{
    // Reload the file if it was modified externally
    if (QFile::exists(path)) {
        m_model->loadFromFile(path);
        m_fileWatcher->addPath(path); // Re-add the watch
    }
}

void DocumentList::setupActions()
{
    // New Document
    m_newDocumentAction = new QAction(tr("New Note"), this);
    m_newDocumentAction->setIcon(QIcon::fromTheme("document-new"));
    connect(m_newDocumentAction, &QAction::triggered, 
            this, &DocumentList::createNewDocument);
    
    // New Folder
    m_newFolderAction = new QAction(tr("New Folder"), this);
    m_newFolderAction->setIcon(QIcon::fromTheme("folder-new"));
    connect(m_newFolderAction, &QAction::triggered, 
            this, &DocumentList::createNewFolder);
    
    // Rename
    m_renameAction = new QAction(tr("Rename"), this);
    m_renameAction->setIcon(QIcon::fromTheme("edit-rename"));
    connect(m_renameAction, &QAction::triggered, 
            this, &DocumentList::renameItem);
    
    // Delete
    m_deleteAction = new QAction(tr("Delete"), this);
    m_deleteAction->setIcon(QIcon::fromTheme("edit-delete"));
    connect(m_deleteAction, &QAction::triggered, 
            this, &DocumentList::deleteItem);
    
    // Add keyboard shortcuts
    m_newDocumentAction->setShortcut(QKeySequence::New);
    m_renameAction->setShortcut(Qt::Key_F2);
    m_deleteAction->setShortcut(QKeySequence::Delete);
    
    addAction(m_newDocumentAction);
    addAction(m_newFolderAction);
    addAction(m_renameAction);
    addAction(m_deleteAction);
}

QString DocumentList::getNewDocumentName(const QString &baseName) const
{
    QString name = baseName;
    int counter = 1;
    
    // Check if name already exists
    while (m_model->match(m_model->index(0, 0, QModelIndex()), Qt::DisplayRole, name, 
                         -1, Qt::MatchRecursive).count() > 0) {
        name = QString("%1 %2").arg(baseName).arg(++counter);
    }
    
    return name;
}

QString DocumentList::getNewFolderName(const QString &baseName) const
{
    return getNewDocumentName(baseName);
}
