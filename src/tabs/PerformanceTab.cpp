#include "PerformanceTab.h"
#include "../utils/Toast.h"
#include "../core/SessionManager.h"
#include "../core/GlobalEvents.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QRegularExpression>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>
#include <QIcon>
#include <QPixmap>
#include <QSqlRelation>
#include <QFrame>

// Circular letter avatar icon creator
static QIcon createAvatarIcon(const QString &name)
{
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    uint hash = qHash(name);
    int hue = hash % 360;
    QColor bgColor = QColor::fromHsl(hue, 120, 160);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 24, 24);
    painter.setPen(Qt::white);
    QFont f = painter.font();
    f.setBold(true);
    f.setPointSize(9);
    painter.setFont(f);
    painter.drawText(pix.rect(), Qt::AlignCenter, name.left(1));
    return QIcon(pix);
}

// Custom delegate to render circular avatar next to name in employees column
class EmployeeAvatarDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QString name = index.data(Qt::DisplayRole).toString();
        if (name.isEmpty()) {
            QStyledItemDelegate::paint(painter, opt, index);
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // Highlight selection
        if (opt.state & QStyle::State_Selected) {
            painter->fillRect(opt.rect, QColor("#f1f5f9"));
            opt.state &= ~QStyle::State_Selected;
        }

        // Draw circle avatar
        uint hash = qHash(name);
        int hue = hash % 360;
        QColor avatarBg = QColor::fromHsl(hue, 120, 165);

        int avatarSize = 24;
        int paddingX = 8;
        int avatarX = opt.rect.x() + paddingX;
        int avatarY = opt.rect.y() + (opt.rect.height() - avatarSize) / 2;
        QRect avatarRect(avatarX, avatarY, avatarSize, avatarSize);

        painter->setBrush(avatarBg);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(avatarRect);

        // Draw letter
        painter->setPen(Qt::white);
        QFont f = opt.font;
        f.setBold(true);
        f.setPointSize(9);
        painter->setFont(f);
        painter->drawText(avatarRect, Qt::AlignCenter, name.left(1));

        // Draw name label
        QRect textRect = opt.rect;
        textRect.setLeft(avatarX + avatarSize + 8);
        painter->setPen(QColor("#0f172a"));
        f.setBold(false);
        f.setPointSize(10);
        painter->setFont(f);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, name);

        painter->restore();
    }
};

