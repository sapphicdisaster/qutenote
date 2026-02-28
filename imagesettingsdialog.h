#ifndef IMAGESETTINGSDIALOG_H
#define IMAGESETTINGSDIALOG_H

#include <QFrame>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>

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

    QSlider *m_widthSlider{};
    QLabel *m_widthLabel{};
    QButtonGroup *m_alignGroup{};
    QPushButton *m_btnLeft{};
    QPushButton *m_btnCenter{};
    QPushButton *m_btnRight{};
    
    int m_currentWidthPct;
    Qt::Alignment m_currentAlignment;
};

#endif // IMAGESETTINGSDIALOG_H
