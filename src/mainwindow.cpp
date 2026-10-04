#include "mainwindow.h"
#include "authdialogs.h"
#include "editdialogs.h"
#include "icons.h"
#include "theme.h"
#include "tokendetail.h"
#include "uihelpers.h"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

using namespace tv;

namespace {

enum Roles {
    RoleGroup = Qt::UserRole + 1, RoleToken, RoleSub, RoleBadge, RoleBadgeColor, RoleCount
};

QString statusText(const TokenInfo &t)
{
    switch (t.status()) {
    case TokenInfo::Revoked: return QObject::tr("已作廢");
    case TokenInfo::Expired: return QObject::tr("已過期");
    default: break;
    }
    if (!t.expires.isValid()) return QObject::tr("永不過期");
    return icons::remainingText(t);
}

QColor statusColor(const TokenInfo &t)
{
    const auto &k = theme::c();
    if (t.status() != TokenInfo::Valid) return k.bad;
    if (!t.expires.isValid()) return k.muted;
    return t.daysLeft() < 3 ? k.warn : k.ok;
}

// ---- sidebar row --------------------------------------------------------------------------------
class GroupDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override { return {200, 48}; }
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        const auto &k = theme::c();
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const QRect r = opt.rect.adjusted(8, 2, -8, -2);
        const bool sel = opt.state & QStyle::State_Selected, hov = opt.state & QStyle::State_MouseOver;
        if (sel || hov) {
            p->setPen(Qt::NoPen);
            p->setBrush(sel ? k.cardSelected : k.cardHover);
            p->drawRoundedRect(r, 10, 10);
        }
        const QIcon ic = qvariant_cast<QIcon>(idx.data(Qt::DecorationRole));
        ic.paint(p, QRect(r.left() + 10, r.center().y() - 14, 28, 28));
        QFont f = opt.font;
        f.setPixelSize(14);
        f.setBold(sel);
        p->setFont(f);
        p->setPen(k.text);
        const QString cnt = idx.data(RoleCount).toString();
        QFontMetrics cm(opt.font);
        const int cw = cm.horizontalAdvance(cnt) + 4;
        const QRect tr(r.left() + 48, r.top(), r.width() - 48 - cw - 12, r.height());
        p->drawText(tr, Qt::AlignVCenter | Qt::AlignLeft, QFontMetrics(f).elidedText(idx.data(Qt::DisplayRole).toString(), Qt::ElideRight, tr.width()));
        QFont cf = opt.font;
        cf.setPixelSize(12);
        p->setFont(cf);
        p->setPen(k.muted);
        p->drawText(QRect(r.right() - cw - 10, r.top(), cw, r.height()), Qt::AlignVCenter | Qt::AlignRight, cnt);
        p->restore();
    }
};

// ---- token card ------------------------------------------------------------------------------------
class TokenDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override { return {300, 68}; }
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        const auto &k = theme::c();
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const QRect r = opt.rect.adjusted(2, 4, -2, -4);
        const bool sel = opt.state & QStyle::State_Selected, hov = opt.state & QStyle::State_MouseOver;
        p->setPen(QPen(sel ? k.accent : k.border, 1));
        p->setBrush(sel ? k.cardSelected : (hov ? k.cardHover : k.card));
        p->drawRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);

        const QIcon ic = qvariant_cast<QIcon>(idx.data(Qt::DecorationRole));
        ic.paint(p, QRect(r.left() + 16, r.center().y() - 13, 26, 26));

        // expiry chip
        const QString badge = idx.data(RoleBadge).toString();
        const QColor bc = qvariant_cast<QColor>(idx.data(RoleBadgeColor));
        QFont bf = opt.font;
        bf.setPixelSize(12);
        bf.setBold(true);
        const int bw = QFontMetrics(bf).horizontalAdvance(badge) + 22;
        const QRect br(r.right() - 16 - bw, r.center().y() - 12, bw, 24);
        QColor soft = bc;
        soft.setAlpha(34);
        p->setPen(Qt::NoPen);
        p->setBrush(soft);
        p->drawRoundedRect(br, 12, 12);
        p->setFont(bf);
        p->setPen(bc);
        p->drawText(br, Qt::AlignCenter, badge);

        const int tx = r.left() + 56, tw = br.left() - 14 - tx;
        QFont nf = opt.font;
        nf.setPixelSize(14);
        nf.setBold(true);
        p->setFont(nf);
        p->setPen(k.text);
        p->drawText(QRect(tx, r.top() + 11, tw, 22), Qt::AlignVCenter,
                    QFontMetrics(nf).elidedText(idx.data(Qt::DisplayRole).toString(), Qt::ElideRight, tw));
        QFont sf = opt.font;
        sf.setPixelSize(12);
        p->setFont(sf);
        p->setPen(k.muted);
        p->drawText(QRect(tx, r.top() + 34, tw, 18), Qt::AlignVCenter,
                    QFontMetrics(sf).elidedText(idx.data(RoleSub).toString(), Qt::ElideRight, tw));
        p->restore();
    }
};

} // namespace

