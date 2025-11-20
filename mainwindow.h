#include "titlebarwidget.h"
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QStatusBar>
#include "mainview.h"
#include "settingsview.h"
#include "touchinteraction.h"
#include "thememanager.h"

// Forward declaration for FileBrowser
class FileBrowser;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showSettings();
    void showMainView();
    void onTitleBarFilenameChanged(const QString &newName);
    void applySettings();
    void onThemeChanged(const Theme &newTheme);

private:
    void setupUI();
    void setupTouchInteraction();
    void handleViewTransition(QWidget *from, QWidget *to);
#ifdef Q_OS_ANDROID
    void setupAndroidSystemUI();
#endif
    
    int m_backPressCount;
    
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    Ui::MainWindow *ui;
    QStackedWidget *m_stackedWidget;
    TitleBarWidget *m_titleBarWidget;
    MainView *m_mainView;
    ThemeManager *m_themeManager;
    SettingsView *m_settingsView;
    TouchInteraction *m_touchInteraction;
    QPropertyAnimation *m_transitionAnimation;
    QStatusBar *m_statusBar;
};
#endif // MAINWINDOW_H
