#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class LoginWindow;
}
QT_END_NAMESPACE

class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

    void setConfigPath(const QString &path);
    void setDbConnected(bool ok);

private slots:
    void on_btnLogin_clicked();
    void on_btnServerSettings_clicked();
    void on_btnForgotPassword_clicked();
    void tryReconnect();
    void tryAutoLogin();

private:
    Ui::LoginWindow *ui;
    QString m_configPath;
    bool m_dbConnected = false;
    bool m_autoLoginTried = false;
};

#endif // LOGINWINDOW_H
