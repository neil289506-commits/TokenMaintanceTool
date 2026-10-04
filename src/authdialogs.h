#pragma once
#include "vault.h"
#include <QDialog>
#include <functional>

class QCheckBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QTimer;

// Password field / key file picker / 6-digit TOTP field, depending on the mode.
class CredentialsWidget : public QWidget {
    Q_OBJECT
public:
    CredentialsWidget(tv::AuthMode mode, QWidget *parent = nullptr);
    tv::Credentials credentials() const;
    QString validate() const;           // error text, or empty
    void clearSecrets();                // wipe password + code fields after a failed attempt
    void focusFirst();
private:
    tv::AuthMode m_mode;
    QLineEdit *m_pass = nullptr, *m_file = nullptr, *m_code = nullptr;
};

// Shows a QR code + Base32 secret; enabled "完成" only after the user typed a valid current code.
class TotpEnrollDialog : public QDialog {
    Q_OBJECT
public:
    explicit TotpEnrollDialog(QWidget *parent = nullptr);
    QByteArray secret() const { return m_secret; }
private:
    void check();
    QByteArray m_secret;
    QLineEdit *m_code;
    QLabel *m_state;
    QPushButton *m_ok;
};

// "Forgot password": explains the consequences and requires typing a confirmation word.
class ForgotDialog : public QDialog {
    Q_OBJECT
public:
    explicit ForgotDialog(QWidget *parent = nullptr);
};

// Asks for the current credentials and runs `action` on a worker thread. Used for unlock and for every
// re-verification. If `forgot` is given, a "忘記密碼？" link wipes the vault and ends with ForgotWiped.
class AuthDialog : public QDialog {
    Q_OBJECT
public:
    enum { ForgotWiped = 2 };
    using Action = std::function<tv::Result(const tv::Credentials &)>;
    using ForgotAction = std::function<tv::Result()>;
    AuthDialog(tv::AuthMode mode, const QString &title, const QString &reason, Action action,
               QWidget *parent = nullptr, ForgotAction forgot = {});
    void accept() override;
    tv::Credentials usedCredentials() const { return m_used; }
private:
    void tickLockout();
    CredentialsWidget *m_cred;
    Action m_action;
    ForgotAction m_forgot;
    QLabel *m_err;
    QPushButton *m_ok;
    QTimer *m_lockTimer;
    int m_failures = 0, m_lockLeft = 0;
    tv::Credentials m_used;
};

// Choose / change the protection: password, key file, TOTP.
// Rules: at least one of password / key file; password + key file ("two-factor") forces TOTP on.
class SetupDialog : public QDialog {
    Q_OBJECT
public:
    using Action = std::function<tv::Result(tv::AuthMode, const tv::Credentials &, const QByteArray &totpSecret)>;
    // `current` = existing mode when changing (0 for first run). With an existing TOTP the user can keep it.
    SetupDialog(const QString &title, const QString &intro, Action action, QWidget *parent = nullptr,
                tv::AuthMode current = tv::AuthMode(0));
    void accept() override;
private:
    void updateRules();
    tv::AuthMode selectedMode() const;
    bool needsEnrollment() const;
    QFrame *makeOption(const QString &title, const QString &desc, QCheckBox **box, QWidget **body);

    QCheckBox *m_usePass, *m_useFile, *m_useTotp, *m_regen = nullptr;
    QWidget *m_passBody, *m_fileBody, *m_totpBody;
    QFrame *m_passCard, *m_fileCard, *m_totpCard;
    QLineEdit *m_pass, *m_pass2, *m_file;
    QLabel *m_totpState, *m_totpHint, *m_err;
    QPushButton *m_enrollBtn, *m_ok;
    QByteArray m_secret;
    bool m_hadTotp;
    bool m_first;
    Action m_action;
};

// First-run setup. Returns true when a vault was created (and is unlocked).
bool runSetupFlow(tv::Vault &v, QWidget *parent = nullptr);
// Unlock with the "forgot password" escape hatch (wipe -> setup again). False if the user gave up.
bool runUnlockFlow(tv::Vault &v, QWidget *parent = nullptr, const QString &reason = QString());
