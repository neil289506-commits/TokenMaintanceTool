#include "mainwindow.h"
#include "authdialogs.h"
#include "editdialogs.h"
#include "icons.h"
#include "tokendetail.h"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>

using namespace tv;

namespace {
enum Roles { RoleKind = Qt::UserRole + 1, RoleGroup, RoleToken };
enum Kind { KindGroup = 1, KindToken = 2 };

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
}

MainWindow::MainWindow(Vault &vault, QWidget *parent) : QMainWindow(parent), m_vault(vault)
{
    setWindowTitle(tr("Token 管理工具"));
    setWindowIcon(icons::appIcon());
    resize(720, 560);

    m_stack = new QStackedWidget;
    auto *empty = new QLabel(tr("還沒有任何內容\n點左下角的 ＋ 建立第一個群組"));
    empty->setAlignment(Qt::AlignCenter);
    empty->setStyleSheet("color:gray;font-size:15px");
    m_tree = new QTreeWidget;
    m_tree->setColumnCount(2);
    m_tree->setHeaderHidden(true);
    m_tree->setIconSize(QSize(24, 24));
    m_tree->setRootIsDecorated(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_stack->addWidget(empty);
    m_stack->addWidget(m_tree);
    setCentralWidget(m_stack);

    m_fab = new QToolButton(m_stack);
    m_fab->setText(QStringLiteral("+"));
    m_fab->setFixedSize(56, 56);
    m_fab->setCursor(Qt::PointingHandCursor);
    m_fab->setToolTip(tr("新增"));
    m_fab->setStyleSheet("QToolButton{background:#3B6FD8;color:white;border:none;border-radius:28px;font-size:30px;padding-bottom:3px}"
                         "QToolButton:hover{background:#2F5CB8}QToolButton:pressed{background:#274D9A}");
    connect(m_fab, &QToolButton::clicked, this, &MainWindow::showAddMenu);
    m_stack->installEventFilter(this);

    connect(m_tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *it, int) {
        if (it->data(0, RoleKind).toInt() == KindToken)
            openToken(it->data(0, RoleGroup).toString(), it->data(0, RoleToken).toString());
    });
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::contextMenu);

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

    buildMenus();
    reload();
    restartIdle();
    QTimer::singleShot(0, this, [this] { placeFab(); checkExpiry(true); });
}

