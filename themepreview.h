#ifndef THEMEPREVIEW_H
#define THEMEPREVIEW_H

#include "thememanager.h"
#include <QWidget>

class ThemePreview : public QWidget
{
    Q_OBJECT

public:
    explicit ThemePreview(QWidget *parent = nullptr);
    void setTheme(const Theme &theme);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Theme m_theme;
    void drawPreview(QPainter &painter);
};

#endif // THEMEPREVIEW_H