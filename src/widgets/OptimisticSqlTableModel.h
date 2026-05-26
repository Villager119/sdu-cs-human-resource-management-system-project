#ifndef OPTIMISTICSQLTABLEMODEL_H
#define OPTIMISTICSQLTABLEMODEL_H

#include <QSqlTableModel>

class OptimisticSqlTableModel : public QSqlTableModel
{
    Q_OBJECT
public:
    explicit OptimisticSqlTableModel(QObject *parent = nullptr, const QSqlDatabase &db = QSqlDatabase());
    ~OptimisticSqlTableModel() override = default;

protected:
    bool updateRowInTable(int row, const QSqlRecord &values) override;
};

#endif // OPTIMISTICSQLTABLEMODEL_H
