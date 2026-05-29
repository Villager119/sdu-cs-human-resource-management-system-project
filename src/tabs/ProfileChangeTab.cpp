#include "ProfileChangeTab.h"
#include "../services/ProfileChangeService.h"
#include "../utils/Toast.h"
#include "../utils/DbUtils.h"
#include "../core/GlobalEvents.h"
#include "../core/SessionManager.h"
#include "../widgets/CommonDelegates.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QTextEdit>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QHeaderView>

ProfileChangeTab::ProfileChangeTab(int empId, const QString &role,
                                   std::function<void(const QString&, const QString&)> logFn,
                                   std::function<void(int, const QString&, const QString&)> notifyFn,
                                   QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn), m_notify(notifyFn)
{
    m_model = new QSqlRelationalTableModel(this, createClonedDatabaseConnection("profile_change_model"));
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

    if (!SessionManager::instance()->hasPermission("approve_profile_change"))
        m_model->setFilter(QString("emp_id=%1").arg(m_empId));

    // Main layout
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    // Left Panel: Apply Modification Form
    QFrame *leftPanel = new QFrame(this);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setStyleSheet(
        "QFrame#leftPanel {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 10px;"
        "}"
    );
    leftPanel->setFixedWidth(320);

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(20, 20, 20, 20);
    leftLayout->setSpacing(12);

    QLabel *leftTitle = new QLabel("申请修改个人信息", leftPanel);
    leftTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #1d4ed8; border: none; background: transparent;");
    leftLayout->addWidget(leftTitle);

    QLabel *fieldLabel = new QLabel("修改字段", leftPanel);
    fieldLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #475569; border: none; background: transparent;");
    leftLayout->addWidget(fieldLabel);

    m_fieldCombo = new QComboBox(leftPanel);
    m_fieldCombo->addItem("📞 联系电话", "phone");
    m_fieldCombo->addItem("🎓 学历", "education");
    m_fieldCombo->addItem("💍 婚姻状况", "marital_status");
    m_fieldCombo->setStyleSheet(
        "QComboBox {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 8px 12px;"
        "  font-size: 13px;"
        "  color: #1e293b;"
        "  background-color: #ffffff;"
        "}"
        "QComboBox:hover {"
        "  border: 1px solid #3b82f6;"
        "}"
    );
    leftLayout->addWidget(m_fieldCombo);

    QLabel *newValueLabel = new QLabel("新值", leftPanel);
    newValueLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #475569; border: none; background: transparent;");
    leftLayout->addWidget(newValueLabel);

    QWidget *inputContainer = new QWidget(leftPanel);
    inputContainer->setObjectName("inputContainer");
    inputContainer->setStyleSheet(
        "QWidget#inputContainer {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  background-color: #ffffff;"
        "}"
        "QWidget#inputContainer:hover {"
        "  border: 1px solid #3b82f6;"
        "}"
    );
    QHBoxLayout *inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(10, 4, 10, 4);
    inputLayout->setSpacing(6);

    QLabel *inputIconLabel = new QLabel("📞", inputContainer);
    inputIconLabel->setStyleSheet("font-size: 14px; border: none; background: transparent;");
    inputLayout->addWidget(inputIconLabel);

    m_newValueEdit = new QLineEdit(inputContainer);
    m_newValueEdit->setObjectName("newValueEdit");
    m_newValueEdit->setPlaceholderText("请输入新的电话号码");
    m_newValueEdit->setStyleSheet(
        "QLineEdit#newValueEdit {"
        "  border: none;"
        "  background: transparent;"
        "  font-size: 13px;"
        "  color: #1e293b;"
        "  padding: 4px 0px;"
        "}"
    );
    inputLayout->addWidget(m_newValueEdit, 1);
    leftLayout->addWidget(inputContainer);

    QLabel *reasonLabel = new QLabel("理由", leftPanel);
    reasonLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #475569; border: none; background: transparent;");
    leftLayout->addWidget(reasonLabel);

    m_reasonEdit = new QTextEdit(leftPanel);
    m_reasonEdit->setObjectName("reasonEdit");
    m_reasonEdit->setPlaceholderText("申请理由 (必填)");
    m_reasonEdit->setStyleSheet(
        "QTextEdit#reasonEdit {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 8px 12px;"
        "  font-size: 13px;"
        "  color: #1e293b;"
        "  background-color: #ffffff;"
        "}"
        "QTextEdit#reasonEdit:focus {"
        "  border: 1px solid #3b82f6;"
        "}"
    );
    leftLayout->addWidget(m_reasonEdit);

    connect(m_fieldCombo, &QComboBox::currentIndexChanged, this, [inputIconLabel, this](int index) {
        if (index == 0) {
            inputIconLabel->setText("📞");
            m_newValueEdit->setPlaceholderText("请输入新的电话号码");
        } else if (index == 1) {
            inputIconLabel->setText("🎓");
            m_newValueEdit->setPlaceholderText("请输入新的学历 (如：硕士)");
        } else if (index == 2) {
            inputIconLabel->setText("💍");
            m_newValueEdit->setPlaceholderText("请输入新的婚姻状况 (如：已婚)");
        }
    });

    auto *btnSubmitLayout = new QHBoxLayout;
    btnSubmitLayout->setContentsMargins(0, 0, 0, 0);
    btnSubmitLayout->addStretch();

    m_btnSubmit = new QPushButton("提交申请", leftPanel);
    m_btnSubmit->setObjectName("btnSubmit");
    m_btnSubmit->setStyleSheet(
        "QPushButton#btnSubmit {"
        "  background-color: #2563eb;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 20px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton#btnSubmit:hover {"
        "  background-color: #1d4ed8;"
        "}"
        "QPushButton#btnSubmit:pressed {"
        "  background-color: #1e40af;"
        "}"
    );
    btnSubmitLayout->addWidget(m_btnSubmit);
    leftLayout->addLayout(btnSubmitLayout);

    connect(m_btnSubmit, &QPushButton::clicked, this, &ProfileChangeTab::submitRequest);

    mainLayout->addWidget(leftPanel);

    // Right Panel: History and Management table
    QFrame *rightPanel = new QFrame(this);
    rightPanel->setObjectName("rightPanel");
    rightPanel->setStyleSheet(
        "QFrame#rightPanel {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 10px;"
        "}"
    );
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(15);

    QLabel *rightTitle = new QLabel("历史申请记录", rightPanel);
    rightTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #0f172a; border: none; background: transparent;");
    rightLayout->addWidget(rightTitle);

    m_table = new QTableView(rightPanel);
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "  background-color: #f1f5f9;"
        "  color: #475569;"
        "  padding: 8px;"
        "  border: none;"
        "  font-weight: bold;"
        "  border-bottom: 2px solid #e2e8f0;"
        "}"
    );
    m_table->setStyleSheet(
        "QTableView {"
        "  border: none;"
        "  background-color: #ffffff;"
        "  gridline-color: #f1f5f9;"
        "  selection-background-color: #f1f5f9;"
        "  selection-color: #0f172a;"
        "}"
        "QTableView::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid #f1f5f9;"
        "}"
    );
    m_table->setItemDelegate(new ProfileChangeDelegate(m_table));
    rightLayout->addWidget(m_table, 1);

    // Approval row
    auto *approveWidget = new QWidget(rightPanel);
    auto *approveRow = new QHBoxLayout(approveWidget);
    approveRow->setContentsMargins(0, 0, 0, 0);
    approveRow->setSpacing(10);

    m_btnApprove = new QPushButton("同意", approveWidget);
    m_btnApprove->setStyleSheet(
        "QPushButton {"
        "  background-color: #22c55e;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 20px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #16a34a;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #15803d;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #f1f5f9;"
        "  color: #94a3b8;"
        "  border: 1px solid #e2e8f0;"
        "}"
    );

    m_btnReject = new QPushButton("拒绝", approveWidget);
    m_btnReject->setStyleSheet(
        "QPushButton {"
        "  background-color: #ef4444;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 20px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #dc2626;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #b91c1c;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #f1f5f9;"
        "  color: #94a3b8;"
        "  border: 1px solid #e2e8f0;"
        "}"
    );

    approveRow->addStretch();
    approveRow->addWidget(m_btnApprove);
    approveRow->addWidget(m_btnReject);
    rightLayout->addWidget(approveWidget);

    mainLayout->addWidget(rightPanel, 1);

    if (!SessionManager::instance()->hasPermission("approve_profile_change")) {
        approveWidget->setVisible(false);
    }

    m_btnApprove->setEnabled(false);
    m_btnReject->setEnabled(false);

    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ProfileChangeTab::updateApprovalButtons);

    if (!SessionManager::instance()->hasPermission("request_profile_change")) {
        leftPanel->setVisible(false);
    }

    connect(m_btnApprove, &QPushButton::clicked, this, &ProfileChangeTab::approve);
    connect(m_btnReject, &QPushButton::clicked, this, &ProfileChangeTab::reject);

    m_model->select();
}

