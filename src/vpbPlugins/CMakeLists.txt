#---------------------------------------------------
# VPB CMAKE SUPPORT 
# (C) by Michael Wagner, mtw@shared-reality.com 2005
# (C) Eric Wing, Luigi Calori and Robert Osfield 2006-2007
#---------------------------------------------------

PROJECT(VPB_PLUGINS_MASTER)


SET(LIBRARY_OUTPUT_PATH "${LIBRARY_OUTPUT_PATH}/${VPB_PLUGINS}")


IF(NOT MINGW)
	SET(CMAKE_SHARED_MODULE_PREFIX "")
ENDIF(NOT MINGW)


SET(TARGET_DEFAULT_PREFIX "osgdb_")
SET(TARGET_DEFAULT_LABEL_PREFIX "Plugins")
SET(TARGET_COMMON_LIBRARIES 
        vpb
)

SET(TARGET_DEFAULT_LABEL_PREFIX "Plug commons")
###########################################################
#  this is for asking the production of a single CMakeLists.txt
#  the BUILD_CMAKELIST can be set at the macro call level or
#  globally by configuring 
#  TARGET_DEFAULT_BUILD_CMAKELIST
#
#	ADD_VPB_PLUGIN(VPBFX VPBFX BUILD_CMAKELIST)
#
###########################################################

ADD_SUBDIRECTORY(gdal)

