#include "ProfileChangeTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>

ProfileChangeTab::ProfileChangeTab(int empId, const QString &role,
                                   std::function<void(const QString&, const QString&)> logFn,
                                   std::function<void(int, const QString&, const QString&)> notifyFn,
                                   QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn), m_notify(notifyFn)
{
    QSqlQuery q;
    q.exec("CREATE TABLE IF NOT EXISTS profile_change_requests ("
           "request_id INT PRIMARY KEY AUTO_INCREMENT,"
           "emp_id INT NOT NULL,"
           "field_name VARCHAR(30) NOT NULL,"
           "old_value VARCHAR(200),"
           "new_value VARCHAR(200) NOT NULL,"
           "status VARCHAR(20) DEFAULT '待审批',"
           "reason TEXT,"
           "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
           "FOREIGN KEY (emp_id) REFERENCES employees(emp_id)"
           ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

    m_model = new QSqlRelationalTableModel(this);
    m_model->setTable("profile_change_requests");
    m_model->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_model->setHeaderData(0, Qt::Horizontal, "编号");
    m_model->setHeaderData(1, Qt::Horizontal, "申请人");
    m_model->setHeaderData(2, Qt::Horizontal, "字段");
    m_model->setHeaderData(3, Qt::Horizontal, "原值");
    m_model->setHeaderData(4, Qt::Horizontal, "新值");
    m_model->setHeaderData(5, Qt::Horizontal, "状态");
    m_model->setHeaderData(6, Qt::Horizontal, "理由");
    m_model->setHeaderData(7, Qt::Horizontal, "提交时间");

    if (m_role == "user")
        m_model->setFilter(QString("emp_id=%1").arg(m_empId));

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table, 1);

    // 申请面板（普通员工使用）
    auto *panel = new QGroupBox("申请修改个人信息");
    auto *form = new QFormLayout(panel);

    m_fieldCombo = new QComboBox;
    m_fieldCombo->addItems({"phone", "education", "marital_status", "position", "gender"});
    m_fieldCombo->setItemText(0, "联系电话");
    m_fieldCombo->setItemText(1, "学历");
    m_fieldCombo->setItemText(2, "婚姻状况");
    m_fieldCombo->setItemText(3, "岗位");
    m_fieldCombo->setItemText(4, "性别");
    form->addRow("修改字段:", m_fieldCombo);

    m_newValueEdit = new QLineEdit;
    m_newValueEdit->setPlaceholderText("请输入新的值");
    form->addRow("新值:", m_newValueEdit);

    m_reasonEdit = new QLineEdit;
    m_reasonEdit->setPlaceholderText("申请理由（必填）");
    form->addRow("理由:", m_reasonEdit);

    auto *btnSubmit = new QPushButton("提交申请");
    form->addRow(btnSubmit);
    connect(btnSubmit, &QPushButton::clicked, this, &ProfileChangeTab::submitRequest);

    // 审批按钮行
    auto *approveRow = new QHBoxLayout;
    m_btnApprove = new QPushButton("同意");
    m_btnReject = new QPushButton("拒绝");
    approveRow->addStretch();
    approveRow->addWidget(m_btnApprove);
    approveRow->addWidget(m_btnReject);

    if (m_role == "user") {
        m_btnApprove->setVisible(false);
        m_btnReject->setVisible(false);
    }

    layout->addWidget(panel);
    layout->addLayout(approveRow);

    connect(m_btnApprove, &QPushButton::clicked, this, &ProfileChangeTab::approve);
    connect(m_btnReject, &QPushButton::clicked, this, &ProfileChangeTab::reject);

    m_model->select();
}

void ProfileChangeTab::submitRequest()
{
    QString field = m_fieldCombo->currentText();
    QString nv = m_newValueEdit->text().trimmed();
    QString reason = m_reasonEdit->text().trimmed();
    if (nv.isEmpty() || reason.isEmpty()) { QMessageBox::warning(this, "提示", "新值和理由不能为空"); return; }

    QSqlQuery q;
    q.prepare("SELECT " + field + " FROM employees WHERE emp_id=?");
    q.addBindValue(m_empId); q.exec();
    QString ov = q.next() ? q.value(0).toString() : "";

    q.prepare("INSERT INTO profile_change_requests(emp_id,field_name,old_value,new_value,reason) VALUES(?,?,?,?,?)");
    q.addBindValue(m_empId); q.addBindValue(field); q.addBindValue(ov); q.addBindValue(nv); q.addBindValue(reason);
    if (q.exec()) {
        QMessageBox::information(this, "成功", "你的修改申请已提交，请等待管理员审批");
        m_log("提交信息修改申请", field + " → " + nv);
        m_notify(0, "信息修改申请", QString("员工申请修改%1为%2").arg(field, nv));
        m_model->select();
        m_newValueEdit->clear(); m_reasonEdit->clear();
    } else {
        QMessageBox::critical(this, "错误", q.lastError().text());
    }
}

void ProfileChangeTab::approve()
{
    int row = m_table->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, "提示", "请选中一条申请"); return; }
    int id = m_model->data(m_model->index(row, 0)).toInt();
    QString field = m_model->data(m_model->index(row, 2)).toString();
    QString nv = m_model->data(m_model->index(row, 4)).toString();
    // 从关联模型获取原始 emp_id
    int eid = m_model->index(row, 1).data(Qt::EditRole).toInt();

    QSqlQuery q;
    q.prepare("UPDATE employees SET " + field + "=? WHERE emp_id=?");
    q.addBindValue(nv); q.addBindValue(eid);
    if (!q.exec()) { QMessageBox::critical(this, "失败", q.lastError().text()); return; }

    q.prepare("UPDATE profile_change_requests SET status='已同意' WHERE request_id=?");
    q.addBindValue(id); q.exec();
    m_log("同意信息修改", QString("申请#%1 %2→%3").arg(id).arg(field, nv));
    m_notify(eid, "信息修改已批准", QString("你的%1修改申请已通过审批").arg(field));
    QMessageBox::information(this, "成功", "已批准，数据已更新");
    m_model->select();
}

void ProfileChangeTab::reject()
{
    int row = m_table->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, "提示", "请选中一条申请"); return; }
    int id = m_model->data(m_model->index(row, 0)).toInt();
    int eid = m_model->index(row, 1).data(Qt::EditRole).toInt();
    QSqlQuery q;
    q.prepare("UPDATE profile_change_requests SET status='已拒绝' WHERE request_id=?");
    q.addBindValue(id);
    if (q.exec()) {
        m_log("拒绝信息修改", QString("申请#%1").arg(id));
        m_notify(eid, "信息修改已拒绝", "你的信息修改申请未通过审批");
        QMessageBox::information(this, "成功", "已拒绝该申请");
        m_model->select();
    } else {
        QMessageBox::critical(this, "失败", q.lastError().text());
    }
}
