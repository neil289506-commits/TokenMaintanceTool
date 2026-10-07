#include "tokendetail.h"
#include "authdialogs.h"
#include "editdialogs.h"
#include "icons.h"
#include "uihelpers.h"
#include <QFrame>
#include <QApplication>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

using namespace tv;

static constexpr int kRevealSeconds = 30;
static const QString kDot = QStringLiteral("●");

// Pop-up that shows the token for 30 seconds, then locks itself again. There is deliberately no copy
// button and no clipboard handling here; the text is selectable like any read-only text.
class RevealDialog : public QDialog {
public:
    RevealDialog(const QString &tokenName, const QString &token, QWidget *parent) : QDialog(parent), m_left(kRevealSeconds)
    {
        setWindowTitle(tr("Token"));
        setModal(true);
        setMinimumWidth(460);
        auto *l = new QVBoxLayout(this);
        l->setContentsMargins(24, 22, 24, 20);
        l->setSpacing(12);
        l->addWidget(ui::header(ui::lockBadge(), tokenName, tr("驗證成功。倒數結束後會自動鎖定。")));
        m_text = new QPlainTextEdit;
        m_text->setReadOnly(true);
        m_text->setPlainText(token);
        QFont mono(QStringLiteral("monospace"));
        mono.setStyleHint(QFont::Monospace);
        mono.setPixelSize(15);
        m_text->setFont(mono);
        m_text->setFixedHeight(110);
        l->addWidget(m_text);
        m_bar = new QProgressBar;
        m_bar->setRange(0, kRevealSeconds);
        m_bar->setValue(kRevealSeconds);
        m_bar->setTextVisible(false);
        l->addWidget(m_bar);
        m_label = ui::label(tr("%1 秒後自動鎖定").arg(m_left), "muted");
        l->addWidget(m_label);
        auto *row = new QHBoxLayout;
        row->addStretch(1);
        auto *lock = ui::button(tr("鎖定"), "primary");
        lock->setDefault(true);
        row->addWidget(lock);
        l->addLayout(row);
        connect(lock, &QPushButton::clicked, this, &QDialog::accept);
        m_timer = new QTimer(this);
        m_timer->setInterval(1000);
        connect(m_timer, &QTimer::timeout, this, [this] {
            if (--m_left <= 0) { accept(); return; }
            m_bar->setValue(m_left);
            m_label->setText(tr("%1 秒後自動鎖定").arg(m_left));
        });
        m_timer->start();
        connect(this, &QDialog::finished, this, [this] { m_text->clear(); });     // drop the plaintext on close
    }
private:
    int m_left;
    QPlainTextEdit *m_text;
    QProgressBar *m_bar;
    QLabel *m_label;
    QTimer *m_timer;
};

