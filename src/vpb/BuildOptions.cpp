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

BuildOptions::BuildOptions():
    osg::Object(true)
{
    _archiveName = "";
    _buildOverlays = false;
    _comment = "";
    _convertFromGeographicToGeocentric = false;
    _databaseType = PagedLOD_DATABASE;
    _decorateWithCoordinateSystemNode = true;
    _decorateWithMultiTextureControl = true;
    _defaultColor.set(0.5f,0.5f,1.0f,1.0f);
    _destinationCoordinateSystemString = "";
    _destinationCoordinateSystem = new osg::CoordinateSystemNode; _destinationCoordinateSystem->setEllipsoidModel(new osg::EllipsoidModel);
    _directory = "";
    _geometryType = POLYGONAL;
    _imageExtension = ".dds";
    _intermediateBuildName = "";
    _maxAnisotropy = 1.0;
    _maximumNumOfLevels = 30;
    _maximumTileImageSize = 256;
    _maximumTileTerrainSize = 64;
    _maximumVisiableDistanceOfTopLevel = 1e10;
    _mipMappingMode = MIP_MAPPING_IMAGERY;
    _radiusToMaxVisibleDistanceRatio = 7.0f;
    _simplifyTerrain = true;
    _skirtRatio = 0.02f;
    _textureType = COMPRESSED_TEXTURE;
    _tileBasename = "output";
    _tileExtension = ".ive";
    _useLocalTileTransform = true;
    _verticalScale = 1.0f;
    _writeNodeBeforeSimplification = false;
}

BuildOptions::BuildOptions(const BuildOptions& rhs,const osg::CopyOp& copyop):
    osg::Object(rhs,copyop)
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
    _archiveName = rhs._archiveName;
    _buildOverlays = rhs._buildOverlays;
    _comment = rhs._comment;
    _convertFromGeographicToGeocentric = rhs._convertFromGeographicToGeocentric;
    _databaseType = rhs._databaseType;
    _decorateWithCoordinateSystemNode = rhs._decorateWithCoordinateSystemNode;
    _decorateWithMultiTextureControl = rhs._decorateWithMultiTextureControl;
    _defaultColor = rhs._defaultColor;
    _destinationCoordinateSystemString = rhs._destinationCoordinateSystemString;
    _destinationCoordinateSystem = rhs._destinationCoordinateSystem;
    _directory = rhs._directory;
    _extents = rhs._extents;
    _geometryType = rhs._geometryType;
    _imageExtension = rhs._imageExtension;
    _intermediateBuildName = rhs._intermediateBuildName;
    _maxAnisotropy = rhs._maxAnisotropy;
    _maximumNumOfLevels = rhs._maximumNumOfLevels;
    _maximumTileImageSize = rhs._maximumTileImageSize;
    _maximumTileTerrainSize = rhs._maximumTileTerrainSize;
    _maximumVisiableDistanceOfTopLevel = rhs._maximumVisiableDistanceOfTopLevel;
    _mipMappingMode = rhs._mipMappingMode;
    _radiusToMaxVisibleDistanceRatio = rhs._radiusToMaxVisibleDistanceRatio;
    _simplifyTerrain = rhs._simplifyTerrain;
    _skirtRatio = rhs._skirtRatio;
    _textureType = rhs._textureType;
    _tileBasename = rhs._tileBasename;
    _tileExtension = rhs._tileExtension;
    _useLocalTileTransform = rhs._useLocalTileTransform;
    _verticalScale = rhs._verticalScale;
    _writeNodeBeforeSimplification = rhs._writeNodeBeforeSimplification;
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
    _destinationCoordinateSystemString = wellKnownText;
    setDestinationCoordinateSystemNode(new osg::CoordinateSystemNode("WKT",wellKnownText));
}

void BuildOptions::setDestinationCoordinateSystemNode(osg::CoordinateSystemNode* cs)
{
    _destinationCoordinateSystem = cs;
    
    if (_destinationCoordinateSystem.valid())
    {
        if (_destinationCoordinateSystem->getEllipsoidModel()==0)
        {
            _destinationCoordinateSystem->setEllipsoidModel(new osg::EllipsoidModel);
        }
        
        _destinationCoordinateSystemString = _destinationCoordinateSystem->getCoordinateSystem();
    }
    else
    {
        _destinationCoordinateSystemString.clear();
    }
    

}
