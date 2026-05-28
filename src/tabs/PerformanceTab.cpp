#include "PerformanceTab.h"
#include "performance/PerformanceDrawer.h"
#include "../widgets/CommonDelegates.h"
#include "../services/PerformanceService.h"
#include "../utils/DbUtils.h"
#include "../core/SessionManager.h"
#include "../core/GlobalEvents.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QDate>

PerformanceTab::PerformanceTab(int empId, const QString &role,
                               std::function<void(const QString&, const QString&)> logFn,
                               QWidget *parent)
    : QWidget(parent), m_empId(empId), m_role(role), m_log(logFn)
{
    // Main layout splits left (filters + table) and right (drawer panel)
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(0);

    // Left Container
    QWidget *leftContainer = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 10, 0);
    leftLayout->setSpacing(10);

    // Filters row
    QHBoxLayout *filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(8);

    filterLayout->addWidget(new QLabel("月份:"));
    m_monthFilter = new QComboBox(leftContainer);
    m_monthFilter->addItem("全部月份");
    for (int i = 0; i < 12; ++i) {
        m_monthFilter->addItem(QDate::currentDate().addMonths(-i).toString("yyyy-MM"));
    }
    m_monthFilter->setFixedWidth(110);
    m_monthFilter->setStyleSheet("QComboBox { border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px 8px; background: white; }");
    filterLayout->addWidget(m_monthFilter);

    filterLayout->addWidget(new QLabel("员工姓名:"));
    m_nameFilter = new QLineEdit(leftContainer);
    m_nameFilter->setPlaceholderText("员工姓名");
    m_nameFilter->setFixedWidth(110);
    m_nameFilter->setStyleSheet("QLineEdit { border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px 8px; background: white; }");
    filterLayout->addWidget(m_nameFilter);

    filterLayout->addWidget(new QLabel("部门:"));
    m_deptFilter = new QComboBox(leftContainer);
    m_deptFilter->addItem("全部部门");
    {
        QSqlQuery dq;
        dq.exec("SELECT DISTINCT department FROM employees WHERE department IS NOT NULL AND department != '' AND status='在职'");
        while (dq.next()) {
            m_deptFilter->addItem(dq.value(0).toString());
        }
        dq.finish();
    }
    m_deptFilter->setFixedWidth(110);
    m_deptFilter->setStyleSheet("QComboBox { border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px 8px; background: white; }");
    filterLayout->addWidget(m_deptFilter);

    filterLayout->addStretch();

    // Add Score Button
    m_btnAddScore = new QPushButton("+ 新增评分", leftContainer);
    m_btnAddScore->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 6px 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );
    if (!SessionManager::instance()->hasPermission("evaluate_performance")) {
        m_btnAddScore->setVisible(false);
    }
    filterLayout->addWidget(m_btnAddScore);
    leftLayout->addLayout(filterLayout);

    // Relational Table Model Setup
    m_model = new QSqlRelationalTableModel(this, createClonedDatabaseConnection("performance_model"));
    m_model->setTable("performance_scores");
    m_model->setRelation(1, QSqlRelation("employees", "emp_id", "name"));
    m_model->setHeaderData(0, Qt::Horizontal, "编号");
    m_model->setHeaderData(1, Qt::Horizontal, "员工");
    m_model->setHeaderData(2, Qt::Horizontal, "月份");
    m_model->setHeaderData(7, Qt::Horizontal, "总分");
    m_model->setHeaderData(10, Qt::Horizontal, "状态");
    m_model->setHeaderData(11, Qt::Horizontal, "评分人");

    m_table = new QTableView(leftContainer);
    m_table->setModel(m_model);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setShowGrid(false);
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
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 8px;"
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

    // Hide extra breakdown detail columns in table
    m_table->hideColumn(3); // attitude
    m_table->hideColumn(4); // capability
    m_table->hideColumn(5); // teamwork
    m_table->hideColumn(6); // innovation
    m_table->hideColumn(8); // comment
    m_table->hideColumn(9); // created_at

    m_table->horizontalHeader()->moveSection(11, 3);
    m_table->horizontalHeader()->moveSection(10, 5);

    m_table->setItemDelegateForColumn(1, new EmployeeAvatarDelegate(m_table));
    m_table->setItemDelegateForColumn(10, new PerformanceStatusDelegate(m_table));

    leftLayout->addWidget(m_table, 1);
    mainLayout->addWidget(leftContainer, 1);

    // Drawer Setup
    m_drawer = new PerformanceDrawer(this);
    mainLayout->addWidget(m_drawer);
    m_drawer->setVisible(false); // Hidden by default

    // Connect filters
    connect(m_monthFilter, &QComboBox::currentIndexChanged, this, &PerformanceTab::filterScores);
    connect(m_nameFilter, &QLineEdit::textChanged, this, &PerformanceTab::filterScores);
    connect(m_deptFilter, &QComboBox::currentIndexChanged, this, &PerformanceTab::filterScores);

    // Connect drawer actions
    connect(m_btnAddScore, &QPushButton::clicked, this, &PerformanceTab::onAddScoreClicked);
    connect(m_drawer, &PerformanceDrawer::closeRequested, this, &PerformanceTab::closeDrawer);
    connect(m_drawer, &PerformanceDrawer::logRequested, this, [this](const QString &act, const QString &det) {
        if (m_log) m_log(act, det);
    });
    connect(m_drawer, &PerformanceDrawer::scoreSubmitted, this, [this]() {
        m_model->select();
        closeDrawer();
        GlobalEvents::instance()->dataChanged();
    });

    // Double-click row to edit details
    connect(m_table, &QTableView::doubleClicked, this, &PerformanceTab::onTableDoubleClicked);

    // Initial load
    filterScores();
}

