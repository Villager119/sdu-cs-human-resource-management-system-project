cmake_minimum_required(VERSION 3.16)
project(HRMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Network Charts Concurrent)

set(PROJECT_SOURCES
    src/main.cpp
    src/ui/LoginWindow.cpp src/ui/LoginWindow.h src/ui/LoginWindow.ui
    src/ui/MainWindow.cpp src/ui/MainWindow.h src/ui/MainWindow.ui
    src/ui/ServerSettingsDialog.cpp src/ui/ServerSettingsDialog.h
    src/ui/ChangePasswordDialog.cpp src/ui/ChangePasswordDialog.h
    src/ui/RecoverPasswordDialog.cpp src/ui/RecoverPasswordDialog.h
    src/tabs/EmployeeTab.cpp src/tabs/EmployeeTab.h
    src/tabs/MyAttendanceTab.cpp src/tabs/MyAttendanceTab.h
    src/tabs/my_attendance/DailyClockWidget.cpp src/tabs/my_attendance/DailyClockWidget.h
    src/tabs/my_attendance/AppCenterWidget.cpp src/tabs/my_attendance/AppCenterWidget.h
    src/tabs/PayrollTab.cpp src/tabs/PayrollTab.h
    src/tabs/AuditTab.cpp src/tabs/AuditTab.h
    src/tabs/DashboardTab.cpp src/tabs/DashboardTab.h
    src/tabs/PerformanceTab.cpp src/tabs/PerformanceTab.h
    src/tabs/performance/PerformanceDrawer.cpp src/tabs/performance/PerformanceDrawer.h
    src/tabs/ProfileChangeTab.cpp src/tabs/ProfileChangeTab.h
    src/tabs/AttendManageTab.cpp src/tabs/AttendManageTab.h
    src/tabs/attend_manage/AttendBoardWidget.cpp src/tabs/attend_manage/AttendBoardWidget.h
    src/tabs/attend_manage/AttendApprovalWidget.cpp src/tabs/attend_manage/AttendApprovalWidget.h
    src/tabs/attend_manage/ShiftSettingsWidget.cpp src/tabs/attend_manage/ShiftSettingsWidget.h
    src/tabs/OrgTab.cpp src/tabs/OrgTab.h
    src/tabs/RbacTab.cpp src/tabs/RbacTab.h
    src/widgets/PaginationBar.cpp src/widgets/PaginationBar.h
    src/widgets/ComboDelegate.cpp src/widgets/ComboDelegate.h
    src/widgets/CommonDelegates.cpp src/widgets/CommonDelegates.h
    src/widgets/SafeEditDelegate.h
    src/widgets/TaxConfigPanel.cpp src/widgets/TaxConfigPanel.h
    src/widgets/OrgChartView.cpp src/widgets/OrgChartView.h
    src/widgets/OptimisticSqlTableModel.cpp src/widgets/OptimisticSqlTableModel.h
    src/utils/CsvExport.cpp src/utils/CsvExport.h
    src/utils/DbUtils.cpp src/utils/DbUtils.h
    src/utils/UiStyles.cpp src/utils/UiStyles.h
    src/core/Logger.cpp src/core/Logger.h
    src/core/SessionManager.cpp src/core/SessionManager.h
    src/core/GlobalEvents.cpp src/core/GlobalEvents.h
)

add_executable(HRMS ${PROJECT_SOURCES})

target_link_libraries(HRMS PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql Qt6::Network Qt6::Charts Qt6::Concurrent)
