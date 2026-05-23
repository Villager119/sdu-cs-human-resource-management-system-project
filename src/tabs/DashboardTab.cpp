#include "DashboardTab.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QSqlQuery>
#include <QDate>
#include "../core/SessionManager.h"
#include <QSqlError>
#include <QDebug>

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

    // 个人信息卡片面板
    m_profileFrame = new QFrame;
    m_profileFrame->setObjectName("profileFrame");
    m_profileFrame->setStyleSheet("QFrame#profileFrame { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    auto *pLayout = new QVBoxLayout(m_profileFrame);
    pLayout->setContentsMargins(20, 16, 20, 16);
    pLayout->setSpacing(10);

    m_welcomeLabel = new QLabel("欢迎您！");
    m_welcomeLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #0f172a; border: none; background: transparent;");
    pLayout->addWidget(m_welcomeLabel);

    auto *infoGrid = new QGridLayout;
    infoGrid->setSpacing(10);
    pLayout->addLayout(infoGrid);

    m_infoEmpId = new QLabel("-");
    m_infoDept = new QLabel("-");
    m_infoPos = new QLabel("-");
    m_infoPhone = new QLabel("-");
    m_infoHireDate = new QLabel("-");
    m_infoEdu = new QLabel("-");

    QString labelStyles = "font-size: 13px; color: #334155; border: none; background: transparent;";
    QString titleStyles = "font-size: 13px; color: #64748b; font-weight: bold; border: none; background: transparent;";

    m_infoEmpId->setStyleSheet(labelStyles);
    m_infoDept->setStyleSheet(labelStyles);
    m_infoPos->setStyleSheet(labelStyles);
    m_infoPhone->setStyleSheet(labelStyles);
    m_infoHireDate->setStyleSheet(labelStyles);
    m_infoEdu->setStyleSheet(labelStyles);

    auto addInfoItem = [&](const QString &title, QLabel *valLabel, int r, int c) {
        auto *lbl = new QLabel(title);
        lbl->setStyleSheet(titleStyles);
        infoGrid->addWidget(lbl, r, c * 2);
        infoGrid->addWidget(valLabel, r, c * 2 + 1);
    };

    addInfoItem("员工工号:", m_infoEmpId, 0, 0);
    addInfoItem("所属部门:", m_infoDept, 0, 1);
    addInfoItem("岗位职务:", m_infoPos, 0, 2);
    addInfoItem("联系电话:", m_infoPhone, 1, 0);
    addInfoItem("入职日期:", m_infoHireDate, 1, 1);
    addInfoItem("最高学历:", m_infoEdu, 1, 2);

    mainLayout->addWidget(m_profileFrame);

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

        m_iconLabels[i] = new QLabel(icons[i]);
        m_iconLabels[i]->setAlignment(Qt::AlignLeft);
        m_iconLabels[i]->setStyleSheet("font-size: 22px; border: none; background: transparent;");

        m_titleLabels[i] = new QLabel(titles[i]);
        m_titleLabels[i]->setAlignment(Qt::AlignLeft);
        m_titleLabels[i]->setStyleSheet("font-size: 12px; color: #64748b; border: none; background: transparent;");

        m_labels[i] = new QLabel("-");
        m_labels[i]->setAlignment(Qt::AlignRight | Qt::AlignBottom);
        m_labels[i]->setStyleSheet("font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        l->addWidget(m_iconLabels[i]);
        l->addWidget(m_titleLabels[i]);
        l->addWidget(m_labels[i], 1);
        grid->addWidget(frame, i / 3, i % 3);
    }
}

