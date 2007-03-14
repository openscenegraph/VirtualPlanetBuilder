TOPDIR = ../..
include $(TOPDIR)/Make/makedefs 


CXXFILES = \
    DataSet.cpp\
    Version.cpp\

DEF     += -DVPB_LIBRARY

INC += $(GDAL_INCLUDES)

LIBS  +=  -losgFX -losgDB -losgUtil -losg $(GDAL_LIBS) $(GL_LIBS) $(OTHER_LIBS)

TARGET_BASENAME = vpb
LIB = $(LIB_PREFIX)$(TARGET_BASENAME).$(LIB_EXT)

include $(TOPDIR)/Make/makerules 
