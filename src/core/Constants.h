#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace HR {
    inline const QString DEFAULT_PASSWORD = "123456";

    namespace LeaveStatus {
        inline const QString PENDING  = "待审批";
        inline const QString APPROVED = "已同意";
        inline const QString REJECTED = "已拒绝";
    }
    namespace EmpStatus {
        inline const QString ACTIVE   = "在职";
        inline const QString INACTIVE = "离职";
        inline const QString TRANSFERRED_OUT = "转出";
        inline const QString RESIGNED = "辞职";
        inline const QString DISMISSED = "辞退";
        inline const QString RETIRED = "退休";
    }
    namespace Role {
        inline const QString ADMIN = "admin";
        inline const QString USER  = "user";
    }
    namespace AttStatus {
        inline const QString NORMAL  = "正常";
        inline const QString LATE    = "迟到";
        inline const QString EARLY   = "早退";
        inline const QString MISSING = "缺卡";
        inline const QString LEAVE   = "请假";
    }
    namespace Config {
        inline const QString WORK_DAYS = "work_days_per_month";
        inline const QString TAX_THRESHOLD = "tax_threshold";
    }
}

#endif
