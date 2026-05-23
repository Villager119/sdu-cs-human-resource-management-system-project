#include "RecoverPasswordDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include <QStackedWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDate>
#include <QFrame>

RecoverPasswordDialog::RecoverPasswordDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
}

RecoverPasswordDialog::~RecoverPasswordDialog()
{
}

void RecoverPasswordDialog::setupUi()
{
    setWindowTitle("找回密码");
    setFixedSize(380, 320);

    // Style the dialog matching LoginWindow
    setStyleSheet(
        "QDialog { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f8f9fb, stop:1 #eef0f4); }"
        "QLabel { color: #374151; font-size: 13px; }"
        "QLineEdit, QDateEdit { padding: 6px; border: 1px solid #e2e5ea; border-radius: 6px; font-size: 13px; background: #ffffff; }"
        "QLineEdit:focus, QDateEdit:focus { border-color: #1e3a5f; }"
        "QPushButton { background: #1e3a5f; color: #fff; border: none; border-radius: 6px; font-size: 13px; font-weight: 600; padding: 8px 16px; }"
        "QPushButton:hover { background: #2a5080; }"
        "QPushButton:pressed { background: #162d4a; }"
        "QPushButton#backBtn { background: transparent; color: #8893a4; border: 1px solid #cbd5e1; }"
        "QPushButton#backBtn:hover { background: #f1f5f9; color: #334155; }"
    );

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    m_stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackedWidget);

    // --- Page 1: Identity Verification ---
    auto *page1 = new QWidget(this);
    auto *layoutPage1 = new QVBoxLayout(page1);
    layoutPage1->setContentsMargins(0, 0, 0, 0);

    auto *titlePage1 = new QLabel("第一步：身份验证", page1);
    titlePage1->setStyleSheet("font-size: 16px; font-weight: 700; color: #1e3a5f; margin-bottom: 10px;");
    layoutPage1->addWidget(titlePage1);

    auto *cardPage1 = new QFrame(page1);
    cardPage1->setObjectName("card");
    cardPage1->setStyleSheet("QFrame#card { background: #fff; border: 1px solid #e2e5ea; border-radius: 8px; padding: 15px; }");
    auto *cardLayoutPage1 = new QVBoxLayout(cardPage1);

    // Name input
    auto *nameLayout = new QHBoxLayout();
    auto *nameLabel = new QLabel("真实姓名：", cardPage1);
    nameLabel->setFixedWidth(70);
    m_nameEdit = new QLineEdit(cardPage1);
    m_nameEdit->setPlaceholderText("请输入您的姓名");
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_nameEdit);
    cardLayoutPage1->addLayout(nameLayout);

    // Phone input
    auto *phoneLayout = new QHBoxLayout();
    auto *phoneLabel = new QLabel("预留手机：", cardPage1);
    phoneLabel->setFixedWidth(70);
    m_phoneEdit = new QLineEdit(cardPage1);
    m_phoneEdit->setPlaceholderText("请输入预留手机号");
    phoneLayout->addWidget(phoneLabel);
    phoneLayout->addWidget(m_phoneEdit);
    cardLayoutPage1->addLayout(phoneLayout);

    // Employee ID input
    auto *idLayout = new QHBoxLayout();
    auto *idLabel = new QLabel("员工工号：", cardPage1);
    idLabel->setFixedWidth(70);
    m_empIdEdit = new QLineEdit(cardPage1);
    m_empIdEdit->setPlaceholderText("请输入您的员工工号");
    idLayout->addWidget(idLabel);
    idLayout->addWidget(m_empIdEdit);
    cardLayoutPage1->addLayout(idLayout);

    layoutPage1->addWidget(cardPage1);

    // Error label Page 1
    m_errorLabelPage1 = new QLabel("", page1);
    m_errorLabelPage1->setStyleSheet("color: #ef4444; font-size: 12px; margin-top: 5px;");
    m_errorLabelPage1->setAlignment(Qt::AlignCenter);
    layoutPage1->addWidget(m_errorLabelPage1);

    layoutPage1->addStretch();

    // Bottom buttons Page 1
    auto *btnLayoutPage1 = new QHBoxLayout();
    btnLayoutPage1->addStretch();
    m_nextBtn = new QPushButton("下一步", page1);
    auto *cancelBtn1 = new QPushButton("取消", page1);
    cancelBtn1->setObjectName("backBtn");
    btnLayoutPage1->addWidget(cancelBtn1);
    btnLayoutPage1->addWidget(m_nextBtn);
    layoutPage1->addLayout(btnLayoutPage1);

    m_stackedWidget->addWidget(page1);

    // --- Page 2: Reset Password ---
    auto *page2 = new QWidget(this);
    auto *layoutPage2 = new QVBoxLayout(page2);
    layoutPage2->setContentsMargins(0, 0, 0, 0);

    auto *titlePage2 = new QLabel("第二步：重置密码", page2);
    titlePage2->setStyleSheet("font-size: 16px; font-weight: 700; color: #1e3a5f; margin-bottom: 10px;");
    layoutPage2->addWidget(titlePage2);

    auto *cardPage2 = new QFrame(page2);
    cardPage2->setObjectName("card2");
    cardPage2->setStyleSheet("QFrame#card2 { background: #fff; border: 1px solid #e2e5ea; border-radius: 8px; padding: 15px; }");
    auto *cardLayoutPage2 = new QVBoxLayout(cardPage2);

    // New password
    auto *newPwdLayout = new QHBoxLayout();
    auto *newPwdLabel = new QLabel("新 密 码：", cardPage2);
    newPwdLabel->setFixedWidth(70);
    m_newPasswordEdit = new QLineEdit(cardPage2);
    m_newPasswordEdit->setEchoMode(QLineEdit::Password);
    m_newPasswordEdit->setPlaceholderText("请输入新密码");
    newPwdLayout->addWidget(newPwdLabel);
    newPwdLayout->addWidget(m_newPasswordEdit);
    cardLayoutPage2->addLayout(newPwdLayout);

    // Confirm password
    auto *confirmPwdLayout = new QHBoxLayout();
    auto *confirmPwdLabel = new QLabel("确认密码：", cardPage2);
    confirmPwdLabel->setFixedWidth(70);
    m_confirmPasswordEdit = new QLineEdit(cardPage2);
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_confirmPasswordEdit->setPlaceholderText("请再次输入新密码");
    confirmPwdLayout->addWidget(confirmPwdLabel);
    confirmPwdLayout->addWidget(m_confirmPasswordEdit);
    cardLayoutPage2->addLayout(confirmPwdLayout);

    layoutPage2->addWidget(cardPage2);

    // Error label Page 2
    m_errorLabelPage2 = new QLabel("", page2);
    m_errorLabelPage2->setStyleSheet("color: #ef4444; font-size: 12px; margin-top: 5px;");
    m_errorLabelPage2->setAlignment(Qt::AlignCenter);
    layoutPage2->addWidget(m_errorLabelPage2);

    layoutPage2->addStretch();

    // Bottom buttons Page 2
    auto *btnLayoutPage2 = new QHBoxLayout();
    m_backBtn = new QPushButton("返回上一步", page2);
    m_backBtn->setObjectName("backBtn");
    m_resetBtn = new QPushButton("提交重置", page2);
    btnLayoutPage2->addWidget(m_backBtn);
    btnLayoutPage2->addStretch();
    btnLayoutPage2->addWidget(m_resetBtn);
    layoutPage2->addLayout(btnLayoutPage2);

    m_stackedWidget->addWidget(page2);

    // --- Connect slots ---
    connect(m_nextBtn, &QPushButton::clicked, this, &RecoverPasswordDialog::onNextStep);
    connect(m_resetBtn, &QPushButton::clicked, this, &RecoverPasswordDialog::onResetPassword);
    connect(m_backBtn, &QPushButton::clicked, this, &RecoverPasswordDialog::onBackStep);
    connect(cancelBtn1, &QPushButton::clicked, this, &QDialog::reject);

    // Connect returnPress signals for natural keyboard flow
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &RecoverPasswordDialog::onNextStep);
    connect(m_phoneEdit, &QLineEdit::returnPressed, this, &RecoverPasswordDialog::onNextStep);
    connect(m_empIdEdit, &QLineEdit::returnPressed, this, &RecoverPasswordDialog::onNextStep);
    connect(m_newPasswordEdit, &QLineEdit::returnPressed, this, &RecoverPasswordDialog::onResetPassword);
    connect(m_confirmPasswordEdit, &QLineEdit::returnPressed, this, &RecoverPasswordDialog::onResetPassword);
}

