#ifndef IMAGESETTINGSDIALOG_H
#define IMAGESETTINGSDIALOG_H

#include <QFrame>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include "smartpointers.h"

class ImageSettingsDialog : public QFrame
{
    Q_OBJECT

public:
    explicit ImageSettingsDialog(QWidget *parent = nullptr);
    ~ImageSettingsDialog();

    void setImageProperties(int widthPct, Qt::Alignment alignment);
    
    int widthPercentage() const;
    Qt::Alignment alignment() const;

signals:
    void settingsChanged(int widthPct, Qt::Alignment alignment);
    void dialogHidden();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onSliderValueChanged(int value);
    void onAlignmentChanged(QAbstractButton *button);

private:
    void setupUI();
    void updateSliderLabel(int value);

    QuteNote::OwnedPtr<QSlider> m_widthSlider;
    QuteNote::OwnedPtr<QLabel> m_widthLabel;
    QuteNote::OwnedPtr<QButtonGroup> m_alignGroup;
    QPushButton *m_btnLeft = nullptr;
    QPushButton *m_btnCenter = nullptr;
    QPushButton *m_btnRight = nullptr;
    
    int m_currentWidthPct;
    Qt::Alignment m_currentAlignment;
};

#endif // IMAGESETTINGSDIALOG_H