// Custom delegate to render and toggle published status
class PerformanceStatusDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QString status = index.data(Qt::DisplayRole).toString();

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        if (opt.state & QStyle::State_Selected) {
            painter->fillRect(opt.rect, QColor("#f1f5f9"));
            opt.state &= ~QStyle::State_Selected;
        }

        bool isPublished = (status == "已发布");

        int trackWidth = 56;
        int trackHeight = 22;
        int trackX = opt.rect.x() + (opt.rect.width() - trackWidth) / 2;
        int trackY = opt.rect.y() + (opt.rect.height() - trackHeight) / 2;
        QRect trackRect(trackX, trackY, trackWidth, trackHeight);

        QColor trackColor = isPublished ? QColor("#22c55e") : QColor("#94a3b8"); // Green vs Slate-Gray
        QColor knobColor = Qt::white;

        // Draw switch track
        painter->setBrush(trackColor);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(trackRect, trackHeight / 2, trackHeight / 2);

        // Draw switch knob
        int knobSize = 16;
        int knobY = trackY + (trackHeight - knobSize) / 2;
        int knobX = isPublished ? (trackX + trackWidth - knobSize - 3) : (trackX + 3);
        QRect knobRect(knobX, knobY, knobSize, knobSize);
        painter->setBrush(knobColor);
        painter->drawEllipse(knobRect);

        // Draw inline text
        QFont font = opt.font;
        font.setPointSize(8);
        font.setBold(true);
        painter->setFont(font);
        painter->setPen(isPublished ? Qt::white : QColor("#f1f5f9"));

        QString text = isPublished ? "发布" : "草稿";
        if (isPublished) {
            QRect textRect(trackX + 4, trackY, trackWidth - knobSize - 6, trackHeight);
            painter->drawText(textRect, Qt::AlignCenter, text);
        } else {
            QRect textRect(trackX + knobSize + 2, trackY, trackWidth - knobSize - 6, trackHeight);
            painter->drawText(textRect, Qt::AlignCenter, text);
        }

        painter->restore();
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override
    {
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (!SessionManager::instance()->hasPermission("evaluate_performance")) {
                    return false;
                }
                QString currentStatus = index.data(Qt::DisplayRole).toString();
                QString newStatus = (currentStatus == "已发布") ? "未发布" : "已发布";

                int row = index.row();
                int scoreId = model->index(row, 0).data().toInt();

                QSqlQuery q;
                q.prepare("UPDATE performance_scores SET status=? WHERE score_id=?");
                q.addBindValue(newStatus);
                q.addBindValue(scoreId);
                if (q.exec()) {
                    model->setData(index, newStatus, Qt::EditRole);
                    QSqlTableModel *sqlModel = qobject_cast<QSqlTableModel*>(model);
                    if (sqlModel) sqlModel->select();
                    return true;
                }
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
};

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
    QSqlQuery dq("SELECT DISTINCT department FROM employees WHERE department IS NOT NULL AND department != '' AND status='在职'");
    while (dq.next()) {
        m_deptFilter->addItem(dq.value(0).toString());
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
    m_model = new QSqlRelationalTableModel(this);
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

    // Move columns to visual matching order: 编号, 员工, 月份, 评分人, 总分, 状态
    // Relational model visual indices map to:
    // 0: score_id
    // 1: employee name (relational)
    // 2: eval_month
    // 11: evaluator
    // 7: score (total)
    // 10: status
    m_table->horizontalHeader()->moveSection(11, 3);
    m_table->horizontalHeader()->moveSection(10, 5);

    // Delegates for Employee card (avatar + name) and interactive status toggle switch
    m_table->setItemDelegateForColumn(1, new EmployeeAvatarDelegate(m_table));
    m_table->setItemDelegateForColumn(10, new PerformanceStatusDelegate(m_table));

    leftLayout->addWidget(m_table, 1);
    mainLayout->addWidget(leftContainer, 1);

    // Right collapsible drawer panel
    m_scorePanel = new QWidget(this);
    m_scorePanel->setFixedWidth(380);
    m_scorePanel->setObjectName("drawerPanel");
    m_scorePanel->setStyleSheet(
        "QWidget#drawerPanel {"
        "  background-color: #ffffff;"
        "  border-left: 1px solid #cbd5e1;"
        "}"
    );

    QVBoxLayout *drawerLayout = new QVBoxLayout(m_scorePanel);
    drawerLayout->setContentsMargins(15, 15, 15, 15);
    drawerLayout->setSpacing(12);

    // Title Row with close button
    QHBoxLayout *titleRow = new QHBoxLayout;
    m_drawerTitleLabel = new QLabel("新增绩效评分", m_scorePanel);
    m_drawerTitleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #0f172a;");
    m_btnCloseDrawer = new QPushButton("✕", m_scorePanel);
    m_btnCloseDrawer->setFixedSize(24, 24);
    m_btnCloseDrawer->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  color: #94a3b8;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #f1f5f9; color: #475569; }"
    );
    titleRow->addWidget(m_drawerTitleLabel);
    titleRow->addStretch();
    titleRow->addWidget(m_btnCloseDrawer);
    drawerLayout->addLayout(titleRow);

    // Form items
    QFormLayout *formLayout = new QFormLayout;
    formLayout->setSpacing(8);

    m_empCombo = new QComboBox(m_scorePanel);
    m_empCombo->setStyleSheet("QComboBox { border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; background: white; }");
    formLayout->addRow("员工:", m_empCombo);

    m_drawerMonthEdit = new QLineEdit(m_scorePanel);
    m_drawerMonthEdit->setReadOnly(true);
    m_drawerMonthEdit->setStyleSheet("QLineEdit { border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; background: #f8fafc; color: #64748b; }");
    formLayout->addRow("月份:", m_drawerMonthEdit);
    drawerLayout->addLayout(formLayout);

    // Total score card box
    QFrame *totalCard = new QFrame(m_scorePanel);
    totalCard->setObjectName("totalCard");
    totalCard->setStyleSheet(
        "QFrame#totalCard {"
        "  background-color: #eff6ff;"
        "  border: 1px solid #bfdbfe;"
        "  border-radius: 8px;"
        "  padding: 12px;"
        "}"
    );
    QVBoxLayout *totalCardLayout = new QVBoxLayout(totalCard);
    totalCardLayout->setContentsMargins(0, 0, 0, 0);
    m_totalValLabel = new QLabel("总分: 60", totalCard);
    m_totalValLabel->setObjectName("totalValLabel");
    m_totalValLabel->setStyleSheet("font-size: 26px; font-weight: 800; color: #1e3a8a;");
    m_totalValLabel->setAlignment(Qt::AlignCenter);
    totalCardLayout->addWidget(m_totalValLabel);
    drawerLayout->addWidget(totalCard);

    // 2x2 Grid Layout for scores
    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(10);

    // Setup QSpinBoxes
    m_s1 = new QSpinBox(m_scorePanel); m_s1->setRange(0, 25); m_s1->setAlignment(Qt::AlignCenter); m_s1->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_s2 = new QSpinBox(m_scorePanel); m_s2->setRange(0, 25); m_s2->setAlignment(Qt::AlignCenter); m_s2->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_s3 = new QSpinBox(m_scorePanel); m_s3->setRange(0, 25); m_s3->setAlignment(Qt::AlignCenter); m_s3->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_s4 = new QSpinBox(m_scorePanel); m_s4->setRange(0, 25); m_s4->setAlignment(Qt::AlignCenter); m_s4->setButtonSymbols(QAbstractSpinBox::NoButtons);

    QString spinStyle = 
        "QSpinBox {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  font-size: 14px;"
        "  background: white;"
        "  font-weight: bold;"
        "  color: #0f172a;"
        "}"
        "QSpinBox:focus { border: 1px solid #3b82f6; }";
    m_s1->setStyleSheet(spinStyle);
    m_s2->setStyleSheet(spinStyle);
    m_s3->setStyleSheet(spinStyle);
    m_s4->setStyleSheet(spinStyle);

    // Add columns with icons and "/ 25分"
    auto makeGridCell = [](const QString &icon, const QString &text, QSpinBox *spin, QWidget *parent) {
        QWidget *w = new QWidget(parent);
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(4);

        QLabel *lblTitle = new QLabel(icon + " " + text, w);
        lblTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #475569;");
        
        QHBoxLayout *hl = new QHBoxLayout;
        hl->setSpacing(4);
        hl->addWidget(spin);
        QLabel *lblMax = new QLabel("/ 25分", w);
        lblMax->setStyleSheet("color: #64748b; font-size: 12px;");
        hl->addWidget(lblMax);

        vl->addWidget(lblTitle);
        vl->addLayout(hl);
        return w;
    };

    grid->addWidget(makeGridCell("🤝", "工作态度", m_s1, m_scorePanel), 0, 0);
    grid->addWidget(makeGridCell("👥", "团队协作", m_s3, m_scorePanel), 0, 1);
    grid->addWidget(makeGridCell("💡", "创新能力", m_s4, m_scorePanel), 1, 0);
    grid->addWidget(makeGridCell("🛠️", "专业能力", m_s2, m_scorePanel), 1, 1);
    drawerLayout->addLayout(grid);

    // Comment
    m_commentEdit = new QTextEdit(m_scorePanel);
    m_commentEdit->setPlaceholderText("评语(可选)");
    m_commentEdit->setStyleSheet(
        "QTextEdit {"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 8px;"
        "  font-size: 13px;"
        "  background: white;"
        "}"
        "QTextEdit:focus { border: 1px solid #3b82f6; }"
    );
    m_commentEdit->setMaximumHeight(80);
    drawerLayout->addWidget(m_commentEdit);
    drawerLayout->addStretch(1);

    // Action buttons row
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    m_btnCancel = new QPushButton("取消", m_scorePanel);
    m_btnCancel->setStyleSheet(
        "QPushButton {"
        "  background-color: #f1f5f9;"
        "  color: #475569;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 8px 18px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #e2e8f0; }"
    );
    m_btnSubmit = new QPushButton("提交评分", m_scorePanel);
    m_btnSubmit->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );
    btnRow->addStretch();
    btnRow->addWidget(m_btnCancel);
    btnRow->addWidget(m_btnSubmit);
    drawerLayout->addLayout(btnRow);

    mainLayout->addWidget(m_scorePanel);
    m_scorePanel->setVisible(false); // Hidden by default

    // Load active employees list for scoring combo
    QSqlQuery eq("SELECT emp_id, name FROM employees WHERE status='在职'");
    while (eq.next()) {
        m_empCombo->addItem(createAvatarIcon(eq.value(1).toString()), eq.value(1).toString(), eq.value(0).toInt());
    }

    // Connect filters
    connect(m_monthFilter, &QComboBox::currentIndexChanged, this, &PerformanceTab::filterScores);
    connect(m_nameFilter, &QLineEdit::textChanged, this, &PerformanceTab::filterScores);
    connect(m_deptFilter, &QComboBox::currentIndexChanged, this, &PerformanceTab::filterScores);

    // Connect drawer actions
    connect(m_btnAddScore, &QPushButton::clicked, this, &PerformanceTab::onAddScoreClicked);
    connect(m_btnCloseDrawer, &QPushButton::clicked, this, &PerformanceTab::closeDrawer);
    connect(m_btnCancel, &QPushButton::clicked, this, &PerformanceTab::closeDrawer);
    connect(m_btnSubmit, &QPushButton::clicked, this, &PerformanceTab::submitScore);

    // Connect spinbox value changes to total
    connect(m_s1, &QSpinBox::valueChanged, this, &PerformanceTab::updateTotal);
    connect(m_s2, &QSpinBox::valueChanged, this, &PerformanceTab::updateTotal);
    connect(m_s3, &QSpinBox::valueChanged, this, &PerformanceTab::updateTotal);
    connect(m_s4, &QSpinBox::valueChanged, this, &PerformanceTab::updateTotal);

    // Double-click row to edit details
    connect(m_table, &QTableView::doubleClicked, this, &PerformanceTab::onTableDoubleClicked);

    // Initial load
    filterScores();
}

