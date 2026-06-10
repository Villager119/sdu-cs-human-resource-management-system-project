#include "TaxConfigPanel.h"
#include "../core/Constants.h"
#include "../utils/DbUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "SafeEditDelegate.h"

TaxConfigPanel::TaxConfigPanel(std::function<void(const QString&, const QString&)> logFn,
                               QWidget *parent)
    : QWidget(parent), m_log(logFn)
{
    // 全局薪资参数配置
    auto *settingsGroup = new QGroupBox("全局薪酬核算参数");
    auto *formLayout = new QFormLayout(settingsGroup);

    m_workDaysEdit = new QLineEdit;
    m_workDaysEdit->setPlaceholderText("21.75");
    formLayout->addRow("月度标准工作天数:", m_workDaysEdit);

    m_taxThresholdEdit = new QLineEdit;
    m_taxThresholdEdit->setPlaceholderText("5000");
    formLayout->addRow("个人所得税起征点 (元):", m_taxThresholdEdit);

    loadSettings();

    // 社保公积金比例模型
    m_model = new QSqlTableModel(this, createClonedDatabaseConnection("tax_config_model"));
    m_model->setTable("salary_config");
    m_model->setHeaderData(1, Qt::Horizontal, "保险项目");
    m_model->setHeaderData(2, Qt::Horizontal, "个人比例");
    m_model->setHeaderData(3, Qt::Horizontal, "启用");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->select();

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setItemDelegate(new SafeEditDelegate(this));
    m_table->hideColumn(0);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked);

    m_btnSave = new QPushButton("保存配置");
    m_btnSave->setStyleSheet("QPushButton{background:#1a2233;color:#fff;} QPushButton:hover{background:#2a3a55;}");

    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(settingsGroup);
    l->addWidget(new QLabel("社保公积金比例配置（双击表格项修改，保存后下次核算生效）"));
    l->addWidget(m_table, 1);
    
    auto *row = new QHBoxLayout;
    row->addStretch();
    row->addWidget(m_btnSave);
    l->addLayout(row);

    connect(m_btnSave, &QPushButton::clicked, this, &TaxConfigPanel::save);
}

void TaxConfigPanel::save()
{
    saveInternal(true);
}

void TaxConfigPanel::loadSettings()
{
    QSqlQuery q;
    q.prepare("SELECT value FROM system_settings WHERE key_name=?");
    q.addBindValue(HR::Config::WORK_DAYS);
    q.exec();
    m_savedWorkDays = q.next() ? q.value(0).toString() : QStringLiteral("21.75");
    q.finish();

    q.prepare("SELECT value FROM system_settings WHERE key_name=?");
    q.addBindValue(HR::Config::TAX_THRESHOLD);
    q.exec();
    m_savedTaxThreshold = q.next() ? q.value(0).toString() : QStringLiteral("5000");
    q.finish();

    m_workDaysEdit->setText(m_savedWorkDays);
    m_taxThresholdEdit->setText(m_savedTaxThreshold);
}

bool TaxConfigPanel::hasUnsavedChanges() const
{
    return (m_model && m_model->isDirty())
        || m_workDaysEdit->text().trimmed() != m_savedWorkDays
        || m_taxThresholdEdit->text().trimmed() != m_savedTaxThreshold;
}

bool TaxConfigPanel::saveChanges()
{
    return saveInternal(true);
}

void TaxConfigPanel::discardChanges()
{
    if (m_model) {
        m_model->revertAll();
        m_model->select();
    }
    loadSettings();
}

bool TaxConfigPanel::saveInternal(bool showMessage)
{
    // 输入校验
    bool ok1 = false, ok2 = false;
    double wd = m_workDaysEdit->text().toDouble(&ok1);
    double threshold = m_taxThresholdEdit->text().toDouble(&ok2);
    if (!ok1 || wd <= 0) {
        QMessageBox::warning(this, "提示", "请输入有效的工作天数！");
        return false;
    }
    if (!ok2 || threshold < 0) {
        QMessageBox::warning(this, "提示", "请输入有效的个税起征点！");
        return false;
    }
    for (int row = 0; row < m_model->rowCount(); ++row) {
        bool rateOk = false;
        const double rate = m_model->data(m_model->index(row, 2)).toDouble(&rateOk);
        if (!rateOk || rate < 0 || rate > 1) {
            QMessageBox::warning(this, "提示", "社保公积金个人比例必须是 0 到 1 之间的小数！");
            return false;
        }
    }

    QSqlDatabase db = m_model->database();
    if (!db.transaction()) {
        QMessageBox::critical(this, "失败", "启动薪资配置保存事务失败: " + db.lastError().text());
        return false;
    }

    QString err;
    QSqlQuery q(db);
    q.prepare("INSERT INTO system_settings (key_name, value) VALUES (?, ?) "
              "ON DUPLICATE KEY UPDATE value=VALUES(value)");
    q.addBindValue(HR::Config::WORK_DAYS);
    q.addBindValue(m_workDaysEdit->text().trimmed());
    if (!q.exec()) {
        err = "保存月工作天数失败: " + q.lastError().text();
    }
    q.finish();

    if (err.isEmpty()) {
        q.prepare("INSERT INTO system_settings (key_name, value) VALUES (?, ?) "
                  "ON DUPLICATE KEY UPDATE value=VALUES(value)");
        q.addBindValue(HR::Config::TAX_THRESHOLD);
        q.addBindValue(m_taxThresholdEdit->text().trimmed());
        if (!q.exec()) {
            err = "保存个税起征点失败: " + q.lastError().text();
        }
        q.finish();
    }

    if (!err.isEmpty()) {
        db.rollback();
        QMessageBox::critical(this, "失败", err);
        return false;
    }

    if (!m_model->submitAll()) {
        db.rollback();
        QMessageBox::critical(this, "失败", "社保比例保存失败: " + m_model->lastError().text());
        return false;
    }

    if (!db.commit()) {
        const QString commitError = db.lastError().text();
        db.rollback();
        QMessageBox::critical(this, "失败", "提交薪资配置保存事务失败: " + commitError);
        return false;
    }

    m_savedWorkDays = m_workDaysEdit->text().trimmed();
    m_savedTaxThreshold = m_taxThresholdEdit->text().trimmed();
    if (showMessage) {
        QMessageBox::information(this, "成功", "薪资及社保比例配置已保存");
    }
    m_log("修改薪资及社保配置", QString("月天数:%1, 起征点:%2").arg(wd).arg(threshold));
    return true;
}
