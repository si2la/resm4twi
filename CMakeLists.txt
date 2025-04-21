cmake_minimum_required(VERSION 3.5)

project(resm4twi LANGUAGES C)

add_executable(resm4twi resmgr_twi.c resm_iface.c resm_iface.h h3_i2c.c h3_i2c.h)

set(INSTALL_DESTDIR "/tmp")

install(TARGETS resm4twi
    RUNTIME DESTINATION "${INSTALL_DESTDIR}"
    BUNDLE DESTINATION "${INSTALL_DESTDIR}"
    LIBRARY DESTINATION "${INSTALL_DESTDIR}"
)
