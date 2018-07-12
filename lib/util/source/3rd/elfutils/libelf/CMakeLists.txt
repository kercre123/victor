#
# lib/util/source/3rd/elfutils/libelf/CMakeLists.txt
#
# cmake declarations for building libdw
#
project(libelf)

add_library(libelf STATIC
  elf32_checksum.c
  elf32_fsize.c
  elf32_getchdr.c
  elf32_getehdr.c
  elf32_getphdr.c
  elf32_getshdr.c
  elf32_newehdr.c
  elf32_newphdr.c
  elf32_offscn.c
  elf32_updatefile.c
  elf32_updatenull.c
  elf32_xlatetof.c
  elf32_xlatetom.c
  elf64_checksum.c
  elf64_fsize.c
  elf64_getchdr.c
  elf64_getehdr.c
  elf64_getphdr.c
  elf64_getshdr.c
  elf64_newehdr.c
  elf64_newphdr.c
  elf64_offscn.c
  elf64_updatefile.c
  elf64_updatenull.c
  elf64_xlatetof.c
  elf64_xlatetom.c
  elf_begin.c
  elf_clone.c
  elf_cntl.c
  elf_compress.c
  elf_compress_gnu.c
  elf_end.c
  elf_error.c
  elf_fill.c
  elf_flagdata.c
  elf_flagehdr.c
  elf_flagelf.c
  elf_flagphdr.c
  elf_flagscn.c
  elf_flagshdr.c
  elf_getarhdr.c
  elf_getaroff.c
  elf_getarsym.c
  elf_getbase.c
  elf_getdata.c
  elf_getdata_rawchunk.c
  elf_getident.c
  elf_getphdrnum.c
  elf_getscn.c
  elf_getshdrnum.c
  elf_getshdrstrndx.c
  elf_gnu_hash.c
  elf_hash.c
  elf_kind.c
  elf_memory.c
  elf_ndxscn.c
  elf_newdata.c
  elf_newscn.c
  elf_next.c
  elf_nextscn.c
  elf_rand.c
  elf_rawdata.c
  elf_rawfile.c
  elf_readall.c
  elf_scnshndx.c
  elf_strptr.c
  elf_update.c
  elf_version.c
  gelf_checksum.c
  gelf_fsize.c
  gelf_getauxv.c
  gelf_getchdr.c
  gelf_getclass.c
  gelf_getdyn.c
  gelf_getehdr.c
  gelf_getlib.c
  gelf_getmove.c
  gelf_getnote.c
  gelf_getphdr.c
  gelf_getrel.c
  gelf_getrela.c
  gelf_getshdr.c
  gelf_getsym.c
  gelf_getsyminfo.c
  gelf_getsymshndx.c
  gelf_getverdaux.c
  gelf_getverdef.c
  gelf_getvernaux.c
  gelf_getverneed.c
  gelf_getversym.c
  gelf_newehdr.c
  gelf_newphdr.c
  gelf_offscn.c
  gelf_update_auxv.c
  gelf_update_dyn.c
  gelf_update_ehdr.c
  gelf_update_lib.c
  gelf_update_move.c
  gelf_update_phdr.c
  gelf_update_rel.c
  gelf_update_rela.c
  gelf_update_shdr.c
  gelf_update_sym.c
  gelf_update_syminfo.c
  gelf_update_symshndx.c
  gelf_update_verdaux.c
  gelf_update_verdef.c
  gelf_update_vernaux.c
  gelf_update_verneed.c
  gelf_update_versym.c
  gelf_xlate.c
  gelf_xlatetof.c
  gelf_xlatetom.c
  libelf_crc32.c
  libelf_next_prime.c
  nlist.c
)

anki_build_target_license(libelf "LGPL-3.0,${CMAKE_CURRENT_SOURCE_DIR}/../COPYING-LGPLV3")

# target_compile_options(libdw PRIVATE
#   -Wno-comment
#   -Wno-strict-aliasing
#   -Wno-unused-function
#   -Wno-unused-value
#   -Wno-parentheses
#   -Wno-extern-initializer
# )
# 
# target_compile_options(libev PUBLIC
#   -Wno-c++1z-compat-mangling
# )
# 
# target_compile_definitions(libev PUBLIC
#   EV_STANDALONE=1
#   EV_USE_MONOTONIC=1
#   EV_USE_SELECT=1
#   EV_USE_POLL=0
#   EV_USE_EPOLL=0
#   EV_MULTIPLICITY=1
#   EV_IDLE_ENABLE=1
#   EV_CHILD_ENABLE=0
# )
# 

target_link_libraries(libelf libeu)

target_include_directories(libelf PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
