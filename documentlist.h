#ifndef DOCUMENTLIST_H
#define DOCUMENTLIST_H

#include <QTreeView>
#include "documentmodel.h"
#include "filewatcherguard.h"

class DocumentList : public QTreeView {
    Q_OBJECT
    
public:
    explicit DocumentList(QWidget *parent = nullptr);
    ~DocumentList();
    
    bool loadDocuments(const QString &filePath);
    bool saveDocuments() const;
    
Q_SIGNALS:
    void documentSelected(const QString &filePath);
    void documentCreated(const QString &filePath);
    
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    
private slots:
    void onItemClicked(const QModelIndex &index);
    void createNewDocument();
    void createNewFolder();
    void deleteItem();
    void renameItem();
    void onFileChanged(const QString &path);
    
private:
    void setupActions();
    QString getNewDocumentName(const QString &baseName = "New Note") const;
    QString getNewFolderName(const QString &baseName = "New Folder") const;
    
    DocumentModel *m_model;
    FileWatcherGuard *m_fileWatcher;
    QString m_documentsPath;
    
    // Actions
    QAction *m_newDocumentAction;
    QAction *m_newFolderAction;
    QAction *m_renameAction;
    QAction *m_deleteAction;
};

#endif // DOCUMENTLIST_H
