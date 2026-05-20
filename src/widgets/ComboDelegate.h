#ifndef COMBODELEGATE_H
#define COMBODELEGATE_H

#include <QStyledItemDelegate>

class ComboDelegate : public QStyledItemDelegate
{
public:
    explicit ComboDelegate(const QStringList &options, QObject *parent = nullptr);
    explicit ComboDelegate(QAbstractItemModel *model, int valueColumn, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:
    QStringList m_options;
    QAbstractItemModel *m_lookupModel = nullptr;
    int m_lookupColumn = 0;
};

#endif
