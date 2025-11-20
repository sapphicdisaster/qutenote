#ifndef DOCUMENTMODEL_H
#define DOCUMENTMODEL_H

#include <QAbstractItemModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QIcon>

class DocumentItem {
public:
    enum Type { Document, Folder };
    
    explicit DocumentItem(Type type, const QString &title, DocumentItem *parent = nullptr);
    ~DocumentItem();
    
    void appendChild(DocumentItem *child);
    void insertChild(int row, DocumentItem *child);
    void removeChild(int row);
    DocumentItem *child(int row) const;
    int childCount() const;
    int columnCount() const;
    QVariant data(int role) const;
    bool setData(const QVariant &value, int role);
    int row() const;
    DocumentItem *parent() const;
    
    Type type;
    QString title;
    QString path;
    bool expanded;
    
private:
    QList<DocumentItem*> m_children;
    DocumentItem *m_parent;
};

class DocumentModel : public QAbstractItemModel {
    Q_OBJECT
    
public:
    explicit DocumentModel(QObject *parent = nullptr);
    ~DocumentModel();
    
    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    
    // Drag and drop support
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
                        int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                     int row, int column, const QModelIndex &parent) override;
    
    // Custom methods
    bool loadFromFile(const QString &filePath);
    bool saveToFile(const QString &filePath) const;
    
    // Item manipulation
    bool insertRows(int row, int count, const QModelIndex &parent) override;
    bool removeRows(int row, int count, const QModelIndex &parent) override;
    
    // Helper methods
    QModelIndex addDocument(const QString &title, const QModelIndex &parent);
    QModelIndex addFolder(const QString &title, const QModelIndex &parent);
    bool removeItem(const QModelIndex &index);
    
private:
    void loadFromJson(const QJsonArray &array, DocumentItem *parent);
    QJsonArray toJson(DocumentItem *parent) const;
    
protected:
    DocumentItem *m_rootItem;
    QHash<QString, QIcon> m_icons;
    QString m_filePath;
};

#endif // DOCUMENTMODEL_H