// ---------------------------------------------------------------------------------------------------
MainWindow::MainWindow(Vault &vault, QWidget *parent) : QMainWindow(parent), m_vault(vault)
{
    setWindowTitle(tr("TokenVault"));
    setWindowIcon(icons::appIcon());
    resize(980, 640);
    setMinimumSize(720, 460);
    buildUi();

    QSettings s;
    m_autoLockMin = s.value("autoLockMinutes", 5).toInt();
    m_idle = new QTimer(this);
    m_idle->setSingleShot(true);
    connect(m_idle, &QTimer::timeout, this, &MainWindow::lockNow);
    qApp->installEventFilter(this);
    m_expiryTimer = new QTimer(this);
    m_expiryTimer->setInterval(60 * 1000);
    connect(m_expiryTimer, &QTimer::timeout, this, [this] { if (m_vault.isUnlocked() && !m_locking) checkExpiry(false); });
    m_expiryTimer->start();

    reload();
    restartIdle();
    QTimer::singleShot(0, this, [this] { placeFab(); checkExpiry(true); });
}

void MainWindow::buildUi()
{
    auto *central = new QWidget;
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---- top bar
    auto *bar = new QWidget;
    auto *bl = new QHBoxLayout(bar);
    bl->setContentsMargins(18, 12, 14, 12);
    bl->setSpacing(10);
    auto *logo = new QLabel;
    logo->setPixmap(icons::appIcon().pixmap(28, 28));
    auto *name = new QLabel(tr("TokenVault"));
    QFont nf = name->font();
    nf.setPixelSize(17);
    nf.setBold(true);
    name->setFont(nf);
    m_search = new QLineEdit;
    m_search->setPlaceholderText(tr("搜尋名稱、說明或群組…"));
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumWidth(260);
    auto *lock = new QToolButton;
    lock->setText(tr("鎖定"));
    lock->setProperty("ghost", true);
    lock->setCursor(Qt::PointingHandCursor);
    lock->setToolTip(tr("立即鎖定 (Ctrl+L)"));
    auto *more = new QToolButton;
    more->setText(tr("設定"));
    more->setProperty("ghost", true);
    more->setCursor(Qt::PointingHandCursor);
    bl->addWidget(logo);
    bl->addWidget(name);
    bl->addStretch(1);
    bl->addWidget(m_search);
    bl->addWidget(lock);
    bl->addWidget(more);
    root->addWidget(bar);
    auto *line = new QFrame;
    line->setFixedHeight(1);
    line->setStyleSheet(QStringLiteral("background:%1").arg(theme::c().border.name()));
    root->addWidget(line);

    // ---- body
    m_stack = new QStackedWidget;
    root->addWidget(m_stack, 1);

    auto *empty = new QWidget;
    auto *el = new QVBoxLayout(empty);
    el->setAlignment(Qt::AlignCenter);
    auto *eic = new QLabel;
    eic->setPixmap(ui::lockBadge(72));
    eic->setAlignment(Qt::AlignCenter);
    el->addWidget(eic);
    el->addSpacing(8);
    el->addWidget(ui::label(tr("還沒有任何群組"), "title"), 0, Qt::AlignCenter);
    el->addWidget(ui::label(tr("點左下角的 ＋ 建立第一個群組，再把 Token 放進去"), "subtitle"), 0, Qt::AlignCenter);
    m_stack->addWidget(empty);

    auto *split = new QSplitter;
    split->setChildrenCollapsible(false);
    split->setHandleWidth(1);

    auto *side = new QWidget;
    side->setMinimumWidth(210);
    side->setMaximumWidth(320);
    auto *sl = new QVBoxLayout(side);
    sl->setContentsMargins(0, 10, 0, 84);       // bottom room for the floating +
    sl->setSpacing(4);
    auto *sh = ui::label(tr("群組"), "muted");
    sh->setContentsMargins(18, 4, 0, 4);
    sl->addWidget(sh);
    m_groups = new QListWidget;
    m_groups->setItemDelegate(new GroupDelegate(m_groups));
    m_groups->setMouseTracking(true);
    m_groups->setContextMenuPolicy(Qt::CustomContextMenu);
    m_groups->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    sl->addWidget(m_groups, 1);
    split->addWidget(side);

    auto *right = new QWidget;
    auto *rl = new QVBoxLayout(right);
    rl->setContentsMargins(24, 18, 24, 16);
    rl->setSpacing(10);
    auto *hr = new QHBoxLayout;
    auto *tc = new QVBoxLayout;
    tc->setSpacing(0);
    m_title = ui::label(QString(), "title");
    m_subtitle = ui::label(QString(), "subtitle");
    tc->addWidget(m_title);
    tc->addWidget(m_subtitle);
    hr->addLayout(tc, 1);
    m_editGroup = ui::button(tr("編輯群組"));
    m_delGroup = ui::button(tr("刪除群組"), "danger");
    hr->addWidget(m_editGroup, 0, Qt::AlignTop);
    hr->addWidget(m_delGroup, 0, Qt::AlignTop);
    rl->addLayout(hr);

    m_rightStack = new QStackedWidget;
    m_emptyMsg = ui::label(QString(), "subtitle", true);
    m_emptyMsg->setAlignment(Qt::AlignCenter);
    m_rightStack->addWidget(m_emptyMsg);
    m_tokens = new QListWidget;
    m_tokens->setItemDelegate(new TokenDelegate(m_tokens));
    m_tokens->setMouseTracking(true);
    m_tokens->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tokens->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_rightStack->addWidget(m_tokens);
    rl->addWidget(m_rightStack, 1);
    split->addWidget(right);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setSizes({250, 730});
    m_stack->addWidget(split);
    setCentralWidget(central);

    // ---- floating action button (bottom-left)
    m_fab = new QToolButton(m_stack);
    m_fab->setText(QStringLiteral("+"));
    m_fab->setProperty("fab", true);
    m_fab->setFixedSize(56, 56);
    m_fab->setCursor(Qt::PointingHandCursor);
    m_fab->setToolTip(tr("新增"));
    connect(m_fab, &QToolButton::clicked, this, &MainWindow::showAddMenu);
    m_stack->installEventFilter(this);

    auto *lockAct = new QAction(tr("立即鎖定"), this);
    lockAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    addAction(lockAct);
    connect(lockAct, &QAction::triggered, this, &MainWindow::lockNow);
    connect(lock, &QToolButton::clicked, this, &MainWindow::lockNow);
    connect(more, &QToolButton::clicked, this, &MainWindow::showSettingsMenu);

    connect(m_groups, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *it) {
        if (!it) return;
        m_selectedGroup = it->data(RoleGroup).toString();
        populateTokens();
        updateHeader();
    });
    connect(m_groups, &QListWidget::customContextMenuRequested, this, &MainWindow::groupMenu);
    connect(m_tokens, &QListWidget::itemActivated, this, [this](QListWidgetItem *it) {
        openToken(it->data(RoleGroup).toString(), it->data(RoleToken).toString());
    });
    connect(m_tokens, &QListWidget::customContextMenuRequested, this, &MainWindow::tokenMenu);
    connect(m_search, &QLineEdit::textChanged, this, [this] { populateTokens(); updateHeader(); });
    connect(m_editGroup, &QPushButton::clicked, this, [this] { editGroup(m_selectedGroup); });
    connect(m_delGroup, &QPushButton::clicked, this, [this] { deleteGroup(m_selectedGroup); });
}

