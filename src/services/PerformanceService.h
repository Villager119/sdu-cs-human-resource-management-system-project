#ifndef PERFORMANCESERVICE_H
#define PERFORMANCESERVICE_H

#include <QList>
#include <QSqlDatabase>
#include <QString>

class PerformanceService
{
public:
    struct EmployeeOption {
        int employeeId = 0;
        QString name;
    };

    struct ScoreDetail {
        bool found = false;
        int employeeId = 0;
        QString month;
        int attitude = 0;
        int capability = 0;
        int teamwork = 0;
        int innovation = 0;
        QString comment;
    };

    struct ScoreInput {
        int employeeId = 0;
        QString employeeName;
        QString month;
        int attitude = 0;
        int capability = 0;
        int teamwork = 0;
        int innovation = 0;
        QString comment;
        QString evaluator;
    };

    struct Result {
        bool success = false;
        int totalScore = 0;
        QString logAction;
        QString logDetails;
        QString message;
        QString errorMessage;
    };

    explicit PerformanceService(const QSqlDatabase &db = QSqlDatabase::database());

    QList<EmployeeOption> activeEmployees() const;
    ScoreDetail scoreDetail(int scoreId) const;
    Result saveScore(const ScoreInput &input);

private:
    Result fail(const QString &message) const;
    bool scoreExists(int employeeId, const QString &month) const;

    QSqlDatabase m_db;
};

#endif
