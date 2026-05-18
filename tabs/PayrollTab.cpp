#include "PayrollTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRelation>
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
    m_model->setHeaderData(5, Qt::Horizontal, "实发最终工资");
    m_model->setHeaderData(6, Qt::Horizontal, "结算发薪日期");

    if (m_role == "user")
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
    if (m_role == "user") m_btnCalc->setVisible(false);
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

    QSqlQuery emp("SELECT emp_id, base_salary FROM employees");
    int ok = 0;
    while (emp.next()) {
        int eid = emp.value("emp_id").toInt();
        double bs = emp.value("base_salary").toDouble();
        QSqlQuery lv; lv.prepare("SELECT SUM(DATEDIFF(end_date, start_date)+1) FROM leave_requests WHERE emp_id=? AND status='已同意' AND start_date LIKE ?");
        lv.addBindValue(eid); lv.addBindValue(month + "%"); lv.exec();
        int days = (lv.next()) ? lv.value(0).toInt() : 0;
        double ded = (bs / 21.75) * days;
        double net = bs - ded;
        QSqlQuery ins; ins.prepare("INSERT INTO payroll(emp_id,month,base_salary,leave_deduction,net_salary,issue_date) VALUES(?,?,?,?,?,?)");
        ins.addBindValue(eid); ins.addBindValue(month); ins.addBindValue(bs);
        ins.addBindValue(ded); ins.addBindValue(net); ins.addBindValue(today);
        if (ins.exec()) ok++;
    }
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