void PerformanceTab::updateTotal()
{
    int t = m_s1->value() + m_s2->value() + m_s3->value() + m_s4->value();
    m_totalValLabel->setText(QString("总分: %1").arg(t));
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
    QString currentMonth = QDate::currentDate().toString("yyyy-MM");
    m_drawerTitleLabel->setText("新增绩效评分");
    m_drawerMonthEdit->setText(currentMonth);

    m_empCombo->setEnabled(true);
    if (m_empCombo->count() > 0) m_empCombo->setCurrentIndex(0);

    // Reset scores to default (15 out of 25)
    m_s1->setValue(15);
    m_s2->setValue(15);
    m_s3->setValue(15);
    m_s4->setValue(15);
    m_commentEdit->clear();

    updateTotal();
    m_scorePanel->setVisible(true);
}

void PerformanceTab::onTableDoubleClicked(const QModelIndex &index)
{
    if (!SessionManager::instance()->hasPermission("evaluate_performance")) {
        return;
    }

    int row = index.row();
    int scoreId = m_model->index(row, 0).data().toInt();

    // Query details of scoreId from database to get precise integer values
    QSqlQuery q;
    q.prepare("SELECT emp_id, eval_month, attitude, capability, teamwork, innovation, comment FROM performance_scores WHERE score_id = ?");
    q.addBindValue(scoreId);
    if (q.exec() && q.next()) {
        int empId = q.value(0).toInt();
        QString month = q.value(1).toString();
        int a1 = q.value(2).toInt();
        int a2 = q.value(3).toInt();
        int a3 = q.value(4).toInt();
        int a4 = q.value(5).toInt();
        QString comment = q.value(6).toString();

        m_drawerTitleLabel->setText("编辑绩效评分");
        m_drawerMonthEdit->setText(month);

        int empIndex = m_empCombo->findData(empId);
        if (empIndex >= 0) m_empCombo->setCurrentIndex(empIndex);
        m_empCombo->setEnabled(false); // Disable changing employee during editing

        m_s1->setValue(a1);
        m_s2->setValue(a2);
        m_s3->setValue(a3);
        m_s4->setValue(a4);
        m_commentEdit->setPlainText(comment);

        updateTotal();
        m_scorePanel->setVisible(true);
    }
}

