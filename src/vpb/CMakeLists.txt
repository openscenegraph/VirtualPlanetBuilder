IF   (DYNAMIC_VIRTUALPLANETBUILDER)
    ADD_DEFINITIONS(-DVPB_LIBRARY)
ELSE (DYNAMIC_VIRTUALPLANETBUILDER)
    ADD_DEFINITIONS(-DVPB_LIBRARY_STATIC)
ENDIF(DYNAMIC_VIRTUALPLANETBUILDER)
SET(LIB_NAME vpb)


SET(HEADER_PATH ${VirtualPlanetBuilder_SOURCE_DIR}/include/${LIB_NAME})
SET(LIB_PUBLIC_HEADERS
    ${HEADER_PATH}/BlockOperation
    ${HEADER_PATH}/BuildLog
    ${HEADER_PATH}/BuildOperation
    ${HEADER_PATH}/BuildOptions
    ${HEADER_PATH}/Commandline
    ${HEADER_PATH}/DatabaseBuilder
    ${HEADER_PATH}/DataSet
    ${HEADER_PATH}/Date
    ${HEADER_PATH}/Destination
    ${HEADER_PATH}/Export
    ${HEADER_PATH}/ExtrudeVisitor
    ${HEADER_PATH}/FileCache
    ${HEADER_PATH}/FileDetails
    ${HEADER_PATH}/FileUtils
    ${HEADER_PATH}/FilePathManager
    ${HEADER_PATH}/GeospatialDataset
    ${HEADER_PATH}/HeightFieldMapper
    ${HEADER_PATH}/MachinePool
    ${HEADER_PATH}/ObjectPlacer
    ${HEADER_PATH}/PropertyFile
    ${HEADER_PATH}/ShapeFilePlacer
    ${HEADER_PATH}/Source
    ${HEADER_PATH}/SourceData
    ${HEADER_PATH}/SpatialProperties
    ${HEADER_PATH}/System
    ${HEADER_PATH}/TextureUtils
    ${HEADER_PATH}/Task
    ${HEADER_PATH}/TaskManager
    ${HEADER_PATH}/ThreadPool
    ${HEADER_PATH}/Version
)

ADD_LIBRARY(${LIB_NAME}
    ${VIRTUALPLANETBUILDER_USER_DEFINED_DYNAMIC_OR_STATIC}
    ${LIB_PUBLIC_HEADERS}
    BuildLog.cpp
    BuildOperation.cpp
    BuildOptions.cpp
    BuildOptionsIO.cpp
    Commandline.cpp
    DatabaseBuilder.cpp
    DatabaseBuilderIO.cpp
    DataSet.cpp
    Date.cpp
    Destination.cpp
    ExtrudeVisitor.cpp
    FileCache.cpp
    FileDetails.cpp
    FileUtils.cpp
    FilePathManager.cpp
    GeospatialDataset.cpp
    HeightFieldMapper.cpp
    MachinePool.cpp
    ObjectPlacer.cpp
    PropertyFile.cpp
    ShapeFilePlacer.cpp
    Source.cpp
    SourceData.cpp
    SpatialProperties.cpp
    System.cpp
    TextureUtils.cpp
    Task.cpp
    TaskManager.cpp
    ThreadPool.cpp
    Version.cpp
)


INCLUDE_DIRECTORIES(${GDAL_INCLUDE_DIR} ${OPENSCENEGRAPH_INCLUDE_DIRS} )

IF (WIN32)
  LINK_EXTERNAL(${LIB_NAME}  ${CMAKE_THREAD_LIBS_INIT} ${MATH_LIBRARY} ${GDAL_LIBRARY} ${EXTRA_LIBS} Ws2_32.lib)
ELSE(WIN32)
  LINK_EXTERNAL(${LIB_NAME}  ${CMAKE_THREAD_LIBS_INIT} ${MATH_LIBRARY} ${GDAL_LIBRARY} ${EXTRA_LIBS})
ENDIF(WIN32)


LINK_WITH_VARIABLES(${LIB_NAME} OSG_LIBRARY OSGUTIL_LIBRARY OSGSIM_LIBRARY OSGTERRAIN_LIBRARY OSGDB_LIBRARY OSGFX_LIBRARY OSGVIEWER_LIBRARY OSGTEXT_LIBRARY OSGGA_LIBRARY OPENTHREADS_LIBRARY)
LINK_CORELIB_DEFAULT(${LIB_NAME} ${CMAKE_THREAD_LIBS_INIT} ${MATH_LIBRARY} )

INCLUDE(ModuleInstall OPTIONAL)
