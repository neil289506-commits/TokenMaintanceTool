#include "backupui.h"
#include "authdialogs.h"
#include "busy.h"
#include "icons.h"
#include "theme.h"
#include "uihelpers.h"
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDate>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <memory>

using namespace tv;

// ---------------------------------------------------------------------------------------------------
BackupKeysDialog::BackupKeysDialog(const QString &archivePath, const BackupKeys &keys, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("匯出完成"));
    setModal(true);
    setMinimumWidth(540);
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(24, 22, 24, 20);
    l->setSpacing(12);
    l->addWidget(ui::header(ui::lockBadge(), tr("備份已建立"), QDir::toNativeSeparators(archivePath)));

    auto addKey = [&](const QString &title, const QString &desc, const QString &key) {
        auto *card = new QFrame;
        card->setProperty("card", true);
        auto *cl = new QVBoxLayout(card);
        cl->setContentsMargins(16, 14, 16, 14);
        cl->setSpacing(6);
        QLabel *t = ui::label(title);
        QFont f = t->font();
        f.setBold(true);
        t->setFont(f);
        cl->addWidget(t);
        cl->addWidget(ui::label(desc, "muted", true));
        auto *row = new QHBoxLayout;
        const QString shown = formatBackupKey(key);                       // aaaaaa-bbbbbb-... (8 groups of 6)
        auto *e = new QPlainTextEdit(shown.left(27) + QLatin1Char('\n') + shown.mid(28));   // 4 groups per line, always fully visible
        e->setReadOnly(true);
        e->setObjectName(QStringLiteral("backupKey"));
        e->setProperty("backupKey", key);
        QFont mono(QStringLiteral("monospace"));
        mono.setStyleHint(QFont::Monospace);
        mono.setPixelSize(15);
        e->setFont(mono);
        e->setLineWrapMode(QPlainTextEdit::NoWrap);
        e->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        e->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        e->setFixedHeight(QFontMetrics(mono).lineSpacing() * 2 + 22);
        auto *copy = ui::button(tr("複製"));
        connect(copy, &QPushButton::clicked, this, [key] { QApplication::clipboard()->setText(key); });
        row->addWidget(e, 1);
        row->addWidget(copy, 0, Qt::AlignTop);
        cl->addLayout(row);
        l->addWidget(card);
    };
    addKey(tr("第一組金鑰：壓縮包"), tr("用來解開 .zip 壓縮包（也可以用 7-Zip 直接開啟）。"), keys.zipKey);
    addKey(tr("第二組金鑰：RSA"), tr("用來解開壓縮包裡的 RSA-8192 私鑰，匯入時需要。"), keys.rsaKey);

    l->addWidget(ui::label(tr("• 這兩組金鑰只會顯示這一次，之後無法再查看或找回。\n"
                              "• 請分開保存，不要和壓縮包放在一起。\n"
                              "• 壓縮包內含目前的密碼與 TOTP 金鑰（皆已加密）。同時取得壓縮包與兩組金鑰的人，等於取得全部資料。\n"
                              "• 壓縮包的檔案清單會以明文顯示群組與 Token 的名稱（ZIP 格式的限制），內容則已加密。"),
                           "warn", true));
    m_ack = new QCheckBox(tr("我已安全保存這兩組金鑰"));
    l->addWidget(m_ack);
    auto *row = new QHBoxLayout;
    row->addStretch(1);
    m_done = ui::button(tr("完成"), "primary");
    m_done->setEnabled(false);
    row->addWidget(m_done);
    l->addLayout(row);
    connect(m_ack, &QCheckBox::toggled, m_done, &QPushButton::setEnabled);
    connect(m_done, &QPushButton::clicked, this, &QDialog::accept);
    ui::fitHeight(this);
}

void BackupKeysDialog::reject()
{
    if (m_ack->isChecked()) QDialog::reject();       // Esc / close is ignored until the keys were acknowledged
}

// ---------------------------------------------------------------------------------------------------
ImportDialog::ImportDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("匯入備份"));
    setModal(true);
    setMinimumWidth(500);
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(24, 22, 24, 20);
    l->setSpacing(10);
    l->addWidget(ui::header(ui::badge(QStringLiteral("↓")), tr("匯入備份"),
                            tr("選擇備份壓縮包，並輸入匯出時產生的兩組金鑰。")));
    l->addWidget(ui::label(tr("備份壓縮包"), "muted"));
    auto *row = new QHBoxLayout;
    m_path = new QLineEdit;
    m_path->setPlaceholderText(tr("選擇 .zip 備份檔"));
    auto *browse = ui::button(tr("瀏覽…"));
    row->addWidget(m_path, 1);
    row->addWidget(browse);
    l->addLayout(row);
    connect(browse, &QPushButton::clicked, this, [this] {
        const QString f = QFileDialog::getOpenFileName(this, tr("選擇備份壓縮包"), QString(), tr("ZIP 壓縮包 (*.zip);;所有檔案 (*)"));
        if (!f.isEmpty()) m_path->setText(f);
    });
    l->addWidget(ui::label(tr("第一組金鑰（壓縮包）"), "muted"));
    m_key1 = new QLineEdit;
    m_key1->setEchoMode(QLineEdit::Password);
    m_key1->setPlaceholderText(tr("48 位英文數字"));
    l->addWidget(m_key1);
    l->addWidget(ui::label(tr("第二組金鑰（RSA）"), "muted"));
    m_key2 = new QLineEdit;
    m_key2->setEchoMode(QLineEdit::Password);
    m_key2->setPlaceholderText(tr("48 位英文數字"));
    l->addWidget(m_key2);
    auto *show = new QCheckBox(tr("顯示金鑰"));
    connect(show, &QCheckBox::toggled, this, [this](bool on) {
        m_key1->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
        m_key2->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
    });
    l->addWidget(show);
    l->addWidget(ui::label(tr("匯入後會還原備份當時的密碼、鑰匙檔雜湊與 TOTP。若有使用鑰匙檔，請繼續使用原本那個檔案。"), "muted", true));
    m_err = ui::label(QString(), "error", true);
    m_err->hide();
    l->addWidget(m_err);
    l->addLayout(ui::footer(this, nullptr, nullptr, tr("匯入")));
    m_path->setFocus();
    ui::fitHeight(this);
}

