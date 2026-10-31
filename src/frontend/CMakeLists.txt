find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Concurrent)
set(CMAKE_AUTOMOC ON)

add_executable(AetherWorkbench
  main.cpp
  ../application/controllers/app_context.hpp
  views/main_window.hpp
  views/pages.hpp
  widgets/plot_widget.hpp
  widgets/viewport3d.hpp
  themes/theme.hpp
  themes/theme.cpp
  widgets/ui_common.cpp
  widgets/plot_widget.cpp
  widgets/viewport3d.cpp
  views/main_window.cpp
  views/pages_geometry_thermal.cpp
  views/pages_other.cpp)
target_include_directories(AetherWorkbench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(AetherWorkbench PRIVATE aether_application Qt6::Widgets Qt6::Concurrent)
set_target_properties(AetherWorkbench PROPERTIES WIN32_EXECUTABLE ON)
