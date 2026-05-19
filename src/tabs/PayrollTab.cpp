#include "PayrollTab.h"
#include "../core/Constants.h"
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
    // system_settings 表
    QSqlQuery q;
    q.exec("CREATE TABLE IF NOT EXISTS system_settings (key_name VARCHAR(50) PRIMARY KEY, value VARCHAR(200) NOT NULL) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    q.exec("INSERT IGNORE INTO system_settings VALUES('work_days_per_month','21.75'),('tax_threshold','5000')");

    // 扩展 payroll 表
    struct Col { QString name, after; };
    for (auto &c : {Col{"performance_bonus","leave_deduction"},
                    Col{"pension","performance_bonus"},
                    Col{"medical","pension"},
                    Col{"unemployment","medical"},
                    Col{"housing_fund","unemployment"},
                    Col{"income_tax","housing_fund"}}) {
        q.exec(QString("SHOW COLUMNS FROM payroll LIKE '%1'").arg(c.name));
        if (q.size() == 0)
            q.exec(QString("ALTER TABLE payroll ADD COLUMN %1 DECIMAL(10,2) DEFAULT 0.00 AFTER %2").arg(c.name, c.after));
    }

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

    if (m_role == HR::Role::USER)
        m_model->setFilter(QString("payroll.emp_id=%1").arg(m_empId));

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table, 1);

    auto *row = new QHBoxLayout;
    row->addStretch();
    m_btnCalc = new QPushButton("一键核算本月工资");
    if (m_role == HR::Role::USER) m_btnCalc->setVisible(false);
    row->addWidget(m_btnCalc);
    auto *btnCSV = new QPushButton("导出CSV");
    row->addWidget(btnCSV);
    row->addStretch();
    layout->addLayout(row);

    connect(m_btnCalc, &QPushButton::clicked, this, &PayrollTab::calculate);
    connect(btnCSV, &QPushButton::clicked, this, &PayrollTab::exportCSV);

    m_model->select();
}

void PayrollTab::calculate()
{
    QString month = QDate::currentDate().toString("yyyy-MM");
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM payroll WHERE month=?");
    check.addBindValue(month);
    check.exec();
    if (check.next() && check.value(0).toInt() > 0) {
        int r = QMessageBox::question(this, "重新核算警告",
            month + " 月份的工资已经核算过了！\n再次核算将覆盖原有数据，确定要继续吗？",
            QMessageBox::Yes | QMessageBox::No);
        if (r == QMessageBox::No) return;
        QSqlQuery del; del.prepare("DELETE FROM payroll WHERE month=?"); del.addBindValue(month); del.exec();
    }

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QSqlQuery emp("SELECT emp_id, base_salary FROM employees");
    int ok = 0;
    while (emp.next()) {
        int eid = emp.value("emp_id").toInt();
        double bs = emp.value("base_salary").toDouble();
        QSqlQuery lv; lv.prepare("SELECT SUM(DATEDIFF(end_date, start_date)+1) FROM leave_requests WHERE emp_id=? AND status='已同意' AND start_date LIKE ?");
        lv.addBindValue(eid); lv.addBindValue(month + "%"); lv.exec();
        int days = (lv.next()) ? lv.value(0).toInt() : 0;
        double wd = 21.75;
        QSqlQuery sq; sq.exec("SELECT value FROM system_settings WHERE key_name='work_days_per_month'");
        if (sq.next()) wd = sq.value(0).toDouble();
        double ded = (bs / wd) * days;
        // 绩效奖金：>=90 分 10%，>=70 分 5%
        double perf = 0.0;
        QSqlQuery pq; pq.prepare("SELECT score FROM performance_scores WHERE emp_id=? AND eval_month=?");
        pq.addBindValue(eid); pq.addBindValue(month); pq.exec();
        if (pq.next()) {
            double s = pq.value(0).toDouble();
            if (s >= 90) perf = bs * 0.10;
            else if (s >= 70) perf = bs * 0.05;
        }
        // 五险一金
        double pension = 0, medical = 0, unemployment = 0, housing = 0;
        QSqlQuery sc("SELECT item_name, rate_personal FROM salary_config WHERE enabled=1");
        while (sc.next()) {
            QString name = sc.value(0).toString();
            double rate = sc.value(1).toDouble();
            double val = bs * rate;
            if (name == "养老保险") pension = val;
            else if (name == "医疗保险") medical = val;
            else if (name == "失业保险") unemployment = val;
            else if (name == "住房公积金") housing = val;
        }
        // 个税（阶梯税率）
        double threshold = 5000;
        QSqlQuery tq; tq.exec("SELECT value FROM system_settings WHERE key_name='tax_threshold'");
        if (tq.next()) threshold = tq.value(0).toDouble();
        double taxable = bs - ded + perf - pension - medical - unemployment - housing - threshold;
        double tax = 0;
        if (taxable > 25000) tax = taxable * 0.25 - 2660;
        else if (taxable > 12000) tax = taxable * 0.20 - 1410;
        else if (taxable > 3000) tax = taxable * 0.10 - 210;
        else if (taxable > 0) tax = taxable * 0.03;
        if (tax < 0) tax = 0;

        double net = bs - ded + perf - pension - medical - unemployment - housing - tax;
        QSqlQuery ins; ins.prepare("INSERT INTO payroll(emp_id,month,base_salary,leave_deduction,performance_bonus,pension,medical,unemployment,housing_fund,income_tax,net_salary,issue_date) VALUES(?,?,?,?,?,?,?,?,?,?,?,?)");
        ins.addBindValue(eid); ins.addBindValue(month); ins.addBindValue(bs);
        ins.addBindValue(ded); ins.addBindValue(perf);
        ins.addBindValue(pension); ins.addBindValue(medical); ins.addBindValue(unemployment); ins.addBindValue(housing); ins.addBindValue(tax);
        ins.addBindValue(net); ins.addBindValue(today);
        if (!ins.exec()) {
            db.rollback();
            QMessageBox::critical(this, "核算失败", "写入工资条时出错: " + ins.lastError().text());
            return;
        }
        ok++;
    }
    db.commit();
    QMessageBox::information(this, "核算完成", QString("一键核算完毕！\n成功生成了 %1 名员工的 %2 月份工资单。").arg(ok).arg(month));
    m_log("核算工资", month + " (" + QString::number(ok) + "人)");
    m_model->select();
}

void PayrollTab::exportCSV()
{
    QString path = QFileDialog::getSaveFileName(this, "导出工资表", "薪酬数据.csv", "CSV文件 (*.csv)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";
    for (int c = 0; c < m_model->columnCount(); c++)
        out << "\"" << m_model->headerData(c, Qt::Horizontal).toString() << "\"" << (c < m_model->columnCount()-1 ? "," : "\n");
    for (int r = 0; r < m_model->rowCount(); r++) {
        for (int c = 0; c < m_model->columnCount(); c++)
            out << "\"" << m_model->data(m_model->index(r, c)).toString().replace("\"", "\"\"") << "\"" << (c < m_model->columnCount()-1 ? "," : "\n");
    }
    f.close();
    QMessageBox::information(this, "导出成功", "薪酬数据已导出至:\n" + path);
}
