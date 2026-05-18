cmake_minimum_required(VERSION 3.16)
project(HRMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Network Charts)

set(PROJECT_SOURCES
    main.cpp
    loginwindow.cpp loginwindow.h loginwindow.ui
    mainwindow.cpp mainwindow.h mainwindow.ui
    serversettingsdialog.cpp serversettingsdialog.h
    changepassworddialog.cpp changepassworddialog.h
    tabs/EmployeeTab.cpp tabs/EmployeeTab.h
    tabs/LeaveTab.cpp tabs/LeaveTab.h
    tabs/PayrollTab.cpp tabs/PayrollTab.h
    tabs/AuditTab.cpp tabs/AuditTab.h
    tabs/ReportsTab.cpp tabs/ReportsTab.h
    tabs/DashboardTab.cpp tabs/DashboardTab.h
)

add_executable(HRMS ${PROJECT_SOURCES})

target_link_libraries(HRMS PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql Qt6::Network Qt6::Charts)
