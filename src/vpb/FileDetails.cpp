/* -*-c++-*- VirtualPlanetBuilder - Copyright (C) 1998-2009 Robert Osfield
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

#include <vpb/FileCache>

using namespace vpb;

FileDetails::FileDetails()
{
}

FileDetails::FileDetails(const FileDetails& fd,const osg::CopyOp& copyop):
    osg::Object(fd,copyop),
    _originalSourceFile(fd._originalSourceFile),
    _buildApplication(fd._buildApplication),
    _hostname(fd._hostname),
    _filename(fd._filename),
    _spatialProperties(fd._spatialProperties)
{
}

FileDetails::~FileDetails()
{
}
