#include "authdialogs.h"
#include "backupui.h"
#include "busy.h"
#include "icons.h"
#include "qrimage.h"
#include "theme.h"
#include "totp.h"
#include "uihelpers.h"
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QRegularExpressionValidator>
#include <QSysInfo>
#include <QTimer>
#include <QVBoxLayout>

using namespace tv;

namespace {
QLineEdit *makeOtpEdit()
{
    auto *e = new QLineEdit;
    e->setProperty("otp", true);
    e->setMaxLength(6);
    e->setAlignment(Qt::AlignCenter);
    e->setPlaceholderText(QStringLiteral("000000"));
    e->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("\\d{0,6}")), e));
    e->setInputMethodHints(Qt::ImhDigitsOnly | Qt::ImhNoPredictiveText);
    return e;
}
QLineEdit *makePassEdit(const QString &ph)
{
    auto *e = new QLineEdit;
    e->setEchoMode(QLineEdit::Password);
    e->setPlaceholderText(ph);
    return e;
}
QLineEdit *makeFileRow(QWidget *host, QVBoxLayout *into)
{
    auto *edit = new QLineEdit;
    edit->setPlaceholderText(QObject::tr("選擇鑰匙檔（任何檔案皆可）"));
    auto *browse = ui::button(QObject::tr("瀏覽…"));
    auto *row = new QHBoxLayout;
    row->addWidget(edit, 1);
    row->addWidget(browse);
    into->addLayout(row);
    QObject::connect(browse, &QPushButton::clicked, host, [host, edit] {
        const QString f = QFileDialog::getOpenFileName(host, QObject::tr("選擇鑰匙檔"));
        if (!f.isEmpty()) edit->setText(f);
    });
    return edit;
}
} // namespace

// ---------------------------------------------------------------------------
CredentialsWidget::CredentialsWidget(AuthMode mode, QWidget *parent) : QWidget(parent), m_mode(mode)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(10);
    if (mode & AuthPassphrase) {
        l->addWidget(ui::label(tr("密碼"), "muted"));
        m_pass = makePassEdit(tr("輸入密碼"));
        l->addWidget(m_pass);
    }
    if (mode & AuthKeyfile) {
        l->addWidget(ui::label(tr("鑰匙檔"), "muted"));
        m_file = makeFileRow(this, l);
    }
    if (mode & AuthTotp) {
        l->addWidget(ui::label(tr("驗證 App 上的 6 位數驗證碼"), "muted"));
        m_code = makeOtpEdit();
        l->addWidget(m_code);
    }
}

Credentials CredentialsWidget::credentials() const
{
    Credentials c;
    if (m_pass) c.passphrase = m_pass->text();
    if (m_file) c.keyfile = m_file->text().trimmed();
    if (m_code) c.totpCode = m_code->text();
    return c;
}

QString CredentialsWidget::validate() const
{
    if (m_pass && m_pass->text().isEmpty()) return tr("請輸入密碼");
    if (m_file && m_file->text().trimmed().isEmpty()) return tr("請選擇鑰匙檔");
    if (m_code && m_code->text().size() != totp::kDigits) return tr("請輸入 6 位數驗證碼");
    return {};
}

void CredentialsWidget::clearSecrets()
{
    if (m_pass) m_pass->clear();
    if (m_code) m_code->clear();
}

void CredentialsWidget::focusFirst()
{
    if (m_pass) m_pass->setFocus();
    else if (m_file) m_file->setFocus();
    else if (m_code) m_code->setFocus();
}

