# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/mnt/d/Master/M2/Projet3D/git/Projet3D/Engine/build/_deps/lua-src"
  "/mnt/d/Master/M2/Projet3D/git/Projet3D/Engine/build/_deps/lua-build"
  "/mnt/d/Master/M2/Projet3D/git/Projet3D/Engine/build/_deps/lua-subbuild/lua-populate-prefix"
  "/mnt/d/Master/M2/Projet3D/git/Projet3D/Engine/build/_deps/lua-subbuild/lua-populate-prefix/tmp"
  "/mnt/d/Master/M2/Projet3D/git/Projet3D/Engine/build/_deps/lua-subbuild/lua-populate-prefix/src/lua-populate-stamp"
  "/mnt/d/Master/M2/Projet3D/git/Projet3D/Engine/build/_deps/lua-subbuild/lua-populate-prefix/src"
  "/mnt/d/Master/M2/Projet3D/git/Projet3D/Engine/build/_deps/lua-subbuild/lua-populate-prefix/src/lua-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/d/Master/M2/Projet3D/git/Projet3D/Engine/build/_deps/lua-subbuild/lua-populate-prefix/src/lua-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/d/Master/M2/Projet3D/git/Projet3D/Engine/build/_deps/lua-subbuild/lua-populate-prefix/src/lua-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
