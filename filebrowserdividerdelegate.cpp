#include "filebrowserdividerdelegate.h"
#include <QApplication>
#include <QFontMetrics>
#include <QPalette>

FileBrowserDividerDelegate::FileBrowserDividerDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {}

void FileBrowserDividerDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QString path = index.data(Qt::UserRole).toString();
    if (path.endsWith(".divider", Qt::CaseInsensitive)) {
        painter->save();
        QRect r = option.rect;
        QString title = index.data(Qt::DisplayRole).toString().remove('-').trimmed();
        QPalette pal = option.palette;

        // Draw full-width line
        int lineY = r.center().y();
        painter->setPen(pal.color(QPalette::Mid));
        painter->drawLine(r.left(), lineY, r.right(), lineY);

        // Draw rounded box for title
        QFont font = option.font;
        font.setBold(true);
        painter->setFont(font);
        QFontMetrics fm(font);
        int pad = 12;
        int boxW = fm.horizontalAdvance(title) + pad * 2;
        int boxH = fm.height() + 6;
        QRect boxRect(r.center().x() - boxW/2, lineY - boxH/2, boxW, boxH);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(pal.color(QPalette::Button));
        painter->setPen(pal.color(QPalette::Shadow));
        painter->drawRoundedRect(boxRect, boxH/2, boxH/2);

        // Draw title text
        painter->setPen(pal.color(QPalette::ButtonText));
        painter->drawText(boxRect, Qt::AlignCenter, title);
        painter->restore();
    } else {
        // For non-divider items, use the default painting
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize FileBrowserDividerDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QString path = index.data(Qt::UserRole).toString();
    if (path.endsWith(".divider", Qt::CaseInsensitive)) {
        QFont font = option.font;
        font.setBold(true);
        QFontMetrics fm(font);
        int h = fm.height() + 18;
        return QSize(option.rect.width(), h);
    } else {
        // For non-divider items (files and folders), return a fixed height
        return QSize(option.rect.width(), 38); // Match button height
    }
}