void PerformanceTab::filterScores()
{
    QStringList conds;

    // Check permission - employees only see published scores for themselves
    if (!SessionManager::instance()->hasPermission("evaluate_performance")) {
        conds << QString("performance_scores.emp_id = %1").arg(m_empId);
        conds << "performance_scores.status = '已发布'";
    }

    // Month filter
    if (m_monthFilter->currentIndex() > 0) {
        conds << QString("performance_scores.eval_month = '%1'").arg(m_monthFilter->currentText());
    }

    // Name search (fuzzy)
    if (!m_nameFilter->text().trimmed().isEmpty()) {
        QString namePat = m_nameFilter->text().trimmed();
        namePat.replace("'", "''");
        conds << QString("performance_scores.emp_id IN (SELECT emp_id FROM employees WHERE name LIKE '%%1%')").arg(namePat);
    }

    // Department filter
    if (m_deptFilter->currentIndex() > 0) {
        QString dept = m_deptFilter->currentText();
        dept.replace("'", "''");
        conds << QString("performance_scores.emp_id IN (SELECT emp_id FROM employees WHERE department = '%1')").arg(dept);
    }

    m_model->setFilter(conds.join(" AND "));
    m_model->select();
}

void PerformanceTab::onAddScoreClicked()
{
    m_drawer->setupAddMode();
    m_drawer->setVisible(true);
}

void PerformanceTab::onTableDoubleClicked(const QModelIndex &index)
{
    if (!SessionManager::instance()->hasPermission("evaluate_performance")) {
        return;
    }

    int row = index.row();
    int scoreId = m_model->index(row, 0).data().toInt();

    const PerformanceService::ScoreDetail detail = PerformanceService().scoreDetail(scoreId);
    if (detail.found) {
        m_drawer->setupEditMode(detail.employeeId, detail.month, detail.attitude,
                                detail.capability, detail.teamwork, detail.innovation,
                                detail.comment);
        m_drawer->setVisible(true);
    }
}

void PerformanceTab::closeDrawer()
{
    m_drawer->setVisible(false);
}

void PerformanceTab::refresh()
{
    filterScores();
}
