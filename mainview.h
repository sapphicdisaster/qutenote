#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QWidget>
#include <QString>
#include <QFileSystemModel>
#include <QResizeEvent>

// Forward declarations
class QHBoxLayout;
class Theme;
class QVBoxLayout;
class QSplitter;
class QPushButton;
class QToolButton;
class QToolBar;
class QAction;
class QMenuBar;
class QStatusBar;
class QFileDialog;
class QPropertyAnimation;
class TextEditor;
class QTimer;
class QApplication;
class QScreen;
class FileBrowser;
class QHBoxLayout;

class QScrollArea;

class QProgressBar;

// Forward declaration
class TitleBarWidget;

class MainView : public QWidget
{
    Q_OBJECT

public:
    explicit MainView(QWidget *parent = nullptr);
    ~MainView();

    void setRootDirectory(const QString &path);
    QString rootDirectory() const { return m_rootDirectory; }
    QString currentDirectory() const;

    QString currentFile() const { return m_currentFile; }

    FileBrowser *fileBrowser() { return m_fileBrowser; }
    TextEditor *textEditor() { return m_textEditor; }

    QToolButton* sidebarToggleButton() { return m_toggleSidebarBtn; }
    // Allow a title widget (e.g. TitleBarWidget) to be inserted into the toolbar
    void setTitleWidget(QWidget *widget);
    void updateStatusBar(const QString &message, int timeout = 2000);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

public slots:
    void loadFile(const QString &filePath);
    void newFile();
    void saveFile();
    void toggleSidebar(bool visible = true);
    void showSettings();
    void onFileSelected(const QString &filePath);

Q_SIGNALS:
    void settingsRequested();
    void fileSaved(const QString &filePath);
    void fileOpened(const QString &filePath);

private slots:
    void onEditorModified(bool modified);
    void onFileSaved(const QString &filePath);
    void onThemeChanged(const Theme &newTheme);
    void onThemeApplyStarted();
    void onThemeApplyFinished();

public:

private:
    // Helper to remove a widget previously added to the toolbar via addWidget()
    void removeToolbarWidget(QWidget *widget);
    QWidget *m_leftToolbarSpacer = nullptr;
    QWidget *m_rightToolbarSpacer = nullptr;
    void setupUI();
    void setupMenus();
    void setupToolbar();
    void updateWindowTitle();
    void createActions();
    void setupConnections();
    void updateOverscrollIndicators();
    void scrollToolbarLeft();
    void scrollToolbarRight();
    bool promptSaveIfModified(); // Returns true if it's safe to proceed, false if cancelled
    void applyOverlayStyleToMain();
    void applyToggleStyle();
    void applySettingsStyle();
    void recomputeToolbarContentWidth();
    void applyToolbarStyle();

    QString m_rootDirectory;
    QString m_currentFile;
    bool m_sidebarVisible;
    int m_sidebarWidth; // Store sidebar width for restoration

    // UI Components
    QWidget *m_sidebar;
    QVBoxLayout *m_sidebarLayout;
    QToolButton *m_toggleSidebarBtn;
    QScrollArea *m_toolbarArea = nullptr;
    QToolButton *m_settingsBtn = nullptr;
    QWidget *m_toolbarRow = nullptr;
    QWidget *m_toolbarLeftFixed = nullptr; // holds toggle + title (non-scrollable)
    QToolButton *m_overscrollLeftWidget = nullptr;
    QToolButton *m_overscrollRightWidget = nullptr;
    QWidget *m_themeOverlay = nullptr;
    QProgressBar *m_themeProgressBar = nullptr;
    FileBrowser *m_fileBrowser;
    TextEditor *m_textEditor;
    TitleBarWidget *m_titleBarWidget;
    QSplitter *m_splitter;
    QFileSystemModel *m_fileSystemModel;
    QVBoxLayout *m_mainLayout;
    QMenuBar *m_menuBar;
    QToolBar *m_toolbar;
    QTimer *m_resizeTimer;  // Timer for throttling resize updates

    // Actions
    QAction *m_newAction;
    QAction *m_openAction;
    QAction *m_saveAction;
    QAction *m_settingsAction;
    QAction *m_exitAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
};

#endif // MAINVIEW_H
