# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/david/esp/v5.3.1/esp-idf/components/bootloader/subproject"
  "/home/david/repos/calypso-ble/build/bootloader"
  "/home/david/repos/calypso-ble/build/bootloader-prefix"
  "/home/david/repos/calypso-ble/build/bootloader-prefix/tmp"
  "/home/david/repos/calypso-ble/build/bootloader-prefix/src/bootloader-stamp"
  "/home/david/repos/calypso-ble/build/bootloader-prefix/src"
  "/home/david/repos/calypso-ble/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/david/repos/calypso-ble/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/david/repos/calypso-ble/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
