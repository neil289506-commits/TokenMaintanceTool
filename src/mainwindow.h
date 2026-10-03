#pragma once
#include "vault.h"
#include <QMainWindow>
#include <QSet>

class QTreeWidget;
class QTreeWidgetItem;
class QToolButton;
class QTimer;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(tv::Vault &vault, QWidget *parent = nullptr);
    void reload();
    void checkExpiry(bool includeSoon);
    void lockNow();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private:
    void buildMenus();
    void showAddMenu();
    void createGroup();
    void createToken(const QString &preselectGroupId = QString());
    void editGroup(const QString &gid);
    void deleteGroup(const QString &gid);
    void openToken(const QString &gid, const QString &id);
    void changeMasterSecret();
    void setAutoLock();
    void restartIdle();
    void placeFab();
    void contextMenu(const QPoint &pos);
    bool promptUnlock();

    tv::Vault &m_vault;
    QTreeWidget *m_tree;
    QStackedWidget *m_stack;
    QToolButton *m_fab;
    QTimer *m_idle, *m_expiryTimer;
    QSet<QString> m_notified;
    QList<tv::GroupInfo> m_groups;
    bool m_locking = false;
    int m_autoLockMin = 5;
};
