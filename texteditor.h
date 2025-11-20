#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QWidget>
#include <QTextEdit>
#include "texteditortouchhandler.h"
#include "uiutils.h"
#include "componentbase.h"
#include "smartpointers.h"
#include "touchinteraction.h"

#ifdef Q_OS_ANDROID
#include <QScroller>
#include <QScrollerProperties>
#endif

// Forward declarations
class QVBoxLayout;
class QHBoxLayout;
class QToolBar;
class QToolButton;
class QComboBox;
class QFontComboBox;
class QAction;
class QMenu;
class QActionGroup;
class QScrollArea;
class QScrollBar;

class TextEditor : public QuteNote::ComponentBase
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TextEditor)

public:
    explicit TextEditor(QWidget *parent = nullptr);
    ~TextEditor() override;

    void setContent(const QString &content);
    QString getContent() const;
    void setFilePath(const QString &filePath);
    QString filePath() const;
    bool isModified() const;
    void setModified(bool modified);
    
    // Directory management
    void setDefaultSaveDirectory(const QString &directory);
    QString defaultSaveDirectory() const;
    
    // Touch-related methods
    qreal zoomFactor() const;
    void setZoomFactor(qreal factor);
    QWidget* viewport() const;
    QTextDocument* document() const;
    QScrollBar* verticalScrollBar() const;
    
Q_SIGNALS:
    void contentChanged();
    void zoomFactorChanged(qreal factor);
    void filePathChanged(const QString &filePath);
    void modificationChanged(bool modified);
    void fileSaved(const QString &filePath);

public slots:
    void newDocument();
    void openDocument();
    void saveDocument();
    bool saveDocumentAs();

    // Standard text editing operations
    void cut();
    void copy();
    void paste();
    void undo();
    void redo();
    
    // Overscroll indicator management
    void updateOverscrollIndicators();
    void scrollToolbarLeft();
    void scrollToolbarRight();
    
    // Focus management for Android
#ifdef Q_OS_ANDROID
    void updateEditorGeometry();
    void refreshToolbar();
#endif

private slots:
    void onTextChanged();
    void onBold();
    void onItalic();
    void onUnderline();
    void onStrikethrough();
    void onTextColor();
    void onBackgroundColor();
    void onAlignLeft();
    void onAlignCenter();
    void onAlignRight();
    void onAlignJustify();
    void onBulletList();
    void onNumberedList();
    void onInsertLink();
    void onInsertImage();
    void onFontChanged(const QFont &font);
    void onFontSizeChanged(const QString &size);
    void onCursorPositionChanged();


private:
    void setupUI();
    void setupToolbar();
    void createToolbarSection(const QString &title, const QList<QAction*> &actions);
    
    void setupEditor();
    void setupActions();
    void setupMenus();
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void fontChanged(const QFont &f);
    void applyFormat(QTextListFormat::Style style);
    void applyOverlayButtonTheme();
    void updateToolbarContentWidth();
    void updateToolbarTheme();
    
    
    // Touch event handling
    bool event(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    bool gestureEvent(QGestureEvent *event);
    
    // Resize handling
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    

    // Component functionality
    void initializeComponent() override;
    void setupConnections() override;
    void cleanupResources() override;
    void handleMemoryWarning() override;

    QString m_filePath;
    QString m_defaultSaveDirectory;
    bool m_modified;
    bool m_changingText = false;

    // UI Elements
    QuteNote::OwnedPtr<QWidget> m_editorContainer;
    QuteNote::OwnedPtr<QTextEdit> m_editor;
    QuteNote::OwnedPtr<QScrollArea> m_toolbarArea; // Horizontal scroll container for toolbar
    QuteNote::OwnedPtr<QToolBar> m_toolbar;
    QuteNote::OwnedPtr<QFontComboBox> m_fontCombo;
    QuteNote::OwnedPtr<QComboBox> m_sizeCombo;

    // Formatting actions
    QuteNote::OwnedPtr<QAction> m_boldAction;
    QuteNote::OwnedPtr<QAction> m_italicAction;
    QuteNote::OwnedPtr<QAction> m_underlineAction;
    QuteNote::OwnedPtr<QAction> m_strikeAction;
    QuteNote::OwnedPtr<QAction> m_textColorAction;
    QuteNote::OwnedPtr<QAction> m_bgColorAction;

    QuteNote::OwnedPtr<QAction> m_alignLeftAction;
    QuteNote::OwnedPtr<QAction> m_alignCenterAction;
    QuteNote::OwnedPtr<QAction> m_alignRightAction;
    QuteNote::OwnedPtr<QAction> m_alignJustifyAction;

    QuteNote::OwnedPtr<QAction> m_bulletListAction;
    QuteNote::OwnedPtr<QAction> m_numberedListAction;

    QuteNote::OwnedPtr<QAction> m_linkAction;
    QuteNote::OwnedPtr<QAction> m_imageAction;

    QuteNote::OwnedPtr<QAction> m_undoAction;
    QuteNote::OwnedPtr<QAction> m_redoAction;
    
    // Overscroll indicator overlay widgets (positioned independently)
    QuteNote::OwnedPtr<QToolButton> m_overscrollLeftWidget;
    QuteNote::OwnedPtr<QToolButton> m_overscrollRightWidget;
    
    // Touch handling
    QuteNote::OwnedPtr<TextEditorTouchHandler> m_touchHandler;
};

#endif // TEXTEDITOR_H