void RecoverPasswordDialog::onNextStep()
{
    m_errorLabelPage1->setText("");
    QString name = m_nameEdit->text().trimmed();
    QString phone = m_phoneEdit->text().trimmed();
    QString empIdStr = m_empIdEdit->text().trimmed();

    if (name.isEmpty() || phone.isEmpty() || empIdStr.isEmpty()) {
        m_errorLabelPage1->setText("工号、姓名和预留手机号不能为空！");
        return;
    }

    // Query DB to verify employee
    QSqlQuery query;
    query.prepare("SELECT emp_id, name FROM employees WHERE emp_id = :emp_id AND name = :name AND phone = :phone AND status = '在职'");
    query.bindValue(":emp_id", empIdStr.toInt());
    query.bindValue(":name", name);
    query.bindValue(":phone", phone);

    if (!query.exec()) {
        m_errorLabelPage1->setText("数据库查询失败: " + query.lastError().text());
        return;
    }

    if (!query.next()) {
        m_errorLabelPage1->setText("身份校验失败，工号、姓名或预留手机号不匹配！");
        return;
    }

    m_verifiedEmpId = query.value("emp_id").toInt();
    m_verifiedEmpName = query.value("name").toString();

    // Switch to step 2 page
    m_stackedWidget->setCurrentIndex(1);
    m_newPasswordEdit->setFocus();
}