void ImportDialog::accept()
{
    const QString path = m_path->text().trimmed();
    if (path.isEmpty() || !QFileInfo(path).isFile()) {
        m_err->setText(tr("請選擇備份壓縮包"));
        m_err->show();
        return;
    }
    const QString k1 = m_key1->text(), k2 = m_key2->text();
    auto data = std::make_shared<BackupData>();
    const Result r = runBusy<Result>(this, tr("正在解密並檢查備份…"), [path, k1, k2, data] { return readBackup(path, k1, k2, data.get()); });
    if (!r) {
        m_err->setText(r.error);
        m_err->show();
        return;
    }
    m_data = *data;
    QDialog::accept();
}

// ---------------------------------------------------------------------------------------------------
WipeConfirmDialog::WipeConfirmDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("確認清除並匯入"));
    setModal(true);
    setMinimumWidth(460);
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(24, 22, 24, 20);
    l->setSpacing(12);
    l->addWidget(ui::header(ui::badge(QStringLiteral("!")), tr("匯入會刪除目前所有資料"),
                            tr("現有的所有群組、Token、密碼／鑰匙檔與 TOTP 設定都會被永久刪除，改用備份裡的內容。")));
    l->addWidget(ui::label(tr("這個動作無法復原。請輸入下面這句英文以確認："), "error", true));
    QFont mono(QStringLiteral("monospace"));
    mono.setStyleHint(QFont::Monospace);
    auto *ph = ui::label(phrase(), "title");
    ph->setFont(mono);
    l->addWidget(ph);
    auto *edit = new QLineEdit;
    edit->setPlaceholderText(phrase());
    l->addWidget(edit);
    QPushButton *ok = nullptr;
    l->addLayout(ui::footer(this, &ok, nullptr, tr("清除並繼續")));
    ok->setProperty("primary", false);
    ok->setProperty("danger", true);
    ok->setEnabled(false);
    ui::repolish(ok);
    connect(edit, &QLineEdit::textChanged, this, [ok](const QString &t) { ok->setEnabled(t == WipeConfirmDialog::phrase()); });
    edit->setFocus();
    ui::fitHeight(this);
}

// ---------------------------------------------------------------------------------------------------
bool runExportFlow(Vault &vault, QWidget *parent)
{
    // RSA-8192 key generation takes seconds to a minute: start it now so it overlaps with the verification below.
    auto rsaFuture = QtConcurrent::run([] {
        PKey k = generateBackupRsa();
        return std::shared_ptr<EVP_PKEY>(k.release(), EVP_PKEY_free);
    });

    auto data = std::make_shared<BackupData>();
    const Vault *v = &vault;
    AuthDialog verify(vault.mode(), QObject::tr("驗證身分"),
                      QObject::tr("匯出會把所有 Token 與驗證資訊打包，請先驗證身分。"),
                      [v, data](const Credentials &c) { return v->collectBackup(c, data.get()); }, parent);
    if (verify.exec() != QDialog::Accepted) return false;

    const QString def = QDir::homePath() + QStringLiteral("/TokenVault-backup-") + QDate::currentDate().toString(QStringLiteral("yyyyMMdd")) + QStringLiteral(".zip");
    QString path = QFileDialog::getSaveFileName(parent, QObject::tr("儲存備份壓縮包"), def, QObject::tr("ZIP 壓縮包 (*.zip)"));
    if (path.isEmpty()) return false;
    if (!path.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) path += QStringLiteral(".zip");

    auto keys = std::make_shared<BackupKeys>();
    const Result r = runBusy<Result>(parent, QObject::tr("正在產生 RSA-8192 金鑰並加密備份，可能需要數十秒…"),
                                     [rsaFuture, data, path, keys]() mutable {
                                         auto rsa = rsaFuture.result();                 // waits if keygen is still running
                                         return writeBackup(*data, rsa.get(), path, keys.get());
                                     });
    if (!r) {
        QMessageBox::warning(parent, QObject::tr("匯出備份"), r.error);
        return false;
    }
    BackupKeysDialog dlg(path, *keys, parent);
    dlg.exec();
    return true;
}

bool runImportFlow(Vault &vault, QWidget *parent)
{
    ImportDialog dlg(parent);
    if (dlg.exec() != QDialog::Accepted) return false;
    auto data = std::make_shared<BackupData>(dlg.data());
    Vault *v = &vault;
    const Result r = runBusy<Result>(parent, QObject::tr("正在還原資料，請稍候…"), [v, data] { return v->importBackup(*data); });
    if (!r) {
        QMessageBox::warning(parent, QObject::tr("匯入備份"), r.error);
        return false;
    }
    return true;
}

bool runReplaceFromBackupFlow(Vault &vault, QWidget *parent)
{
    // Order matters: confirm -> read + validate the archive COMPLETELY -> only then delete anything.
    WipeConfirmDialog confirm(parent);
    if (confirm.exec() != QDialog::Accepted) return false;
    return runImportFlow(vault, parent);
}
