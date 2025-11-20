#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFontDatabase>
#include <QDebug>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Attempt to load a bundled custom font. Place your TTF in the resource path
    // (e.g. ":/fonts/NunitoSans-Regular.ttf") or adjust the path accordingly.
    // Resource paths use the full path as listed in resources.qrc (files are
    // under resources/fonts/). Use those resource paths here.
    int fontId = QFontDatabase::addApplicationFont(":/resources/fonts/NunitoSans-Variable.ttf");
    int fontIdItalic = QFontDatabase::addApplicationFont(":/resources/fonts/NunitoSans-Italic-Variable.ttf");
    QString nunitoSansFamily;
    if (fontId != -1) {
        QStringList familyNames = QFontDatabase::applicationFontFamilies(fontId);
        if (!familyNames.isEmpty()) {
            nunitoSansFamily = familyNames.at(0);
            qDebug() << "Loaded font family:" << nunitoSansFamily;
            QApplication::setFont(QFont(nunitoSansFamily));
        }
    } else {
        qWarning() << "Failed to load custom font. If you want a bundled font, add it to resources.qrc under :/fonts/";
    }

    if (fontIdItalic == -1) {
        qWarning() << "Failed to load italic font variant.";
    } else {
        QStringList italicFamilies = QFontDatabase::applicationFontFamilies(fontIdItalic);
        if (!italicFamilies.isEmpty()) {
            qDebug() << "Loaded italic font family:" << italicFamilies.at(0);
        }
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "QuteNote_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    // (startup diagnostics removed)

    MainWindow w;
    w.show();
    return a.exec();
}
