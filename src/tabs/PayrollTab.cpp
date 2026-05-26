#include "PayrollTab.h"
#include "../utils/CsvExport.h"
#include "../core/Constants.h"
#include "../core/GlobalEvents.h"
#include "../core/SessionManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRelation>
#include <QSqlDatabase>
#include <QDate>
#include <QFileDialog>
#include <QTextStream>
#include <QSqlError>

PayrollTab::PayrollTab(int empId, const QString &role,
                       std::function<void(const QString&, const QString&)> logFn,
                       QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn)
{
    m_model = new QSqlRelationalTableModel(this);
    m_model->setTable("payroll");
    m_model->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_model->setHeaderData(0, Qt::Horizontal, "工资条ID");
    m_model->setHeaderData(1, Qt::Horizontal, "姓名");
    m_model->setHeaderData(2, Qt::Horizontal, "薪资月份");
    m_model->setHeaderData(3, Qt::Horizontal, "基础工资");
    m_model->setHeaderData(4, Qt::Horizontal, "请假扣款");
    m_model->setHeaderData(5, Qt::Horizontal, "绩效奖金");
    m_model->setHeaderData(6, Qt::Horizontal, "养老保险");
    m_model->setHeaderData(7, Qt::Horizontal, "医疗保险");
    m_model->setHeaderData(8, Qt::Horizontal, "失业保险");
    m_model->setHeaderData(9, Qt::Horizontal, "住房公积金");
    m_model->setHeaderData(10, Qt::Horizontal, "个人所得税");
    m_model->setHeaderData(11, Qt::Horizontal, "实发最终工资");
    m_model->setHeaderData(12, Qt::Horizontal, "结算发薪日期");

    if (!SessionManager::instance()->hasPermission("calculate_payroll"))
        m_model->setFilter(QString("payroll.emp_id=%1").arg(m_empId));

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 筛选栏
    auto *filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("选择月份:"));
    m_monthCombo = new QComboBox;
    filterRow->addWidget(m_monthCombo);

    m_btnSearch = new QPushButton("查询");
    m_btnReset = new QPushButton("重置");
    filterRow->addWidget(m_btnSearch);
    filterRow->addWidget(m_btnReset);
    filterRow->addStretch();
    layout->addLayout(filterRow);

    layout->addWidget(m_table, 1);

    auto *row = new QHBoxLayout;
    row->addStretch();
    m_btnCalc = new QPushButton("一键核算本月工资");
    if (!SessionManager::instance()->hasPermission("calculate_payroll")) m_btnCalc->setVisible(false);
    row->addWidget(m_btnCalc);
    auto *btnCSV = new QPushButton("导出CSV");
    row->addWidget(btnCSV);
    row->addStretch();
    layout->addLayout(row);

    connect(m_btnCalc, &QPushButton::clicked, this, &PayrollTab::calculate);
    connect(btnCSV, &QPushButton::clicked, this, &PayrollTab::exportCSV);
    connect(m_btnSearch, &QPushButton::clicked, this, &PayrollTab::filterPayroll);
    connect(m_btnReset, &QPushButton::clicked, this, [this]() {
        m_monthCombo->setCurrentIndex(0);
        filterPayroll();
    });

    refreshMonthCombo();
    m_model->select();
}

