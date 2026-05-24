#ifndef PROFILECHANGETAB_H
#define PROFILECHANGETAB_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QSqlRelation>
#include <QTableView>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <functional>

class ProfileChangeTab : public QWidget
{
    Q_OBJECT
public:
    ProfileChangeTab(int empId, const QString &role,
                     std::function<void(const QString&, const QString&)> logFn,
                     std::function<void(int, const QString&, const QString&)> notifyFn,
                     QWidget *parent = nullptr);
    void refresh();

private slots:
    void submitRequest();
    void approve();
    void reject();

private:
    QSqlRelationalTableModel *m_model;
    QTableView *m_table;
    QComboBox *m_fieldCombo;
    QLineEdit *m_newValueEdit;
    class QTextEdit *m_reasonEdit;
    QPushButton *m_btnApprove, *m_btnReject;
    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
    std::function<void(int, const QString&, const QString&)> m_notify;
};

#endif
