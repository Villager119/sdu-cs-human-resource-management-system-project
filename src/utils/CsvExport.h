#ifndef CSVEXPORT_H
#define CSVEXPORT_H

#include <QAbstractItemModel>
#include <QString>

void exportModelToCSV(QAbstractItemModel *model, const QString &filePath,
                      const QList<int> &skipCols = {});

#endif