TokenDetailDialog::TokenDetailDialog(Vault &vault, const GroupInfo &group, const TokenInfo &token, QWidget *parent)
    : QDialog(parent), m_vault(vault), m_group(group), m_t(token)
{
    setWindowTitle(tr("Token 詳細資料"));
    setModal(true);
    setMinimumWidth(500);
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(24, 22, 24, 20);
    l->setSpacing(12);

    // status header
    auto *top = new QHBoxLayout;
    top->setSpacing(12);
    m_statusIcon = new QLabel;
    m_statusIcon->setFixedSize(40, 40);
    m_statusText = ui::label(QString(), "title");
    top->addWidget(m_statusIcon);
    top->addWidget(m_statusText, 1);
    l->addLayout(top);

    // card: name + note (no verification needed)
    auto *c1 = new QFrame;
    c1->setProperty("card", true);
    auto *f1 = new QVBoxLayout(c1);
    f1->setContentsMargins(16, 14, 16, 14);
    f1->setSpacing(8);
    f1->addWidget(ui::label(tr("名稱"), "muted"));
    m_name = new QLineEdit;
    f1->addWidget(m_name);
    f1->addWidget(ui::label(tr("說明"), "muted"));
    m_note = new QPlainTextEdit;
    m_note->setFixedHeight(76);
    f1->addWidget(m_note);
    auto *saveRow = new QHBoxLayout;
    saveRow->addStretch(1);
    auto *save = ui::button(tr("儲存名稱與說明"));
    saveRow->addWidget(save);
    f1->addLayout(saveRow);
    l->addWidget(c1);

    // card: the secret
    auto *c2 = new QFrame;
    c2->setProperty("card", true);
    auto *f2 = new QVBoxLayout(c2);
    f2->setContentsMargins(16, 14, 16, 14);
    f2->setSpacing(8);
    f2->addWidget(ui::label(tr("Token"), "muted"));
    m_secret = new QLineEdit(kDot);
    m_secret->setReadOnly(true);
    m_show = ui::button(tr("顯示"), "primary");
    auto *srow = new QHBoxLayout;
    srow->addWidget(m_secret, 1);
    srow->addWidget(m_show);
    f2->addLayout(srow);
    l->addWidget(c2);

    // card: dates + expiry
    auto *c3 = new QFrame;
    c3->setProperty("card", true);
    auto *f3 = new QVBoxLayout(c3);
    f3->setContentsMargins(16, 14, 16, 14);
    f3->setSpacing(8);
    m_info = new QLabel;
    m_info->setTextInteractionFlags(Qt::TextSelectableByMouse);
    f3->addWidget(m_info);
    f3->addWidget(ui::label(tr("有效期限"), "muted"));
    m_expiry = new ExpiryPicker;
    auto *erow = new QHBoxLayout;
    auto *ebtn = ui::button(tr("變更期限"));
    erow->addWidget(m_expiry, 1);
    erow->addWidget(ebtn);
    f3->addLayout(erow);
    l->addWidget(c3);

    auto *act = new QHBoxLayout;
    auto *bRevoke = ui::button(tr("作廢"), "danger");
    auto *bRenew = ui::button(tr("更新 Token"));
    auto *bDel = ui::button(tr("刪除"), "danger");
    auto *bClose = ui::button(tr("關閉"));
    act->addWidget(bRevoke);
    act->addWidget(bRenew);
    act->addWidget(bDel);
    act->addStretch(1);
    act->addWidget(bClose);
    l->addLayout(act);
    l->addWidget(ui::label(tr("顯示、作廢、更新、刪除與變更期限都需要重新驗證；名稱與說明不需要。"), "muted", true));

    connect(save, &QPushButton::clicked, this, &TokenDetailDialog::saveTextMeta);
    connect(m_show, &QPushButton::clicked, this, &TokenDetailDialog::showSecret);
    connect(ebtn, &QPushButton::clicked, this, &TokenDetailDialog::changeExpiry);
    connect(bRevoke, &QPushButton::clicked, this, &TokenDetailDialog::revoke);
    connect(bRenew, &QPushButton::clicked, this, &TokenDetailDialog::renew);
    connect(bDel, &QPushButton::clicked, this, &TokenDetailDialog::remove);
    connect(bClose, &QPushButton::clicked, this, &QDialog::accept);

    m_name->setText(m_t.name);
    m_note->setPlainText(m_t.note);
    m_expiry->setValue(m_t.expires);
    refresh();
}

bool TokenDetailDialog::authorize(const QString &reason)
{
    if (!m_vault.isUnlocked()) return false;
    Vault *v = &m_vault;
    AuthDialog dlg(m_vault.mode(), tr("身分驗證"), reason, [v](const Credentials &c) { return v->verify(c); }, this);
    return dlg.exec() == QDialog::Accepted;
}

void TokenDetailDialog::refresh()
{
    const auto st = m_t.status();
    m_statusIcon->setPixmap(icons::statusIcon(st, 40).pixmap(40, 40));
    switch (st) {
    case TokenInfo::Valid: m_statusText->setText(tr("有效")); break;
    case TokenInfo::Expired: m_statusText->setText(tr("已過期")); break;
    case TokenInfo::Revoked: m_statusText->setText(tr("已作廢")); break;
    }
    const QLocale loc;
    QString exp = m_t.expires.isValid() ? loc.toString(m_t.expires.toLocalTime(), QLocale::ShortFormat) : tr("永不過期");
    if (m_t.expires.isValid() && st != TokenInfo::Expired) {
        exp += QStringLiteral(" (%1)").arg(icons::remainingText(m_t));
    }
    m_info->setText(tr("群組：%1\n編號：%2.tkn\n建立：%3\n更新：%4\n到期：%5")
                        .arg(m_group.name, m_t.id,
                             loc.toString(m_t.created.toLocalTime(), QLocale::ShortFormat),
                             loc.toString(m_t.updated.toLocalTime(), QLocale::ShortFormat), exp));
}

