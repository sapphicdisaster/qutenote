#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

class QTextBrowser;
class LicenseSettingsPage;
class QStackedWidget;
class QPushButton;
class QLabel;

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onCloseClicked();
    void onToggleView();

private:
    QStackedWidget *m_stack;
    LicenseSettingsPage *m_licensePage;
    QTextBrowser *m_qtBrowser;
    QPushButton *m_toggleBtn;
    QPushButton *m_closeBtn;
    QLabel *m_title;
};

#endif // ABOUTDIALOG_H
