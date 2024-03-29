# idf_component_register(SRCS "main.cpp" "wifimanager.cpp" "nv.cpp" "smartconfig_main.c nv.hpp"
idf_component_register(SRCS main.cpp wifimanager.cpp nv.cpp counter.c test_10.c
                    INCLUDE_DIRS ".")
