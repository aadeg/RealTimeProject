cmake_minimum_required(VERSION 2.8.3)
project(rt-project)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-Wpedantic -Wall ${CMAKE_CXX_FLAGS}")

link_directories(/usr/lib/x86_64-linux-gnu)
include_directories(
	include/
)

add_executable(prototype_input_task
  src/ptask.c
  src/graphics.c
  prototypes/input_task.c
)
target_link_libraries(prototype_input_task
	pthread
	rt
	alleg
	m
)

add_executable(prototype_graphic_task
  src/ptask.c
  src/graphics.c
  prototypes/graphic_task.c
)
target_link_libraries(prototype_graphic_task
	pthread
	rt
	alleg
	m
)

add_executable(prototype_airplane_task
  src/ptask.c
  src/graphics.c
  prototypes/airplane_task.c
)
target_link_libraries(prototype_airplane_task
	pthread
	rt
	alleg
	m
)

add_executable(prototype_airplane_task_wayp
  src/ptask.c
  src/graphics.c
  prototypes/airplane_task_wayp.c
)
target_link_libraries(prototype_airplane_task_wayp
	pthread
	rt
	alleg
	m
)

add_executable(prototype_waypoints
  src/ptask.c
  src/graphics.c
  prototypes/waypoints.c
)
target_link_libraries(prototype_waypoints
	pthread
	rt
	alleg
	m
)

add_executable(main
  src/ptask.c
  src/graphics.c
  src/main.c
  src/structs.c
)
target_link_libraries(main
	pthread
	rt
	alleg
	m
)

# add_executable(allegro_mouse
# 	src/allegro_mouse.c
# 	src/ptask.c
# )
# target_link_libraries(allegro_mouse
# 	pthread
# 	rt
# 	alleg
# 	m
# )