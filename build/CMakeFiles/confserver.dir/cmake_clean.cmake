file(REMOVE_RECURSE
  "bootloader/bootloader.bin"
  "bootloader/bootloader.elf"
  "bootloader/bootloader.map"
  "config/sdkconfig.cmake"
  "config/sdkconfig.h"
  "hid_gamepad_demo.bin"
  "hid_gamepad_demo.map"
  "project_elf_src.c"
  "CMakeFiles/confserver"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/confserver.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
