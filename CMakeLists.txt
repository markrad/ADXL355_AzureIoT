cmake_minimum_required(VERSION 3.0.0)
cmake_policy(SET CMP0048 OLD)

project(ADXL355_AzureIoT)


set(build_sample FALSE CACHE BOOL "Build the sample code" FORCE)
set(use_amqp FALSE CACHE BOOL "set use_amqp to ON if amqp is to be used, set to OFF to not use amqp" FORCE)
set(skip_samples ON CACHE BOOL "set skip_samples to ON to skip building samples (default is OFF)[if possible, they are always build]" FORCE)

include(CTest)
enable_testing()

add_subdirectory(ADXL355)
add_subdirectory(Azure-IoT-CPP-Wrapper)

set(headers 
    "ConfigSettings.h" 
)
#    "CIoTHubDevice.h"
#    "CIoTHubMessage.h"
#    "CMapUtil.h"
set(source 
    "main.cpp" 
    "ConfigSettings.cpp" 
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

add_executable(ADXL355_AzureIoT ${source} ${headers})

target_include_directories(ADXL355_AzureIoT PRIVATE 
    "./Azure-IoT-CPP-Wrapper"
	"./Azure-IoT-CPP-Wrapper/azure-iot-sdk-c/deps"
	"./Azure-IoT-CPP-Wrapper/azure-iot-sdk-c/iothub_client/inc"
)

target_link_libraries(ADXL355_AzureIoT 
    parson
    iothub_client
    AzureIoTDevice
    ADXL355_I2C
    wiringPi
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
