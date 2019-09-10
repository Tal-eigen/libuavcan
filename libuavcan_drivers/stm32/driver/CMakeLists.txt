include_directories(
    ./include
    )

set(UAVCAN_STM32_INC ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "stm32 uavcan driver include")

add_library(uavcan_stm32_driver STATIC
    ./src/uc_stm32_can.cpp
    ./src/uc_stm32_clock.cpp
    ./src/uc_stm32_thread.cpp
    )

target_include_directories(uavcan_stm32_driver PUBLIC
										 					${PROJECT_CONF_DIR}
                             ${CHIBIOS_INCLUDE_DIRS}
														 ${CHIBIOS_ROOT}/os/common/ext/CMSIS/ST/STM32${STM32_FAMILY}xx/
                             ${CHIBIOS_ROOT}/os/common/ext/CMSIS/include/
                             #TODO: Add better dependency for board.h
														 ${CMAKE_SOURCE_DIR}/src/stm32_uavcan/${MCU}/
                           )
									 target_compile_definitions(uavcan_stm32_driver PRIVATE STM32${MCU}xx)

add_dependencies(uavcan_stm32_driver uavcan)

install(DIRECTORY include/uavcan_stm32 DESTINATION include)
install(TARGETS uavcan_stm32_driver DESTINATION lib/${MCU}/)

# vim: set et ft=cmake fenc=utf-8 ff=unix sts=4 sw=4 ts=4 :)

