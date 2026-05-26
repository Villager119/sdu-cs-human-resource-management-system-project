#include "OrgChartView.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QGraphicsTextItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QLinearGradient>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QVariant>
#include <QDebug>

OrgChartView::OrgChartView(QWidget *parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    // Smooth rendering settings
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    
    // Background style
    setBackgroundBrush(QBrush(QColor(0x1e, 0x29, 0x3b))); // Modern slate background
    
    // Enable dragging to pan the view
    setDragMode(QGraphicsView::ScrollHandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

OrgChartView::~OrgChartView()
{
    qDeleteAll(m_nodeMap.values());
}

void OrgChartView::clearTree(OrgNode *node)
{
    if (!node) return;
    for (auto *child : node->children) {
        clearTree(child);
    }
    delete node;
}

void OrgChartView::refresh()
{
    // Clear previous tree structure
    qDeleteAll(m_nodeMap.values());
    m_rootNodes.clear();
    m_nodeMap.clear();
    m_scene->clear();

    // Reset zoom transformation
    resetTransform();

    // 1. Fetch employee names
    QMap<int, QString> empNames;
    {
        QSqlQuery eq("SELECT emp_id, name FROM employees");
        while (eq.next()) {
            empNames[eq.value(0).toInt()] = eq.value(1).toString();
        }
    }

    // 2. Fetch employee count per department
    QMap<QString, int> empCounts;
    {
        QSqlQuery ec("SELECT department, COUNT(*) FROM employees WHERE department IS NOT NULL AND department!='' AND status='在职' GROUP BY department");
        while (ec.next()) {
            empCounts[ec.value(0).toString()] = ec.value(1).toInt();
        }
    }

    // 3. Fetch departments
    {
        QSqlQuery dq("SELECT dept_id, dept_name, parent_id, manager_id FROM departments");
        while (dq.next()) {
            int id = dq.value(0).toInt();
            QString name = dq.value(1).toString();
            int parentId = dq.value(2).isNull() ? -1 : dq.value(2).toInt();
            int managerId = dq.value(3).isNull() ? -1 : dq.value(3).toInt();

            OrgNode *node = new OrgNode;
            node->id = id;
            node->name = name;
            node->parentId = parentId;
            node->manager = (managerId != -1) ? empNames.value(managerId, "无") : "无";
            node->empCount = empCounts.value(name, 0);

            m_nodeMap[id] = node;
        }
    }

    if (m_nodeMap.isEmpty()) {
        m_scene->addText("暂无部门信息，请先添加部门。")->setDefaultTextColor(QColor(0x94, 0xa3, 0xb8));
        return;
    }

    // 4. Construct tree links
    for (auto *node : m_nodeMap.values()) {
        if (node->parentId == -1 || !m_nodeMap.contains(node->parentId)) {
            m_rootNodes.append(node);
        } else {
            m_nodeMap[node->parentId]->children.append(node);
        }
    }

    // 5. Layout calculations
    qreal leftOffset = 0;
    for (auto *root : m_rootNodes) {
        computeSubtreeWidth(root);
        calculatePositions(root, leftOffset, 0);
        leftOffset += root->subtreeWidth;
    }

    // 6. Draw tree elements (cards & lines)
    for (auto *root : m_rootNodes) {
        drawTree(root);
    }

    // 7. Draw interactive overlay label with navigation tips
    QGraphicsTextItem *tips = m_scene->addText("💡 提示：按住鼠标左键拖拽平移，Ctrl + 鼠标滚轮缩放，点击卡片可选中编辑。");
    tips->setDefaultTextColor(QColor(0x64, 0x74, 0x8b));
    QFont tipsFont = tips->font();
    tipsFont.setPointSize(9);
    tips->setFont(tipsFont);
    tips->setPos(15, 15);
    tips->setFlag(QGraphicsItem::ItemIgnoresTransformations); // Overlay mode: remains constant size/position

    // Update scene bounds
    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-50, -50, 50, 50));
}

void OrgChartView::computeSubtreeWidth(OrgNode *node)
{
    if (node->children.isEmpty()) {
        node->subtreeWidth = m_nodeWidth + m_siblingSpacing;
        return;
    }
    qreal childrenWidth = 0;
    for (auto *child : node->children) {
        computeSubtreeWidth(child);
        childrenWidth += child->subtreeWidth;
    }
    node->subtreeWidth = qMax(m_nodeWidth + m_siblingSpacing, childrenWidth);
}

