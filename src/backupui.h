#pragma once
#include "backup.h"
#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

// Shows the two freshly generated keys exactly once.
class BackupKeysDialog : public QDialog {
    Q_OBJECT
public:
    BackupKeysDialog(const QString &archivePath, const tv::BackupKeys &keys, QWidget *parent = nullptr);
    void reject() override;
private:
    QCheckBox *m_ack;
    QPushButton *m_done;
};

// Archive + KEY 1 + KEY 2. On accept the archive has been fully decrypted and validated (nothing is changed yet).
class ImportDialog : public QDialog {
    Q_OBJECT
public:
    explicit ImportDialog(QWidget *parent = nullptr);
    tv::BackupData data() const { return m_data; }
    void accept() override;
private:
    QLineEdit *m_path, *m_key1, *m_key2;
    QLabel *m_err;
    tv::BackupData m_data;
};

// Typing "I understand" is required before existing data may be replaced.
class WipeConfirmDialog : public QDialog {
    Q_OBJECT
public:
    explicit WipeConfirmDialog(QWidget *parent = nullptr);
    static QString phrase() { return QStringLiteral("I understand"); }
};

// Verify -> choose file -> encrypt -> show keys. Returns true if an archive was written.
bool runExportFlow(tv::Vault &v, QWidget *parent = nullptr);
// First run (no vault yet): archive + keys -> create the vault from it. Returns true on success (vault unlocked).
bool runImportFlow(tv::Vault &v, QWidget *parent = nullptr);
// From settings: "I understand" -> archive + keys (validated first) -> wipe and restore. Returns true on success.
bool runReplaceFromBackupFlow(tv::Vault &v, QWidget *parent = nullptr);
