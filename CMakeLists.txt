zephyr_library()
zephyr_library_sources(src/led_mode.c)
zephyr_library_sources(src/layer_cycle.c)
zephyr_library_include_directories(${APPLICATION_SOURCE_DIR}/include)
