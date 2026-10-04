#include "tokendetail.h"
#include "authdialogs.h"
#include "editdialogs.h"
#include "icons.h"
#include "uihelpers.h"
#include <QFrame>
#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

using namespace tv;

static constexpr int kRevealSeconds = 30;
static const QString kDot = QStringLiteral("●");

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
    m_copy = ui::button(tr("複製"));
    m_copy->setEnabled(false);
    auto *srow = new QHBoxLayout;
    srow->addWidget(m_secret, 1);
    srow->addWidget(m_show);
    srow->addWidget(m_copy);
    f2->addLayout(srow);
    m_countdown = ui::label(QString(), "muted");
    f2->addWidget(m_countdown);
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

    m_hideTimer = new QTimer(this);
    m_hideTimer->setInterval(1000);
    connect(m_hideTimer, &QTimer::timeout, this, &TokenDetailDialog::tick);

    connect(save, &QPushButton::clicked, this, &TokenDetailDialog::saveTextMeta);
    connect(m_show, &QPushButton::clicked, this, [this] { m_revealed.isEmpty() ? showSecret() : hideSecret(); });
    connect(m_copy, &QPushButton::clicked, this, &TokenDetailDialog::copySecret);
    connect(ebtn, &QPushButton::clicked, this, &TokenDetailDialog::changeExpiry);
    connect(bRevoke, &QPushButton::clicked, this, &TokenDetailDialog::revoke);
    connect(bRenew, &QPushButton::clicked, this, &TokenDetailDialog::renew);
    connect(bDel, &QPushButton::clicked, this, &TokenDetailDialog::remove);
    connect(bClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(this, &QDialog::finished, this, [this] { hideSecret(); });

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
        exp += QStringLiteral("（%1）").arg(icons::remainingText(m_t));
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
    m_revealed = out;
    m_secret->setText(QString::fromUtf8(m_revealed.data()));
    m_secret->setCursorPosition(0);
    m_show->setText(tr("隱藏"));
    m_copy->setEnabled(true);
    m_left = kRevealSeconds;
    m_countdown->setText(tr("%1 秒後自動隱藏").arg(m_left));
    m_hideTimer->start();
}

void TokenDetailDialog::hideSecret()
{
    m_hideTimer->stop();
    m_revealed.wipe();
    m_secret->setText(kDot);
    m_show->setText(tr("顯示"));
    m_copy->setEnabled(false);
    m_countdown->clear();
}

void TokenDetailDialog::tick()
{
    if (--m_left <= 0) hideSecret();
    else m_countdown->setText(tr("%1 秒後自動隱藏").arg(m_left));
}

void TokenDetailDialog::copySecret()
{
    if (m_revealed.isEmpty()) return;
    const QString text = QString::fromUtf8(m_revealed.data());
    auto *md = new QMimeData;
    md->setText(text);
    // Best effort: ask clipboard managers / history (Windows, macOS) not to record this.
    md->setData(QStringLiteral("ExcludeClipboardContentFromMonitorProcessing"), QByteArray(4, '\0'));
    md->setData(QStringLiteral("org.nspasteboard.ConcealedType"), QByteArray("1"));
    QApplication::clipboard()->setMimeData(md);
    const QByteArray fp = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256);
    m_countdown->setText(tr("已複製，30 秒後自動清除剪貼簿"));
    QTimer::singleShot(30000, qApp, [fp] {
        const QString cur = QApplication::clipboard()->text();
        if (QCryptographicHash::hash(cur.toUtf8(), QCryptographicHash::Sha256) == fp)
            QApplication::clipboard()->clear();
    });
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
    hideSecret();
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
    hideSecret();
    QDialog::accept();
}