QString MainWindow::currentGroupId() const { return m_selectedGroup; }

// ---------------------------------------------------------------------------------------------------
void MainWindow::reload()
{
    QStringList warn;
    m_groupData = m_vault.groups(&warn);
    m_tokenData.clear();
    for (const GroupInfo &g : m_groupData) m_tokenData[g.id] = m_vault.tokens(g.id, &warn);
    bool found = m_selectedGroup.isEmpty();
    for (const GroupInfo &g : m_groupData) found |= (g.id == m_selectedGroup);
    if (!found) m_selectedGroup.clear();
    populateSidebar();
    populateTokens();
    updateHeader();
    m_stack->setCurrentIndex(m_groupData.isEmpty() ? 0 : 1);
    placeFab();
    if (!warn.isEmpty()) QMessageBox::warning(this, tr("部分資料無法讀取"), warn.join('\n'));
}

void MainWindow::populateSidebar()
{
    const QSignalBlocker blk(m_groups);
    m_groups->clear();
    int total = 0;
    for (const GroupInfo &g : m_groupData) total += int(m_tokenData.value(g.id).size());
    auto *all = new QListWidgetItem(icons::shapeIcon(5, 6, 28), tr("全部"));
    all->setData(RoleGroup, QString());
    all->setData(RoleCount, QString::number(total));
    m_groups->addItem(all);
    for (const GroupInfo &g : m_groupData) {
        auto *it = new QListWidgetItem(icons::groupIcon(g.icon, 28), g.name);
        it->setData(RoleGroup, g.id);
        it->setData(RoleCount, QString::number(m_tokenData.value(g.id).size()));
        it->setToolTip(g.note);
        m_groups->addItem(it);
    }
    for (int i = 0; i < m_groups->count(); ++i)
        if (m_groups->item(i)->data(RoleGroup).toString() == m_selectedGroup) m_groups->setCurrentRow(i);
}

