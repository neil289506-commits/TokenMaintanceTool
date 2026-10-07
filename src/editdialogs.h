#pragma once
#include "vault.h"
#include <QDialog>
#include <QDateTime>
#include <QWidget>

class QComboBox;
class QDateTimeEdit;
class QLineEdit;
class QPlainTextEdit;
class QLabel;
class QButtonGroup;

// Expiry chooser: 自訂 / 1天 / 7天 / 14天 / 30天 / 1年 / 永不 (+ optional "keep current").
class ExpiryPicker : public QWidget {
    Q_OBJECT
public:
    explicit ExpiryPicker(bool allowKeep = false, QWidget *parent = nullptr);
    // Invalid result == never expires (or "keep" when keepSelected()).
    QDateTime value() const;
    bool keepSelected() const;
    void setValue(const QDateTime &utcOrInvalid);    // selects "自訂" with that date, or "永不"
    void selectPreset(int days);                     // 0 = never
private:
    void onChanged();
    QComboBox *m_combo;
    QDateTimeEdit *m_custom;
    bool m_allowKeep;
};

class GroupDialog : public QDialog {
    Q_OBJECT
public:
    explicit GroupDialog(const tv::GroupInfo &initial, bool editing, QWidget *parent = nullptr);
    tv::GroupInfo result() const;
    void accept() override;
private:
    QLineEdit *m_name;
    QPlainTextEdit *m_note;
    QButtonGroup *m_shapes, *m_colors;
    QString m_id;
    QByteArray m_image;
    QLabel *m_preview;
    QPushButton *m_pick, *m_clear;
    void refreshShapeIcons();
    void refreshImage();
};

class TokenDialog : public QDialog {
    Q_OBJECT
public:
    TokenDialog(const QList<tv::GroupInfo> &groups, const QString &preselectGroupId, QWidget *parent = nullptr);
    QString groupId() const;
    QString name() const;
    QString note() const;
    QDateTime expires() const;
    tv::SecureBytes token() const;
    void accept() override;
private:
    QComboBox *m_group;
    QLineEdit *m_name, *m_token;
    QPlainTextEdit *m_note;
    QLabel *m_len;
    ExpiryPicker *m_expiry;
};

// New secret for an existing token (renewal).
class RenewDialog : public QDialog {
    Q_OBJECT
public:
    explicit RenewDialog(const QString &tokenName, QWidget *parent = nullptr);
    tv::SecureBytes token() const;
    bool changeExpiry() const;         // false => keep current expiry
    QDateTime newExpires() const;      // invalid (with changeExpiry) => never expires
    void accept() override;
private:
    QLineEdit *m_token;
    QLabel *m_len;
    ExpiryPicker *m_expiry;
};