void TokenDetailDialog::saveTextMeta()
{
    TokenInfo t = m_t;
    t.name = m_name->text();
    t.note = m_note->toPlainText();
    const Result r = m_vault.updateTokenMeta(t);
    if (!r) { QMessageBox::warning(this, windowTitle(), r.error); return; }
    m_t.name = t.name.trimmed();
    m_t.note = t.note;
    m_t.updated = QDateTime::currentDateTimeUtc();
    m_changed = true;
    refresh();
}

void TokenDetailDialog::showSecret()
{
    SecureBytes out;
    Vault *v = &m_vault;
    const QString gid = m_t.groupId, id = m_t.id;
    SecureBytes *outp = &out;
    AuthDialog dlg(m_vault.mode(), tr("身分驗證"), tr("需要驗證才能顯示 Token。"),
                   [v, gid, id, outp](const Credentials &c) { return v->revealToken(gid, id, c, outp); }, this);
    if (dlg.exec() != QDialog::Accepted) return;
    RevealDialog reveal(m_t.name, QString::fromUtf8(out.data()), this);
    reveal.exec();
}

void TokenDetailDialog::changeExpiry()
{
    const QDateTime e = m_expiry->value();
    if (e.isValid() && e <= QDateTime::currentDateTimeUtc()) {
        QMessageBox::warning(this, windowTitle(), tr("有效期限必須晚於現在"));
        return;
    }
    if (!authorize(tr("變更有效期限需要驗證。"))) return;
    TokenInfo t = m_t;
    t.expires = e;
    const Result r = m_vault.updateTokenMeta(t);
    if (!r) { QMessageBox::warning(this, windowTitle(), r.error); return; }
    m_t.expires = e;
    m_changed = true;
    refresh();
}

void TokenDetailDialog::revoke()
{
    if (m_t.revoked) { QMessageBox::information(this, windowTitle(), tr("這個 Token 已經是作廢狀態。")); return; }
    if (QMessageBox::question(this, tr("作廢 Token"),
                              tr("確定要將「%1」標記為作廢嗎？\n（Token 內容仍會保留，之後可以用「更新 Token」恢復。）").arg(m_t.name))
        != QMessageBox::Yes)
        return;
    if (!authorize(tr("作廢 Token 需要驗證。"))) return;
    TokenInfo t = m_t;
    t.revoked = true;
    const Result r = m_vault.updateTokenMeta(t);
    if (!r) { QMessageBox::warning(this, windowTitle(), r.error); return; }
    m_t.revoked = true;
    m_changed = true;
    refresh();
}

void TokenDetailDialog::renew()
{
    RenewDialog d(m_t.name, this);
    if (d.exec() != QDialog::Accepted) return;
    if (!authorize(tr("更新 Token 需要驗證。"))) return;
    const Result r = m_vault.replaceTokenSecret(m_t.groupId, m_t.id, d.token(), d.changeExpiry(), d.newExpires(), true);
    if (!r) { QMessageBox::warning(this, windowTitle(), r.error); return; }
    m_t.revoked = false;
    if (d.changeExpiry()) { m_t.expires = d.newExpires(); m_expiry->setValue(m_t.expires); }
    m_t.updated = QDateTime::currentDateTimeUtc();
    m_changed = true;
    refresh();
}

void TokenDetailDialog::remove()
{
    if (QMessageBox::warning(this, tr("刪除 Token"),
                             tr("確定要永久刪除「%1」嗎？此動作無法復原。").arg(m_t.name),
                             QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel)
        != QMessageBox::Yes)
        return;
    if (!authorize(tr("刪除 Token 需要驗證。"))) return;
    const Result r = m_vault.deleteToken(m_t.groupId, m_t.id);
    if (!r) { QMessageBox::warning(this, windowTitle(), r.error); return; }
    m_changed = true;
    QDialog::accept();
}
