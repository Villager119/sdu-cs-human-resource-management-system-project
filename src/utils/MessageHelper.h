#ifndef MESSAGEHELPER_H
#define MESSAGEHELPER_H

#include <QMessageBox>
#include <QString>
#include <QWidget>

class MessageHelper
{
public:
    static void formWarning(QWidget *parent, const QString &field, const QString &hint)
    {
        QMessageBox::warning(parent, "请检查输入", QString("%1：%2").arg(field, hint));
    }

    static void operationFailed(QWidget *parent, const QString &action, const QString &hint)
    {
        QMessageBox::critical(parent, "操作失败",
                              QString("%1失败。\n\n处理建议：%2").arg(action, hint));
    }

    static void databaseFailed(QWidget *parent, const QString &action)
    {
        operationFailed(parent, action, "请刷新当前页面后重试；若仍失败，请联系管理员检查数据库连接。");
    }

    static bool confirm(QWidget *parent, const QString &title, const QString &message)
    {
        return QMessageBox::question(parent, title, message,
                                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
    }
};

#endif