void PerformanceTab::closeDrawer()
{
    m_scorePanel->setVisible(false);
}

void PerformanceTab::submitScore()
{
    int eid = m_empCombo->currentData().toInt();
    QString month = m_drawerMonthEdit->text().trimmed();
    int a1 = m_s1->value(), a2 = m_s2->value(), a3 = m_s3->value(), a4 = m_s4->value();
    int total = a1 + a2 + a3 + a4;
    QString comment = m_commentEdit->toPlainText().trimmed();
    
    if (month.isEmpty()) { 
        Toast::show(this, "请输入考核月份", Toast::Warning); 
        return; 
    }

    QRegularExpression re("^\\d{4}-(0[1-9]|1[0-2])$");
    if (!re.match(month).hasMatch()) {
        Toast::show(this, "考核月份格式不正确，请输入 YYYY-MM 格式", Toast::Warning);
        return;
    }

    QString evaluator = SessionManager::instance()->empName;

    // Check if score record already exists for this employee and month
    QSqlQuery ck; 
    ck.prepare("SELECT COUNT(*) FROM performance_scores WHERE emp_id=? AND eval_month=?");
    ck.addBindValue(eid); 
    ck.addBindValue(month); 
    ck.exec();
    
    QSqlQuery q;
    if (ck.next() && ck.value(0).toInt() > 0) {
        q.prepare("UPDATE performance_scores SET attitude=?, capability=?, teamwork=?, innovation=?, score=?, comment=?, status='已发布', evaluator=?, created_at=NOW() WHERE emp_id=? AND eval_month=?");
        q.addBindValue(a1); q.addBindValue(a2); q.addBindValue(a3); q.addBindValue(a4);
        q.addBindValue(total); q.addBindValue(comment); q.addBindValue(evaluator);
        q.addBindValue(eid); q.addBindValue(month);
    } else {
        q.prepare("INSERT INTO performance_scores(emp_id, eval_month, attitude, capability, teamwork, innovation, score, comment, status, evaluator) VALUES(?,?,?,?,?,?,?,?,'已发布',?)");
        q.addBindValue(eid); q.addBindValue(month); q.addBindValue(a1); q.addBindValue(a2);
        q.addBindValue(a3); q.addBindValue(a4); q.addBindValue(total); q.addBindValue(comment);
        q.addBindValue(evaluator);
    }

    if (!q.exec()) { 
        QMessageBox::critical(this, "错误", q.lastError().text()); 
        return; 
    }

    m_log("绩效评分", QString("%1 %2月 态度%3 能力%4 协作%5 创新%6 → 总分%7")
          .arg(m_empCombo->currentText(), month).arg(a1).arg(a2).arg(a3).arg(a4).arg(total));
    Toast::show(this, QString("%1月份绩效已提交，总分: %2").arg(month).arg(total), Toast::Success);
    
    closeDrawer();
    m_model->select();
    GlobalEvents::instance()->dataChanged();
}

void PerformanceTab::refresh() 
{ 
    filterScores(); 
}
