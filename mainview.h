#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QWidget>
#include <QString>
#include <QResizeEvent>
#include <QStyle>
#include "panelmanager.h"
#include "smartpointers.h"

// Forward declarations
class QHBoxLayout;
class Theme;
class QVBoxLayout;
class QSplitter;
class QPushButton;
class QToolButton;
class QToolBar;
class QAction;
class QStatusBar;
class QFileDialog;
class QPropertyAnimation;
class QVariantAnimation;
class TextEditor;
class QTimer;
class QApplication;
class QScreen;
class FileBrowser;
class QHBoxLayout;
class QScrollArea;
class QProgressBar;
class SettingsView;
class TitleBarWidget;
class QFileSystemModel;

class MainView : public QWidget
{
    Q_OBJECT

public:
    using PanelSide = PanelManager::PanelSide;

    explicit MainView(QWidget *parent = nullptr);
    ~MainView();

    void setRootDirectory(const QString &path);
    QString rootDirectory() const { return m_rootDirectory; }
    QString currentDirectory() const;

    QString currentFile() const { return m_currentFile; }

    FileBrowser *fileBrowser() { return m_fileBrowser.get(); }
    TextEditor *textEditor() { return m_textEditor.get(); }
    SettingsView *settingsView() { return m_settingsView.get(); }

    // Panel system
    void togglePanel(PanelManager::PanelSide side, bool open);
    bool isPanelOpen(PanelManager::PanelSide side) const;
    QToolButton* leftPanelHandle() { return m_panelManager ? m_panelManager->leftHandle() : nullptr; }
    QToolButton* rightPanelHandle() { return m_panelManager ? m_panelManager->rightHandle() : nullptr; }

    // Backward compatibility — delegates to panel system
    void toggleSidebar(bool visible = true);

    // Allow external code to mark user toggle for sidebar auto-hide logic
    bool userToggledSidebar() const { return m_panelManager ? m_panelManager->userToggledSidebar() : false; }
    void setUserToggledSidebar(bool toggled) { if (m_panelManager) m_panelManager->setUserToggledSidebar(toggled); }

    // Allow a title widget (e.g. TitleBarWidget) to be inserted into the toolbar
    void setTitleWidget(QWidget *widget);
    void updateStatusBar(const QString &message, int timeout = 2000);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

public slots:
    void loadFile(const QString &filePath);
    void newFile();
    void saveFile();
    void showSettings();
    void onFileSelected(const QString &filePath);

Q_SIGNALS:
    void fileSaved(const QString &filePath);
    void fileOpened(const QString &filePath);

private slots:
    void onEditorModified(bool modified);
    void onFileSaved(const QString &filePath);
    void onThemeChanged(const Theme &newTheme);
    void onThemeApplyStarted();
    void onThemeApplyFinished();
    void onSettingsChanged();
    void onSettingsBackToMain();

private:
    // Helper to remove a widget previously added to the toolbar via addWidget()
    void removeToolbarWidget(QWidget *widget);
    QIcon loadThemedIcon(const QString &resourcePath, QStyle::StandardPixmap fallback) const;
    void setupUI();
    void setupToolbar();
    void updateWindowTitle();
    void setupConnections();
    void updateOverscrollIndicators();
    void scrollToolbarLeft();
    void scrollToolbarRight();
    void scrollToolbarBy(int delta);
    bool promptSaveIfModified(); // Returns true if it's safe to proceed, false if cancelled
    void applyOverlayStyleToMain();
    void applyToggleStyle();
    void applySettingsStyle();
    void recomputeToolbarContentWidth();
    void applyToolbarStyle();

    QString m_rootDirectory;
    QString m_currentFile;
    
    // Rotation tracking
    bool m_isPortrait = false;
    bool m_wasSidebarVisibleBeforePortrait = true;

    // UI Components
    QuteNote::OwnedPtr<QWidget> m_sidebar;
    QuteNote::OwnedPtr<QWidget> m_rightPanel;
    QuteNote::OwnedPtr<QVBoxLayout> m_sidebarLayout;
    QuteNote::OwnedPtr<QVBoxLayout> m_rightPanelLayout;
    QuteNote::OwnedPtr<QSplitter> m_splitter;
    QuteNote::OwnedPtr<QToolButton> m_toggleSidebarBtn;
    QuteNote::OwnedPtr<QScrollArea> m_toolbarArea;
    QuteNote::OwnedPtr<QToolButton> m_settingsBtn;
    QuteNote::OwnedPtr<QWidget> m_toolbarRow;
    QuteNote::OwnedPtr<QWidget> m_toolbarLeftFixed;
    QuteNote::OwnedPtr<QToolButton> m_overscrollLeftWidget;
    QuteNote::OwnedPtr<QToolButton> m_overscrollRightWidget;
    QuteNote::OwnedPtr<QWidget> m_themeOverlay;
    QuteNote::OwnedPtr<QProgressBar> m_themeProgressBar;
    QuteNote::OwnedPtr<FileBrowser> m_fileBrowser;
    QuteNote::OwnedPtr<SettingsView> m_settingsView;
    QuteNote::OwnedPtr<TextEditor> m_textEditor;
    TitleBarWidget *m_titleBarWidget = nullptr;  // non-owning — owned by MainWindow
    QuteNote::OwnedPtr<QFileSystemModel> m_fileSystemModel;
    QuteNote::OwnedPtr<QVBoxLayout> m_mainLayout;
    QuteNote::OwnedPtr<QToolBar> m_toolbar;
    QuteNote::OwnedPtr<QTimer> m_resizeTimer;

    // Panel handles — now owned by PanelManager
    QuteNote::OwnedPtr<PanelManager> m_panelManager;

    // Actions
    QuteNote::OwnedPtr<QAction> m_newAction;
    QuteNote::OwnedPtr<QAction> m_openAction;
    QuteNote::OwnedPtr<QAction> m_saveAction;
    QuteNote::OwnedPtr<QAction> m_settingsAction;
    QuteNote::OwnedPtr<QAction> m_undoAction;
    QuteNote::OwnedPtr<QAction> m_redoAction;
    QuteNote::OwnedPtr<QAction> m_cutAction;
    QuteNote::OwnedPtr<QAction> m_copyAction;
    QuteNote::OwnedPtr<QAction> m_pasteAction;
};

#endif // MAINVIEW_H