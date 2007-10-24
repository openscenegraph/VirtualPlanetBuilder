/* -*-c++-*- VirtualPlanetBuilder - Copyright (C) 1998-2007 Robert Osfield 
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
*/

#include <vpb/FileSystem>

using namespace vpb;

osg::ref_ptr<FileSystem>& FileSystem::instance()
{
    static osg::ref_ptr<FileSystem> s_FileSystem = new FileSystem;
    return s_FileSystem;
}

FileSystem::FileSystem()
{
    readEnvironmentVariables();
}

FileSystem::~FileSystem()
{
}

void FileSystem::readEnvironmentVariables()
{
    const char* str = getenv("VPB_SOURCE_PATHS");
    if (str) 
    {
        osgDB::convertStringPathIntoFilePathList(std::string(str),_sourcePaths);
    }

    str = getenv("VPB_DESTINATION_DIR");
    if (str) 
    {
        _destinationDirectory = str;
    }

    str = getenv("VPB_INTERMEDIATE_DIR");
    if (str) 
    {
        _intermediateDirectory = str;
    }

    str = getenv("VPB_LOG_DIR");
    if (str) 
    {
        _logDirectory = str;
    }

    str = getenv("VPB_TASK_DIR");
    if (str) 
    {
        _taskDirectory = str;
    }

    str = getenv("VPB_MACHINE_FILE");
    if (str) 
    {
        _machineFileName = str;
    }
}

osgDB::FilePathList& vpb::getSourcePaths() { return FileSystem::instance()->getSourcePaths(); }
std::string& vpb::getDestinationDirectory() { return FileSystem::instance()->getDestinationDirectory(); }
std::string& vpb::getIntermediateDirectory() { return FileSystem::instance()->getIntermediateDirectory(); }
std::string& vpb::getLogDirectory() { return FileSystem::instance()->getLogDirectory(); }
std::string& vpb::getTaskDirectory() { return FileSystem::instance()->getTaskDirectory(); }
std::string& vpb::getMachineFileName() { return FileSystem::instance()->getMachineFileName(); }
