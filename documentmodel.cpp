#include "documentmodel.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMimeData>
#include <QIODevice>

DocumentItem::DocumentItem(Type type, const QString &title, DocumentItem *parent)
    : type(type), title(title), m_parent(parent), expanded(true)
{
    if (type == Folder) {
        path = "";
    } else {
        path = title.toLower().replace(' ', '_') + ".md";
    }
}

DocumentItem::~DocumentItem()
{
    qDeleteAll(m_children);
}

void DocumentItem::appendChild(DocumentItem *child)
{
    m_children.append(child);
}

void DocumentItem::insertChild(int row, DocumentItem *child)
{
    m_children.insert(row, child);
}

void DocumentItem::removeChild(int row)
{
    delete m_children.takeAt(row);
}

DocumentItem *DocumentItem::child(int row) const
{
    return m_children.value(row);
}

int DocumentItem::childCount() const
{
    return m_children.count();
}

int DocumentItem::row() const
{
    if (m_parent)
        return m_parent->m_children.indexOf(const_cast<DocumentItem*>(this));
    return 0;
}

DocumentItem *DocumentItem::parent() const
{
    return m_parent;
}

QVariant DocumentItem::data(int role) const
{
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return title;
    case Qt::UserRole:
        return path;
    case Qt::UserRole + 1:
        return type == Folder;
    case Qt::UserRole + 2:
        return expanded;
    default:
        return QVariant();
    }
}

bool DocumentItem::setData(const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        title = value.toString();
        if (type == Document) {
            path = title.toLower().replace(' ', '_') + ".md";
        }
        return true;
    } else if (role == (Qt::UserRole + 2)) {
        expanded = value.toBool();
        return true;
    }
    return false;
}

DocumentModel::DocumentModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_rootItem = new DocumentItem(DocumentItem::Folder, tr("Root"));
    
    // Load default icons
    m_icons["folder"] = QIcon::fromTheme("folder");
    m_icons["document"] = QIcon::fromTheme("text-plain");
}

DocumentModel::~DocumentModel()
{
    delete m_rootItem;
}

QModelIndex DocumentModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    DocumentItem *parentItem = parent.isValid() 
        ? static_cast<DocumentItem*>(parent.internalPointer()) 
        : m_rootItem;

    DocumentItem *childItem = parentItem->child(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex DocumentModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    DocumentItem *childItem = static_cast<DocumentItem*>(index.internalPointer());
    DocumentItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int DocumentModel::rowCount(const QModelIndex &parent) const
{
    DocumentItem *parentItem = parent.isValid() 
        ? static_cast<DocumentItem*>(parent.internalPointer())
        : m_rootItem;
    
    return parentItem->childCount();
}

int DocumentModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant DocumentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    DocumentItem *item = static_cast<DocumentItem*>(index.internalPointer());
    
    if (role == Qt::DecorationRole) {
        return item->type == DocumentItem::Folder 
            ? m_icons["folder"] 
            : m_icons["document"];
    }
    
    return item->data(role);
}

bool DocumentModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    DocumentItem *item = static_cast<DocumentItem*>(index.internalPointer());
    bool result = item->setData(value, role);
    
    if (result) {
        emit dataChanged(index, index, {role});
        if (role == (Qt::UserRole + 2)) { // Expanded state changed
            emit layoutChanged();
        }
    }
    
    return result;
}

Qt::ItemFlags DocumentModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;

    Qt::ItemFlags flags = QAbstractItemModel::flags(index) 
        | Qt::ItemIsDragEnabled 
        | Qt::ItemIsEditable
        | Qt::ItemIsSelectable;

    if (static_cast<DocumentItem*>(index.internalPointer())->type == DocumentItem::Folder)
        flags |= Qt::ItemIsDropEnabled;

    return flags;
}

QStringList DocumentModel::mimeTypes() const
{
    return {"application/vnd.qutenote.itemlist"};
}

QMimeData *DocumentModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    
    for (const QModelIndex &index : indexes) {
        if (index.isValid()) {
            QString text = data(index, Qt::DisplayRole).toString();
            stream << text;
        }
    }
    
    mimeData->setData("application/vnd.qutenote.itemlist", encodedData);
    return mimeData;
}

bool DocumentModel::canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(parent);
    
    if (!data->hasFormat("application/vnd.qutenote.itemlist"))
        return false;
    
    if (column > 0)
        return false;
        
    return true;
}

