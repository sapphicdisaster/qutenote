#pragma once
#include <QStyledItemDelegate>
#include <QPainter>

class FileBrowserDividerDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit FileBrowserDividerDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
