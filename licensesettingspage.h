#ifndef LICENSESETTINGSPAGE_H
#define LICENSESETTINGSPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QPushButton>

class LicenseSettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit LicenseSettingsPage(QWidget *parent = nullptr);
    ~LicenseSettingsPage() = default;

private:
    void setupUI();
    QString loadLicenseText();
};

#endif // LICENSESETTINGSPAGE_H