# - Find Tcl stub libraries.
# This module finds Tcl stub libraries. It first finds Tcl include files and
# libraries by calling FindTCL.cmake.
# How to Use the Tcl Stubs Library:
#   http://tcl.activestate.com/doc/howto/stubs.html
# Using Stub Libraries:
#   http://safari.oreilly.com/0130385603/ch48lev1sec3
# This code sets the following variables:
#  TCL_STUB_LIBRARY       = path to Tcl stub library
#  TK_STUB_LIBRARY        = path to Tk stub library
#
# In an effort to remove some clutter and clear up some issues for people
# who are not necessarily Tcl/Tk gurus/developpers, some variables were
# moved or removed. Changes compared to CMake 2.4 are:
# - TCL_STUB_LIBRARY_DEBUG and TK_STUB_LIBRARY_DEBUG were removed.
#   => these libs are not packaged by default with Tcl/Tk distributions. 
#      Even when Tcl/Tk is built from source, several flavors of debug libs
#      are created and there is no real reason to pick a single one
#      specifically (say, amongst tclstub84g, tclstub84gs, or tclstub84sgx). 
#      Let's leave that choice to the user by allowing him to assign 
#      TCL_STUB_LIBRARY to any Tcl library, debug or not.

INCLUDE(FindTCL)

GET_FILENAME_COMPONENT(TCL_TCLSH_PATH "${TCL_TCLSH}" PATH)
GET_FILENAME_COMPONENT(TCL_TCLSH_PATH_PARENT "${TCL_TCLSH_PATH}" PATH)

GET_FILENAME_COMPONENT(TK_WISH_PATH "${TK_WISH}" PATH)
GET_FILENAME_COMPONENT(TK_WISH_PATH_PARENT "${TK_WISH_PATH}" PATH)

GET_FILENAME_COMPONENT(TCL_INCLUDE_PATH_PARENT "${TCL_INCLUDE_PATH}" PATH)
GET_FILENAME_COMPONENT(TK_INCLUDE_PATH_PARENT "${TK_INCLUDE_PATH}" PATH)

GET_FILENAME_COMPONENT(TCL_LIBRARY_PATH "${TCL_LIBRARY}" PATH)
GET_FILENAME_COMPONENT(TCL_LIBRARY_PATH_PARENT "${TCL_LIBRARY_PATH}" PATH)

GET_FILENAME_COMPONENT(TK_LIBRARY_PATH "${TK_LIBRARY}" PATH)
GET_FILENAME_COMPONENT(TK_LIBRARY_PATH_PARENT "${TK_LIBRARY_PATH}" PATH)

SET(TCLTK_POSSIBLE_LIB_PATHS
  "${TCL_INCLUDE_PATH_PARENT}/lib"
  "${TK_INCLUDE_PATH_PARENT}/lib"
  "${TCL_LIBRARY_PATH}"
  "${TK_LIBRARY_PATH}"
  "${TCL_TCLSH_PATH_PARENT}/lib"
  "${TK_WISH_PATH_PARENT}/lib"
  /usr/lib 
  /usr/local/lib
)

IF(WIN32)
  GET_FILENAME_COMPONENT(
    ActiveTcl_CurrentVersion 
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ActiveState\\ActiveTcl;CurrentVersion]" 
    NAME)
  SET(TCLTK_POSSIBLE_LIB_PATHS ${TCLTK_POSSIBLE_LIB_PATHS}
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ActiveState\\ActiveTcl\\${ActiveTcl_CurrentVersion}]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.6;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.5;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.4;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.3;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.2;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.0;Root]/lib"
    "$ENV{ProgramFiles}/Tcl/Lib"
    "C:/Program Files/Tcl/lib" 
    "C:/Tcl/lib" 
    )
ENDIF(WIN32)

FIND_LIBRARY(TCL_STUB_LIBRARY
  NAMES tclstub 
  tclstub86 tclstub8.6
  tclstub85 tclstub8.5 
  tclstub84 tclstub8.4 
  tclstub83 tclstub8.3 
  tclstub82 tclstub8.2 
  tclstub80 tclstub8.0
  PATHS ${TCLTK_POSSIBLE_LIB_PATHS}
)

FIND_LIBRARY(TK_STUB_LIBRARY 
  NAMES tkstub 
  tkstub86 tkstub8.6
  tkstub85 tkstub8.5 
  tkstub84 tkstub8.4 
  tkstub83 tkstub8.3 
  tkstub82 tkstub8.2 
  tkstub80 tkstub8.0
  PATHS ${TCLTK_POSSIBLE_LIB_PATHS}
)

MARK_AS_ADVANCED(
  TCL_STUB_LIBRARY
  TK_STUB_LIBRARY
  )