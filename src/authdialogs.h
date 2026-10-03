#pragma once
#include "vault.h"
#include <QDialog>
#include <functional>

class QLineEdit;
class QCheckBox;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QTimer;

// Reusable credential entry block: shows a password field and/or a key file picker depending on mode.
class CredentialsWidget : public QWidget {
    Q_OBJECT
public:
    CredentialsWidget(tv::AuthMode mode, bool confirmPassphrase, QWidget *parent = nullptr);
    tv::Credentials credentials() const;
    // Returns an error message, or empty if inputs are acceptable.
    QString validate(bool enforceStrength) const;
    void clear();
    void focusFirst();
private:
    tv::AuthMode m_mode;
    QLineEdit *m_pass = nullptr, *m_pass2 = nullptr, *m_file = nullptr;
};

// Asks for the master secret and runs `action` on a worker thread. Stays open (with an error)
// until the action succeeds or the user cancels. Used for unlock and for every re-verification.
class AuthDialog : public QDialog {
    Q_OBJECT
public:
    using Action = std::function<tv::Result(const tv::Credentials &)>;
    AuthDialog(tv::AuthMode mode, const QString &title, const QString &reason, Action action,
               QWidget *parent = nullptr);
    void accept() override;
    tv::Credentials usedCredentials() const { return m_used; }
private:
    void tickLockout();
    CredentialsWidget *m_cred;
    Action m_action;
    QLabel *m_err;
    QPushButton *m_ok;
    QTimer *m_lockTimer;
    int m_failures = 0, m_lockLeft = 0;
    tv::Credentials m_used;
};

// First-run / change-credentials dialog: choose passphrase and/or key file, confirm, then run `action`.
class SetupDialog : public QDialog {
    Q_OBJECT
public:
    using Action = std::function<tv::Result(tv::AuthMode, const tv::Credentials &)>;
    SetupDialog(const QString &title, const QString &intro, Action action, QWidget *parent = nullptr);
    void accept() override;
private:
    void rebuild();
    QCheckBox *m_usePass, *m_useFile;
    QVBoxLayout *m_holder;
    CredentialsWidget *m_cred = nullptr;
    QLabel *m_err;
    Action m_action;
};