void OrgChartView::calculatePositions(OrgNode *node, qreal currentLeft, qreal depth)
{
    node->y = depth * m_levelHeight + 60; // Offset down slightly to avoid overlay tips

    if (node->children.isEmpty()) {
        node->x = currentLeft + (node->subtreeWidth - m_siblingSpacing - m_nodeWidth) / 2;
        return;
    }

    qreal childLeft = currentLeft;
    for (auto *child : node->children) {
        calculatePositions(child, childLeft, depth + 1);
        childLeft += child->subtreeWidth;
    }

    // Center parent above its children
    qreal firstChildX = node->children.first()->x;
    qreal lastChildX = node->children.last()->x;
    node->x = firstChildX + (lastChildX - firstChildX) / 2;
}

void OrgChartView::drawTree(OrgNode *node)
{
    // Draw cards for children first so links render behind the cards if desired
    for (auto *child : node->children) {
        drawTree(child);
        drawConnection(node, child);
    }

    // Draw current card using path for rounded rect
    QPainterPath path;
    path.addRoundedRect(node->x, node->y, m_nodeWidth, m_nodeHeight, 8, 8);
    
    QGraphicsPathItem *card = m_scene->addPath(path, QPen(QColor(0x47, 0x55, 0x69), 1.2), QBrush());
    card->setData(0, node->id);

    // Apply linear gradient background
    QLinearGradient gradient(node->x, node->y, node->x, node->y + m_nodeHeight);
    gradient.setColorAt(0, QColor(0x1e, 0x29, 0x3b)); // Dark slate top
    gradient.setColorAt(1, QColor(0x0f, 0x17, 0x2a)); // Deeper slate bottom
    card->setBrush(QBrush(gradient));

    // Shadow effect for visual depth
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(8);
    shadow->setOffset(2, 3);
    shadow->setColor(QColor(0, 0, 0, 100));
    card->setGraphicsEffect(shadow);

    // Add texts inside card
    QGraphicsTextItem *nameText = m_scene->addText(node->name);
    nameText->setDefaultTextColor(QColor(0xf8, 0xfa, 0xfc));
    QFont nameFont = nameText->font();
    nameFont.setBold(true);
    nameFont.setPointSize(11);
    nameText->setFont(nameFont);
    nameText->setPos(node->x + 10, node->y + 8);
    nameText->setParentItem(card);

    QGraphicsTextItem *managerText = m_scene->addText(QString("负责人: %1").arg(node->manager));
    managerText->setDefaultTextColor(QColor(0xcb, 0xd5, 0xe1));
    QFont mgrFont = managerText->font();
    mgrFont.setPointSize(9);
    managerText->setFont(mgrFont);
    managerText->setPos(node->x + 10, node->y + 32);
    managerText->setParentItem(card);

    QGraphicsTextItem *countText = m_scene->addText(QString("在职人数: %1人").arg(node->empCount));
    countText->setDefaultTextColor(QColor(0x38, 0xbd, 0xf8)); // Sky blue
    QFont countFont = countText->font();
    countFont.setPointSize(9);
    countText->setFont(countFont);
    countText->setPos(node->x + 10, node->y + 52);
    countText->setParentItem(card);
}

void OrgChartView::drawConnection(OrgNode *parent, OrgNode *child)
{
    qreal parentBottomX = parent->x + m_nodeWidth / 2;
    qreal parentBottomY = parent->y + m_nodeHeight;
    qreal childTopX = child->x + m_nodeWidth / 2;
    qreal childTopY = child->y;
    
    qreal midY = parentBottomY + (childTopY - parentBottomY) / 2;
    
    QPainterPath path;
    path.moveTo(parentBottomX, parentBottomY);
    path.lineTo(parentBottomX, midY);
    path.lineTo(childTopX, midY);
    path.lineTo(childTopX, childTopY);
    
    QGraphicsPathItem *lineItem = m_scene->addPath(path, QPen(QColor(0x47, 0x55, 0x69), 1.5));
    lineItem->setZValue(-1); // Connectors drawn behind cards
}

void OrgChartView::mousePressEvent(QMouseEvent *event)
{
    // Check if clicked item maps to a department card
    QGraphicsItem *item = itemAt(event->pos());
    if (item) {
        QGraphicsItem *parent = item;
        while (parent) {
            QVariant val = parent->data(0);
            if (val.isValid() && val.toInt() > 0) {
                emit departmentSelected(val.toInt());
                break;
            }
            parent = parent->parentItem();
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void OrgChartView::wheelEvent(QWheelEvent *event)
{
    // Zoom in/out with Ctrl + Wheel
    if (event->modifiers() & Qt::ControlModifier) {
        const double scaleFactor = 1.15;
        if (event->angleDelta().y() > 0) {
            scale(scaleFactor, scaleFactor);
        } else {
            scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}