// ---------------------------------------------------------------------------
TotpEnrollDialog::TotpEnrollDialog(QWidget *parent) : QDialog(parent), m_secret(totp::newSecret())
{
    setWindowTitle(tr("設定 TOTP"));
    setModal(true);
    setMinimumWidth(440);
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(24, 22, 24, 20);
    l->setSpacing(14);
    l->addWidget(ui::header(ui::badge(QStringLiteral("123")), tr("設定 TOTP 驗證碼"),
                            tr("用 Google / Microsoft Authenticator、Aegis、1Password 等 App 掃描 QR Code，再輸入 App 顯示的 6 位數驗證碼。")));

    const QString host = QSysInfo::machineHostName().isEmpty() ? QStringLiteral("vault") : QSysInfo::machineHostName();
    const QString uri = totp::otpauthUri(QStringLiteral("TokenVault"), QStringLiteral("vault@") + host, m_secret);
    auto *qrHolder = new QFrame;                       // always dark-on-white so scanners get full contrast
    qrHolder->setStyleSheet("QFrame{background:white;border-radius:12px;}");
    auto *qrl = new QVBoxLayout(qrHolder);
    auto *qr = new QLabel;
    const QImage img = makeQrImage(uri, 5, Qt::black, Qt::white);
    qr->setPixmap(QPixmap::fromImage(img));
    qr->setAlignment(Qt::AlignCenter);
    qrl->addWidget(qr);
    l->addWidget(qrHolder, 0, Qt::AlignHCenter);

    const QString b32 = totp::base32Encode(m_secret);
    QString spaced;
    for (int i = 0; i < b32.size(); i += 4) spaced += b32.mid(i, 4) + QLatin1Char(' ');
    l->addWidget(ui::label(tr("無法掃描？手動輸入這組金鑰（類型：以時間為基礎）："), "muted", true));
    auto *row = new QHBoxLayout;
    auto *sec = new QLineEdit(spaced.trimmed());
    sec->setReadOnly(true);
    QFont mono(QStringLiteral("monospace"));
    mono.setStyleHint(QFont::Monospace);
    sec->setFont(mono);
    auto *copy = ui::button(tr("複製"));
    connect(copy, &QPushButton::clicked, this, [b32] { QApplication::clipboard()->setText(b32); });
    row->addWidget(sec, 1);
    row->addWidget(copy);
    l->addLayout(row);

    l->addWidget(ui::label(tr("輸入 App 顯示的驗證碼以確認設定成功"), "muted"));
    m_code = makeOtpEdit();
    l->addWidget(m_code);
    m_state = ui::label(QString(), "muted");
    m_state->setAlignment(Qt::AlignCenter);
    l->addWidget(m_state);
    l->addLayout(ui::footer(this, &m_ok, nullptr, tr("完成")));
    m_ok->setEnabled(false);
    connect(m_code, &QLineEdit::textChanged, this, &TotpEnrollDialog::check);
    m_code->setFocus();
    ui::fitHeight(this);
}

void TotpEnrollDialog::check()
{
    if (m_code->text().size() < totp::kDigits) {
        m_state->clear();
        m_ok->setEnabled(false);
        return;
    }
    const bool ok = totp::verify(m_secret, m_code->text(), QDateTime::currentSecsSinceEpoch());
    m_state->setText(ok ? tr("✓ 驗證成功") : tr("驗證碼不正確，請確認手機時間是否準確"));
    m_state->setProperty("role", ok ? "muted" : "error");
    m_state->setStyleSheet(ok ? QStringLiteral("color:%1;font-weight:600").arg(theme::c().ok.name()) : QString());
    ui::repolish(m_state);
    m_ok->setEnabled(ok);
}

// ---------------------------------------------------------------------------
ForgotDialog::ForgotDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("忘記密碼"));
    setModal(true);
    setMinimumWidth(440);
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(24, 22, 24, 20);
    l->setSpacing(14);
    l->addWidget(ui::header(ui::badge(QStringLiteral("!")), tr("清空所有資料並重新開始"),
                            tr("密碼與鑰匙檔無法找回。要繼續使用，只能刪除整個保險庫後重新設定。")));
    l->addWidget(ui::label(tr("下列內容會被永久刪除，無法復原：\n• 所有群組\n• 所有 Token（含名稱、說明、有效期限）\n• 加密金鑰與 TOTP 設定"),
                           "error", true));
    l->addWidget(ui::label(tr("請輸入「刪除全部」以確認"), "muted"));
    auto *edit = new QLineEdit;
    edit->setPlaceholderText(tr("刪除全部"));
    l->addWidget(edit);
    QPushButton *ok = nullptr;
    l->addLayout(ui::footer(this, &ok, nullptr, tr("永久刪除")));
    ok->setProperty("primary", false);
    ok->setProperty("danger", true);
    ok->setEnabled(false);
    ui::repolish(ok);
    connect(edit, &QLineEdit::textChanged, this, [ok](const QString &t) { ok->setEnabled(t.trimmed() == QObject::tr("刪除全部")); });
    ui::fitHeight(this);
}

