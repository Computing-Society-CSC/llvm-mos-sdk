platform(6502fun COMPLETE HOSTED PARENT common)

if(NOT CMAKE_CROSSCOMPILING)
  return()
endif()

install(FILES
  6502fun.h
  via.h
  screen.h
TYPE INCLUDE)
install(FILES link.ld TYPE LIB)

add_platform_library(6502fun-crt0
  crt0/reset.S
  crt0/systick.S
)
merge_libraries(6502fun-crt0
  common-crt0
  common-init-stack
  common-copy-data
  common-zero-bss
  common-exit-loop
)

add_platform_library(6502fun-c
  delay.c
  utils.c
  via.c
  screen.c
)

target_compile_options(6502fun-crt0 PUBLIC -mcpu=mosw65c02)
target_link_libraries(6502fun-crt0 PRIVATE common-asminc)

target_include_directories(6502fun-c BEFORE PUBLIC .)
target_compile_options(6502fun-c PUBLIC -mcpu=mosw65c02)
target_link_libraries(6502fun-c PRIVATE common-asminc)
