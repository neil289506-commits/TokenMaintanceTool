#pragma once
#include "vault.h"
#include <QDialog>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class ExpiryPicker;

class TokenDetailDialog : public QDialog {
    Q_OBJECT
public:
    TokenDetailDialog(tv::Vault &vault, const tv::GroupInfo &group, const tv::TokenInfo &token, QWidget *parent = nullptr);
    bool changed() const { return m_changed; }

private:
    bool authorize(const QString &reason);          // re-verification (password / key file)
    void refresh();
    void showSecret();
    void hideSecret();
    void copySecret();
    void saveTextMeta();
    void changeExpiry();
    void revoke();
    void renew();
    void remove();
    void tick();

    tv::Vault &m_vault;
    tv::GroupInfo m_group;
    tv::TokenInfo m_t;
    bool m_changed = false;

    QLabel *m_statusIcon, *m_statusText, *m_info, *m_countdown;
    QLineEdit *m_name, *m_secret;
    QPlainTextEdit *m_note;
    QPushButton *m_show, *m_copy;
    ExpiryPicker *m_expiry;
    QTimer *m_hideTimer;
    int m_left = 0;
    tv::SecureBytes m_revealed;
};
