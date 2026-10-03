#include "authdialogs.h"
#include "icons.h"
#include "mainwindow.h"
#include "vault.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("TokenVault"));
    QApplication::setApplicationName(QStringLiteral("TokenVault"));
    QApplication::setApplicationDisplayName(QObject::tr("Token 管理工具"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setWindowIcon(icons::appIcon());
    QApplication::setQuitOnLastWindowClosed(false);   // MainWindow quits explicitly; hide() during lock must not exit

    tv::Vault vault(tv::Vault::defaultDir());
    tv::Vault *v = &vault;

    if (!vault.exists()) {
        SetupDialog setup(QObject::tr("首次設定"),
                          QObject::tr("歡迎使用。請設定用來保護所有 Token 的密碼和／或鑰匙檔。\n"
                                      "資料會以 SHA-512 + AES-256 + RSA-4096 加密後存放在本機。"),
                          [v](tv::AuthMode m, const tv::Credentials &c) { return v->create(m, c); });
        if (setup.exec() != QDialog::Accepted) return 0;
    } else {
        AuthDialog dlg(vault.mode(), QObject::tr("解鎖保險庫"), QObject::tr("請輸入密碼／鑰匙檔。"),
                       [v](const tv::Credentials &c) { return v->unlock(c); });
        if (dlg.exec() != QDialog::Accepted) return 0;
    }

    MainWindow w(vault);
    w.show();
    return app.exec();
}
