#include "editdialogs.h"
#include "icons.h"
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

using namespace tv;

namespace {
QDialogButtonBox *okCancel(QDialog *d)
{
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bb->button(QDialogButtonBox::Ok)->setText(QObject::tr("確定"));
    bb->button(QDialogButtonBox::Cancel)->setText(QObject::tr("取消"));
    QObject::connect(bb, &QDialogButtonBox::accepted, d, &QDialog::accept);
    QObject::connect(bb, &QDialogButtonBox::rejected, d, &QDialog::reject);
    return bb;
}

QLineEdit *makeTokenEdit(QLabel **lenLabel)
{
    auto *t = new QLineEdit;
    t->setEchoMode(QLineEdit::Password);
    t->setMaxLength(65536);
    t->setPlaceholderText(QObject::tr("貼上或輸入 Token（輸入後會隱藏）"));
    t->setClearButtonEnabled(false);
    auto *len = new QLabel;
    len->setStyleSheet("color:gray");
    QObject::connect(t, &QLineEdit::textChanged, len, [len](const QString &s) {
        len->setText(s.isEmpty() ? QString() : QObject::tr("已輸入 %1 個字元").arg(s.size()));
    });
    *lenLabel = len;
    return t;
}
} // namespace

// ---------------------------------------------------------------------------
ExpiryPicker::ExpiryPicker(bool allowKeep, QWidget *parent) : QWidget(parent), m_allowKeep(allowKeep)
{
    auto *l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_combo = new QComboBox;
    if (allowKeep) m_combo->addItem(tr("保持原有期限"), -2);
    m_combo->addItem(tr("自訂"), -1);
    m_combo->addItem(tr("1 天"), 1);
    m_combo->addItem(tr("7 天"), 7);
    m_combo->addItem(tr("14 天"), 14);
    m_combo->addItem(tr("30 天"), 30);
    m_combo->addItem(tr("1 年"), 365);
    m_combo->addItem(tr("永不過期"), 0);
    m_custom = new QDateTimeEdit(QDateTime::currentDateTime().addDays(7));
    m_custom->setCalendarPopup(true);
    m_custom->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    m_custom->setMinimumDateTime(QDateTime::currentDateTime());
    l->addWidget(m_combo);
    l->addWidget(m_custom, 1);
    connect(m_combo, &QComboBox::currentIndexChanged, this, [this] { onChanged(); });
    selectPreset(allowKeep ? -2 : 7);
}

void ExpiryPicker::selectPreset(int days)
{
    const int i = m_combo->findData(days);
    if (i >= 0) m_combo->setCurrentIndex(i);
    onChanged();
}

void ExpiryPicker::onChanged() { m_custom->setVisible(m_combo->currentData().toInt() == -1); }

bool ExpiryPicker::keepSelected() const { return m_combo->currentData().toInt() == -2; }

QDateTime ExpiryPicker::value() const
{
    const int d = m_combo->currentData().toInt();
    if (d == -2 || d == 0) return {};
    if (d == -1) return m_custom->dateTime().toUTC();
    const QDateTime now = QDateTime::currentDateTime();
    return (d == 365 ? now.addYears(1) : now.addDays(d)).toUTC();
}

void ExpiryPicker::setValue(const QDateTime &utc)
{
    if (!utc.isValid()) { selectPreset(0); return; }
    m_custom->setMinimumDateTime(QDateTime::fromSecsSinceEpoch(0));   // allow showing past dates
    m_custom->setDateTime(utc.toLocalTime());
    selectPreset(-1);
}

// ---------------------------------------------------------------------------
GroupDialog::GroupDialog(const GroupInfo &g, bool editing, QWidget *parent) : QDialog(parent), m_id(g.id)
{
    setWindowTitle(editing ? tr("編輯群組") : tr("建立群組"));
    setModal(true);
    setMinimumWidth(420);
    auto *l = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    m_name = new QLineEdit(g.name);
    m_name->setPlaceholderText(tr("群組名稱"));
    m_note = new QPlainTextEdit(g.note);
    m_note->setPlaceholderText(tr("說明（選填）"));
    m_note->setFixedHeight(70);
    form->addRow(tr("名稱"), m_name);
    form->addRow(tr("說明"), m_note);
    l->addLayout(form);

    int shape = 0, color = 0;
    if (!g.icon.isEmpty()) icons::parseSpec(g.icon, &shape, &color);
    m_shapes = new QButtonGroup(this);
    m_colors = new QButtonGroup(this);
    l->addWidget(new QLabel(tr("圖示")));
    auto *srow = new QHBoxLayout;
    for (int i = 0; i < icons::shapeCount(); ++i) {
        auto *b = new QToolButton;
        b->setCheckable(true);
        b->setAutoRaise(true);
        b->setIconSize(QSize(28, 28));
        b->setToolTip(icons::shapeName(i));
        m_shapes->addButton(b, i);
        srow->addWidget(b);
    }
    l->addLayout(srow);
    l->addWidget(new QLabel(tr("顏色")));
    auto *crow = new QHBoxLayout;
    for (int i = 0; i < icons::colorCount(); ++i) {
        auto *b = new QToolButton;
        b->setCheckable(true);
        b->setIconSize(QSize(20, 20));
        b->setIcon(icons::shapeIcon(0, i, 20));
        m_colors->addButton(b, i);
        crow->addWidget(b);
    }
    l->addLayout(crow);
    m_shapes->button(shape)->setChecked(true);
    m_colors->button(color)->setChecked(true);
    connect(m_colors, &QButtonGroup::idClicked, this, [this] { refreshShapeIcons(); });
    refreshShapeIcons();
    l->addWidget(okCancel(this));
    m_name->setFocus();
}

