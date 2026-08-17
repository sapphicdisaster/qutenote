#ifndef PANELMANAGER_H
#define PANELMANAGER_H

#include <QObject>
#include "smartpointers.h"

class QSplitter;
class QToolButton;
class QVariantAnimation;
class QWidget;
class Theme;

class PanelManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PanelManager)

public:
    enum class PanelSide { Left, Right };

    explicit PanelManager(QSplitter *splitter, QWidget *leftPanel, QWidget *rightPanel,
                          QWidget *handleParent, QObject *parent = nullptr);
    ~PanelManager();

    void togglePanel(PanelSide side, bool open);
    bool isPanelOpen(PanelSide side) const;

    QToolButton *leftHandle() const { return m_leftHandle; }
    QToolButton *rightHandle() const { return m_rightHandle; }

    bool userToggledSidebar() const { return m_userToggledSidebar; }
    void setUserToggledSidebar(bool toggled) { m_userToggledSidebar = toggled; }

    void applyHandleStyle(QToolButton *handle, PanelSide side);
    void positionHandles();

Q_SIGNALS:
    void settingsBackToMain();

private slots:
    void onAnimationValueChanged(const QVariant &value);

private:
    void startAnimation();
    void createHandles(QWidget *handleParent);
    void createAnimation();

    int handleThumbWidth() const;
    int handleThumbHeight() const;
    int defaultWidth(PanelSide side) const;

    static constexpr int kEditorMinWidth = 120;
    static constexpr int kHandleMaxHeight = 120;

    QSplitter *m_splitter;
    QWidget *m_leftPanel;
    QWidget *m_rightPanel;
    QToolButton *m_leftHandle = nullptr;
    QToolButton *m_rightHandle = nullptr;
    QuteNote::OwnedPtr<QVariantAnimation> m_animation;

    bool m_leftOpen = false;
    bool m_rightOpen = false;
    int m_leftWidth = 0;
    int m_rightWidth = 0;

    int m_animStartLeft = 0;
    int m_animStartRight = 0;
    int m_animEndLeft = 0;
    int m_animEndRight = 0;

    bool m_userToggledSidebar = false;
};

#endif // PANELMANAGER_H
