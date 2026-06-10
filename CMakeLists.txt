cmake_minimum_required(VERSION 3.16)
project(HRMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Network Charts Concurrent)

if(WIN32)
    option(HRMS_DEPLOY_QT_RUNTIME "Deploy Qt runtime files after Windows builds" ON)
endif()

function(hrms_qt_runtime_paths out_qt_bin_dir out_qt_plugins_dir out_compiler_bin_dir)
    get_target_property(qt_core_location Qt6::Core IMPORTED_LOCATION)
    if(NOT qt_core_location)
        get_target_property(qt_core_location Qt6::Core IMPORTED_LOCATION_RELEASE)
    endif()
    if(NOT qt_core_location)
        get_target_property(qt_core_location Qt6::Core LOCATION)
    endif()

    if(NOT qt_core_location)
        message(FATAL_ERROR "Unable to locate Qt6::Core runtime library")
    endif()

    get_filename_component(qt_bin_dir "${qt_core_location}" DIRECTORY)
    get_filename_component(qt_prefix_dir "${qt_bin_dir}" DIRECTORY)
    get_filename_component(compiler_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)

    set(${out_qt_bin_dir} "${qt_bin_dir}" PARENT_SCOPE)
    set(${out_qt_plugins_dir} "${qt_prefix_dir}/plugins" PARENT_SCOPE)
    set(${out_compiler_bin_dir} "${compiler_bin_dir}" PARENT_SCOPE)
endfunction()

function(hrms_deploy_qt_runtime target_name)
    if(NOT WIN32 OR NOT HRMS_DEPLOY_QT_RUNTIME)
        return()
    endif()

    hrms_qt_runtime_paths(qt_bin_dir qt_plugins_dir compiler_bin_dir)
    find_program(HRMS_WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${qt_bin_dir}")
    if(NOT HRMS_WINDEPLOYQT_EXECUTABLE)
        message(WARNING "windeployqt was not found; ${target_name} may require Qt bin in PATH at runtime")
        return()
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND "${HRMS_WINDEPLOYQT_EXECUTABLE}"
            --no-translations
            --no-system-d3d-compiler
            --no-opengl-sw
            --compiler-runtime
            "$<TARGET_FILE:${target_name}>"
        COMMENT "Deploying Qt runtime for ${target_name}"
        VERBATIM
    )
endfunction()

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
    src/services/PayrollCalculator.h
    src/services/PayrollService.cpp src/services/PayrollService.h
    src/services/EmployeeService.cpp src/services/EmployeeService.h
    src/services/ApprovalService.cpp src/services/ApprovalService.h
    src/services/NotificationService.cpp src/services/NotificationService.h
    src/services/ProfileChangeService.cpp src/services/ProfileChangeService.h
    src/services/AuditService.cpp src/services/AuditService.h
    src/services/AttendanceService.cpp src/services/AttendanceService.h
    src/services/PerformanceService.cpp src/services/PerformanceService.h
    src/services/OrgService.cpp src/services/OrgService.h
    src/services/RbacService.cpp src/services/RbacService.h
    src/services/AuthService.cpp src/services/AuthService.h
    src/widgets/PaginationBar.cpp src/widgets/PaginationBar.h
    src/widgets/ComboDelegate.cpp src/widgets/ComboDelegate.h
    src/widgets/CommonDelegates.cpp src/widgets/CommonDelegates.h
    src/widgets/SafeEditDelegate.h
    src/widgets/TaxConfigPanel.cpp src/widgets/TaxConfigPanel.h
    src/widgets/OrgChartView.cpp src/widgets/OrgChartView.h
    src/widgets/OptimisticSqlTableModel.cpp src/widgets/OptimisticSqlTableModel.h
    src/db/DbConnection.cpp src/db/DbConnection.h
    src/db/DbMigration.cpp src/db/DbMigration.h
    src/db/DbSchema.cpp src/db/DbSchema.h
    src/utils/CsvExport.cpp src/utils/CsvExport.h
    src/utils/DbQuery.h
    src/utils/DbUtils.cpp src/utils/DbUtils.h
    src/utils/MessageHelper.h
    src/utils/SqlFilterBuilder.h
    src/utils/UnsavedChangesGuard.h
    src/utils/UiStyles.cpp src/utils/UiStyles.h
    src/core/Logger.cpp src/core/Logger.h
    src/core/SessionManager.cpp src/core/SessionManager.h
    src/core/GlobalEvents.cpp src/core/GlobalEvents.h
)

add_executable(HRMS ${PROJECT_SOURCES})

target_link_libraries(HRMS PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql Qt6::Network Qt6::Charts Qt6::Concurrent)

hrms_deploy_qt_runtime(HRMS)
