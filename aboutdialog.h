#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include "smartpointers.h"

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
    QuteNote::OwnedPtr<QStackedWidget> m_stack;
    QuteNote::OwnedPtr<LicenseSettingsPage> m_licensePage;
    QuteNote::OwnedPtr<QTextBrowser> m_qtBrowser;
    QuteNote::OwnedPtr<QPushButton> m_toggleBtn;
    QuteNote::OwnedPtr<QPushButton> m_closeBtn;
    QuteNote::OwnedPtr<QLabel> m_title;
};

#endif // ABOUTDIALOG_H
