#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include<QSqlDatabase>
#include<QSqlQuery>
#include<QSqlError>
#include<qmessagebox.h>

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
private slots:
    void on_btnLogin_clicked();
private:
    Ui::LoginWindow *ui;
};

#endif // LOGINWINDOW_H
