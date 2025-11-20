#pragma once
#include <QWidget>
#include <QString>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainterPath>
#include "thememanager.h"

class TitleBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit TitleBarWidget(QWidget *parent = nullptr);
    void setFilename(const QString &filename);
    void setThemeManager(ThemeManager *themeManager);
    QString filename() const;

signals:
    void filenameEditRequested();
    void filenameChanged(const QString &newName);
    void saveRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void startEditing();
    void finishEditing();
    void onSaveClicked();
    void onThemeChanged(const Theme &newTheme);

private:
    void updateDisplay();
    QString m_filename;
    QLineEdit *m_edit;
    QLabel *m_label;
    QPushButton *m_saveButton;
    QHBoxLayout *m_layout;
    ThemeManager *m_themeManager;
    QString nameWithoutExtension(const QString &filename) const;
    QString extension(const QString &filename) const;
};
