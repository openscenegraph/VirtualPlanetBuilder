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

#include <vpb/BuildOptions>

#include <osgDB/FileNameUtils>

using namespace vpb;

BuildOptions::BuildOptions()
{
    _maximumTileImageSize = 256;
    _maximumTileTerrainSize = 64;
    
    _maximumVisiableDistanceOfTopLevel = 1e10;
    
    _radiusToMaxVisibleDistanceRatio = 7.0f;
    _verticalScale = 1.0f;
    _skirtRatio = 0.02f;

    _convertFromGeographicToGeocentric = false;
    
    _tileBasename = "output";
    _tileExtension = ".ive";
    _imageExtension = ".dds";

    
    _defaultColor.set(0.5f,0.5f,1.0f,1.0f);
    _databaseType = PagedLOD_DATABASE;
    _geometryType = POLYGONAL;
    _textureType = COMPRESSED_TEXTURE;
    _maxAnisotropy = 1.0;
    _mipMappingMode = MIP_MAPPING_IMAGERY;

    _useLocalTileTransform = true;
    
    _decorateWithCoordinateSystemNode = true;
    _decorateWithMultiTextureControl = true;
    
    _writeNodeBeforeSimplification = false;

    _simplifyTerrain = true;
    
    _maximumNumOfLevels = 30;
}

BuildOptions::BuildOptions(const BuildOptions& rhs)
{
    setBuildOptions(rhs);
}

BuildOptions& BuildOptions::operator = (const BuildOptions& rhs)
{
    if (this==&rhs) return *this;

    setBuildOptions(rhs);

    return *this;
}


void BuildOptions::setBuildOptions(const BuildOptions& rhs)
{
    _maximumTileImageSize = rhs._maximumTileImageSize;
    _maximumTileTerrainSize = rhs._maximumTileTerrainSize;
    
    _maximumVisiableDistanceOfTopLevel = rhs._maximumVisiableDistanceOfTopLevel;
    
    _radiusToMaxVisibleDistanceRatio = rhs._radiusToMaxVisibleDistanceRatio;
    _verticalScale = rhs._verticalScale;
    _skirtRatio = rhs._skirtRatio;

    _convertFromGeographicToGeocentric = rhs._convertFromGeographicToGeocentric;
    
    _tileBasename = rhs._tileBasename;
    _tileExtension = rhs._tileExtension;
    _imageExtension = rhs._imageExtension;

    
    _defaultColor = rhs._defaultColor;
    _databaseType = rhs._databaseType;
    _geometryType = rhs._geometryType;
    _textureType = rhs._textureType;
    _maxAnisotropy = rhs._maxAnisotropy;
    _mipMappingMode = rhs._mipMappingMode;

    _useLocalTileTransform = rhs._useLocalTileTransform;
    
    _decorateWithCoordinateSystemNode = rhs._decorateWithCoordinateSystemNode;
    _decorateWithMultiTextureControl = rhs._decorateWithMultiTextureControl;
    
    _comment = rhs._comment;
    
    _writeNodeBeforeSimplification = rhs._writeNodeBeforeSimplification;

    _simplifyTerrain = rhs._simplifyTerrain;
    
    _destinationCoordinateSystem = rhs._destinationCoordinateSystem;
    _extents = rhs._extents;
    
    _maximumNumOfLevels = rhs._maximumNumOfLevels;
}

void BuildOptions::setDestinationName(const std::string& filename)
{
    std::string path = osgDB::getFilePath(filename);
    std::string base = osgDB::getStrippedName(filename);
    std::string extension = '.'+osgDB::getLowerCaseFileExtension(filename);

    osg::notify(osg::INFO)<<"setDestinationName("<<filename<<")"<<std::endl;
    osg::notify(osg::INFO)<<"   path "<<path<<std::endl;
    osg::notify(osg::INFO)<<"   base "<<base<<std::endl;
    osg::notify(osg::INFO)<<"   extension "<<extension<<std::endl;

    setDirectory(path);
    setDestinationTileBaseName(base);
    setDestinationTileExtension(extension);
} 

void BuildOptions::setDirectory(const std::string& directory)
{
    _directory = directory;
    
    if (_directory.empty()) return;
    
#ifdef WIN32    
    // convert trailing forward slash if any to back slash.
    if (_directory[_directory.size()-1]=='/') _directory[_directory.size()-1] = '\\';

    // if no trailing back slash exists add one.
    if (_directory[_directory.size()-1]!='\\') _directory.push_back('\\');
#else
    // convert trailing back slash if any to forward slash.
    if (_directory[_directory.size()-1]=='\\') _directory[_directory.size()-1] = '/';

    // if no trailing forward slash exists add one.
    if (_directory[_directory.size()-1]!='/') _directory.push_back('/');
#endif    
    osg::notify(osg::NOTICE)<<"directory name set "<<_directory<<std::endl;
}
 
void BuildOptions::setDestinationCoordinateSystem(const std::string& wellKnownText)
{
    setDestinationCoordinateSystem(new osg::CoordinateSystemNode("WKT",wellKnownText));
}
