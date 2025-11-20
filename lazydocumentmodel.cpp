#include "lazydocumentmodel.h"
#include <QDir>
#include <QDirIterator>
#include <QtConcurrent>
#include <QApplication>

LazyDocumentItem::LazyDocumentItem(Type type, const QString &title, DocumentItem *parent)
    : DocumentItem(type, title, parent)
    , m_loaded(false)
{
}

LazyDocumentModel::LazyDocumentModel(QObject *parent)
    : DocumentModel(parent)
    , m_lazyLoadingEnabled(true)
    , m_batchSize(DEFAULT_BATCH_SIZE)
    , m_loadDelay(DEFAULT_LOAD_DELAY)
    , m_watcher(new QFutureWatcher<QList<QPair<QString, bool>>>(this))
{
    connect(&m_loadTimer, &QTimer::timeout,
            this, &LazyDocumentModel::processLoadQueue);
    
    connect(m_watcher, &QFutureWatcher<QList<QPair<QString, bool>>>::finished,
            this, &LazyDocumentModel::onLoadingFinished);
    
    m_loadTimer.setSingleShot(true);
}

LazyDocumentModel::~LazyDocumentModel()
{
    m_loadTimer.stop();
    m_watcher->waitForFinished();
    delete m_watcher;
}

bool LazyDocumentModel::loadFromFile(const QString &filePath)
{
    // Only load root level items initially
    QDir dir(filePath);
    if (!dir.exists())
        return false;
        
    beginResetModel();
    
    // Create root item
    auto rootItem = new LazyDocumentItem(LazyDocumentItem::Folder, dir.dirName());
    rootItem->path = dir.absolutePath();
    setRootItem(rootItem);
    
    // Queue loading of first level
    queueLoad(QModelIndex(), dir.absolutePath());
    
    endResetModel();
    return true;
}

void LazyDocumentModel::setLazyLoadingEnabled(bool enabled)
{
    m_lazyLoadingEnabled = enabled;
}

void LazyDocumentModel::setLoadBatchSize(int size)
{
    m_batchSize = qBound(1, size, 1000);
}

void LazyDocumentModel::setLoadDelay(int msecs)
{
    m_loadDelay = qBound(0, msecs, 1000);
}

bool LazyDocumentModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return true;
        
    LazyDocumentItem *item = static_cast<LazyDocumentItem*>(itemFromIndex(parent));
    if (!item->isLoaded()) {
        QDir dir(item->path);
        return dir.exists() && !dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty();
    }
    
    return item->childCount() > 0;
}

bool LazyDocumentModel::canFetchMore(const QModelIndex &parent) const
{
    if (!m_lazyLoadingEnabled)
        return false;
        
    if (!parent.isValid())
        return false;
        
    LazyDocumentItem *item = static_cast<LazyDocumentItem*>(itemFromIndex(parent));
    return item && item->type == LazyDocumentItem::Folder && !item->isLoaded();
}

void LazyDocumentModel::fetchMore(const QModelIndex &parent)
{
    if (!canFetchMore(parent))
        return;
        
    LazyDocumentItem *item = static_cast<LazyDocumentItem*>(itemFromIndex(parent));
    queueLoad(parent, item->path);
}

void LazyDocumentModel::queueLoad(const QModelIndex &parent, const QString &path)
{
    LoadRequest request{parent, path};
    if (!m_loadQueue.contains(request)) {
        m_loadQueue.enqueue(request);
        if (!m_loadTimer.isActive()) {
            m_loadTimer.start(m_loadDelay);
        }
    }
}

void LazyDocumentModel::processLoadQueue()
{
    if (m_loadQueue.isEmpty() || m_watcher->isRunning())
        return;
        
    LoadRequest request = m_loadQueue.dequeue();
    LazyDocumentItem *parentItem = static_cast<LazyDocumentItem*>(itemFromIndex(request.parent));
    
    // Start async loading
    QFuture<QList<QPair<QString, bool>>> future = QtConcurrent::run([path = request.path]() {
        QList<QPair<QString, bool>> entries;
        QDir dir(path);
        QDirIterator it(path, QDir::AllEntries | QDir::NoDotAndDotDot);
        
        while (it.hasNext()) {
            it.next();
            entries.append({it.fileName(), it.fileInfo().isDir()});
            
            // Process events to keep UI responsive
            QApplication::processEvents();
        }
        return entries;
    });
    
    m_watcher->setFuture(future);
}

void LazyDocumentModel::onLoadingFinished()
{
    QList<QPair<QString, bool>> entries = m_watcher->result();
    LazyDocumentItem *parentItem = static_cast<LazyDocumentItem*>(itemFromIndex(m_loadQueue.head().parent));
    
    if (!parentItem)
        return;
        
    beginInsertRows(m_loadQueue.head().parent, 0, entries.size() - 1);
    
    for (const auto &entry : entries) {
        const QString &name = entry.first;
        bool isDir = entry.second;
        
        LazyDocumentItem *item = new LazyDocumentItem(
            isDir ? LazyDocumentItem::Folder : LazyDocumentItem::Document,
            name,
            parentItem
        );
        item->path = QDir(parentItem->path).filePath(name);
        parentItem->appendChild(item);
    }
    
    parentItem->setLoaded(true);
    endInsertRows();
    
    // Process next batch if there are more items in queue
    if (!m_loadQueue.isEmpty()) {
        m_loadTimer.start(m_loadDelay);
    }
}