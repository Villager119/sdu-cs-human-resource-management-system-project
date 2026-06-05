#include "DashboardTab.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QSqlQuery>
#include <QDate>
#include "../core/SessionManager.h"
#include <QSqlError>
#include <QDebug>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QPdfWriter>
#include <QPainter>
#include <QMessageBox>
#include <QCursor>
#include <QDateTime>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QAbstractBarSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QChart>

DashboardTab::DashboardTab(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    m_alertLabel = new QLabel;
    m_alertLabel->setVisible(false);
    m_alertLabel->setStyleSheet("background: #fef3c7; border: 1px solid #f59e0b; border-radius: 8px; padding: 12px 18px; color: #b45309; font-size: 13px; font-weight: bold;");
    m_alertLabel->setWordWrap(true);
    mainLayout->addWidget(m_alertLabel);

    // Create horizontal layout to hold left metrics and right chart
    auto *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(20);
    mainLayout->addLayout(contentLayout);

    // Left Container
    auto *leftContainer = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(15);
    contentLayout->addWidget(leftContainer, 5);

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

    leftLayout->addWidget(m_profileFrame);

    m_quickActionsFrame = new QFrame;
    m_quickActionsFrame->setObjectName("quickActionsFrame");
    m_quickActionsFrame->setStyleSheet("QFrame#quickActionsFrame { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    auto *quickLayout = new QHBoxLayout(m_quickActionsFrame);
    quickLayout->setContentsMargins(16, 14, 16, 14);
    quickLayout->setSpacing(10);

    auto makeQuickButton = [&](const QString &text, const QString &target) {
        auto *btn = new QPushButton(text, m_quickActionsFrame);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);
        btn->setStyleSheet(
            "QPushButton { background-color: #eff6ff; color: #1d4ed8; border: 1px solid #bfdbfe; border-radius: 6px; padding: 8px 12px; font-weight: bold; }"
            "QPushButton:hover { background-color: #dbeafe; border-color: #60a5fa; }"
        );
        connect(btn, &QPushButton::clicked, this, [this, target]() {
            emit shortcutRequested(target);
        });
        quickLayout->addWidget(btn);
    };
    makeQuickButton("上班/下班打卡", "attendance");
    makeQuickButton("请假/补卡申请", "application");
    makeQuickButton("查看工资条", "payroll");
    makeQuickButton("信息修改", "profile");
    leftLayout->addWidget(m_quickActionsFrame);

    auto *grid = new QGridLayout;
    grid->setSpacing(15);
    leftLayout->addLayout(grid);

    QString titles[] = {"总员工数", "在职人数", "离职人数", "本月请假", "待审批", "本月薪资总额"};
    QString icons[] = {"👥", "✅", "🚪", "📋", "⏳", "💰"};
    for (int i = 0; i < 6; i++) {
        auto *frame = new QFrame;
        frame->setMinimumSize(180, 110);
        frame->setStyleSheet("QFrame { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; } QFrame:hover { border: 1px solid #3b82f6; }");

        auto *l = new QVBoxLayout(frame);
        l->setSpacing(4);
        l->setContentsMargins(18, 14, 18, 14);

        m_iconLabels[i] = new QLabel(icons[i]);
        m_iconLabels[i]->setAlignment(Qt::AlignLeft);
        m_iconLabels[i]->setStyleSheet("font-size: 22px; border: none; background: transparent;");

        m_titleLabels[i] = new QLabel(titles[i]);
        m_titleLabels[i]->setAlignment(Qt::AlignLeft);
        m_titleLabels[i]->setStyleSheet("font-size: 12px; color: #64748b; border: none; background: transparent;");

        m_labels[i] = new QLabel("-");
        m_labels[i]->setAlignment(Qt::AlignRight | Qt::AlignBottom);
        m_labels[i]->setStyleSheet("font-size: 26px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        l->addWidget(m_iconLabels[i]);
        l->addWidget(m_titleLabels[i]);
        l->addWidget(m_labels[i], 1);
        grid->addWidget(frame, i / 2, i % 2); // Arrange KPI cards in 3x2 format
    }

    // Right Container: Chart Panel
    QFrame *chartPanel = new QFrame(this);
    chartPanel->setObjectName("chartPanel");
    chartPanel->setStyleSheet("QFrame#chartPanel { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    chartPanel->setMinimumWidth(400);

    QVBoxLayout *chartPanelLayout = new QVBoxLayout(chartPanel);
    chartPanelLayout->setContentsMargins(15, 15, 15, 15);
    chartPanelLayout->setSpacing(10);

    // Chart header widget (combobox and export button)
    QWidget *chartHeaderWidget = new QWidget(chartPanel);
    QHBoxLayout *chartHeaderLayout = new QHBoxLayout(chartHeaderWidget);
    chartHeaderLayout->setContentsMargins(0, 0, 0, 0);
    chartHeaderLayout->setSpacing(8);

    chartHeaderLayout->addWidget(new QLabel("分析图表:", chartHeaderWidget));

    m_chartCombo = new QComboBox(chartHeaderWidget);
    m_chartCombo->addItems({
        "部门人数分布",
        "在职/离职比例",
        "学历分布",
        "婚姻状况比例",
        "岗位分布",
        "入职年份分布",
        "工资区间统计",
        "各部门平均薪资",
        "月度请假统计",
        "月度薪资趋势"
    });
    chartHeaderLayout->addWidget(m_chartCombo, 1);

    m_exportPdfBtn = new QPushButton("导出PDF", chartHeaderWidget);
    m_exportPdfBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #0284c7;"
        "  border: 1px solid #0369a1;"
        "  color: #ffffff;"
        "  padding: 5px 12px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0ea5e9;"
        "}"
    );
    chartHeaderLayout->addWidget(m_exportPdfBtn);

    chartPanelLayout->addWidget(chartHeaderWidget);

    m_chartView = new QChartView(chartPanel);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setStyleSheet("background: transparent; border: none;");
    m_chartHoverTip = new QLabel(m_chartView);
    m_chartHoverTip->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_chartHoverTip->setVisible(false);
    m_chartHoverTip->setStyleSheet(
        "QLabel {"
        "  background-color: #ffffff;"
        "  color: #0f172a;"
        "  border: 1px solid #94a3b8;"
        "  border-radius: 6px;"
        "  padding: 6px 8px;"
        "  font-size: 12px;"
        "}"
    );
    chartPanelLayout->addWidget(m_chartView, 1);

    contentLayout->addWidget(chartPanel, 4);

    m_isAdmin = SessionManager::instance()->hasPermission("view_reports");
    chartHeaderWidget->setVisible(m_isAdmin);

    connect(m_chartCombo, &QComboBox::currentIndexChanged, this, &DashboardTab::onChartTypeChanged);
    connect(m_exportPdfBtn, &QPushButton::clicked, this, &DashboardTab::exportPDF);
}

void DashboardTab::refresh()
{
    int empId = SessionManager::instance()->empId;
    if (empId == 0) return;

    {
        QSqlQuery q;

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
    q.finish();

    bool isAdmin = SessionManager::instance()->hasPermission("manage_employees") || SessionManager::instance()->hasPermission("calculate_payroll");

    if (isAdmin) {
        m_quickActionsFrame->setVisible(false);
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
        q.finish();
        q.exec("SELECT COUNT(*) FROM employees WHERE status='在职'");
        int active = q.next() ? q.value(0).toInt() : 0;
        q.finish();
        q.exec("SELECT COUNT(*) FROM employees WHERE status!='在职'");
        int inactive = q.next() ? q.value(0).toInt() : 0;
        q.finish();
        QString month = QDate::currentDate().toString("yyyy-MM");
        QDate monthStart = QDate::fromString(month + "-01", "yyyy-MM-dd");
        QString monthStartStr = monthStart.toString("yyyy-MM-dd");
        QString monthEndStr = monthStart.addMonths(1).addDays(-1).toString("yyyy-MM-dd");
        q.prepare("SELECT COUNT(*) FROM leave_requests WHERE start_date <= ? AND end_date >= ?");
        q.addBindValue(monthEndStr);
        q.addBindValue(monthStartStr);
        q.exec();
        int leaves = q.next() ? q.value(0).toInt() : 0;
        q.finish();
        int pending = 0;
        q.exec("SELECT COUNT(*) FROM leave_requests WHERE status='待审批'");
        if (q.next()) pending += q.value(0).toInt();
        q.finish();
        q.exec("SELECT COUNT(*) FROM makeup_requests WHERE status='待审批'");
        if (q.next()) pending += q.value(0).toInt();
        q.finish();
        q.exec("SELECT COUNT(*) FROM profile_change_requests WHERE status='待审批'");
        if (q.next()) pending += q.value(0).toInt();
        q.finish();
        q.prepare("SELECT SUM(net_salary) FROM payroll WHERE month=?");
        q.addBindValue(month);
        q.exec();
        double salary = q.next() ? q.value(0).toDouble() : 0;
        q.finish();

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
        q.finish();
    } else {
        m_quickActionsFrame->setVisible(true);
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
        q.finish();
        m_labels[0]->setText(attStatus);
        m_labels[0]->setStyleSheet(attStatus.length() > 6 ? 
            "font-size: 20px; font-weight: 700; color: #2563eb; border: none; background: transparent;" :
            "font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        // 2. 本月请假次数
        QString month = QDate::currentDate().toString("yyyy-MM");
        QDate monthStart = QDate::fromString(month + "-01", "yyyy-MM-dd");
        QString monthStartStr = monthStart.toString("yyyy-MM-dd");
        QString monthEndStr = monthStart.addMonths(1).addDays(-1).toString("yyyy-MM-dd");
        q.prepare("SELECT COUNT(*) FROM leave_requests WHERE emp_id = ? AND status = '已同意' AND start_date <= ? AND end_date >= ?");
        q.addBindValue(empId);
        q.addBindValue(monthEndStr);
        q.addBindValue(monthStartStr);
        q.exec();
        int leavesCount = q.next() ? q.value(0).toInt() : 0;
        q.finish();
        m_labels[1]->setText(QString("%1 次").arg(leavesCount));
        m_labels[1]->setStyleSheet("font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        // 3. 我的待办事项
        q.prepare("SELECT COUNT(*) FROM leave_requests WHERE emp_id = ? AND status = '待审批'");
        q.addBindValue(empId);
        q.exec();
        int pLeaves = q.next() ? q.value(0).toInt() : 0;
        q.finish();
        q.prepare("SELECT COUNT(*) FROM makeup_requests WHERE emp_id = ? AND status = '待审批'");
        q.addBindValue(empId);
        q.exec();
        int pMakeups = q.next() ? q.value(0).toInt() : 0;
        q.finish();
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
        q.finish();
        m_labels[3]->setText(salaryStr);
        m_labels[3]->setStyleSheet(salaryStr.length() > 8 ? 
            "font-size: 20px; font-weight: 700; color: #2563eb; border: none; background: transparent;" :
            "font-size: 30px; font-weight: 700; color: #2563eb; border: none; background: transparent;");

        // 5. 最新绩效评分
        q.prepare("SELECT score FROM performance_scores WHERE emp_id = ? AND status = '已发布' "
                  "ORDER BY eval_month DESC LIMIT 1");
        q.addBindValue(empId);
        QString perfStr = "暂无评分";
        if (q.exec() && q.next()) {
            perfStr = QString("%1 分").arg(QString::number(q.value(0).toDouble(), 'f', 1));
        }
        q.finish();
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
        q.finish();
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
        q.finish();
    }
    }

    // Dynamic Chart rendering
    m_isAdmin = SessionManager::instance()->hasPermission("view_reports");
    m_chartCombo->parentWidget()->setVisible(m_isAdmin);
    refreshChart();
}

void DashboardTab::onChartTypeChanged(int)
{
    refreshChart();
}

void DashboardTab::exportPDF()
{
    QString path = QFileDialog::getSaveFileName(this, "导出PDF", "报表.pdf", "PDF文件 (*.pdf)");
    if (path.isEmpty()) return;

    QPdfWriter w(path);
    w.setPageSize(QPageSize(QPageSize::A4));
    w.setPageOrientation(QPageLayout::Landscape);
    w.setResolution(300);

    QPainter p(&w);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect page = w.pageLayout().paintRectPixels(w.resolution());
    const int margin = 120;
    QRect content = page.adjusted(margin, margin, -margin, -margin);

    QFont titleFont("Microsoft YaHei", 18, QFont::Bold);
    QFont normalFont("Microsoft YaHei", 10);
    QFont headerFont("Microsoft YaHei", 10, QFont::Bold);

    p.setFont(titleFont);
    p.setPen(QColor("#0f172a"));
    const QString chartTitle = m_chartCombo ? m_chartCombo->currentText() : "分析图表";
    p.drawText(content.left(), content.top(), content.width(), 70,
               Qt::AlignLeft | Qt::AlignVCenter, chartTitle + "报表");

    p.setFont(normalFont);
    p.setPen(QColor("#475569"));
    p.drawText(content.left(), content.top() + 75, content.width(), 45,
               Qt::AlignLeft | Qt::AlignVCenter,
               "生成时间：" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    QRect chartRect(content.left(), content.top() + 140, content.width() * 0.58, content.height() - 180);
    QRect tableRect(chartRect.right() + 80, chartRect.top(), content.right() - chartRect.right() - 80, chartRect.height());

    p.save();
    p.setClipRect(chartRect);
    m_chartView->render(&p, chartRect, m_chartView->rect());
    p.restore();

    const QList<QPair<QString, double>> rows = currentChartData();
    p.setPen(QPen(QColor("#cbd5e1"), 2));
    p.setBrush(QColor("#f8fafc"));
    p.drawRect(tableRect);

    const int rowHeight = 42;
    const int nameWidth = tableRect.width() * 0.62;
    int y = tableRect.top();
    p.setFont(headerFont);
    p.setPen(QColor("#0f172a"));
    p.drawText(tableRect.left() + 18, y, nameWidth - 18, rowHeight, Qt::AlignVCenter, "分类");
    p.drawText(tableRect.left() + nameWidth, y, tableRect.width() - nameWidth - 18, rowHeight, Qt::AlignVCenter | Qt::AlignRight, "数值");
    y += rowHeight;

    p.setFont(normalFont);
    double total = 0;
    for (const auto &row : rows) total += row.second;
    for (const auto &row : rows) {
        if (y + rowHeight > tableRect.bottom()) {
            p.setPen(QColor("#64748b"));
            p.drawText(tableRect.left() + 18, y, tableRect.width() - 36, rowHeight,
                       Qt::AlignVCenter, "数据较多，剩余项目请在系统图表中查看。");
            break;
        }
        p.setPen(QPen(QColor("#e2e8f0"), 1));
        p.drawLine(tableRect.left(), y, tableRect.right(), y);
        p.setPen(QColor("#334155"));
        p.drawText(tableRect.left() + 18, y, nameWidth - 24, rowHeight,
                   Qt::AlignVCenter | Qt::TextWordWrap, row.first);
        const QString valueText = QString("%1  (%2%)")
            .arg(QString::number(row.second, 'f', row.second == int(row.second) ? 0 : 2))
            .arg(total > 0 ? QString::number(row.second * 100.0 / total, 'f', 1) : "0.0");
        p.drawText(tableRect.left() + nameWidth, y, tableRect.width() - nameWidth - 18, rowHeight,
                   Qt::AlignVCenter | Qt::AlignRight, valueText);
        y += rowHeight;
    }

    p.end();

    QMessageBox::information(this, "导出成功", "报表已导出至:\n" + path);
}

QList<QPair<QString, double>> DashboardTab::currentChartData() const
{
    QList<QPair<QString, double>> rows;
    if (!m_chartCombo) return rows;

    QSqlQuery q;
    switch (m_chartCombo->currentIndex()) {
    case 0:
        q.exec("SELECT COALESCE(NULLIF(department,''),'未分配'), COUNT(*) FROM employees WHERE status='在职' GROUP BY department");
        break;
    case 1:
        q.exec("SELECT status, COUNT(*) FROM employees GROUP BY status");
        break;
    case 2:
        q.exec("SELECT COALESCE(NULLIF(education,''),'未填写'), COUNT(*) FROM employees WHERE status='在职' GROUP BY education");
        break;
    case 3:
        q.exec("SELECT COALESCE(NULLIF(marital_status,''),'未填写'), COUNT(*) FROM employees WHERE status='在职' GROUP BY marital_status");
        break;
    case 4:
        q.exec("SELECT COALESCE(NULLIF(position,''),'未指定'), COUNT(*) FROM employees WHERE status='在职' GROUP BY position");
        break;
    case 5:
        q.exec("SELECT DATE_FORMAT(hire_date, '%Y年'), COUNT(*) FROM employees WHERE hire_date IS NOT NULL GROUP BY 1 ORDER BY 1");
        break;
    case 6:
        rows << qMakePair(QString("<5000元"), 0.0)
             << qMakePair(QString("5000-10000元"), 0.0)
             << qMakePair(QString("10000-20000元"), 0.0)
             << qMakePair(QString(">=20000元"), 0.0);
        if (q.exec("SELECT "
                   "SUM(CASE WHEN base_salary < 5000 THEN 1 ELSE 0 END), "
                   "SUM(CASE WHEN base_salary >= 5000 AND base_salary < 10000 THEN 1 ELSE 0 END), "
                   "SUM(CASE WHEN base_salary >= 10000 AND base_salary < 20000 THEN 1 ELSE 0 END), "
                   "SUM(CASE WHEN base_salary >= 20000 THEN 1 ELSE 0 END) "
                   "FROM employees WHERE status='在职'") && q.next()) {
            for (int i = 0; i < rows.size(); ++i) rows[i].second = q.value(i).toDouble();
        }
        q.finish();
        return rows;
    case 7:
        q.exec("SELECT COALESCE(NULLIF(department,''),'未分配'), AVG(base_salary) FROM employees WHERE base_salary>0 AND status='在职' GROUP BY department");
        break;
    case 8:
        q.exec("SELECT DATE_FORMAT(start_date,'%Y-%m'), COUNT(*) FROM leave_requests GROUP BY 1 ORDER BY 1");
        break;
    case 9:
        q.exec("SELECT month, SUM(net_salary) FROM payroll GROUP BY month ORDER BY month");
        break;
    default:
        return rows;
    }

    while (q.next()) {
        rows.append({q.value(0).toString(), q.value(1).toDouble()});
    }
    q.finish();
    return rows;
}

void DashboardTab::refreshChart()
{
    auto *old = m_chartView->chart();
    auto *chart = new QChart;
    chart->setTheme(QChart::ChartThemeLight);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundRoundness(10);

    QFont font = chart->titleFont();
    font.setFamily("Microsoft YaHei");
    font.setPointSize(11);
    font.setBold(true);
    chart->setTitleFont(font);

    auto compactLabel = [](QString label, int maxChars = 8) {
        label = label.trimmed();
        if (label.isEmpty()) return QString("未填写");
        return label.size() > maxChars ? label.left(maxChars - 1) + "…" : label;
    };

    auto formatPieLabel = [&](QPieSlice *slice, const QString &unit) {
        const QString full = slice->label();
        slice->setLabel(QString("%1: %2%3")
            .arg(compactLabel(full))
            .arg(slice->value())
            .arg(unit));
    };

    auto installPieTooltip = [&](QPieSlice *slice, const QString &unit) {
        const QString fullName = slice->label();
        connect(slice, &QPieSlice::hovered, this, [this, slice, fullName, unit](bool hovered) {
            slice->setExploded(hovered);
            if (hovered) {
                m_chartHoverTip->setText(QString("%1\n数量：%2%3\n占比：%4%")
                    .arg(fullName)
                    .arg(slice->value())
                    .arg(unit)
                    .arg(QString::number(slice->percentage() * 100, 'f', 1)));
                m_chartHoverTip->adjustSize();
                QPoint p = m_chartView->mapFromGlobal(QCursor::pos()) + QPoint(12, 12);
                p.setX(qMin(p.x(), m_chartView->width() - m_chartHoverTip->width() - 8));
                p.setY(qMin(p.y(), m_chartView->height() - m_chartHoverTip->height() - 8));
                p.setX(qMax(p.x(), 8));
                p.setY(qMax(p.y(), 8));
                m_chartHoverTip->move(p);
                m_chartHoverTip->raise();
                m_chartHoverTip->show();
            } else {
                m_chartHoverTip->hide();
            }
        });
    };

    auto tunePieSeries = [](QPieSeries *series) {
        series->setLabelsPosition(QPieSlice::LabelOutside);
        series->setLabelsVisible(series->count() <= 6);
        if (series->count() > 6) {
            series->setPieSize(0.72);
            series->setHoleSize(0.18);
        }
    };

    auto applyDistinctPieColors = [](QPieSeries *series) {
        const QList<QColor> colors = {
            QColor("#2563eb"), // blue
            QColor("#f97316"), // orange
            QColor("#16a34a"), // green
            QColor("#9333ea"), // purple
            QColor("#dc2626"), // red
            QColor("#0891b2"), // cyan
            QColor("#ca8a04"), // amber
            QColor("#db2777"), // pink
            QColor("#4f46e5"), // indigo
            QColor("#65a30d"), // lime
            QColor("#0f766e"), // teal
            QColor("#7c2d12")  // brown
        };

        int i = 0;
        for (auto *slice : series->slices()) {
            slice->setBrush(colors.at(i % colors.size()));
            slice->setPen(QPen(Qt::white, 1.5));
            ++i;
        }
    };

    auto appendCompactCategory = [&](QStringList &categories, const QString &raw) {
        categories << compactLabel(raw, 7);
    };

    auto tuneCategoryAxis = [](QBarCategoryAxis *axis, int count) {
        if (count > 5) {
            axis->setLabelsAngle(-35);
        }
        axis->setLabelsFont(QFont("Microsoft YaHei", 8));
    };

    auto shouldUseBarForCategories = [](const QList<QPair<QString, int>> &items) {
        if (items.size() > 5) return true;
        for (const auto &item : items) {
            if (item.first.trimmed().size() > 6) return true;
        }
        return false;
    };

    auto addCategoryBarChart = [&](const QList<QPair<QString, int>> &items,
                                   const QString &title,
                                   const QString &seriesName,
                                   const QString &axisTitle) {
        auto *set = new QBarSet(seriesName);
        QStringList categories;
        for (const auto &item : items) {
            appendCompactCategory(categories, item.first);
            *set << item.second;
        }

        auto *series = new QBarSeries;
        series->append(set);
        series->setLabelsVisible(true);
        series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);
        chart->addSeries(series);

        auto *axisX = new QBarCategoryAxis;
        axisX->append(categories);
        tuneCategoryAxis(axisX, categories.size());
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        auto *axisY = new QValueAxis;
        axisY->setTitleText(axisTitle);
        axisY->setLabelFormat("%d");
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        chart->setTitle(title);
        chart->legend()->setVisible(false);
    };

    if (!m_isAdmin) {
        chart->setTitle("本月个人考勤状态统计");

        QPieSeries *series = new QPieSeries();
        int normalCount = 0, lateCount = 0, earlyCount = 0, lateEarlyCount = 0;
        int empId = SessionManager::instance()->empId;

        QSqlQuery aq;
        aq.prepare("SELECT status, COUNT(*) FROM attendances "
                   "WHERE emp_id = ? AND att_date LIKE ? "
                   "GROUP BY status");
        aq.addBindValue(empId);
        aq.addBindValue(QDate::currentDate().toString("yyyy-MM") + "%");
        if (aq.exec()) {
            while (aq.next()) {
                QString status = aq.value(0).toString();
                int count = aq.value(1).toInt();
                if (status == "正常") normalCount += count;
                else if (status == "迟到") lateCount += count;
                else if (status == "早退") earlyCount += count;
                else if (status == "迟到/早退") lateEarlyCount += count;
            }
        }
        aq.finish();

        if (normalCount > 0) series->append("正常出勤", normalCount);
        if (lateCount > 0) series->append("迟到", lateCount);
        if (earlyCount > 0) series->append("早退", earlyCount);
        if (lateEarlyCount > 0) series->append("迟到/早退", lateEarlyCount);

        if (series->isEmpty()) {
            series->append("无打卡记录", 1);
        }
        tunePieSeries(series);
        for (auto *slice : series->slices()) {
            if (slice->label() == "正常出勤") {
                slice->setBrush(QBrush(QColor(0x10, 0xb9, 0x81))); // Modern Green
            } else if (slice->label() == "迟到") {
                slice->setBrush(QBrush(QColor(0xf5, 0x9e, 0x0b))); // Modern Orange
            } else if (slice->label() == "早退") {
                slice->setBrush(QBrush(QColor(0xef, 0x44, 0x44))); // Modern Red
            } else if (slice->label() == "迟到/早退") {
                slice->setBrush(QBrush(QColor(0xf9, 0x73, 0x16))); // Modern Orange
            } else {
                slice->setBrush(QBrush(QColor(0x94, 0xa3, 0xb8))); // Slate Grey
            }

            formatPieLabel(slice, "次");

            installPieTooltip(slice, "次");
        }
        chart->addSeries(series);
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);
    } else {
        QSqlQuery q;
        int type = m_chartCombo->currentIndex();
        switch (type) {
        case 0: {
            q.exec("SELECT department, COUNT(*) FROM employees WHERE status='在职' GROUP BY department");
            QList<QPair<QString, int>> items;
            while (q.next()) {
                QString d = q.value(0).toString();
                if (d.isEmpty()) d = "未分配";
                items.append({d, q.value(1).toInt()});
            }
            q.finish();
            auto *s = new QPieSeries;
            for (const auto &item : items) s->append(item.first, item.second);
            tunePieSeries(s);
            applyDistinctPieColors(s);
            for (auto *slice : s->slices()) {
                formatPieLabel(slice, "人");
                installPieTooltip(slice, "人");
            }
            chart->addSeries(s);
            chart->setTitle("部门人数分布");
            chart->legend()->setVisible(true);
            chart->legend()->setAlignment(Qt::AlignBottom);
            break;
        }
        case 1: {
            q.exec("SELECT status, COUNT(*) FROM employees GROUP BY status");
            auto *s = new QPieSeries;
            while (q.next()) s->append(q.value(0).toString(), q.value(1).toInt());
            q.finish();
            tunePieSeries(s);
            applyDistinctPieColors(s);
            for (auto *slice : s->slices()) {
                formatPieLabel(slice, "人");
                installPieTooltip(slice, "人");
            }
            chart->addSeries(s);
            chart->setTitle("在职/离职比例");
            chart->legend()->setVisible(true);
            chart->legend()->setAlignment(Qt::AlignBottom);
            break;
        }
        case 2: {
            q.exec("SELECT education, COUNT(*) FROM employees WHERE status='在职' GROUP BY education");
            QList<QPair<QString, int>> items;
            while (q.next()) {
                QString label = q.value(0).toString().trimmed();
                if (label.isEmpty()) label = "未填写";
                items.append({label, q.value(1).toInt()});
            }
            q.finish();
            auto *s = new QPieSeries;
            for (const auto &item : items) s->append(item.first, item.second);
            tunePieSeries(s);
            applyDistinctPieColors(s);
            for (auto *slice : s->slices()) {
                formatPieLabel(slice, "人");
                installPieTooltip(slice, "人");
            }
            chart->addSeries(s);
            chart->setTitle("学历分布");
            chart->legend()->setVisible(true);
            chart->legend()->setAlignment(Qt::AlignBottom);
            break;
        }
        case 3: {
            q.exec("SELECT marital_status, COUNT(*) FROM employees WHERE status='在职' GROUP BY marital_status");
            QList<QPair<QString, int>> items;
            while (q.next()) {
                QString label = q.value(0).toString().trimmed();
                if (label.isEmpty()) label = "未填写";
                items.append({label, q.value(1).toInt()});
            }
            q.finish();
            auto *s = new QPieSeries;
            for (const auto &item : items) s->append(item.first, item.second);
            tunePieSeries(s);
            applyDistinctPieColors(s);
            for (auto *slice : s->slices()) {
                formatPieLabel(slice, "人");
                installPieTooltip(slice, "人");
            }
            chart->addSeries(s);
            chart->setTitle("婚姻状况比例");
            chart->legend()->setVisible(true);
            chart->legend()->setAlignment(Qt::AlignBottom);
            break;
        }
        case 4: {
            q.exec("SELECT position, COUNT(*) FROM employees WHERE status='在职' GROUP BY position");
            QList<QPair<QString, int>> items;
            while (q.next()) {
                QString label = q.value(0).toString().trimmed();
                if (label.isEmpty()) label = "未指定";
                items.append({label, q.value(1).toInt()});
            }
            q.finish();
            auto *s = new QPieSeries;
            for (const auto &item : items) s->append(item.first, item.second);
            tunePieSeries(s);
            applyDistinctPieColors(s);
            for (auto *slice : s->slices()) {
                formatPieLabel(slice, "人");
                installPieTooltip(slice, "人");
            }
            chart->addSeries(s);
            chart->setTitle("岗位分布");
            chart->legend()->setVisible(true);
            chart->legend()->setAlignment(Qt::AlignBottom);
            break;
        }
        case 5: {
            q.exec("SELECT DATE_FORMAT(hire_date, '%Y'), COUNT(*) FROM employees WHERE hire_date IS NOT NULL GROUP BY 1 ORDER BY 1");
            auto *set = new QBarSet("入职人数");
            QStringList cats;
            while (q.next()) {
                appendCompactCategory(cats, q.value(0).toString() + "年");
                *set << q.value(1).toInt();
            }
            q.finish();
            auto *s = new QBarSeries; s->append(set);
            chart->addSeries(s);
            auto *ax = new QBarCategoryAxis; ax->append(cats);
            tuneCategoryAxis(ax, cats.size());
            chart->addAxis(ax, Qt::AlignBottom); s->attachAxis(ax);
            auto *ay = new QValueAxis; ay->setTitleText("人数");
            chart->addAxis(ay, Qt::AlignLeft); s->attachAxis(ay);
            chart->setTitle("入职年份分布");
            chart->legend()->setVisible(false);
            break;
        }
        case 6: {
            q.exec("SELECT "
                   "SUM(CASE WHEN base_salary < 5000 THEN 1 ELSE 0 END), "
                   "SUM(CASE WHEN base_salary >= 5000 AND base_salary < 10000 THEN 1 ELSE 0 END), "
                   "SUM(CASE WHEN base_salary >= 10000 AND base_salary < 20000 THEN 1 ELSE 0 END), "
                   "SUM(CASE WHEN base_salary >= 20000 THEN 1 ELSE 0 END) "
                   "FROM employees WHERE status='在职'");
            auto *set = new QBarSet("员工人数");
            QStringList cats = {"<5000元", "5000-10000元", "10000-20000元", ">=20000元"};
            if (q.next()) {
                *set << q.value(0).toInt()
                     << q.value(1).toInt()
                     << q.value(2).toInt()
                     << q.value(3).toInt();
            } else {
                *set << 0 << 0 << 0 << 0;
            }
            q.finish();
            auto *s = new QBarSeries; s->append(set);
            chart->addSeries(s);
            auto *ax = new QBarCategoryAxis; ax->append(cats);
            tuneCategoryAxis(ax, cats.size());
            chart->addAxis(ax, Qt::AlignBottom); s->attachAxis(ax);
            auto *ay = new QValueAxis; ay->setTitleText("人数");
            chart->addAxis(ay, Qt::AlignLeft); s->attachAxis(ay);
            chart->setTitle("员工工资区间分布");
            chart->legend()->setVisible(false);
            break;
        }
        case 7: {
            q.exec("SELECT department, AVG(base_salary) FROM employees WHERE base_salary>0 AND status='在职' GROUP BY department");
            auto *set = new QBarSet("平均薪资");
            QStringList cats;
            while (q.next()) { appendCompactCategory(cats, q.value(0).toString()); *set << q.value(1).toDouble(); }
            q.finish();
            auto *s = new QBarSeries; s->append(set);
            chart->addSeries(s);
            auto *ax = new QBarCategoryAxis; ax->append(cats);
            tuneCategoryAxis(ax, cats.size());
            chart->addAxis(ax, Qt::AlignBottom); s->attachAxis(ax);
            auto *ay = new QValueAxis; ay->setTitleText("元");
            chart->addAxis(ay, Qt::AlignLeft); s->attachAxis(ay);
            chart->setTitle("各部门平均薪资");
            chart->legend()->setVisible(false);
            break;
        }
        case 8: {
            q.exec("SELECT DATE_FORMAT(start_date,'%Y-%m'), COUNT(*) FROM leave_requests GROUP BY 1 ORDER BY 1");
            auto *set = new QBarSet("请假人次");
            QStringList cats;
            while (q.next()) { appendCompactCategory(cats, q.value(0).toString()); *set << q.value(1).toInt(); }
            q.finish();
            auto *s = new QBarSeries; s->append(set);
            chart->addSeries(s);
            auto *ax = new QBarCategoryAxis; ax->append(cats);
            tuneCategoryAxis(ax, cats.size());
            chart->addAxis(ax, Qt::AlignBottom); s->attachAxis(ax);
            auto *ay = new QValueAxis; ay->setTitleText("人次");
            chart->addAxis(ay, Qt::AlignLeft); s->attachAxis(ay);
            chart->setTitle("月度请假统计");
            chart->legend()->setVisible(false);
            break;
        }
        case 9: {
            q.exec("SELECT month, SUM(net_salary) FROM payroll GROUP BY month ORDER BY month");
            auto *s = new QLineSeries;
            s->setName("薪资总额");
            QStringList cats;
            while (q.next()) { appendCompactCategory(cats, q.value(0).toString()); s->append(cats.size()-1, q.value(1).toDouble()); }
            q.finish();
            chart->addSeries(s);
            auto *ax = new QBarCategoryAxis; ax->append(cats);
            tuneCategoryAxis(ax, cats.size());
            chart->addAxis(ax, Qt::AlignBottom); s->attachAxis(ax);
            auto *ay = new QValueAxis; ay->setTitleText("元");
            chart->addAxis(ay, Qt::AlignLeft); s->attachAxis(ay);
            chart->setTitle("月度薪资趋势");
            chart->legend()->setVisible(false);
            break;
        }
        }
    }

    m_chartView->setChart(chart);
    delete old;
}