// ---------------------------------------------------------------------------
AuthDialog::AuthDialog(AuthMode mode, const QString &title, const QString &reason, Action action, QWidget *parent,
                       ForgotAction forgot)
    : QDialog(parent), m_action(std::move(action)), m_forgot(std::move(forgot))
{
    setWindowTitle(title);
    setModal(true);
    setMinimumWidth(420);
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(24, 22, 24, 20);
    l->setSpacing(14);
    l->addWidget(ui::header(ui::lockBadge(), title, reason));
    m_cred = new CredentialsWidget(mode);
    l->addWidget(m_cred);
    m_err = ui::label(QString(), "error", true);
    m_err->hide();
    l->addWidget(m_err);
    if (m_forgot) {
        auto *fg = ui::button(tr("忘記密碼？"), "link");
        l->addWidget(fg, 0, Qt::AlignLeft);
        connect(fg, &QPushButton::clicked, this, [this] {
            ForgotDialog d(this);
            if (d.exec() != QDialog::Accepted) return;
            const ForgotAction act = m_forgot;
            const Result r = runBusy<Result>(this, tr("正在刪除資料…"), [act] { return act(); });
            if (!r) {
                m_err->setText(r.error);
                m_err->show();
                return;
            }
            done(ForgotWiped);
        });
    }
    QPushButton *cancel = nullptr;
    l->addLayout(ui::footer(this, &m_ok, &cancel, tr("驗證")));
    m_lockTimer = new QTimer(this);
    m_lockTimer->setInterval(1000);
    connect(m_lockTimer, &QTimer::timeout, this, &AuthDialog::tickLockout);
    m_cred->focusFirst();
    ui::fitHeight(this);
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
    const QString v = m_cred->validate();
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
    m_cred->clearSecrets();
    m_cred->focusFirst();
    if (++m_failures >= 3) {                       // 3rd failure onward: 2^(n-3) seconds, capped at 60
        m_lockLeft = qMin(60, 1 << qMin(m_failures - 3, 6)) + 1;
        m_ok->setEnabled(false);
        tickLockout();
        m_lockTimer->start();
    }
}

// ---------------------------------------------------------------------------
QFrame *SetupDialog::makeOption(const QString &title, const QString &desc, QCheckBox **box, QWidget **body)
{
    auto *f = new QFrame;
    f->setProperty("optionCard", true);
    auto *l = new QVBoxLayout(f);
    l->setContentsMargins(16, 14, 16, 14);
    l->setSpacing(6);
    *box = new QCheckBox(title);
    QFont bf = (*box)->font();
    bf.setBold(true);
    (*box)->setFont(bf);
    l->addWidget(*box);
    l->addWidget(ui::label(desc, "muted", true));
    *body = new QWidget;
    auto *bl = new QVBoxLayout(*body);
    bl->setContentsMargins(0, 6, 0, 0);
    bl->setSpacing(8);
    l->addWidget(*body);
    return f;
}

