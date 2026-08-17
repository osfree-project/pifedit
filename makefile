#
# A Makefile for osFree Janus PIF Editor
# (c) osFree project
#

DESCRIPTION = osFree Janus PIF Editor
TARGET_VERSION = 310
SOURCES = pifedit pif standard advanced utils

# defines additional options for C compiler
ADD_COPT = -DDEBUG=1

!include $(%ROOT)tools/mk/build.mk

