#ifndef CSVEXPORT_H
#define CSVEXPORT_H

#include <QAbstractItemModel>
#include <QString>
#include <QStringList>
#include <QList>
#include <QWidget>

// Legacy synchronous export
void exportModelToCSV(QAbstractItemModel *model, const QString &filePath,
                      const QList<int> &skipCols = {});

// Modern asynchronous non-blocking export
void exportModelToCSVAsync(QAbstractItemModel *model, const QString &filePath,
                           QWidget *parent = nullptr, const QList<int> &skipCols = {});

void exportRowsToCSVAsync(const QStringList &headers, const QList<QStringList> &rows,
                          const QString &filePath, QWidget *parent = nullptr);

#endif // CSVEXPORT_H
