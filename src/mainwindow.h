#pragma once
#include "vault.h"
#include <QMainWindow>
#include <QMap>
#include <QSet>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
class QTimer;
class QToolButton;

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
    void buildUi();
    void populateSidebar();
    void populateTokens();
    void updateHeader();
    void showAddMenu();
    void showSettingsMenu();
    void createGroup();
    void createToken(const QString &preselectGroupId = QString());
    void editGroup(const QString &gid);
    void deleteGroup(const QString &gid);
    void openToken(const QString &gid, const QString &id);
    void resetAuthMethod();
    void chooseLanguage();
    void exportBackup();
    void importBackup();
    void setAutoLock();
    void restartIdle();
    void placeFab();
    void groupMenu(const QPoint &pos);
    void tokenMenu(const QPoint &pos);
    QString currentGroupId() const;

    tv::Vault &m_vault;
    QStackedWidget *m_stack, *m_rightStack;
    QListWidget *m_groups, *m_tokens;
    QLabel *m_title, *m_subtitle, *m_emptyMsg;
    QLineEdit *m_search;
    QPushButton *m_editGroup, *m_delGroup;
    QToolButton *m_fab;
    QTimer *m_idle, *m_expiryTimer;
    QSet<QString> m_notified;
    QList<tv::GroupInfo> m_groupData;
    QMap<QString, QList<tv::TokenInfo>> m_tokenData;
    QString m_selectedGroup;     // empty = "全部"
    bool m_locking = false;
    int m_autoLockMin = 5;
};