void ProfileChangeTab::submitRequest()
{
    m_btnSubmit->setEnabled(false);
    auto restoreSubmitButton = [this]() {
        m_btnSubmit->setEnabled(true);
    };

    QString field = m_fieldCombo->currentData().toString();
    QString nv = m_newValueEdit->text().trimmed();
    QString reason = m_reasonEdit->toPlainText().trimmed();
    if (nv.isEmpty() || reason.isEmpty()) {
        Toast::show(this, "新值和理由不能为空", Toast::Warning);
        restoreSubmitButton();
        return;
    }

    const ProfileChangeService::Result result =
        ProfileChangeService().submitRequest(m_empId, field, nv, reason);
    if (result.success) {
        Toast::show(this, "修改申请已提交，等待审批", Toast::Success);
        m_log(result.logAction, result.logDetails);
        m_notify(0, result.notificationTitle, result.notificationContent);
        m_model->select();
        m_newValueEdit->clear();
        m_reasonEdit->clear();
    } else {
        QMessageBox::critical(this, "提交失败", result.errorMessage);
    }
    restoreSubmitButton();
}

void ProfileChangeTab::approve()
{
    m_btnApprove->setEnabled(false);
    m_btnReject->setEnabled(false);

    int row = m_table->currentIndex().row();
    if (row < 0) {
        Toast::show(this, "请选中一条申请", Toast::Warning);
        updateApprovalButtons();
        return;
    }
    int id = m_model->data(m_model->index(row, 0)).toInt();

    const ProfileChangeService::Result result = ProfileChangeService().approveRequest(id);
    if (!result.success) {
        QMessageBox::critical(this, "审批失败", result.errorMessage);
        updateApprovalButtons();
        return;
    }

    m_log(result.logAction, result.logDetails);
    m_notify(result.employeeId, result.notificationTitle, result.notificationContent);
    Toast::show(this, "信息修改申请已批准，数据已更新", Toast::Success);
    m_model->select();
    GlobalEvents::instance()->dataChanged();
    updateApprovalButtons();
}

