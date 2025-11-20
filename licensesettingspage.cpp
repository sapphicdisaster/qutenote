#include "licensesettingspage.h"
#include <QtCore/qglobal.h>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QStyle>
#include <QFile>

LicenseSettingsPage::LicenseSettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void LicenseSettingsPage::setupUI()
{
    #ifndef Q_OS_ANDROID
    setWindowTitle(tr("License Information"));
    #endif

    auto layout = new QVBoxLayout(this);
    auto licenseBrowser = new QTextBrowser(this);

    // Set up the license text browser
    licenseBrowser->setOpenExternalLinks(true);
    licenseBrowser->setHtml(loadLicenseText());
    licenseBrowser->setReadOnly(true);
    licenseBrowser->setMinimumHeight(200);
    licenseBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    licenseBrowser->setTextInteractionFlags(Qt::TextBrowserInteraction);

    // Add widgets to layout
    layout->addWidget(licenseBrowser, 1);
    setLayout(layout);
}

QString LicenseSettingsPage::loadLicenseText()
{
    // Include an explicit list of components that ship with QuteNote so users
    // can find the relevant licenses for bundled libs.
    return QString(
        "QuteNote is free software, licensed under the GNU General Public License (GPL) version 3 or later. "
        "This program uses Qt, which is available under GPL v3. As a user, you have certain rights under this license:"
        "<h3>Key Rights under GPL v3:</h3>"
        "<ul>"
        "<li>You can use this software for any purpose</li>"
        "<li>You can study how the software works and modify it</li>"
        "<li>You can distribute copies of the software</li>"
        "<li>You can distribute your modified versions</li>"
        "</ul>"
        "<h3>Source Code & Components:</h3>"
        "<p>As required by GPL v3, you can obtain the complete source code for QuteNote and the major components used:</p>"
        "<ul>"
        "<li>QuteNote source code: <a href='https://github.com/sapphicdisaster/QuteNote'>https://github.com/sapphicdisaster/QuteNote</a></li>"
        "<li>GPL v3 License: <a href='https://www.gnu.org/licenses/gpl-3.0.html'>https://www.gnu.org/licenses/gpl-3.0.html</a></li>"
        "<li>Qt source code: <a href='https://code.qt.io/cgit/qt/qtbase.git/'>https://code.qt.io/cgit/qt/qtbase.git/</a></li>"
        "</ul>"
        "<h3>Included QuteNote Components</h3>"
        "<p>The following project components are part of the QuteNote distribution and are covered by the project's license(s):</p>"
        "<ul>"
        "<li>libcolorpicker (color picker widget & support libraries)</li>"
        "<li>QuteNote core (application UI and document model)</li>"
        "<li>Third-party components that are part of Qt (see Qt license)</li>"
        "</ul>"
        "<p>If you require a complete list of third-party licenses or the exact source for a bundled component, please visit the project repository above or contact the maintainers.</p>"
    );
}