void MainWindow::populateTokens()
{
    m_tokens->clear();
    const QString q = m_search->text().trimmed();
    const bool searching = !q.isEmpty();
    int shown = 0;
    for (const GroupInfo &g : m_groupData) {
        if (!searching && !m_selectedGroup.isEmpty() && g.id != m_selectedGroup) continue;
        for (const TokenInfo &t : m_tokenData.value(g.id)) {
            if (searching && !t.name.contains(q, Qt::CaseInsensitive) && !t.note.contains(q, Qt::CaseInsensitive)
                && !g.name.contains(q, Qt::CaseInsensitive))
                continue;
            auto *it = new QListWidgetItem(icons::statusIcon(t.status(), 26), t.name);
            it->setData(RoleGroup, g.id);
            it->setData(RoleToken, t.id);
            const QString sub = t.note.simplified();
            it->setData(RoleSub, (m_selectedGroup.isEmpty() || searching) ? (sub.isEmpty() ? g.name : g.name + QStringLiteral(" · ") + sub)
                                                                            : (sub.isEmpty() ? tr("沒有說明") : sub));
            it->setData(RoleBadge, statusText(t));
            it->setData(RoleBadgeColor, statusColor(t));
            m_tokens->addItem(it);
            ++shown;
        }
    }
    if (shown == 0) {
        m_emptyMsg->setText(searching ? tr("找不到符合「%1」的 Token").arg(q)
                            : m_groupData.isEmpty() ? QString() : tr("這裡還沒有 Token\n點左下角的 ＋ → 建立 Token"));
        m_rightStack->setCurrentIndex(0);
    } else {
        m_rightStack->setCurrentIndex(1);
    }
}

void MainWindow::updateHeader()
{
    const bool searching = !m_search->text().trimmed().isEmpty();
    QString title = tr("全部 Token");
    QString note;
    for (const GroupInfo &g : m_groupData)
        if (g.id == m_selectedGroup) { title = g.name; note = g.note; }
    if (searching) title = tr("搜尋結果");
    m_title->setText(title);
    const int n = m_tokens->count();
    m_subtitle->setText(note.isEmpty() ? tr("%1 個 Token").arg(n) : tr("%1 個 Token · %2").arg(n).arg(note.simplified()));
    const bool isGroup = !m_selectedGroup.isEmpty() && !searching;
    m_editGroup->setVisible(isGroup);
    m_delGroup->setVisible(isGroup);
}

void MainWindow::placeFab()
{
    m_fab->move(22, m_stack->height() - m_fab->height() - 22);
    m_fab->raise();
}

bool MainWindow::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_stack && (e->type() == QEvent::Resize || e->type() == QEvent::Show)) placeFab();
    switch (e->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::KeyPress:
    case QEvent::Wheel:
    case QEvent::MouseMove:
        if (!m_locking) restartIdle();
        break;
    default: break;
    }
    return QMainWindow::eventFilter(o, e);
}

void MainWindow::restartIdle()
{
    if (m_autoLockMin > 0) m_idle->start(m_autoLockMin * 60 * 1000);
    else m_idle->stop();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    m_vault.lock();
    e->accept();
    qApp->quit();
}

