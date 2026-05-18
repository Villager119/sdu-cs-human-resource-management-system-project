#include "DashboardTab.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QSqlQuery>
#include <QDate>

DashboardTab::DashboardTab(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(20, 20, 20, 20);
    grid->setSpacing(15);

    QString titles[] = {"总员工数", "在职人数", "离职人数", "本月请假", "待审批", "本月薪资总额"};
    QString icons[] = {"👥", "✅", "🚪", "📋", "⏳", "💰"};
    for (int i = 0; i < 6; i++) {
        auto *frame = new QFrame;
        frame->setMinimumSize(200, 110);
        frame->setStyleSheet("QFrame { background: #fff; border: 1px solid #e2e5ea; border-radius: 10px; }");

        auto *l = new QVBoxLayout(frame);
        l->setSpacing(4);
        l->setContentsMargins(20, 16, 20, 16);

        auto *icon = new QLabel(icons[i]);
        icon->setAlignment(Qt::AlignLeft);
        icon->setStyleSheet("font-size: 22px; border: none;");

        auto *t = new QLabel(titles[i]);
        t->setAlignment(Qt::AlignLeft);
        t->setStyleSheet("font-size: 12px; color: #8893a4; border: none;");

        m_labels[i] = new QLabel("-");
        m_labels[i]->setAlignment(Qt::AlignRight | Qt::AlignBottom);
        m_labels[i]->setStyleSheet("font-size: 32px; font-weight: 700; color: #1e3a5f; border: none;");

        l->addWidget(icon);
        l->addWidget(t);
        l->addWidget(m_labels[i], 1);
        grid->addWidget(frame, i / 3, i % 3);
    }
}

void DashboardTab::refresh()
{
    QSqlQuery q;
    q.exec("SELECT COUNT(*) FROM employees");
    int total = q.next() ? q.value(0).toInt() : 0;
    q.exec("SELECT COUNT(*) FROM employees WHERE status='在职'");
    int active = q.next() ? q.value(0).toInt() : 0;
    q.exec("SELECT COUNT(*) FROM employees WHERE status='离职'");
    int inactive = q.next() ? q.value(0).toInt() : 0;
    QString month = QDate::currentDate().toString("yyyy-MM");
    q.prepare("SELECT COUNT(*) FROM leave_requests WHERE start_date LIKE ?");
    q.addBindValue(month + "%");
    q.exec();
    int leaves = q.next() ? q.value(0).toInt() : 0;
    q.exec("SELECT COUNT(*) FROM leave_requests WHERE status='待审批'");
    int pending = q.next() ? q.value(0).toInt() : 0;
    q.prepare("SELECT SUM(net_salary) FROM payroll WHERE month=?");
    q.addBindValue(month);
    q.exec();
    double salary = q.next() ? q.value(0).toDouble() : 0;

    m_labels[0]->setText(QString::number(total));
    m_labels[1]->setText(QString::number(active));
    m_labels[2]->setText(QString::number(inactive));
    m_labels[3]->setText(QString::number(leaves));
    m_labels[4]->setText(QString::number(pending));
    m_labels[5]->setText(QString::number(salary, 'f', 2) + " 元");
}