void RecoverPasswordDialog::onResetPassword()
{
    m_errorLabelPage2->setText("");
    QString newPwd = m_newPasswordEdit->text();
    QString confirmPwd = m_confirmPasswordEdit->text();

    if (newPwd.isEmpty()) {
        m_errorLabelPage2->setText("新密码不能为空！");
        return;
    }

    if (newPwd != confirmPwd) {
        m_errorLabelPage2->setText("两次输入的密码不一致！");
        return;
    }

    if (m_verifiedEmpId == -1) {
        m_errorLabelPage2->setText("验证信息失效，请返回上一步重新验证。");
        return;
    }

    // SHA-256 hash the new password
    QString passwordHash = QString(QCryptographicHash::hash(newPwd.toUtf8(), QCryptographicHash::Sha256).toHex());

    // Update password
    QSqlQuery query;
    query.prepare("UPDATE employees SET password_hash = :pwd WHERE emp_id = :emp_id");
    query.bindValue(":pwd", passwordHash);
    query.bindValue(":emp_id", m_verifiedEmpId);

    if (!query.exec()) {
        m_errorLabelPage2->setText("更新密码失败: " + query.lastError().text());
        return;
    }

    // Write audit log
    QSqlQuery logQuery;
    logQuery.prepare("INSERT INTO audit_logs(emp_id, emp_name, action, target, log_time) VALUES(?, ?, ?, ?, NOW())");
    logQuery.addBindValue(m_verifiedEmpId);
    logQuery.addBindValue(m_verifiedEmpName);
    logQuery.addBindValue("找回密码");
    logQuery.addBindValue("通过身份信息自主重置密码");
    if (!logQuery.exec()) {
        // Just log the error, don't block the password reset success
        qWarning() << "Failed to write audit log for password recovery:" << logQuery.lastError().text();
    }

    QMessageBox::information(this, "成功", "密码重置成功，请使用新密码登录！");
    accept();
}

void RecoverPasswordDialog::onBackStep()
{
    m_errorLabelPage2->setText("");
    m_stackedWidget->setCurrentIndex(0);
}
