#
# A Makefile for osFree Janus PIF Editor
# (c) osFree project
#

DESCRIPTION = Janus PIF Editor
TARGET_VERSION = 310
TARGET_PACKAGE = DISK02

# defines additional options for C compiler
ADD_COPT = -DDEBUG=1

!include $(%ROOT)tools/mk/build.mk