SetupDialog::SetupDialog(const QString &title, const QString &intro, Action action, QWidget *parent, AuthMode current)
    : QDialog(parent), m_hadTotp(current & AuthTotp), m_first(int(current) == 0), m_action(std::move(action))
{
    setWindowTitle(title);
    setModal(true);
    setMinimumWidth(480);
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(24, 22, 24, 20);
    l->setSpacing(12);
    l->addWidget(ui::header(ui::lockBadge(), title, intro));

    m_passCard = makeOption(tr("密碼"), tr("以密碼保護，至少 8 個字元。"), &m_usePass, &m_passBody);
    m_pass = makePassEdit(tr("設定密碼"));
    m_pass2 = makePassEdit(tr("再輸入一次"));
    m_passBody->layout()->addWidget(m_pass);
    m_passBody->layout()->addWidget(m_pass2);

    m_fileCard = makeOption(tr("鑰匙檔"), tr("任何檔案都可以。以檔案內容的 SHA-512 雜湊作為鎖，檔案被修改就打不開。"), &m_useFile, &m_fileBody);
    m_file = makeFileRow(this, static_cast<QVBoxLayout *>(m_fileBody->layout()));

    m_totpCard = makeOption(tr("TOTP 驗證碼"), tr("每次驗證都要再輸入驗證 App 的 6 位數驗證碼。"), &m_useTotp, &m_totpBody);
    m_totpHint = ui::label(QString(), "warn", true);
    m_totpState = ui::label(QString(), "muted", true);
    m_enrollBtn = ui::button(tr("掃描 QR Code 設定…"));
    m_totpBody->layout()->addWidget(m_totpHint);
    m_totpBody->layout()->addWidget(m_totpState);
    if (m_hadTotp) {
        m_regen = new QCheckBox(tr("重新產生 TOTP 金鑰（需要重新掃描）"));
        m_totpBody->layout()->addWidget(m_regen);
        connect(m_regen, &QCheckBox::toggled, this, [this] { updateRules(); });
    }
    m_totpBody->layout()->addWidget(m_enrollBtn);

    l->addWidget(m_passCard);
    l->addWidget(m_fileCard);
    l->addWidget(m_totpCard);

    auto *warn = ui::label(tr("⚠ 密碼與鑰匙檔都無法找回。遺失後，保險庫內的所有 Token 將永遠無法解密（只能清空重來）。\n鑰匙檔請勿與保險庫放在同一個位置，內容也不可被修改。"), "warn", true);
    l->addWidget(warn);
    m_err = ui::label(QString(), "error", true);
    m_err->hide();
    l->addWidget(m_err);
    l->addLayout(ui::footer(this, &m_ok, nullptr));
    m_importBtn = ui::button(tr("已有備份？匯入備份…"), "link");
    m_importBtn->hide();
    l->insertWidget(1, m_importBtn, 0, Qt::AlignLeft);
    connect(m_importBtn, &QPushButton::clicked, this, [this] {
        if (m_importHandler && m_importHandler(this)) QDialog::accept();
    });

    m_usePass->setChecked(m_first ? true : bool(current & AuthPassphrase));
    m_useFile->setChecked(bool(current & AuthKeyfile));
    m_useTotp->setChecked(m_hadTotp);
    for (QCheckBox *b : {m_usePass, m_useFile, m_useTotp}) connect(b, &QCheckBox::toggled, this, [this] { updateRules(); });
    connect(m_enrollBtn, &QPushButton::clicked, this, [this] {
        TotpEnrollDialog d(this);
        if (d.exec() == QDialog::Accepted) {
            m_secret = d.secret();
            updateRules();
        }
    });
    updateRules();
    ui::fitHeight(this);
}

void SetupDialog::setImportHandler(std::function<bool(QWidget *)> h)
{
    m_importHandler = std::move(h);
    m_importBtn->setVisible(bool(m_importHandler) && m_first);
}

AuthMode SetupDialog::selectedMode() const
{
    int m = 0;
    if (m_usePass->isChecked()) m |= AuthPassphrase;
    if (m_useFile->isChecked()) m |= AuthKeyfile;
    if (m_useTotp->isChecked()) m |= AuthTotp;
    return AuthMode(m);
}

bool SetupDialog::needsEnrollment() const
{
    return m_useTotp->isChecked() && (!m_hadTotp || (m_regen && m_regen->isChecked()));
}

