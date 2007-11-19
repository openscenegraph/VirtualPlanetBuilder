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

#include <vpb/FileCache>

using namespace vpb;

FileCache::FileCache()
{
}


FileCache::FileCache(const FileCache& fc,const osg::CopyOp& copyop):
    osg::Object(fc, copyop)
{
}

FileCache::~FileCache()
{
}

bool FileCache::read(const std::string& filename)
{
    osg::notify(osg::NOTICE)<<"FileCache::read("<<filename<<")"<<std::endl;
    _filename = filename;
    return false;
}

bool FileCache::write(const std::string& filename)
{
    osg::notify(osg::NOTICE)<<"FileCache::write("<<filename<<")"<<std::endl;
    _filename = filename;
    return false;
}

std::string FileCache::getOptimimumFile(const std::string& filename, const SpatialProperties& sp)
{
    osg::notify(osg::NOTICE)<<"FileCache::getOptimimumFile("<<filename<<")"<<std::endl;
    VariantMap::iterator itr = _variantMap.find(filename);
    if (itr==_variantMap.end())
    {
        return filename;
    }
    
    Variants& variants = itr->second;

    // first check cached files on 

    // second check for files that fit the required spatial properties    
    
    return filename;
}

