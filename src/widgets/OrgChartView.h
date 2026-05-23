#ifndef ORGCHARTVIEW_H
#define ORGCHARTVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMap>
#include <QList>

struct OrgNode {
    int id;
    QString name;
    QString manager;
    int empCount;
    int parentId;
    QList<OrgNode*> children;
    
    // Position metrics for layout
    qreal x = 0;
    qreal y = 0;
    qreal subtreeWidth = 0;
};

class OrgChartView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit OrgChartView(QWidget *parent = nullptr);
    ~OrgChartView();

    void refresh();

signals:
    void departmentSelected(int deptId);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void clearTree(OrgNode *node);
    void computeSubtreeWidth(OrgNode *node);
    void calculatePositions(OrgNode *node, qreal currentLeft, qreal depth);
    void drawTree(OrgNode *node);
    void drawConnection(OrgNode *parent, OrgNode *child);

    QGraphicsScene *m_scene;
    QList<OrgNode*> m_rootNodes;
    QMap<int, OrgNode*> m_nodeMap;

    const qreal m_nodeWidth = 180;
    const qreal m_nodeHeight = 80;
    const qreal m_levelHeight = 160;
    const qreal m_siblingSpacing = 40;
};

#endif // ORGCHARTVIEW_H
