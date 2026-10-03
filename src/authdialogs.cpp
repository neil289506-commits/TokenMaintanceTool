#include "authdialogs.h"
#include "busy.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

using namespace tv;

// ---------------------------------------------------------------------------
CredentialsWidget::CredentialsWidget(AuthMode mode, bool confirmPassphrase, QWidget *parent)
    : QWidget(parent), m_mode(mode)
{
    auto *form = new QFormLayout(this);
    form->setContentsMargins(0, 0, 0, 0);
    if (mode & AuthPassphrase) {
        m_pass = new QLineEdit;
        m_pass->setEchoMode(QLineEdit::Password);
        m_pass->setPlaceholderText(tr("密碼"));
        form->addRow(tr("密碼"), m_pass);
        if (confirmPassphrase) {
            m_pass2 = new QLineEdit;
            m_pass2->setEchoMode(QLineEdit::Password);
            m_pass2->setPlaceholderText(tr("再輸入一次"));
            form->addRow(tr("確認密碼"), m_pass2);
        }
    }
    if (mode & AuthKeyfile) {
        m_file = new QLineEdit;
        m_file->setPlaceholderText(tr("選擇鑰匙檔（任何檔案皆可）"));
        auto *browse = new QPushButton(tr("瀏覽…"));
        auto *row = new QHBoxLayout;
        row->addWidget(m_file, 1);
        row->addWidget(browse);
        form->addRow(tr("鑰匙檔"), row);
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString f = QFileDialog::getOpenFileName(this, tr("選擇鑰匙檔"));
            if (!f.isEmpty()) m_file->setText(f);
        });
    }
}

Credentials CredentialsWidget::credentials() const
{
    Credentials c;
    if (m_pass) c.passphrase = m_pass->text();
    if (m_file) c.keyfile = m_file->text().trimmed();
    return c;
}

QString CredentialsWidget::validate(bool enforceStrength) const
{
    if (m_pass) {
        if (m_pass->text().isEmpty()) return tr("請輸入密碼");
        if (enforceStrength && m_pass->text().size() < 8) return tr("密碼至少需要 8 個字元");
        if (m_pass2 && m_pass->text() != m_pass2->text()) return tr("兩次輸入的密碼不一致");
    }
    if (m_file && m_file->text().trimmed().isEmpty()) return tr("請選擇鑰匙檔");
    return {};
}

void CredentialsWidget::clear()
{
    if (m_pass) m_pass->clear();
    if (m_pass2) m_pass2->clear();
}

void CredentialsWidget::focusFirst()
{
    if (m_pass) m_pass->setFocus();
    else if (m_file) m_file->setFocus();
}

// ---------------------------------------------------------------------------
AuthDialog::AuthDialog(AuthMode mode, const QString &title, const QString &reason, Action action, QWidget *parent)
    : QDialog(parent), m_action(std::move(action))
{
    setWindowTitle(title);
    setModal(true);
    setMinimumWidth(420);
    auto *l = new QVBoxLayout(this);
    if (!reason.isEmpty()) {
        auto *r = new QLabel(reason);
        r->setWordWrap(true);
        l->addWidget(r);
    }
    m_cred = new CredentialsWidget(mode, false);
    l->addWidget(m_cred);
    m_err = new QLabel;
    m_err->setStyleSheet("color:#D93F3F");
    m_err->setWordWrap(true);
    m_err->hide();
    l->addWidget(m_err);
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_ok = bb->button(QDialogButtonBox::Ok);
    m_ok->setText(tr("驗證"));
    bb->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    l->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &AuthDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &AuthDialog::reject);
    m_lockTimer = new QTimer(this);
    m_lockTimer->setInterval(1000);
    connect(m_lockTimer, &QTimer::timeout, this, &AuthDialog::tickLockout);
    m_cred->focusFirst();
}

void AuthDialog::tickLockout()
{
    if (--m_lockLeft <= 0) {
        m_lockTimer->stop();
        m_ok->setEnabled(true);
        m_ok->setText(tr("驗證"));
    } else {
        m_ok->setText(tr("請等待 %1 秒").arg(m_lockLeft));
    }
}