void PayrollTab::calculate()
{
    QString month = QDate::currentDate().toString("yyyy-MM");
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    bool exists = false;
    {
        QSqlQuery check;
        check.prepare("SELECT COUNT(*) FROM payroll WHERE month=?");
        check.addBindValue(month);
        check.exec();
        if (check.next() && check.value(0).toInt() > 0) {
            exists = true;
        }
    }

    if (exists) {
        int r = QMessageBox::question(this, "重新核算警告",
            month + " 月份的工资已经核算过了！\n再次核算将覆盖原有数据，确定要继续吗？",
            QMessageBox::Yes | QMessageBox::No);
        if (r == QMessageBox::No) return;

        QSqlQuery del;
        del.prepare("DELETE FROM payroll WHERE month=?");
        del.addBindValue(month);
        if (!del.exec()) {
            QMessageBox::critical(this, "删除失败", "删除原有工资条失败: " + del.lastError().text());
            return;
        }
    }

    // 获取全局配置（所有员工共用，查询提前到循环外）
    double wd = 21.75;
    {
        QSqlQuery sq; sq.exec("SELECT value FROM system_settings WHERE key_name='work_days_per_month'");
        if (sq.next()) {
            double temp = sq.value(0).toDouble();
            if (temp > 0) wd = temp;
        }
    }
    struct TaxItem { QString name; double rate; };
    QList<TaxItem> taxItems;
    {
        QSqlQuery sc("SELECT item_name, rate_personal FROM salary_config WHERE enabled=1");
        while (sc.next())
            taxItems.append({sc.value(0).toString(), sc.value(1).toDouble()});
    }
    double threshold = 5000;
    {
        QSqlQuery tq; tq.exec("SELECT value FROM system_settings WHERE key_name='tax_threshold'");
        if (tq.next()) threshold = tq.value(0).toDouble();
    }

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    struct ActiveEmp {
        int emp_id;
        double base_salary;
    };
    QList<ActiveEmp> activeEmployees;
    {
        QSqlQuery emp("SELECT emp_id, base_salary FROM employees WHERE status='在职'");
        while (emp.next()) {
            activeEmployees.append({emp.value(0).toInt(), emp.value(1).toDouble()});
        }
    } // QSqlQuery emp goes out of scope and frees the connection statement handle here

    int ok = 0;
    for (const auto &emp : activeEmployees) {
        int eid = emp.emp_id;
        double bs = emp.base_salary;
        int days = 0;
        {
            QSqlQuery lv;
            lv.prepare("SELECT SUM(DATEDIFF(end_date, start_date)+1) FROM leave_requests WHERE emp_id=? AND status='已同意' AND start_date LIKE ?");
            lv.addBindValue(eid); lv.addBindValue(month + "%");
            if (lv.exec() && lv.next()) {
                days = lv.value(0).toInt();
            }
        }
        double ded = (bs / wd) * days;

        // 绩效奖金：>=90 分 10%，>=70 分 5%
        double perf = 0.0;
        {
            QSqlQuery pq;
            pq.prepare("SELECT score FROM performance_scores WHERE emp_id=? AND eval_month=?");
            pq.addBindValue(eid); pq.addBindValue(month);
            if (pq.exec() && pq.next()) {
                double s = pq.value(0).toDouble();
                if (s >= 90) perf = bs * 0.10;
                else if (s >= 70) perf = bs * 0.05;
            }
        }

        // 五险一金
        double pension = 0, medical = 0, unemployment = 0, housing = 0;
        for (const auto &ti : taxItems) {
            double val = bs * ti.rate;
            if (ti.name == "养老保险") pension = val;
            else if (ti.name == "医疗保险") medical = val;
            else if (ti.name == "失业保险") unemployment = val;
            else if (ti.name == "住房公积金") housing = val;
        }
        // 个税（阶梯税率）
        double taxable = bs - ded + perf - pension - medical - unemployment - housing - threshold;
        double tax = 0;
        if (taxable > 25000) tax = taxable * 0.25 - 2660;
        else if (taxable > 12000) tax = taxable * 0.20 - 1410;
        else if (taxable > 3000) tax = taxable * 0.10 - 210;
        else if (taxable > 0) tax = taxable * 0.03;
        if (tax < 0) tax = 0;

        double net = bs - ded + perf - pension - medical - unemployment - housing - tax;
        bool insOk = false;
        QString insErr;
        {
            QSqlQuery ins;
            ins.prepare("INSERT INTO payroll(emp_id,month,base_salary,leave_deduction,performance_bonus,pension,medical,unemployment,housing_fund,income_tax,net_salary,issue_date) VALUES(?,?,?,?,?,?,?,?,?,?,?,?)");
            ins.addBindValue(eid); ins.addBindValue(month); ins.addBindValue(bs);
            ins.addBindValue(ded); ins.addBindValue(perf);
            ins.addBindValue(pension); ins.addBindValue(medical); ins.addBindValue(unemployment); ins.addBindValue(housing); ins.addBindValue(tax);
            ins.addBindValue(net); ins.addBindValue(today);
            insOk = ins.exec();
            if (!insOk) insErr = ins.lastError().text();
        }
        if (!insOk) {
            db.rollback();
            QMessageBox::critical(this, "核算失败", "写入工资条时出错: " + insErr);
            return;
        }
        ok++;
    }
    db.commit();
    QMessageBox::information(this, "核算完成", QString("一键核算完毕！\n成功生成了 %1 名员工的 %2 月份工资单。").arg(ok).arg(month));
    m_log("核算工资", month + " (" + QString::number(ok) + "人)");
    m_model->select();
    GlobalEvents::instance()->dataChanged();
}

void PayrollTab::exportCSV()
{
    QString path = QFileDialog::getSaveFileName(this, "导出工资表", "薪酬数据.csv", "CSV文件 (*.csv)");
    if (path.isEmpty()) return;
    exportModelToCSVAsync(m_model, path, this);
}

void PayrollTab::refresh()
{
    refreshMonthCombo();
    filterPayroll();
}

void PayrollTab::filterPayroll()
{
    QStringList conds;
    if (!SessionManager::instance()->hasPermission("calculate_payroll")) {
        conds << QString("payroll.emp_id = %1").arg(m_empId);
    }
    if (m_monthCombo->currentIndex() > 0) {
        QString month = m_monthCombo->currentText();
        month.replace("'", "''");
        conds << QString("payroll.month = '%1'").arg(month);
    }
    m_model->setFilter(conds.isEmpty() ? "" : conds.join(" AND "));
    m_model->select();
}

void PayrollTab::refreshMonthCombo()
{
    m_monthCombo->blockSignals(true);
    QString curMonth = m_monthCombo->currentText();
    m_monthCombo->clear();
    m_monthCombo->addItem("全部月份");
    
    {
        QSqlQuery q("SELECT DISTINCT month FROM payroll ORDER BY month DESC");
        while (q.next()) {
            m_monthCombo->addItem(q.value(0).toString());
        }
    }
    
    int idx = m_monthCombo->findText(curMonth);
    m_monthCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_monthCombo->blockSignals(false);
}