// ---- actions -------------------------------------------------------------------------------------------
void MainWindow::showAddMenu()
{
    QMenu menu(this);
    QAction *g = menu.addAction(tr("建立群組"));
    QAction *t = menu.addAction(tr("建立 Token"));
    const QSize sz = menu.sizeHint();
    QAction *a = menu.exec(m_fab->mapToGlobal(QPoint(0, -sz.height() - 8)));
    if (a == g) createGroup();
    else if (a == t) createToken(m_selectedGroup);
}

void MainWindow::showSettingsMenu()
{
    QMenu menu(this);
    QAction *lock = menu.addAction(tr("立即鎖定"));
    menu.addSeparator();
    QAction *reset = menu.addAction(tr("重設驗證方法（密碼／鑰匙檔／TOTP）…"));
    QAction *al = menu.addAction(tr("自動鎖定時間…"));
    menu.addSeparator();
    QAction *quit = menu.addAction(tr("離開"));
    QAction *a = menu.exec(QCursor::pos());
    if (a == lock) lockNow();
    else if (a == reset) resetAuthMethod();
    else if (a == al) setAutoLock();
    else if (a == quit) close();
}

void MainWindow::createGroup()
{
    GroupDialog d(GroupInfo{}, false, this);
    if (d.exec() != QDialog::Accepted) return;
    const GroupInfo g = d.result();
    QString id;
    const Result r = m_vault.createGroup(g.name, g.note, g.icon, &id);
    if (!r) QMessageBox::warning(this, tr("建立群組"), r.error);
    else m_selectedGroup = id;
    reload();
}

void MainWindow::createToken(const QString &pre)
{
    if (m_groupData.isEmpty()) {
        QMessageBox::information(this, tr("建立 Token"), tr("請先建立一個群組。"));
        return;
    }
    TokenDialog d(m_groupData, pre, this);
    if (d.exec() != QDialog::Accepted) return;
    const Result r = m_vault.addToken(d.groupId(), d.name(), d.note(), d.expires(), d.token(), nullptr);
    if (!r) QMessageBox::warning(this, tr("建立 Token"), r.error);
    reload();
}

void MainWindow::editGroup(const QString &gid)
{
    for (const GroupInfo &g : m_groupData) {
        if (g.id != gid) continue;
        GroupDialog d(g, true, this);
        if (d.exec() != QDialog::Accepted) return;
        const Result r = m_vault.updateGroup(d.result());
        if (!r) QMessageBox::warning(this, tr("編輯群組"), r.error);
        reload();
        return;
    }
}

