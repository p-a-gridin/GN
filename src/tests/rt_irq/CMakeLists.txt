cmake_minimum_required(VERSION 3.16)

set(APP_NAME rt_irq)

project(${APP_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${APP_NAME} main.cpp)

target_link_libraries(${APP_NAME}
    gpiod
)

set(INSTALL_DESTDIR "/root")
install(TARGETS ${APP_NAME}
    LIBRARY DESTINATION ${INSTALL_DESTDIR}
    RUNTIME DESTINATION ${INSTALL_DESTDIR}
)
