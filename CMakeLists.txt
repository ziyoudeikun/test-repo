# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(HLS_TEST)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件目录
include_directories(include /usr/local/include /usr/include)

# 查找主项目源文件
file(GLOB SOURCES "src/*.cpp" "src/*.c")

# 查找 utils 模块的源文件
file(GLOB UTILS_SOURCES "utils/src/*.cpp" "utils/src/*.c")

# 链接目录  
link_directories(/usr/local/lib /usr/lib/x86_64-linux-gnu)

# 必须首先查找 glog 包
find_package(glog REQUIRED)

# 必须首先查找 jsoncpp 包
find_package(jsoncpp REQUIRED)

# 直接指定已知的路径（Ubuntu 通常在这里）mysql lib
include_directories(/usr/include/mysql-cppconn-8)

# 添加 utils 包含目录
include_directories(utils/include)

# 添加可执行文件（包含主项目和 utils 的源文件）
add_executable(${PROJECT_NAME} ${SOURCES} ${UTILS_SOURCES})

# 或者更好的做法：先创建 utils 库
# add_library(utils STATIC ${UTILS_SOURCES})
# target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils/include)

#redis-plus-plus lib
target_link_libraries(${PROJECT_NAME} 
    redis++
    hiredis
    pthread
    cppkafka
    rdkafka
)

#http-parser lib
target_link_libraries(${PROJECT_NAME} http_parser)

#google log lib
target_link_libraries(${PROJECT_NAME} glog::glog)

#mysql lib
target_link_libraries(${PROJECT_NAME} mysqlcppconn)

#jsoncpp lib
target_link_libraries(${PROJECT_NAME} jsoncpp)

# 如果使用 utils 库的方式，需要链接：
# target_link_libraries(${PROJECT_NAME} utils)

# 包含头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE /usr/include)

# 安装目标（可选）
#install(TARGETS ${PROJECT_NAME} DESTINATION bin)