void MainWindow::deleteGroup(const QString &gid)
{
    const int n = int(m_tokenData.value(gid).size());
    QString name = gid;
    for (const GroupInfo &g : m_groupData) if (g.id == gid) name = g.name;
    const QString msg = n ? tr("群組「%1」內有 %2 個 Token，全部都會被永久刪除。確定嗎？").arg(name).arg(n)
                          : tr("確定要刪除空群組「%1」嗎？").arg(name);
    if (QMessageBox::warning(this, tr("刪除群組"), msg, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
        return;
    if (n > 0) {
        Vault *v = &m_vault;
        AuthDialog dlg(m_vault.mode(), tr("身分驗證"), tr("刪除含有 Token 的群組需要驗證。"),
                       [v](const Credentials &c) { return v->verify(c); }, this);
        if (dlg.exec() != QDialog::Accepted) return;
    }
    const Result r = m_vault.deleteGroup(gid);
    if (!r) QMessageBox::warning(this, tr("刪除群組"), r.error);
    if (m_selectedGroup == gid) m_selectedGroup.clear();
    reload();
}

void MainWindow::openToken(const QString &gid, const QString &id)
{
    GroupInfo grp;
    for (const GroupInfo &g : m_groupData) if (g.id == gid) grp = g;
    for (const TokenInfo &t : m_tokenData.value(gid)) {
        if (t.id != id) continue;
        TokenDetailDialog d(m_vault, grp, t, this);
        d.exec();
        if (m_vault.isUnlocked() && d.changed()) reload();
        return;
    }
}

void MainWindow::groupMenu(const QPoint &pos)
{
    QListWidgetItem *it = m_groups->itemAt(pos);
    if (!it || it->data(RoleGroup).toString().isEmpty()) return;
    const QString gid = it->data(RoleGroup).toString();
    QMenu menu(this);
    QAction *a1 = menu.addAction(tr("在此群組建立 Token"));
    QAction *a2 = menu.addAction(tr("編輯群組"));
    QAction *a3 = menu.addAction(tr("刪除群組"));
    QAction *a = menu.exec(m_groups->viewport()->mapToGlobal(pos));
    if (a == a1) createToken(gid);
    else if (a == a2) editGroup(gid);
    else if (a == a3) deleteGroup(gid);
}

void MainWindow::tokenMenu(const QPoint &pos)
{
    QListWidgetItem *it = m_tokens->itemAt(pos);
    if (!it) return;
    QMenu menu(this);
    QAction *a1 = menu.addAction(tr("開啟"));
    if (menu.exec(m_tokens->viewport()->mapToGlobal(pos)) == a1)
        openToken(it->data(RoleGroup).toString(), it->data(RoleToken).toString());
}

void MainWindow::resetAuthMethod()
{
    Vault *v = &m_vault;
    const AuthMode current = m_vault.mode();
    AuthDialog verify(current, tr("驗證目前身分"), tr("重設驗證方法前，必須先輸入目前的密碼／鑰匙檔／驗證碼。"),
                      [v](const Credentials &c) { return v->verify(c); }, this);
    if (verify.exec() != QDialog::Accepted) return;
    const Credentials old = verify.usedCredentials();
    SetupDialog setup(tr("重設驗證方法"),
                      tr("選擇新的保護方式。RSA 金鑰與既有的 Token 檔案不需要重新加密；完成後舊的密碼／鑰匙檔就不能用了。"),
                      [v, old](AuthMode m, const Credentials &c, const QByteArray &ts) {
                          return v->changeCredentials(old, m, c, ts, /*oldTotpAlreadyVerified=*/true);
                      },
                      this, current);
    if (setup.exec() == QDialog::Accepted)
        QMessageBox::information(this, tr("完成"), tr("驗證方法已更新。下次解鎖與每次敏感操作都會使用新的設定。"));
}

void MainWindow::setAutoLock()
{
    bool ok = false;
    const int v = QInputDialog::getInt(this, tr("自動鎖定"), tr("閒置幾分鐘後自動鎖定？（0 = 不自動鎖定）"),
                                       m_autoLockMin, 0, 1440, 1, &ok);
    if (!ok) return;
    m_autoLockMin = v;
    QSettings().setValue("autoLockMinutes", v);
    restartIdle();
}

void MainWindow::lockNow()
{
    if (m_locking || !m_vault.isUnlocked()) return;
    m_locking = true;
    m_idle->stop();
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (w == this || !w->isVisible()) continue;
        if (auto *d = qobject_cast<QDialog *>(w)) d->reject();
    }
    m_vault.lock();
    m_tokens->clear();
    m_groups->clear();
    m_tokenData.clear();
    m_notified.clear();
    hide();
    if (!runUnlockFlow(m_vault, nullptr, tr("已鎖定。請驗證身分以繼續。"))) {
        qApp->quit();
        return;
    }
    m_selectedGroup.clear();
    reload();
    show();
    m_locking = false;
    restartIdle();
    checkExpiry(true);
}

void MainWindow::checkExpiry(bool includeSoon)
{
    QStringList expired, soon;
    for (const GroupInfo &g : m_groupData) {
        for (const TokenInfo &t : m_tokenData.value(g.id)) {
            const QString key = g.id + '/' + t.id;
            if (t.revoked) continue;
            if (t.status() == TokenInfo::Expired) {
                if (m_notified.contains(key + "#e")) continue;
                m_notified.insert(key + "#e");
                expired << QStringLiteral("• %1 / %2").arg(g.name, t.name);
            } else if (includeSoon && t.expires.isValid() && t.daysLeft() < 3) {
                if (m_notified.contains(key + "#s")) continue;
                m_notified.insert(key + "#s");
                soon << QStringLiteral("• %1 / %2（%3）").arg(g.name, t.name, statusText(t));
            }
        }
    }
    if (expired.isEmpty() && soon.isEmpty()) return;
    QString msg;
    if (!expired.isEmpty()) msg += tr("以下 Token 已過期：\n%1\n\n").arg(expired.join('\n'));
    if (!soon.isEmpty()) msg += tr("以下 Token 即將在 3 天內到期：\n%1").arg(soon.join('\n'));
    auto *box = new QMessageBox(QMessageBox::Warning, tr("Token 期限提醒"), msg.trimmed(), QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->open();
    reload();
}
