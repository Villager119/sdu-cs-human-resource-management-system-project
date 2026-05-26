#ifndef CSVEXPORT_H
#define CSVEXPORT_H

#include <QAbstractItemModel>
#include <QString>
#include <QList>
#include <QWidget>

// Legacy synchronous export
void exportModelToCSV(QAbstractItemModel *model, const QString &filePath,
                      const QList<int> &skipCols = {});

// Modern asynchronous non-blocking export
void exportModelToCSVAsync(QAbstractItemModel *model, const QString &filePath,
                           QWidget *parent = nullptr, const QList<int> &skipCols = {});

#endif // CSVEXPORT_H