void SetupDialog::updateRules()
{
    // 1) at least one key factor
    if (!m_usePass->isChecked() && !m_useFile->isChecked()) {
        QObject *s = sender();
        QCheckBox *b = (s == m_useFile) ? m_useFile : m_usePass;
        const QSignalBlocker blk(b);
        b->setChecked(true);
    }
    // 2) password + key file  =>  TOTP is mandatory
    const bool both = m_usePass->isChecked() && m_useFile->isChecked();
    {
        const QSignalBlocker blk(m_useTotp);
        if (both) m_useTotp->setChecked(true);
        m_useTotp->setEnabled(!both);
    }
    m_totpHint->setText(both ? tr("同時使用密碼與鑰匙檔（雙重認證）時，必須搭配 TOTP。") : QString());
    m_totpHint->setVisible(both);

    m_passBody->setVisible(m_usePass->isChecked());
    m_fileBody->setVisible(m_useFile->isChecked());
    m_totpBody->setVisible(m_useTotp->isChecked());
    for (auto [card, box] : {std::pair<QFrame *, QCheckBox *>{m_passCard, m_usePass}, {m_fileCard, m_useFile}, {m_totpCard, m_useTotp}})
        ui::setProp(card, "on", box->isChecked());

    const bool enroll = needsEnrollment();
    if (m_regen) m_regen->setVisible(m_useTotp->isChecked());
    m_enrollBtn->setVisible(enroll);
    if (!m_useTotp->isChecked()) m_totpState->clear();
    else if (!enroll) m_totpState->setText(tr("沿用目前的 TOTP 設定（不需要重新掃描）。"));
    else if (m_secret.isEmpty()) m_totpState->setText(tr("尚未設定。請點下方按鈕掃描 QR Code。"));
    else m_totpState->setText(tr("✓ 已掃描並驗證，可以繼續。"));
    m_enrollBtn->setText(m_secret.isEmpty() ? tr("掃描 QR Code 設定…") : tr("重新設定…"));
    layout()->invalidate();
    layout()->activate();
    resize(width(), qMax(minimumSizeHint().height(), sizeHint().height()));
}

void SetupDialog::accept()
{
    const AuthMode mode = selectedMode();
    QString err = validateMode(mode);
    if (err.isEmpty() && m_usePass->isChecked()) {
        if (m_pass->text().size() < 8) err = tr("密碼至少需要 8 個字元");
        else if (m_pass->text() != m_pass2->text()) err = tr("兩次輸入的密碼不一致");
    }
    if (err.isEmpty() && m_useFile->isChecked()) {
        const QFileInfo fi(m_file->text().trimmed());
        if (m_file->text().trimmed().isEmpty()) err = tr("請選擇鑰匙檔");
        else if (!fi.isFile() || !fi.isReadable()) err = tr("找不到或無法讀取鑰匙檔");
    }
    if (err.isEmpty() && needsEnrollment() && m_secret.isEmpty()) err = tr("請先完成 TOTP 設定（掃描 QR Code 並輸入驗證碼）");
    if (!err.isEmpty()) {
        m_err->setText(err);
        m_err->show();
        return;
    }
    Credentials c;
    if (m_usePass->isChecked()) c.passphrase = m_pass->text();
    if (m_useFile->isChecked()) c.keyfile = m_file->text().trimmed();
    const Action act = m_action;
    const QByteArray secret = needsEnrollment() ? m_secret : QByteArray();
    const QString busy = m_first ? tr("正在產生 RSA-4096 金鑰並衍生加密金鑰，可能需要數秒…") : tr("正在更新驗證方法…");
    const Result r = runBusy<Result>(this, busy, [act, mode, c, secret] { return act(mode, c, secret); });
    if (!r) {
        m_err->setText(r.error);
        m_err->show();
        return;
    }
    QDialog::accept();
}

// ---------------------------------------------------------------------------
bool runSetupFlow(Vault &vault, QWidget *parent)
{
    Vault *v = &vault;
    SetupDialog setup(QObject::tr("歡迎使用 TokenVault"),
                      QObject::tr("設定用來保護所有 Token 的方式。資料以 SHA-512 + AES-256 + RSA-4096 加密後只存放在這台電腦。"),
                      [v](AuthMode m, const Credentials &c, const QByteArray &ts) { return v->create(m, c, ts); }, parent);
    setup.setWindowIcon(icons::appIcon());
    setup.setImportHandler([v](QWidget *p) { return runImportFlow(*v, p); });
    return setup.exec() == QDialog::Accepted;
}

bool runUnlockFlow(Vault &vault, QWidget *parent, const QString &reason)
{
    Vault *v = &vault;
    AuthDialog dlg(vault.mode(), QObject::tr("解鎖保險庫"), reason.isEmpty() ? QObject::tr("請輸入驗證資訊以繼續。") : reason,
                   [v](const Credentials &c) { return v->unlock(c); }, parent, [v] { return v->wipeAll(); });
    dlg.setWindowIcon(icons::appIcon());
    const int rc = dlg.exec();
    if (rc == QDialog::Accepted) return true;
    if (rc == AuthDialog::ForgotWiped) return runSetupFlow(vault, parent);
    return false;
}
