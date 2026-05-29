#ifndef UNSAVEDCHANGESGUARD_H
#define UNSAVEDCHANGESGUARD_H

#include <QString>

class UnsavedChangesGuard
{
public:
    virtual ~UnsavedChangesGuard() = default;
    virtual bool hasUnsavedChanges() const = 0;
    virtual bool saveChanges() = 0;
    virtual void discardChanges() = 0;
    virtual QString unsavedChangesTitle() const { return QStringLiteral("保存修改"); }
    virtual QString unsavedChangesMessage() const
    {
        return QStringLiteral("当前页面还有未保存的修改，是否先保存再离开？");
    }
};

#endif
