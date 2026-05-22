#include "DashboardTab.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QSqlQuery>
#include <QDate>

DashboardTab::DashboardTab(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    m_alertLabel = new QLabel;
    m_alertLabel->setVisible(false);
    m_alertLabel->setStyleSheet("background: #fef3c7; border: 1px solid #f59e0b; border-radius: 8px; padding: 12px 18px; color: #b45309; font-size: 13px; font-weight: bold;");
    m_alertLabel->setWordWrap(true);
    mainLayout->addWidget(m_alertLabel);

    auto *grid = new QGridLayout;
    grid->setSpacing(15);
    mainLayout->addLayout(grid);

    QString titles[] = {"总员工数", "在职人数", "离职人数", "本月请假", "待审批", "本月薪资总额"};
    QString icons[] = {"👥", "✅", "🚪", "📋", "⏳", "💰"};
    for (int i = 0; i < 6; i++) {
        auto *frame = new QFrame;
        frame->setMinimumSize(200, 110);
        frame->setStyleSheet("QFrame { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; } QFrame:hover { border: 1px solid #3b82f6; }");

        auto *l = new QVBoxLayout(frame);
        l->setSpacing(4);
        l->setContentsMargins(20, 16, 20, 16);

        auto *icon = new QLabel(icons[i]);
        icon->setAlignment(Qt::AlignLeft);
        icon->setStyleSheet("font-size: 22px; border: none; background: transparent;");

        auto *t = new QLabel(titles[i]);
        t->setAlignment(Qt::AlignLeft);
        t->setStyleSheet("font-size: 12px; color: #64748b; border: none; background: transparent;");

        m_labels[i] = new QLabel("-");
        m_labels[i]->setAlignment(Qt::AlignRight | Qt::AlignBottom);
        m_labels[i]->setStyleSheet("font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

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

    // 合同到期提醒
    q.exec("SELECT COUNT(*), GROUP_CONCAT(name SEPARATOR '、') FROM employees WHERE contract_end_date BETWEEN CURDATE() AND DATE_ADD(CURDATE(), INTERVAL 30 DAY)");
    if (q.next() && q.value(0).toInt() > 0) {
        m_alertLabel->setText(QString("⚠ 合同到期提醒：%1 等 %2 名员工劳动合同将在30天内到期，请及时续签！")
            .arg(q.value(1).toString()).arg(q.value(0).toInt()));
        m_alertLabel->setVisible(true);
    } else {
        m_alertLabel->setVisible(false);
    }
}