void MainWindow::buildMenus()
{
    QMenu *m = menuBar()->addMenu(tr("檔案"));
    QAction *lock = m->addAction(tr("立即鎖定"));
    lock->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(lock, &QAction::triggered, this, &MainWindow::lockNow);
    m->addAction(tr("變更主要密碼／鑰匙檔…"), this, &MainWindow::changeMasterSecret);
    m->addAction(tr("自動鎖定時間…"), this, &MainWindow::setAutoLock);
    m->addSeparator();
    QAction *q = m->addAction(tr("離開"));
    q->setMenuRole(QAction::QuitRole);
    connect(q, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::reload()
{
    QSet<QString> expanded;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i)
        if (m_tree->topLevelItem(i)->isExpanded()) expanded.insert(m_tree->topLevelItem(i)->data(0, RoleGroup).toString());
    const bool first = m_tree->topLevelItemCount() == 0;
    m_tree->clear();
    QStringList warn;
    m_groups = m_vault.groups(&warn);
    for (const GroupInfo &g : m_groups) {
        auto *gi = new QTreeWidgetItem(m_tree);
        gi->setData(0, RoleKind, KindGroup);
        gi->setData(0, RoleGroup, g.id);
        gi->setIcon(0, icons::groupIcon(g.icon, 28));
        gi->setToolTip(0, g.note);
        QFont f = gi->font(0);
        f.setBold(true);
        gi->setFont(0, f);
        QList<TokenInfo> toks = m_vault.tokens(g.id, &warn);
        gi->setText(0, g.name);
        gi->setText(1, tr("%1 個").arg(toks.size()));
        gi->setForeground(1, Qt::gray);
        for (const TokenInfo &t : toks) {
            auto *ti = new QTreeWidgetItem(gi);
            ti->setData(0, RoleKind, KindToken);
            ti->setData(0, RoleGroup, g.id);
            ti->setData(0, RoleToken, t.id);
            ti->setIcon(0, icons::statusIcon(t.status(), 22));
            ti->setText(0, t.name);
            ti->setText(1, statusText(t));
            ti->setToolTip(0, t.note);
            if (t.status() != TokenInfo::Valid) ti->setForeground(1, QColor("#D93F3F"));
        }
        if (first || expanded.contains(g.id)) gi->setExpanded(true);
    }
    m_stack->setCurrentIndex(m_groups.isEmpty() ? 0 : 1);
    placeFab();
    if (!warn.isEmpty())
        QMessageBox::warning(this, tr("部分資料無法讀取"), warn.join('\n'));
}

void MainWindow::placeFab()
{
    m_fab->move(20, m_stack->height() - m_fab->height() - 20);
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

void MainWindow::showAddMenu()
{
    QMenu menu(this);
    QAction *g = menu.addAction(tr("建立群組"));
    QAction *t = menu.addAction(tr("建立 Token"));
    const QSize sz = menu.sizeHint();
    const QPoint p = m_fab->mapToGlobal(QPoint(0, -sz.height() - 6));
    QAction *a = menu.exec(p);
    if (a == g) createGroup();
    else if (a == t) createToken();
}

void MainWindow::createGroup()
{
    GroupDialog d(GroupInfo{}, false, this);
    if (d.exec() != QDialog::Accepted) return;
    const GroupInfo g = d.result();
    const Result r = m_vault.createGroup(g.name, g.note, g.icon, nullptr);
    if (!r) QMessageBox::warning(this, tr("建立群組"), r.error);
    reload();
}

void MainWindow::createToken(const QString &pre)
{
    if (m_groups.isEmpty()) {
        QMessageBox::information(this, tr("建立 Token"), tr("請先建立一個群組。"));
        return;
    }
    TokenDialog d(m_groups, pre, this);
    if (d.exec() != QDialog::Accepted) return;
    const Result r = m_vault.addToken(d.groupId(), d.name(), d.note(), d.expires(), d.token(), nullptr);
    if (!r) QMessageBox::warning(this, tr("建立 Token"), r.error);
    reload();
}

void MainWindow::editGroup(const QString &gid)
{
    for (const GroupInfo &g : m_groups) {
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
    const int n = int(m_vault.tokens(gid).size());
    QString name = gid;
    for (const GroupInfo &g : m_groups) if (g.id == gid) name = g.name;
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
    reload();
}

void MainWindow::openToken(const QString &gid, const QString &id)
{
    GroupInfo grp;
    for (const GroupInfo &g : m_groups) if (g.id == gid) grp = g;
    for (const TokenInfo &t : m_vault.tokens(gid)) {
        if (t.id != id) continue;
        TokenDetailDialog d(m_vault, grp, t, this);
        d.exec();
        if (m_vault.isUnlocked()) { if (d.changed()) reload(); }
        return;
    }
}

void MainWindow::contextMenu(const QPoint &pos)
{
    QTreeWidgetItem *it = m_tree->itemAt(pos);
    if (!it) return;
    const QString gid = it->data(0, RoleGroup).toString();
    QMenu menu(this);
    if (it->data(0, RoleKind).toInt() == KindGroup) {
        QAction *a1 = menu.addAction(tr("在此群組建立 Token"));
        QAction *a2 = menu.addAction(tr("編輯群組"));
        QAction *a3 = menu.addAction(tr("刪除群組"));
        QAction *a = menu.exec(m_tree->viewport()->mapToGlobal(pos));
        if (a == a1) createToken(gid);
        else if (a == a2) editGroup(gid);
        else if (a == a3) deleteGroup(gid);
    } else {
        QAction *a1 = menu.addAction(tr("開啟"));
        if (menu.exec(m_tree->viewport()->mapToGlobal(pos)) == a1)
            openToken(gid, it->data(0, RoleToken).toString());
    }
}

void MainWindow::changeMasterSecret()
{
    Vault *v = &m_vault;
    AuthDialog verify(m_vault.mode(), tr("身分驗證"), tr("請先輸入目前的密碼／鑰匙檔。"),
                      [v](const Credentials &c) { return v->verify(c); }, this);
    if (verify.exec() != QDialog::Accepted) return;
    const Credentials old = verify.usedCredentials();
    SetupDialog setup(tr("變更主要密碼／鑰匙檔"),
                      tr("設定新的保護方式。RSA 金鑰與既有的 Token 檔案不需要重新加密。"),
                      [v, old](AuthMode m, const Credentials &c) { return v->changeCredentials(old, m, c); }, this);
    if (setup.exec() == QDialog::Accepted)
        QMessageBox::information(this, tr("完成"), tr("已更新。下次解鎖請使用新的密碼／鑰匙檔。"));
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

bool MainWindow::promptUnlock()
{
    Vault *v = &m_vault;
    AuthDialog dlg(m_vault.mode(), tr("解鎖保險庫"), tr("已鎖定。請輸入密碼／鑰匙檔以繼續。"),
                   [v](const Credentials &c) { return v->unlock(c); });
    dlg.setWindowIcon(icons::appIcon());
    return dlg.exec() == QDialog::Accepted;
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
    m_tree->clear();
    m_notified.clear();
    hide();
    if (!promptUnlock()) {
        qApp->quit();
        return;
    }
    reload();
    show();
    m_locking = false;
    restartIdle();
    checkExpiry(true);
}

void MainWindow::checkExpiry(bool includeSoon)
{
    QStringList expired, soon;
    for (const GroupInfo &g : m_groups) {
        for (const TokenInfo &t : m_vault.tokens(g.id)) {
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
