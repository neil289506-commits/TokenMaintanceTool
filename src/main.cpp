#include "authdialogs.h"
#include "i18n.h"
#include "icons.h"
#include "mainwindow.h"
#include "theme.h"
#include "vault.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("TokenVault"));
    QApplication::setApplicationName(QStringLiteral("TokenVault"));
    tv::i18n::install();                              // before any tr() text is created
    QApplication::setApplicationDisplayName(QObject::tr("TokenVault"));
    QApplication::setApplicationVersion(QStringLiteral("1.1.0"));
    QApplication::setWindowIcon(icons::appIcon());
    QApplication::setQuitOnLastWindowClosed(false);   // MainWindow quits explicitly; hide() during lock must not exit
    theme::install(app);

    tv::Vault vault(tv::Vault::defaultDir());
    if (!vault.exists()) {
        if (!runSetupFlow(vault)) return 0;
    } else {
        if (!runUnlockFlow(vault)) return 0;
    }

    MainWindow w(vault);
    w.show();
    return app.exec();
}
