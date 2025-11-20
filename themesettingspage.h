#ifndef THEMESETTINGSPAGE_H
#define THEMESETTINGSPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFontComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QShowEvent>
#include "thememanager.h"
#include "colorpicker.h"

class ThemeSettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit ThemeSettingsPage(QWidget *parent = nullptr);
    void saveSettings();
    void loadSettings();

protected:
    void showEvent(QShowEvent *event) override;

Q_SIGNALS:
    void settingsChanged();
    void newThemeRequested();
    void saveThemeRequested();

private slots:
    void onThemeChanged(int index);
    void onColorChanged(const QString &role, const QColor &color);
    void onFontChanged();
    void onZoomChanged();
    void onExportTheme();
    void onImportTheme();

private:
    void setupUI();
    void createColorPickers();
    void setupFontSection();
    void setupZoomSection();

    QComboBox *m_themeSelector;
    QMap<QString, QPushButton*> m_colorPickers;
    QFontComboBox *m_defaultFontCombo;
    QFontComboBox *m_menuFontCombo;
    QSpinBox *m_defaultFontSize;
    
    // Zoom slider replaces spacing, border radius, icon size, and touch target
    QSlider *m_zoomSlider;

    Theme m_currentTheme;
    bool m_isUpdating;
    bool m_initialLoadDone = false;
};

#endif // THEMESETTINGSPAGE_H