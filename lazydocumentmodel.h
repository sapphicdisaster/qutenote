#ifndef LAZYDOCUMENTMODEL_H
#define LAZYDOCUMENTMODEL_H

#include "documentmodel.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QQueue>
#include <QTimer>
#include <QThread>

class LazyDocumentItem : public DocumentItem {
public:
    explicit LazyDocumentItem(Type type, const QString &title, DocumentItem *parent = nullptr);
    
    bool isLoaded() const { return m_loaded; }
    void setLoaded(bool loaded) { m_loaded = loaded; }
    
private:
    bool m_loaded = false;
};

class LazyDocumentModel : public DocumentModel {
    Q_OBJECT
    
public:
    explicit LazyDocumentModel(QObject *parent = nullptr);
    ~LazyDocumentModel();
    
    // Override data loading
    bool loadFromFile(const QString &filePath);
    
    // Lazy loading control
    void setLazyLoadingEnabled(bool enabled);
    void setLoadBatchSize(int size);
    void setLoadDelay(int msecs);
    
    // Overridden model methods
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    
private slots:
    void onLoadingFinished();
    void processLoadQueue();
    
protected:
    // Helper to set the root item
    void setRootItem(DocumentItem* item) { m_rootItem = item; }
    // Helper to retrieve item from QModelIndex
    DocumentItem* itemFromIndex(const QModelIndex& index) const {
        return index.isValid() ? static_cast<DocumentItem*>(index.internalPointer()) : m_rootItem;
    }

private:
    struct LoadRequest {
        QModelIndex parent;
        QString path;
        bool operator==(const LoadRequest &other) const {
            return parent == other.parent && path == other.path;
        }
    };
    
    void queueLoad(const QModelIndex &parent, const QString &path);
    void loadChildren(const QString &path, LazyDocumentItem *parent);
    void startNextBatch();
    
    bool m_lazyLoadingEnabled;
    int m_batchSize;
    int m_loadDelay;
    
    QQueue<LoadRequest> m_loadQueue;
    QTimer m_loadTimer;
    QFutureWatcher<QList<QPair<QString, bool>>> *m_watcher;
    QThread m_loaderThread;
    
    static const int DEFAULT_BATCH_SIZE = 50;
    static const int DEFAULT_LOAD_DELAY = 100; // ms
};

#endif // LAZYDOCUMENTMODEL_H