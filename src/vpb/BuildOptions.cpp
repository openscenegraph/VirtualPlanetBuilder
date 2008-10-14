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
    _reprojectSources = true;
    _generateTiles = true;
    _comment = "";
    _convertFromGeographicToGeocentric = false;
    _databaseType = PagedLOD_DATABASE;
    _decorateWithCoordinateSystemNode = true;
    _decorateWithMultiTextureControl = true;
    _defaultColor.set(0.5f,0.5f,1.0f,1.0f);
    _useInterpolatedImagerySampling = true;
    _useInterpolatedTerrainSampling = true;
    _destinationCoordinateSystemString = "";
    _destinationCoordinateSystem = new osg::CoordinateSystemNode; 
    _destinationCoordinateSystem->setEllipsoidModel(new osg::EllipsoidModel);
    _directory = "";
    _outputTaskDirectories = true;
    _geometryType = POLYGONAL;
    _imageExtension = ".dds";
    _powerOfTwoImages = true;
    _intermediateBuildName = "";
    _logFileName = "";
    _taskFileName = "";
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
    
    _distributedBuildSplitLevel = 0;
    _distributedBuildSecondarySplitLevel = 0;
    _recordSubtileFileNamesOnLeafTile = false;
    _generateSubtile = false;
    _subtileLevel = 0;
    _subtileX = 0;
    _subtileY = 0;
    
    _notifyLevel = NOTICE;
    _disableWrites = false;
    
    _numReadThreadsToCoresRatio = 0.0f;
    _numWriteThreadsToCoresRatio = 0.0f;
    
    _layerInheritance = INHERIT_NEAREST_AVAILABLE;
    
    _abortTaskOnError = true;
    _abortRunOnError = false;
    
    _defaultImageLayerOutputPolicy = INLINE;
    _defaultElevationLayerOutputPolicy = INLINE;
    _optionalImageLayerOutputPolicy = EXTERNAL_SET_DIRECTORY;
    _optionalElevationLayerOutputPolicy = EXTERNAL_SET_DIRECTORY;
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
    _reprojectSources = rhs._reprojectSources;
    _generateTiles = rhs._generateTiles;
    _comment = rhs._comment;
    _convertFromGeographicToGeocentric = rhs._convertFromGeographicToGeocentric;
    _databaseType = rhs._databaseType;
    _decorateWithCoordinateSystemNode = rhs._decorateWithCoordinateSystemNode;
    _decorateWithMultiTextureControl = rhs._decorateWithMultiTextureControl;
    _defaultColor = rhs._defaultColor;
    _useInterpolatedImagerySampling = rhs._useInterpolatedImagerySampling;
    _useInterpolatedTerrainSampling = rhs._useInterpolatedTerrainSampling;
    _destinationCoordinateSystemString = rhs._destinationCoordinateSystemString;
    _destinationCoordinateSystem = rhs._destinationCoordinateSystem;
    _directory = rhs._directory;
    _outputTaskDirectories = rhs._outputTaskDirectories;
    _extents = rhs._extents;
    _geometryType = rhs._geometryType;
    _imageExtension = rhs._imageExtension;
    _powerOfTwoImages = rhs._powerOfTwoImages;
    _intermediateBuildName = rhs._intermediateBuildName;
    _logFileName = rhs._logFileName;
    _taskFileName = rhs._taskFileName;
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
    _distributedBuildSplitLevel = rhs._distributedBuildSplitLevel;
    _distributedBuildSecondarySplitLevel = rhs._distributedBuildSecondarySplitLevel;
    _recordSubtileFileNamesOnLeafTile = rhs._recordSubtileFileNamesOnLeafTile;
    _generateSubtile = rhs._generateSubtile;
    _subtileLevel = rhs._subtileLevel;
    _subtileX = rhs._subtileX;
    _subtileY = rhs._subtileY;
    
    _notifyLevel = rhs._notifyLevel;
    _disableWrites = rhs._disableWrites;
    
    _numReadThreadsToCoresRatio = rhs._numReadThreadsToCoresRatio;
    _numWriteThreadsToCoresRatio = rhs._numWriteThreadsToCoresRatio;
    
    _buildOptionsString = rhs._buildOptionsString;
    _writeOptionsString = rhs._writeOptionsString;
    
    _layerInheritance = rhs._layerInheritance;
    
    _abortTaskOnError = rhs._abortTaskOnError;
    _abortRunOnError = rhs._abortRunOnError;
    
    _defaultImageLayerOutputPolicy = rhs._defaultImageLayerOutputPolicy;
    _defaultElevationLayerOutputPolicy = rhs._defaultElevationLayerOutputPolicy;
    _optionalImageLayerOutputPolicy = rhs._optionalImageLayerOutputPolicy;
    _optionalElevationLayerOutputPolicy = rhs._optionalElevationLayerOutputPolicy;
    
    _optionalLayerSet = rhs._optionalLayerSet;
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

    osg::notify(osg::INFO)<<"directory name set "<<_directory<<std::endl;
}
 
void BuildOptions::setDestinationCoordinateSystem(const std::string& wellKnownText)
{
    _destinationCoordinateSystemString = wellKnownText;
    setDestinationCoordinateSystemNode(new osg::CoordinateSystemNode("WKT",wellKnownText));
}

void BuildOptions::setDestinationCoordinateSystemNode(osg::CoordinateSystemNode* cs)
{
    // Keep any settings from the already configured ellipsoid model
    osg::ref_ptr<osg::EllipsoidModel> configuredEllipsoid = _destinationCoordinateSystem->getEllipsoidModel();

    _destinationCoordinateSystem = cs;
    
    if (_destinationCoordinateSystem.valid())
    {
        if (_destinationCoordinateSystem->getEllipsoidModel()==0)
        {
            _destinationCoordinateSystem->setEllipsoidModel( configuredEllipsoid.get() ? configuredEllipsoid.get() : new osg::EllipsoidModel);
        }
        
        _destinationCoordinateSystemString = _destinationCoordinateSystem->getCoordinateSystem();
    }
    else
    {
        _destinationCoordinateSystemString.clear();
    }

}

void BuildOptions::setNotifyLevel(NotifyLevel level)
{
    _notifyLevel = level;
}

void BuildOptions::setNotifyLevel(const std::string& notifyLevel)
{
    if      (notifyLevel=="ALWAYS") setNotifyLevel(ALWAYS);
    else if (notifyLevel=="DISABLE") setNotifyLevel(ALWAYS);
    else if (notifyLevel=="OFF") setNotifyLevel(ALWAYS);
    else if (notifyLevel=="FATAL") setNotifyLevel(FATAL);
    else if (notifyLevel=="WARN") setNotifyLevel(WARN);
    else if (notifyLevel=="NOTICE") setNotifyLevel(NOTICE);
    else if (notifyLevel=="INFO") setNotifyLevel(INFO);
    else if (notifyLevel=="DEBUG") setNotifyLevel(DEBUG_INFO);
    else if (notifyLevel=="DEBUG_INFO") setNotifyLevel(DEBUG_INFO);
    else if (notifyLevel=="DEBUG_FP") setNotifyLevel(DEBUG_FP);
}

