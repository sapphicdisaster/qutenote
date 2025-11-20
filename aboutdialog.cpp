#include "aboutdialog.h"
#include <QVBoxLayout>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QApplication>
#include <QScreen>
#include <QScrollBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QLibraryInfo>
#include <QSysInfo>
#include <QDate>
#include "licensesettingspage.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , m_stack(new QStackedWidget(this))
    , m_licensePage(new LicenseSettingsPage(this))
    , m_qtBrowser(new QTextBrowser(this))
{
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    setModal(true);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8,8,8,8);
    mainLayout->setSpacing(8);

    m_title = new QLabel(tr("%1").arg(QApplication::applicationName()), this);
    QFont f = m_title->font(); f.setPointSize(18); f.setBold(true); m_title->setFont(f);
    m_title->setAlignment(Qt::AlignHCenter);
    mainLayout->addWidget(m_title);

    // Stack contains license page and Qt-about. The short app-about copy
    // now lives in Settings -> About, so we show license first by default.
    m_stack->addWidget(m_licensePage);

    // Qt about page (initially a QTextBrowser)
    m_qtBrowser->setReadOnly(true);
    m_qtBrowser->setOpenExternalLinks(true);
    m_stack->addWidget(m_qtBrowser);

    // Start on the License page
    m_stack->setCurrentWidget(m_licensePage);
    m_title->setText(tr("License"));

    mainLayout->addWidget(m_stack, 1);

    // Buttons: License / Close. The license button will toggle between
    // license and Qt-about when pressed. Donate button was removed and its
    // behavior is handled from the Settings -> About main page.
    auto btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_toggleBtn = new QPushButton(tr("Show Qt About"), this);
    m_closeBtn = new QPushButton(tr("Close"), this);

    btnLayout->addWidget(m_toggleBtn);
    btnLayout->addWidget(m_closeBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_closeBtn, &QPushButton::clicked, this, &AboutDialog::onCloseClicked);
    connect(m_toggleBtn, &QPushButton::clicked, this, &AboutDialog::onToggleView);

    setLayout(mainLayout);
}

void AboutDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    setWindowState(Qt::WindowMaximized);
}

void AboutDialog::onCloseClicked()
{
    accept();
}

void AboutDialog::onToggleView()
{
    int idx = m_stack->currentIndex();
    if (idx == 0) {
        // Currently License, switch to Qt about
        QStringList modules;
        modules << "QtCore" << "QtGui" << "QtWidgets";
        QString moduleListHtml;
        for (const QString &m : modules) moduleListHtml += QString("<li>%1</li>").arg(m);

        QString qtHtml = QString("<h3>Qt Details</h3>") +
                QString("<p>Qt runtime version: <b>%1</b></p>").arg(qVersion()) +
                QString("<p>Qt build version: %1</p>").arg(QLibraryInfo::version().toString()) +
                QString("<p>Operating system: %1</p>").arg(QSysInfo::prettyProductName()) +
                QString("<h4>Linked Qt modules</h4><ul>%1</ul>").arg(moduleListHtml) +
                QString("<p>For Qt source and license details see: <a href='https://code.qt.io/'>https://code.qt.io/</a></p>");

        m_qtBrowser->setHtml(qtHtml);
        m_stack->setCurrentWidget(m_qtBrowser);
        m_toggleBtn->setText(tr("Show License"));
        m_title->setText(tr("Qt Details"));
    } else {
        // Currently Qt about - go back to license
        m_stack->setCurrentWidget(m_licensePage);
        m_toggleBtn->setText(tr("Show Qt Details"));
        m_title->setText(tr("Qutenote License"));
    }
}
