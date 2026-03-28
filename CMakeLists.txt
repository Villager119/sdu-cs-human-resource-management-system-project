cmake_minimum_required(VERSION 3.16)
project(HRMS VERSION 1.0.0 LANGUAGES CXX)

# 虽然我们主要写 C++11 风格的代码，但配合 Qt6 必须声明 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启 Qt 的三大自动生成工具 (MOC: 元对象编译器, UIC: UI 编译器, RCC: 资源编译器)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 寻找 Qt6 的核心组件：Core(基础), Gui(图形), Widgets(窗口部件), Sql(数据库)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

# 你的源文件列表（目前只有 main.cpp）
set(PROJECT_SOURCES
    main.cpp
    loginwindow.cpp
    loginwindow.h
    loginwindow.ui
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# 生成可执行文件
add_executable(HRMS ${PROJECT_SOURCES}
    loginwindow.h loginwindow.cpp loginwindow.ui
    mainwindow.h mainwindow.cpp mainwindow.ui)

# 将 Qt 的库链接到我们的项目中
target_link_libraries(HRMS PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Qt6::Sql
)