void AuthDialog::accept()
{
    if (!m_ok->isEnabled()) return;
    const QString v = m_cred->validate(false);
    if (!v.isEmpty()) {
        m_err->setText(v);
        m_err->show();
        return;
    }
    const Credentials c = m_cred->credentials();
    const Action act = m_action;
    const Result r = runBusy<Result>(this, tr("驗證中…"), [act, c] { return act(c); });
    if (r) {
        m_used = c;
        QDialog::accept();
        return;
    }
    m_err->setText(r.error);
    m_err->show();
    m_cred->clear();
    m_cred->focusFirst();
    if (++m_failures >= 3) {                       // 3rd failure onward: 2^(n-3) seconds, capped at 60
        m_lockLeft = qMin(60, 1 << qMin(m_failures - 3, 6)) + 1;
        m_ok->setEnabled(false);
        tickLockout();
        m_lockTimer->start();
    }
}

// ---------------------------------------------------------------------------
SetupDialog::SetupDialog(const QString &title, const QString &intro, Action action, QWidget *parent)
    : QDialog(parent), m_action(std::move(action))
{
    setWindowTitle(title);
    setModal(true);
    setMinimumWidth(460);
    auto *l = new QVBoxLayout(this);
    auto *i = new QLabel(intro);
    i->setWordWrap(true);
    l->addWidget(i);
    m_usePass = new QCheckBox(tr("使用密碼"));
    m_useFile = new QCheckBox(tr("使用鑰匙檔（以檔案內容的 SHA-512 雜湊作為鎖）"));
    m_usePass->setChecked(true);
    l->addWidget(m_usePass);
    l->addWidget(m_useFile);
    m_holder = new QVBoxLayout;
    l->addLayout(m_holder);
    auto *warn = new QLabel(tr("⚠ 密碼與鑰匙檔都無法找回。遺失後，保險庫內的所有 Token 將永遠無法解密。\n"
                               "鑰匙檔請勿與保險庫放在同一個位置，且內容不可被修改。"));
    warn->setWordWrap(true);
    warn->setStyleSheet("color:#B26A00");
    l->addWidget(warn);
    m_err = new QLabel;
    m_err->setStyleSheet("color:#D93F3F");
    m_err->setWordWrap(true);
    m_err->hide();
    l->addWidget(m_err);
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bb->button(QDialogButtonBox::Ok)->setText(tr("確定"));
    bb->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    l->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &SetupDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &SetupDialog::reject);
    connect(m_usePass, &QCheckBox::toggled, this, &SetupDialog::rebuild);
    connect(m_useFile, &QCheckBox::toggled, this, &SetupDialog::rebuild);
    rebuild();
}

void SetupDialog::rebuild()
{
    if (!m_usePass->isChecked() && !m_useFile->isChecked()) {
        // at least one factor is required
        QCheckBox *sender_ = qobject_cast<QCheckBox *>(sender());
        (sender_ ? sender_ : m_usePass)->setChecked(true);
        return;
    }
    delete m_cred;
    int m = (m_usePass->isChecked() ? AuthPassphrase : 0) | (m_useFile->isChecked() ? AuthKeyfile : 0);
    m_cred = new CredentialsWidget(AuthMode(m), true);
    m_holder->addWidget(m_cred);
    m_cred->focusFirst();
}

void SetupDialog::accept()
{
    const QString v = m_cred->validate(true);
    if (!v.isEmpty()) {
        m_err->setText(v);
        m_err->show();
        return;
    }
    const AuthMode mode = AuthMode((m_usePass->isChecked() ? AuthPassphrase : 0) | (m_useFile->isChecked() ? AuthKeyfile : 0));
    const Credentials c = m_cred->credentials();
    const Action act = m_action;
    const Result r = runBusy<Result>(this, tr("正在產生 RSA-4096 金鑰並衍生加密金鑰，可能需要數秒…"),
                                     [act, mode, c] { return act(mode, c); });
    if (!r) {
        m_err->setText(r.error);
        m_err->show();
        return;
    }
    QDialog::accept();
}