void DashboardTab::refresh()
{
    QSqlQuery q;
    int empId = SessionManager::instance()->empId;
    if (empId == 0) return;

    // Query and display personal welcome info card
    q.prepare("SELECT name, role, department, position, phone, hire_date, education FROM employees WHERE emp_id = ?");
    q.addBindValue(empId);
    if (q.exec() && q.next()) {
        QString name = q.value(0).toString();
        QString role = q.value(1).toString();
        QString dept = q.value(2).toString();
        QString pos = q.value(3).toString();
        QString phone = q.value(4).toString();
        QDate hireDate = q.value(5).toDate();
        QString edu = q.value(6).toString();

        QString friendlyRole = (role == "admin") ? "系统管理员" : ((role == "user") ? "普通员工" : role);
        m_welcomeLabel->setText(QString("👋 您好，%1！您的系统角色是：%2").arg(name, friendlyRole));

        m_infoEmpId->setText(QString::number(empId));
        m_infoDept->setText(dept.isEmpty() ? "未分配" : dept);
        m_infoPos->setText(pos.isEmpty() ? "未设置" : pos);
        m_infoPhone->setText(phone.isEmpty() ? "未填写" : phone);
        m_infoHireDate->setText(hireDate.isValid() ? hireDate.toString("yyyy-MM-dd") : "未知");
        m_infoEdu->setText(edu.isEmpty() ? "未知" : edu);
    }

    bool isAdmin = SessionManager::instance()->hasPermission("manage_employees") || SessionManager::instance()->hasPermission("calculate_payroll");

    if (isAdmin) {
        // --- Mode A: Admin/HR (Company Overview) ---
        QString titles[] = {"总员工数", "在职人数", "离职人数", "本月请假", "待审批", "本月薪资总额"};
        QString icons[] = {"👥", "✅", "🚪", "📋", "⏳", "💰"};
        for (int i = 0; i < 6; i++) {
            m_titleLabels[i]->setText(titles[i]);
            m_iconLabels[i]->setText(icons[i]);
            m_labels[i]->setStyleSheet("font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");
        }

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

        // 合同到期提醒 (全员)
        q.exec("SELECT COUNT(*), GROUP_CONCAT(name SEPARATOR '、') FROM employees WHERE contract_end_date BETWEEN CURDATE() AND DATE_ADD(CURDATE(), INTERVAL 30 DAY)");
        if (q.next() && q.value(0).toInt() > 0) {
            m_alertLabel->setText(QString("⚠ 合同到期提醒：%1 等 %2 名员工劳动合同将在30天内到期，请及时续签！")
                .arg(q.value(1).toString()).arg(q.value(0).toInt()));
            m_alertLabel->setVisible(true);
        } else {
            m_alertLabel->setVisible(false);
        }
    } else {
        // --- Mode B: Normal User (Personal Dashboard) ---
        QString titles[] = {"今日打卡状态", "本月请假次数", "我的待办事项", "最新实发薪资", "最新绩效评分", "劳动合同期限"};
        QString icons[] = {"⏰", "📅", "⏳", "💵", "🏆", "📜"};
        for (int i = 0; i < 6; i++) {
            m_titleLabels[i]->setText(titles[i]);
            m_iconLabels[i]->setText(icons[i]);
        }

        // 1. 今日打卡状态
        q.prepare("SELECT clock_in, clock_out, status FROM attendances WHERE emp_id = ? AND att_date = CURDATE()");
        q.addBindValue(empId);
        QString attStatus = "未打卡";
        if (q.exec() && q.next()) {
            QString ci = q.value(0).toString();
            QString co = q.value(1).toString();
            QString status = q.value(2).toString();
            if (!ci.isEmpty() && !co.isEmpty()) {
                attStatus = QString("打卡完毕 (%1)").arg(status);
            } else if (!ci.isEmpty()) {
                attStatus = "已打上班卡";
            } else {
                attStatus = "打卡异常";
            }
        }
        m_labels[0]->setText(attStatus);
        m_labels[0]->setStyleSheet(attStatus.length() > 6 ? 
            "font-size: 20px; font-weight: 700; color: #2563eb; border: none; background: transparent;" :
            "font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        // 2. 本月请假次数
        QString month = QDate::currentDate().toString("yyyy-MM");
        q.prepare("SELECT COUNT(*) FROM leave_requests WHERE emp_id = ? AND status = '已批准' AND start_date LIKE ?");
        q.addBindValue(empId);
        q.addBindValue(month + "%");
        q.exec();
        int leavesCount = q.next() ? q.value(0).toInt() : 0;
        m_labels[1]->setText(QString("%1 次").arg(leavesCount));
        m_labels[1]->setStyleSheet("font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        // 3. 我的待办事项
        q.prepare("SELECT COUNT(*) FROM leave_requests WHERE emp_id = ? AND status = '待审批'");
        q.addBindValue(empId);
        q.exec();
        int pLeaves = q.next() ? q.value(0).toInt() : 0;
        q.prepare("SELECT COUNT(*) FROM makeup_requests WHERE emp_id = ? AND status = '待审批'");
        q.addBindValue(empId);
        q.exec();
        int pMakeups = q.next() ? q.value(0).toInt() : 0;
        int totalPending = pLeaves + pMakeups;
        m_labels[2]->setText(QString("%1 个").arg(totalPending));
        m_labels[2]->setStyleSheet("font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        // 4. 最新实发薪资
        q.prepare("SELECT month, net_salary FROM payroll WHERE emp_id = ? ORDER BY month DESC LIMIT 1");
        q.addBindValue(empId);
        QString salaryStr = "暂无工资条";
        if (q.exec() && q.next()) {
            salaryStr = QString("%1 元").arg(QString::number(q.value(1).toDouble(), 'f', 2));
        }
        m_labels[3]->setText(salaryStr);
        m_labels[3]->setStyleSheet(salaryStr.length() > 8 ? 
            "font-size: 20px; font-weight: 700; color: #2563eb; border: none; background: transparent;" :
            "font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        // 5. 最新绩效评分
        q.prepare("SELECT score FROM performance_scores WHERE emp_id = ? ORDER BY eval_month DESC LIMIT 1");
        q.addBindValue(empId);
        QString perfStr = "暂无评分";
        if (q.exec() && q.next()) {
            perfStr = QString("%1 分").arg(QString::number(q.value(0).toDouble(), 'f', 1));
        }
        m_labels[4]->setText(perfStr);
        m_labels[4]->setStyleSheet("font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        // 6. 劳动合同期限
        q.prepare("SELECT contract_end_date FROM employees WHERE emp_id = ?");
        q.addBindValue(empId);
        QString contractStr = "无合同";
        if (q.exec() && q.next() && !q.value(0).isNull()) {
            QDate endDate = q.value(0).toDate();
            int days = QDate::currentDate().daysTo(endDate);
            if (days < 0) {
                contractStr = "已到期";
            } else {
                contractStr = QString("%1 天").arg(days);
            }
        }
        m_labels[5]->setText(contractStr);
        m_labels[5]->setStyleSheet("font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        // 个人合同到期提醒
        q.prepare("SELECT contract_end_date FROM employees WHERE emp_id = ? AND contract_end_date BETWEEN CURDATE() AND DATE_ADD(CURDATE(), INTERVAL 30 DAY)");
        q.addBindValue(empId);
        if (q.exec() && q.next()) {
            m_alertLabel->setText(QString("⚠ 合同到期提醒：您的劳动合同将于 30 天内到期（到期日期：%1），请及时联系 HR 续签！")
                .arg(q.value(0).toDate().toString("yyyy-MM-dd")));
            m_alertLabel->setVisible(true);
        } else {
            m_alertLabel->setVisible(false);
        }
    }
}
