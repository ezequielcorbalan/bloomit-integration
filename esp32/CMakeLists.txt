# Bloomit Device API Example - ESP-IDF Project
# 
# Minimum required ESP-IDF version: v5.0
#
# Build instructions:
# 1. Set IDF_PATH environment variable
# 2. Run: idf.py set-target esp32
# 3. Run: idf.py menuconfig (optional, to configure WiFi credentials)
# 4. Run: idf.py build
# 5. Run: idf.py -p (PORT) flash monitor

cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project name
project(bloomit_example)