bool DocumentModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    // Handle the drop
    // Implementation depends on your specific requirements
    
    return true;
}

bool DocumentModel::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open file.");
        return false;
    }
    
    QByteArray saveData = file.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    
    beginResetModel();
    
    // Clear existing items
    m_rootItem = new DocumentItem(DocumentItem::Folder, tr("Root"));
    
    // Load from JSON
    loadFromJson(loadDoc.array(), m_rootItem);
    
    endResetModel();
    
    m_filePath = filePath;
    return true;
}

bool DocumentModel::saveToFile(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open file for writing.");
        return false;
    }
    
    QJsonArray docArray = toJson(m_rootItem);
    QJsonDocument saveDoc(docArray);
    
    file.write(saveDoc.toJson());
    return true;
}

void DocumentModel::loadFromJson(const QJsonArray &array, DocumentItem *parent)
{
    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();
        bool isFolder = obj["type"].toString() == "folder";
        
        DocumentItem::Type type = isFolder ? DocumentItem::Folder : DocumentItem::Document;
        DocumentItem *item = new DocumentItem(type, obj["title"].toString(), parent);
        item->path = obj["path"].toString();
        
        if (isFolder) {
            item->expanded = obj["expanded"].toBool(true);
            loadFromJson(obj["children"].toArray(), item);
        }
        
        parent->appendChild(item);
    }
}

QJsonArray DocumentModel::toJson(DocumentItem *parent) const
{
    QJsonArray array;
    
    for (int i = 0; i < parent->childCount(); ++i) {
        DocumentItem *item = parent->child(i);
        QJsonObject obj;
        
        obj["type"] = item->type == DocumentItem::Folder ? "folder" : "document";
        obj["title"] = item->title;
        obj["path"] = item->path;
        
        if (item->type == DocumentItem::Folder) {
            obj["expanded"] = item->expanded;
            obj["children"] = toJson(item);
        }
        
        array.append(obj);
    }
    
    return array;
}

QModelIndex DocumentModel::addDocument(const QString &title, const QModelIndex &parent)
{
    DocumentItem *parentItem = parent.isValid() 
        ? static_cast<DocumentItem*>(parent.internalPointer())
        : m_rootItem;
        
    int row = parentItem->childCount();
    
    beginInsertRows(parent, row, row);
    DocumentItem *item = new DocumentItem(DocumentItem::Document, title, parentItem);
    parentItem->appendChild(item);
    endInsertRows();
    
    return createIndex(row, 0, item);
}

QModelIndex DocumentModel::addFolder(const QString &title, const QModelIndex &parent)
{
    DocumentItem *parentItem = parent.isValid() 
        ? static_cast<DocumentItem*>(parent.internalPointer())
        : m_rootItem;
        
    int row = parentItem->childCount();
    
    beginInsertRows(parent, row, row);
    DocumentItem *item = new DocumentItem(DocumentItem::Folder, title, parentItem);
    parentItem->appendChild(item);
    endInsertRows();
    
    return createIndex(row, 0, item);
}

bool DocumentModel::insertRows(int row, int count, const QModelIndex &parent)
{
    DocumentItem *parentItem = parent.isValid() ? 
        static_cast<DocumentItem*>(parent.internalPointer()) : m_rootItem;
    
    if (row < 0 || row > parentItem->childCount() || count <= 0)
        return false;

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        DocumentItem *newItem = new DocumentItem(DocumentItem::Document, 
            tr("New Document"), parentItem);
        parentItem->insertChild(row + i, newItem);
    }
    endInsertRows();
    
    return true;
}

bool DocumentModel::removeRows(int row, int count, const QModelIndex &parent)
{
    DocumentItem *parentItem = parent.isValid() ? 
        static_cast<DocumentItem*>(parent.internalPointer()) : m_rootItem;
    
    if (row < 0 || row + count > parentItem->childCount() || count <= 0)
        return false;

    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        // Remove from the end to prevent index shifting issues
        parentItem->removeChild(row);
    }
    endRemoveRows();
    
    return true;
}

bool DocumentModel::removeItem(const QModelIndex &index)
{
    if (!index.isValid())
        return false;

    DocumentItem *item = static_cast<DocumentItem*>(index.internalPointer());
    DocumentItem *parentItem = item->parent();
    
    if (!parentItem)
        return false;

    int row = item->row();
    
    beginRemoveRows(index.parent(), row, row);
    parentItem->removeChild(row);
    endRemoveRows();
    
    return true;
}
