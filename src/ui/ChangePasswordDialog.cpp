#include "ChangePasswordDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

ChangePasswordDialog::ChangePasswordDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("修改密码");
    setFixedSize(320, 220);

    auto *mainLayout = new QVBoxLayout(this);

    // 旧密码
    auto *oldLayout = new QHBoxLayout;
    oldLayout->addWidget(new QLabel("旧密码："));
    m_oldPasswordEdit = new QLineEdit;
    m_oldPasswordEdit->setEchoMode(QLineEdit::Password);
    oldLayout->addWidget(m_oldPasswordEdit);
    mainLayout->addLayout(oldLayout);

    // 新密码
    auto *newLayout = new QHBoxLayout;
    newLayout->addWidget(new QLabel("新密码："));
    m_newPasswordEdit = new QLineEdit;
    m_newPasswordEdit->setEchoMode(QLineEdit::Password);
    newLayout->addWidget(m_newPasswordEdit);
    mainLayout->addLayout(newLayout);

    // 确认密码
    auto *confirmLayout = new QHBoxLayout;
    confirmLayout->addWidget(new QLabel("确认密码："));
    m_confirmPasswordEdit = new QLineEdit;
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmLayout->addWidget(m_confirmPasswordEdit);
    mainLayout->addLayout(confirmLayout);

    mainLayout->addStretch();

    // 按钮
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *okBtn = new QPushButton("确认修改");
    auto *cancelBtn = new QPushButton("取消");
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(okBtn, &QPushButton::clicked, this, [this]() {
        if (m_oldPasswordEdit->text().isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入旧密码");
            return;
        }
        if (m_newPasswordEdit->text().isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入新密码");
            return;
        }
        if (m_newPasswordEdit->text().length() < 6) {
            QMessageBox::warning(this, "提示", "新密码长度不能少于6位");
            return;
        }
        if (m_newPasswordEdit->text() != m_confirmPasswordEdit->text()) {
            QMessageBox::warning(this, "提示", "两次输入的新密码不一致");
            return;
        }
        accept();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QString ChangePasswordDialog::oldPassword() const
{
    return m_oldPasswordEdit->text();
}

QString ChangePasswordDialog::newPassword() const
{
    return m_newPasswordEdit->text();
}
