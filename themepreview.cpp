#include "themepreview.h"
#include <QPainter>
#include <QStyleOption>

ThemePreview::ThemePreview(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 120);
    setMaximumSize(400, 200);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void ThemePreview::setTheme(const Theme &theme)
{
    m_theme = theme;
    update();
}

void ThemePreview::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    drawPreview(painter);
}

void ThemePreview::drawPreview(QPainter &painter)
{
    const int margin = 4;
    QRect rect = this->rect().adjusted(margin, margin, -margin, -margin);
    
    // Split into sidebar (left) and editor (right) like the actual app
    int sidebarWidth = rect.width() / 3;
    QRect sidebarRect(rect.left(), rect.top(), sidebarWidth, rect.height());
    QRect editorRect(sidebarRect.right() + 1, rect.top(), rect.width() - sidebarWidth - 1, rect.height());
    
    // Draw FileBrowser sidebar
    painter.fillRect(sidebarRect, m_theme.colors.surface);
    painter.setPen(m_theme.colors.border);
    painter.drawRect(sidebarRect);
    
    // Draw a few file items in sidebar
    int itemHeight = 24;
    int y = sidebarRect.top() + 4;
    for (int i = 0; i < 3; ++i) {
        QRect itemRect(sidebarRect.left() + 4, y, sidebarRect.width() - 8, itemHeight);
        QColor bgColor = (i == 1) ? m_theme.colors.accent : m_theme.colors.surface;
        QColor textColor = (i == 1) ? m_theme.colors.surface : m_theme.colors.text;
        
        painter.fillRect(itemRect, bgColor);
        painter.setPen(textColor);
        painter.setFont(m_theme.defaultFont);
        painter.drawText(itemRect, Qt::AlignVCenter | Qt::AlignLeft, QString("note%1.md").arg(i + 1));
        y += itemHeight + 2;
    }
    
    // Draw TextEditor area
    painter.fillRect(editorRect, m_theme.colors.background);
    painter.setPen(m_theme.colors.border);
    painter.drawRect(editorRect);
    
    // Draw sample text in editor
    painter.setPen(m_theme.colors.text);
    painter.setFont(m_theme.defaultFont);
    QRect textRect = editorRect.adjusted(8, 8, -8, -8);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop, 
                    QString("# %1\n\nSample text...").arg(m_theme.displayName));
}