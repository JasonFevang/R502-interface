idf_component_register( SRCS "R502Interface.cpp"
                        INCLUDE_DIRS "include"
                        REQUIRES freertos driver log)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Werror)