void ProfileChangeTab::reject()
{
    m_btnApprove->setEnabled(false);
    m_btnReject->setEnabled(false);

    int row = m_table->currentIndex().row();
    if (row < 0) {
        Toast::show(this, "请选中一条申请", Toast::Warning);
        updateApprovalButtons();
        return;
    }
    int id = m_model->data(m_model->index(row, 0)).toInt();

    const ProfileChangeService::Result result = ProfileChangeService().rejectRequest(id);
    if (result.success) {
        m_log(result.logAction, result.logDetails);
        m_notify(result.employeeId, result.notificationTitle, result.notificationContent);
        Toast::show(this, "已拒绝该修改申请", Toast::Info);
        m_model->select();
        GlobalEvents::instance()->dataChanged();
    } else {
        QMessageBox::critical(this, "审批失败", result.errorMessage);
    }
    updateApprovalButtons();
}

void ProfileChangeTab::refresh()
{
    m_model->select();
    updateApprovalButtons();
}

void ProfileChangeTab::updateApprovalButtons()
{
    if (!m_table->selectionModel()) {
        m_btnApprove->setEnabled(false);
        m_btnReject->setEnabled(false);
        return;
    }
    auto selected = m_table->selectionModel()->selectedRows();
    if (selected.size() == 1) {
        int row = selected.first().row();
        QString status = m_model->data(m_model->index(row, 5)).toString();
        bool isPending = (status == "待审批");
        m_btnApprove->setEnabled(isPending);
        m_btnReject->setEnabled(isPending);
    } else {
        m_btnApprove->setEnabled(false);
        m_btnReject->setEnabled(false);
    }
}
