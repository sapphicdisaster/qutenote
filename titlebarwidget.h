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
#include "smartpointers.h"

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
    QuteNote::OwnedPtr<QLineEdit> m_edit;
    QuteNote::OwnedPtr<QLabel> m_label;
    QuteNote::OwnedPtr<QPushButton> m_saveButton;
    QuteNote::OwnedPtr<QHBoxLayout> m_layout;
    ThemeManager *m_themeManager = nullptr;
    QString nameWithoutExtension(const QString &filename) const;
    QString extension(const QString &filename) const;
};