void GroupDialog::refreshShapeIcons()
{
    for (int i = 0; i < icons::shapeCount(); ++i)
        static_cast<QToolButton *>(m_shapes->button(i))->setIcon(icons::shapeIcon(i, m_colors->checkedId(), 28));
}

GroupInfo GroupDialog::result() const
{
    return {m_id, m_name->text().trimmed(), m_note->toPlainText(), icons::makeSpec(m_shapes->checkedId(), m_colors->checkedId())};
}

void GroupDialog::accept()
{
    if (m_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("請輸入群組名稱"));
        return;
    }
    QDialog::accept();
}

// ---------------------------------------------------------------------------
TokenDialog::TokenDialog(const QList<GroupInfo> &groups, const QString &preselect, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("建立 Token"));
    setModal(true);
    setMinimumWidth(440);
    auto *l = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    m_group = new QComboBox;
    for (const GroupInfo &g : groups) {
        m_group->addItem(icons::groupIcon(g.icon, 20), g.name, g.id);
        if (g.id == preselect) m_group->setCurrentIndex(m_group->count() - 1);
    }
    m_name = new QLineEdit;
    m_name->setPlaceholderText(tr("Token 名稱"));
    m_note = new QPlainTextEdit;
    m_note->setPlaceholderText(tr("說明（選填）"));
    m_note->setFixedHeight(70);
    m_token = makeTokenEdit(&m_len);
    m_expiry = new ExpiryPicker;
    form->addRow(tr("群組"), m_group);
    form->addRow(tr("名稱"), m_name);
    form->addRow(tr("說明"), m_note);
    form->addRow(tr("Token"), m_token);
    form->addRow(QString(), m_len);
    form->addRow(tr("有效期限"), m_expiry);
    l->addLayout(form);
    l->addWidget(okCancel(this));
    m_name->setFocus();
}

QString TokenDialog::groupId() const { return m_group->currentData().toString(); }
QString TokenDialog::name() const { return m_name->text().trimmed(); }
QString TokenDialog::note() const { return m_note->toPlainText(); }
QDateTime TokenDialog::expires() const { return m_expiry->value(); }
SecureBytes TokenDialog::token() const { return SecureBytes(m_token->text().toUtf8()); }

void TokenDialog::accept()
{
    if (m_name->text().trimmed().isEmpty()) { QMessageBox::warning(this, windowTitle(), tr("請輸入名稱")); return; }
    if (m_token->text().isEmpty()) { QMessageBox::warning(this, windowTitle(), tr("請輸入 Token")); return; }
    const QDateTime e = m_expiry->value();
    if (e.isValid() && e <= QDateTime::currentDateTimeUtc()) { QMessageBox::warning(this, windowTitle(), tr("有效期限必須晚於現在")); return; }
    QDialog::accept();
}

// ---------------------------------------------------------------------------
RenewDialog::RenewDialog(const QString &tokenName, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("更新 Token"));
    setModal(true);
    setMinimumWidth(420);
    auto *l = new QVBoxLayout(this);
    l->addWidget(new QLabel(tr("輸入「%1」的新 Token，舊的內容會被覆蓋，且會清除「已作廢」狀態。").arg(tokenName)));
    auto *form = new QFormLayout;
    m_token = makeTokenEdit(&m_len);
    m_expiry = new ExpiryPicker(true);
    form->addRow(tr("新 Token"), m_token);
    form->addRow(QString(), m_len);
    form->addRow(tr("有效期限"), m_expiry);
    l->addLayout(form);
    l->addWidget(okCancel(this));
    m_token->setFocus();
}

SecureBytes RenewDialog::token() const { return SecureBytes(m_token->text().toUtf8()); }
bool RenewDialog::changeExpiry() const { return !m_expiry->keepSelected(); }
QDateTime RenewDialog::newExpires() const { return m_expiry->value(); }

void RenewDialog::accept()
{
    if (m_token->text().isEmpty()) { QMessageBox::warning(this, windowTitle(), tr("請輸入 Token")); return; }
    QDialog::accept();
}
