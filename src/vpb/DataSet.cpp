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


#include <osg/Texture2D>
#include <osg/Notify>
#include <osg/io_utils>

#include <osg/GLU>

#include <osgTerrain/GeometryTechnique>

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>

#include <osgFX/MultiTextureControl>

#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>

#include <vpb/DataSet>
#include <vpb/DatabaseBuilder>

// GDAL includes
#include <gdal_priv.h>
#include <ogr_spatialref.h>

// standard library includes
#include <sstream>
#include <iostream>
#include <algorithm>


using namespace vpb;

static int s_notifyOffset = 0;
void DataSet::setNotifyOffset(int level) { s_notifyOffset = level; }
int DataSet::getNotifyOffset() { return s_notifyOffset; }

inline std::ostream& my_notify(osg::NotifySeverity level) { return osg::notify(osg::NotifySeverity(s_notifyOffset+level)); }

DataSet::DataSet()
{
    init();
}

void DataSet::init()
{
    static bool s_initialized = false;
    if (!s_initialized)
    {
        s_initialized = true;
        GDALAllRegister();
    }

    _numTextureLevels = 1;
}

void DataSet::addSource(Source* source)
{
    if (!source) return;
    
    if (!_sourceGraph.valid()) _sourceGraph = new CompositeSource;
    
    _sourceGraph->_sourceList.push_back(source);
}

void DataSet::addSource(CompositeSource* composite)
{
    if (!composite) return;

    if (!_sourceGraph.valid()) _sourceGraph = new CompositeSource;
    
    _sourceGraph->_children.push_back(composite);
}

void DataSet::loadSources()
{
    for(CompositeSource::source_iterator itr(_sourceGraph.get());itr.valid();++itr)
    {
        (*itr)->loadSourceData();
    }
}

CompositeDestination* DataSet::createDestinationGraph(CompositeDestination* parent,
                                                      osg::CoordinateSystemNode* cs,
                                                      const GeospatialExtents& extents,
                                                      unsigned int maxImageSize,
                                                      unsigned int maxTerrainSize,
                                                      unsigned int currentLevel,
                                                      unsigned int currentX,
                                                      unsigned int currentY,
                                                      unsigned int maxNumLevels)
{

    CompositeDestination* destinationGraph = new CompositeDestination(cs,extents);

    if (mapLatLongsToXYZ())
    {
        // we need to project the extents into world coords to get the appropriate size to use for control max visible distance
        float max_range = osg::maximum(extents.xMax()-extents.xMin(),extents.yMax()-extents.yMin());
        float projected_radius =  osg::DegreesToRadians(max_range) * getEllipsoidModel()->getRadiusEquator();
        float center_offset = (max_range/360.0f) * getEllipsoidModel()->getRadiusEquator();
        destinationGraph->_maxVisibleDistance = projected_radius * getRadiusToMaxVisibleDistanceRatio() + center_offset;
    }
    else
    {
        destinationGraph->_maxVisibleDistance = extents.radius()*getRadiusToMaxVisibleDistanceRatio();
    }

    // first create the topmost tile

    // create the name
    std::ostringstream os;
    os << _tileBasename << "_L"<<currentLevel<<"_X"<<currentX<<"_Y"<<currentY;

    destinationGraph->_parent = parent;
    destinationGraph->_name = os.str();
    destinationGraph->_level = currentLevel;
    destinationGraph->_tileX = currentX;
    destinationGraph->_tileY = currentY;
    destinationGraph->_dataSet = this;


    DestinationTile* tile = new DestinationTile;
    tile->_name = destinationGraph->_name;
    tile->_level = currentLevel;
    tile->_tileX = currentX;
    tile->_tileY = currentY;
    tile->_dataSet = this;
    tile->_cs = cs;
    tile->_extents = extents;

    // set to NONE as the tile is a mix of RASTER and VECTOR
    // that way the default of RASTER for image and VECTOR for height is maintained
    tile->_dataType = SpatialProperties::NONE;

    tile->_pixelFormat = (getTextureType()==COMPRESSED_RGBA_TEXTURE||
                          getTextureType()==RGBA ||
                          getTextureType()==RGBA_16) ? GL_RGBA : GL_RGB;
    tile->setMaximumImagerySize(maxImageSize,maxImageSize);
    tile->setMaximumTerrainSize(maxTerrainSize,maxTerrainSize);
    tile->computeMaximumSourceResolution(_sourceGraph.get());

    insertTileToQuadMap(destinationGraph);

    if (currentLevel>=maxNumLevels-1 || currentLevel>=tile->_maxSourceLevel)
    {
        // bottom level can't divide any further.
        destinationGraph->_tiles.push_back(tile);
    }
    else
    {
        destinationGraph->_type = LOD;
        destinationGraph->_tiles.push_back(tile);

        bool needToDivideX = false;
        bool needToDivideY = false;

        // note, resolutionSensitivityScale should probably be customizable.. will consider this option for later inclusion.
        double resolutionSensitivityScale = 0.9;
        for(unsigned int layerNum=0;
            layerNum<tile->_imagery.size();
            ++layerNum)
        {
            unsigned int texture_numColumns;
            unsigned int texture_numRows;
            double texture_dx;
            double texture_dy;
            if (tile->computeImageResolution(layerNum,texture_numColumns,texture_numRows,texture_dx,texture_dy))
            {
                if (texture_dx*resolutionSensitivityScale>tile->_imagery[layerNum]._imagery_maxSourceResolutionX) needToDivideX = true;
                if (texture_dy*resolutionSensitivityScale>tile->_imagery[layerNum]._imagery_maxSourceResolutionY) needToDivideY = true;
            }
        }
                
        unsigned int dem_numColumns;
        unsigned int dem_numRows;
        double dem_dx;
        double dem_dy;
        if (tile->computeTerrainResolution(dem_numColumns,dem_numRows,dem_dx,dem_dy))
        {
            if (dem_dx*resolutionSensitivityScale>tile->_terrain_maxSourceResolutionX) needToDivideX = true;
            if (dem_dy*resolutionSensitivityScale>tile->_terrain_maxSourceResolutionY) needToDivideY = true;
        }
        
        float xCenter = (extents.xMin()+extents.xMax())*0.5f;
        float yCenter = (extents.yMin()+extents.yMax())*0.5f;
        
        unsigned int newLevel = currentLevel+1;
        unsigned int newX = currentX*2;
        unsigned int newY = currentY*2;

        if (needToDivideX && needToDivideY)
        {
            float aspectRatio = (extents.yMax()- extents.yMin())/(extents.xMax()- extents.xMin());
            
            if (aspectRatio>1.414) needToDivideX = false;
            else if (aspectRatio<.707) needToDivideY = false;
        }

        if (needToDivideX && needToDivideY)
        {
            my_notify(osg::INFO)<<"Need to Divide X + Y for level "<<currentLevel<<std::endl;
            // create four tiles.
            GeospatialExtents bottom_left(extents.xMin(),extents.yMin(),xCenter,yCenter, extents._isGeographic);
            GeospatialExtents bottom_right(xCenter,extents.yMin(),extents.xMax(),yCenter, extents._isGeographic);
            GeospatialExtents top_left(extents.xMin(),yCenter,xCenter,extents.yMax(), extents._isGeographic);
            GeospatialExtents top_right(xCenter,yCenter,extents.xMax(),extents.yMax(), extents._isGeographic);

            destinationGraph->_children.push_back(createDestinationGraph(destinationGraph,
                                                                         cs,
                                                                         bottom_left,
                                                                         maxImageSize,
                                                                         maxTerrainSize,
                                                                         newLevel,
                                                                         newX,
                                                                         newY,
                                                                         maxNumLevels));

            destinationGraph->_children.push_back(createDestinationGraph(destinationGraph,
                                                                         cs,
                                                                         bottom_right,
                                                                         maxImageSize,
                                                                         maxTerrainSize,
                                                                         newLevel,
                                                                         newX+1,
                                                                         newY,
                                                                         maxNumLevels));

            destinationGraph->_children.push_back(createDestinationGraph(destinationGraph,
                                                                         cs,
                                                                         top_left,
                                                                         maxImageSize,
                                                                         maxTerrainSize,
                                                                         newLevel,
                                                                         newX,
                                                                         newY+1,
                                                                         maxNumLevels));

            destinationGraph->_children.push_back(createDestinationGraph(destinationGraph,
                                                                         cs,
                                                                         top_right,
                                                                         maxImageSize,
                                                                         maxTerrainSize,
                                                                         newLevel,
                                                                         newX+1,
                                                                         newY+1,
                                                                         maxNumLevels));
                                                                         
            // Set all there max distance to the same value to ensure the same LOD bining.
            float cutOffDistance = destinationGraph->_maxVisibleDistance*0.5f;
            
            for(CompositeDestination::ChildList::iterator citr=destinationGraph->_children.begin();
                citr!=destinationGraph->_children.end();
                ++citr)
            {
                (*citr)->_maxVisibleDistance = cutOffDistance;
            }

        }
        else if (needToDivideX)
        {
            my_notify(osg::INFO)<<"Need to Divide X only"<<std::endl;

            // create two tiles.
            GeospatialExtents left(extents.xMin(),extents.yMin(),xCenter,extents.yMax(), extents._isGeographic);
            GeospatialExtents right(xCenter,extents.yMin(),extents.xMax(),extents.yMax(), extents._isGeographic);

            destinationGraph->_children.push_back(createDestinationGraph(destinationGraph,
                                                                         cs,
                                                                         left,
                                                                         maxImageSize,
                                                                         maxTerrainSize,
                                                                         newLevel,
                                                                         newX,
                                                                         newY,
                                                                         maxNumLevels));

            destinationGraph->_children.push_back(createDestinationGraph(destinationGraph,
                                                                         cs,
                                                                         right,
                                                                         maxImageSize,
                                                                         maxTerrainSize,
                                                                         newLevel,
                                                                         newX+1,
                                                                         newY,
                                                                         maxNumLevels));

                                                                         
            // Set all there max distance to the same value to ensure the same LOD bining.
            float cutOffDistance = destinationGraph->_maxVisibleDistance*0.5f;
            
            for(CompositeDestination::ChildList::iterator citr=destinationGraph->_children.begin();
                citr!=destinationGraph->_children.end();
                ++citr)
            {
                (*citr)->_maxVisibleDistance = cutOffDistance;
            }

        }
        else if (needToDivideY)
        {
            my_notify(osg::INFO)<<"Need to Divide Y only"<<std::endl;

            // create two tiles.
            GeospatialExtents top(extents.xMin(),yCenter,extents.xMax(),extents.yMax(), extents._isGeographic);
            GeospatialExtents bottom(extents.xMin(),extents.yMin(),extents.xMax(),yCenter, extents._isGeographic);

            destinationGraph->_children.push_back(createDestinationGraph(destinationGraph,
                                                                         cs,
                                                                         bottom,
                                                                         maxImageSize,
                                                                         maxTerrainSize,
                                                                         newLevel,
                                                                         newX,
                                                                         newY,
                                                                         maxNumLevels));

            destinationGraph->_children.push_back(createDestinationGraph(destinationGraph,
                                                                         cs,
                                                                         top,
                                                                         maxImageSize,
                                                                         maxTerrainSize,
                                                                         newLevel,
                                                                         newX,
                                                                         newY+1,
                                                                         maxNumLevels));
                                                                         
            // Set all there max distance to the same value to ensure the same LOD bining.
            float cutOffDistance = destinationGraph->_maxVisibleDistance*0.5f;
            
            for(CompositeDestination::ChildList::iterator citr=destinationGraph->_children.begin();
                citr!=destinationGraph->_children.end();
                ++citr)
            {
                (*citr)->_maxVisibleDistance = cutOffDistance;
            }

        }
        else
        {
            my_notify(osg::INFO)<<"No Need to Divide"<<std::endl;
        }
    }
    
    return destinationGraph;
}

void DataSet::computeDestinationGraphFromSources(unsigned int numLevels)
{
    if (!_sourceGraph) return;

    // ensure we have a valid coordinate system
    if (!_destinationCoordinateSystem)
    {
        for(CompositeSource::source_iterator itr(_sourceGraph.get());itr.valid();++itr)
        {
            SourceData* sd = (*itr)->getSourceData();
            if (sd)
            {
                if (sd->_cs.valid())
                {
                    _destinationCoordinateSystem = sd->_cs;
                    my_notify(osg::INFO)<<"Setting coordinate system to "<<_destinationCoordinateSystem->getCoordinateSystem()<<std::endl;
                    break;
                }
            }
        }
    }
    
    if (!_intermediateCoordinateSystem)
    {
        CoordinateSystemType cst = getCoordinateSystemType(_destinationCoordinateSystem.get());

        my_notify(osg::INFO)<<"new DataSet::createDestination()"<<std::endl;
        if (cst!=GEOGRAPHIC && getConvertFromGeographicToGeocentric())
        {
            // need to use the geocentric coordinate system as a base for creating an geographic intermediate
            // coordinate system.
            OGRSpatialReference oSRS;
            
            char    *pszWKT = NULL;
            oSRS.SetWellKnownGeogCS( "WGS84" );
            oSRS.exportToWkt( &pszWKT );
            
            setIntermediateCoordinateSystem(pszWKT);
        }
        else
        {
            _intermediateCoordinateSystem = _destinationCoordinateSystem;
        }
    }


    CoordinateSystemType destinateCoordSytemType = getCoordinateSystemType(_destinationCoordinateSystem.get());
    if (destinateCoordSytemType==GEOGRAPHIC && !getConvertFromGeographicToGeocentric())
    {
        // convert elevation into degrees.
        setVerticalScale(1.0f/111319.0f);
    }

    // get the extents of the sources and
    GeospatialExtents extents(_extents);
    extents._isGeographic = destinateCoordSytemType==GEOGRAPHIC;

    if (!extents.valid()) 
    {
        for(CompositeSource::source_iterator itr(_sourceGraph.get());itr.valid();++itr)
        {
            SourceData* sd = (*itr)->getSourceData();
            if (sd)
            {
                GeospatialExtents local_extents(sd->getExtents(_intermediateCoordinateSystem.get()));
                my_notify(osg::INFO)<<"local_extents = xMin()"<<local_extents.xMin()<<" "<<local_extents.xMax()<<std::endl;
                my_notify(osg::INFO)<<"                yMin()"<<local_extents.yMin()<<" "<<local_extents.yMax()<<std::endl;
                
                if (destinateCoordSytemType==GEOGRAPHIC)
                {
                    // need to clamp within -180 and 180 range.
                    if (local_extents.xMin()>180.0) 
                    {
                        // shift back to -180 to 180 range
                        local_extents.xMin() -= 360.0;
                        local_extents.xMax() -= 360.0;
                    }
                    else if (local_extents.xMin()<-180.0) 
                    {
                        // shift back to -180 to 180 range
                        local_extents.xMin() += 360.0;
                        local_extents.xMax() += 360.0;
                    }
                }

                extents.expandBy(local_extents);
            }
        }
    }
    

    if (destinateCoordSytemType==GEOGRAPHIC)
    {
        double xRange = extents.xMax() - extents.xMin();
        if (xRange>360.0) 
        {
            // clamp to proper 360 range.
            extents.xMin() = -180.0;
            extents.xMax() = 180.0;
        }
    }
    

    // compute the number of texture layers required.
    unsigned int maxTextureUnit = 0;
    for(CompositeSource::source_iterator sitr(_sourceGraph.get());sitr.valid();++sitr)
    {
        Source* source = sitr->get();
        if (source) 
        {
            if (maxTextureUnit<source->getLayer()) maxTextureUnit = source->getLayer();
        }
    }
    _numTextureLevels = maxTextureUnit+1;


    my_notify(osg::INFO)<<"extents = xMin()"<<extents.xMin()<<" "<<extents.xMax()<<std::endl;
    my_notify(osg::INFO)<<"          yMin()"<<extents.yMin()<<" "<<extents.yMax()<<std::endl;

    // then create the destinate graph accordingly.
    _destinationGraph = createDestinationGraph(0,
                                               _intermediateCoordinateSystem.get(),
                                               extents,
                                               _maximumTileImageSize,
                                               _maximumTileTerrainSize,
                                               0,
                                               0,
                                               0,
                                               numLevels);
                                                           
    // now traverse the destination graph to build neighbours.        
    _destinationGraph->computeNeighboursFromQuadMap();

}
void DataSet::updateSourcesForDestinationGraphNeeds()
{
    if (!_destinationGraph || !_sourceGraph) return;


    std::string temporyFilePrefix("temporaryfile_");

    // compute the resolutions of the source that are required.
    {
        _destinationGraph->addRequiredResolutions(_sourceGraph.get());


        for(CompositeSource::source_iterator sitr(_sourceGraph.get());sitr.valid();++sitr)
        {
            Source* source = sitr->get();
            if (source) 
            {
                my_notify(osg::INFO)<<"Source File "<<source->getFileName()<<std::endl;


                const Source::ResolutionList& resolutions = source->getRequiredResolutions();
                my_notify(osg::INFO)<<"    resolutions.size() "<<resolutions.size()<<std::endl;
                my_notify(osg::INFO)<<"    { "<<std::endl;
                Source::ResolutionList::const_iterator itr;
                for(itr=resolutions.begin();
                    itr!=resolutions.end();
                    ++itr)
                {
                    my_notify(osg::INFO)<<"        resX="<<itr->_resX<<" resY="<<itr->_resY<<std::endl;
                }
                my_notify(osg::INFO)<<"    } "<<std::endl;

                source->consolodateRequiredResolutions();

                my_notify(osg::INFO)<<"    consolodated resolutions.size() "<<resolutions.size()<<std::endl;
                my_notify(osg::INFO)<<"    consolodated { "<<std::endl;
                for(itr=resolutions.begin();
                    itr!=resolutions.end();
                    ++itr)
                {
                    my_notify(osg::INFO)<<"        resX="<<itr->_resX<<" resY="<<itr->_resY<<std::endl;
                }
                my_notify(osg::INFO)<<"    } "<<std::endl;
            }

        }


    }

    // do standardisation of coordinates systems.
    // do any reprojection if required.
    {
        for(CompositeSource::source_iterator itr(_sourceGraph.get());itr.valid();++itr)
        {
            Source* source = itr->get();
            if (source && source->needReproject(_intermediateCoordinateSystem.get()))
            {
                // do the reprojection to a tempory file.
                std::string newFileName = temporyFilePrefix + osgDB::getStrippedName(source->getFileName()) + ".tif";
                
                Source* newSource = source->doReproject(newFileName,_intermediateCoordinateSystem.get());
                
                // replace old source by new one.
                if (newSource) *itr = newSource;
                else
                {
                    my_notify(osg::WARN)<<"Failed to reproject"<<source->getFileName()<<std::endl;
                    *itr = 0;
                }
            }
        }
    }
    
    // do sampling of data to required values.
    {
        for(CompositeSource::source_iterator itr(_sourceGraph.get());itr.valid();++itr)
        {
            Source* source = itr->get();
            if (source) source->buildOverviews();
        }
    }

    // sort the sources so that the lowest res tiles are drawn first.
    {
        for(CompositeSource::source_iterator itr(_sourceGraph.get());itr.valid();++itr)
        {
            Source* source = itr->get();
            if (source)
            {
                source->setSortValueFromSourceDataResolution();
                my_notify(osg::INFO)<<"sort "<<source->getFileName()<<" value "<<source->getSortValue()<<std::endl;
            }
            
        }
        
        // sort them so highest sortValue is first.

        _sourceGraph->sort();
    }
    
    my_notify(osg::INFO)<<"Using source_lod_iterator itr"<<std::endl;
        
    // buggy mips compiler requires this local variable in source_lod_iterator
    // usage below, since using _sourceGraph.get() as it should be was causing
    // a MIPSpro compiler error "The member "vpb::DataSet::_sourceGraph" is inaccessible."
    CompositeSource* my_sourceGraph = _sourceGraph.get();

    for(CompositeSource::source_lod_iterator csitr(my_sourceGraph,CompositeSource::LODSourceAdvancer(0.0));csitr.valid();++csitr)
    {
        Source* source = csitr->get();
        if (source)
        {
            my_notify(osg::INFO)<<"  LOD "<<(*csitr)->getFileName()<<std::endl;
        }
    }
    my_notify(osg::INFO)<<"End of Using Source Iterator itr"<<std::endl;
    
}

void DataSet::populateDestinationGraphFromSources()
{
    if (!_destinationGraph || !_sourceGraph) return;

    my_notify(osg::NOTICE)<<"started DataSet::populateDestinationGraphFromSources)"<<std::endl;

    if (_databaseType==LOD_DATABASE)
    {

        // for each DestinationTile populate it.
        _destinationGraph->readFrom(_sourceGraph.get());

        // for each DestinationTile equalize the boundaries so they all fit each other without gaps.
        _destinationGraph->equalizeBoundaries();
        
        
    }
    else
    {
        // for each level
        //  compute x and y range
        //  from top row down to bottom row equalize boundairies a write out
    }
    my_notify(osg::NOTICE)<<"completed DataSet::populateDestinationGraphFromSources)"<<std::endl;
}


void DataSet::_readRow(Row& row)
{
    my_notify(osg::NOTICE)<<"_readRow "<<row.size()<<std::endl;
    for(Row::iterator citr=row.begin();
        citr!=row.end();
        ++citr)
    {
        CompositeDestination* cd = citr->second;
        for(CompositeDestination::TileList::iterator titr=cd->_tiles.begin();
            titr!=cd->_tiles.end();
            ++titr)
        {
            DestinationTile* tile = titr->get();
            my_notify(osg::NOTICE)<<"   reading tile level="<<tile->_level<<" X="<<tile->_tileX<<" Y="<<tile->_tileY<<std::endl;
            tile->readFrom(_sourceGraph.get());
        }
    }
}

void DataSet::_equalizeRow(Row& row)
{
    my_notify(osg::NOTICE)<<"_equalizeRow "<<row.size()<<std::endl;
    for(Row::iterator citr=row.begin();
        citr!=row.end();
        ++citr)
    {
        CompositeDestination* cd = citr->second;
        for(CompositeDestination::TileList::iterator titr=cd->_tiles.begin();
            titr!=cd->_tiles.end();
            ++titr)
        {
            DestinationTile* tile = titr->get();
            my_notify(osg::NOTICE)<<"   equalizing tile level="<<tile->_level<<" X="<<tile->_tileX<<" Y="<<tile->_tileY<<std::endl;
            tile->equalizeBoundaries();
            tile->setTileComplete(true);
        }
    }
}

void DataSet::_writeNodeFile(const osg::Node& node,const std::string& filename)
{
    if (_archive.valid()) _archive->writeNode(node,filename);
    else osgDB::writeNodeFile(node, filename);
}

void DataSet::_writeImageFile(const osg::Image& image,const std::string& filename)
{
    if (_archive.valid()) _archive->writeImage(image,filename);
    else osgDB::writeImageFile(image, filename);
}


class WriteImageFilesVisitor : public osg::NodeVisitor
{
public:

    WriteImageFilesVisitor(vpb::DataSet* dataSet):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _dataSet(dataSet) {}

    vpb::DataSet* _dataSet;
    
    virtual void apply(osg::Node& node)
    {
        if (node.getStateSet()) apply(*(node.getStateSet()));

        traverse(node);
    }

    virtual void apply(osg::Geode& geode)
    {
        if (geode.getStateSet()) apply(*(geode.getStateSet()));

        for(unsigned int i=0;i<geode.getNumDrawables();++i)
        {
            if (geode.getDrawable(i)->getStateSet()) apply(*(geode.getDrawable(i)->getStateSet()));
        }
        
        traverse(geode);
    }

    void apply(osg::StateSet& stateset)
    {
        for(unsigned int i=0;i<stateset.getTextureAttributeList().size();++i)
        {
            osg::Image* image = 0;
            osg::Texture2D* texture2D = dynamic_cast<osg::Texture2D*>(stateset.getTextureAttribute(i,osg::StateAttribute::TEXTURE));
            if (texture2D) image = texture2D->getImage();
            
            if (image)
            {
                _dataSet->_writeImageFile(*image,(_dataSet->getDirectory()+image->getFileName()).c_str());
            }
        }
    }
};

void DataSet::_writeRow(Row& row)
{
    my_notify(osg::NOTICE)<<"_writeRow "<<row.size()<<std::endl;
    for(Row::iterator citr=row.begin();
        citr!=row.end();
        ++citr)
    {
        CompositeDestination* cd = citr->second;
        CompositeDestination* parent = cd->_parent;
        
        if (parent)
        {
            if (!parent->getSubTilesGenerated() && parent->areSubTilesComplete())
            {
                osg::ref_ptr<osg::Node> node = parent->createSubTileScene();
                std::string filename = _directory+parent->getSubTileName();
                if (node.valid())
                {
                    my_notify(osg::NOTICE)<<"   writeSubTile filename="<<filename<<std::endl;
                    _writeNodeFile(*node,filename);

                    if (_tileExtension==".osg")
                    {
                        WriteImageFilesVisitor wifv(this);
                        node->accept(wifv);
                    }

                    parent->setSubTilesGenerated(true);
                    parent->unrefSubTileData();
                }
                else
                {
                    my_notify(osg::WARN)<<"   failed to writeSubTile node for tile, filename="<<filename<<std::endl;
                }
            }
        }
        else
        {
            osg::ref_ptr<osg::Node> node = cd->createPagedLODScene();
            
            if (_decorateWithCoordinateSystemNode)
            {
                node = decorateWithCoordinateSystemNode(node.get());
            }
            
            if (_decorateWithMultiTextureControl)
            {
                node = decorateWithMultiTextureControl(node.get());
            }

            if (!_comment.empty())
            {
                node->addDescription(_comment);
            }

            std::string filename = _directory+_tileBasename+_tileExtension;

            if (node.valid())
            {
                my_notify(osg::NOTICE)<<"   writeNodeFile = "<<cd->_level<<" X="<<cd->_tileX<<" Y="<<cd->_tileY<<" filename="<<filename<<std::endl;
                _writeNodeFile(*node,filename);
                
                if (_tileExtension==".osg")
                {
                    WriteImageFilesVisitor wifv(this);
                    node->accept(wifv);
                }
            }
            else
            {
                my_notify(osg::WARN)<<"   faild to write node for tile = "<<cd->_level<<" X="<<cd->_tileX<<" Y="<<cd->_tileY<<" filename="<<filename<<std::endl;
            }

            // record the top nodes as the rootNode of the database
            _rootNode = node;

        }
    }
}

void DataSet::createDestination(unsigned int numLevels)
{
    my_notify(osg::NOTICE)<<"started DataSet::createDestination("<<numLevels<<")"<<std::endl;

    computeDestinationGraphFromSources(numLevels);
    
    updateSourcesForDestinationGraphNeeds();

    my_notify(osg::NOTICE)<<"completed DataSet::createDestination("<<numLevels<<")"<<std::endl;

}

osg::Node* DataSet::decorateWithCoordinateSystemNode(osg::Node* subgraph)
{
    // don't decorate if no coord system is set.
    if (_destinationCoordinateSystem->getCoordinateSystem().empty()) 
        return subgraph;

    osg::CoordinateSystemNode* csn = new osg::CoordinateSystemNode(
            _destinationCoordinateSystem->getFormat(),
            _destinationCoordinateSystem->getCoordinateSystem());
    
    // set the ellipsoid model if geocentric coords are used.
    if (getConvertFromGeographicToGeocentric()) csn->setEllipsoidModel(getEllipsoidModel());
    
    // add the a subgraph.
    csn->addChild(subgraph);

    return csn;
}

osg::Node* DataSet::decorateWithMultiTextureControl(osg::Node* subgraph)
{
    // if only one layer exists don't need to decorate with MultiTextureControl
    if (_numTextureLevels<=1) return subgraph;
    
    
    // multiple layers active so use osgFX::MultiTextureControl to manage them
    osgFX::MultiTextureControl* mtc = new osgFX::MultiTextureControl;
    float r = 1.0f/(float)_numTextureLevels;
    for(unsigned int i=0;i<_numTextureLevels;++i)
    {
        mtc->setTextureWeight(i,r);
    }

    // add the a subgraph.
    mtc->addChild(subgraph);

    return mtc;
}


void DataSet::_buildDestination(bool writeToDisk)
{
    if (!_state) _state = new osg::State;

    osg::ref_ptr<osgDB::ReaderWriter::Options> previous_options = osgDB::Registry::instance()->getOptions();
    if(previous_options.get()) {
        osg::notify(osg::NOTICE) << "vpb: adding optionstring" << previous_options->getOptionString() << std::endl;
        osgDB::Registry::instance()->setOptions(new osgDB::ReaderWriter::Options(std::string("precision 16") + std::string(" ") + previous_options->getOptionString()) );
    } else {
        osgDB::Registry::instance()->setOptions(new osgDB::ReaderWriter::Options("precision 16"));
    }
    if (!_archive && !_archiveName.empty())
    {
        unsigned int indexBlockSizeHint=4096;
        _archive = osgDB::openArchive(_archiveName, osgDB::Archive::CREATE, indexBlockSizeHint);
    }

    if (_destinationGraph.valid())
    {
        std::string filename = _directory+_tileBasename+_tileExtension;
        
        if (_archive.valid())
        {
            my_notify(osg::NOTICE)<<"started DataSet::writeDestination("<<_archiveName<<")"<<std::endl;
            my_notify(osg::NOTICE)<<"        archive file = "<<_archiveName<<std::endl;
            my_notify(osg::NOTICE)<<"        archive master file = "<<filename<<std::endl;
        }
        else
        {
            my_notify(osg::NOTICE)<<"started DataSet::writeDestination("<<filename<<")"<<std::endl;
        }

        if (_databaseType==LOD_DATABASE)
        {
            populateDestinationGraphFromSources();
            _rootNode = _destinationGraph->createScene();

            if (_decorateWithCoordinateSystemNode)
            {
                _rootNode = decorateWithCoordinateSystemNode(_rootNode.get());
            }

            if (_decorateWithMultiTextureControl)
            {
                _rootNode = decorateWithMultiTextureControl(_rootNode.get());
            }

            if (!_comment.empty())
            {
                _rootNode->addDescription(_comment);
            }

            if (writeToDisk)
            {
                _writeNodeFile(*_rootNode,filename);
                if (_tileExtension==".osg")
                {
                    WriteImageFilesVisitor wifv(this);
                    _rootNode->accept(wifv);
                }
            }
        }
        else  // _databaseType==PagedLOD_DATABASE
        {
            // for each level build read and write the rows.
            for(QuadMap::iterator qitr=_quadMap.begin();
                qitr!=_quadMap.end();
                ++qitr)
            {
                Level& level = qitr->second;
                
                // skip is level is empty.
                if (level.empty()) continue;
                
                my_notify(osg::INFO)<<"New level"<<std::endl;

                Level::iterator prev_itr = level.begin();
                _readRow(prev_itr->second);
                Level::iterator curr_itr = prev_itr;
                ++curr_itr;
                for(;
                    curr_itr!=level.end();
                    ++curr_itr)
                {
                    _readRow(curr_itr->second);
                    
                    _equalizeRow(prev_itr->second);
                    if (writeToDisk) _writeRow(prev_itr->second);
                    
                    prev_itr = curr_itr;
                }
                
                _equalizeRow(prev_itr->second);
                if (writeToDisk)
                {
                    _writeRow(prev_itr->second);
                }
            }
        }

        if (_archive.valid())
        {
            my_notify(osg::NOTICE)<<"completed DataSet::writeDestination("<<_archiveName<<")"<<std::endl;
            my_notify(osg::NOTICE)<<"          archive file = "<<_archiveName<<std::endl;
            my_notify(osg::NOTICE)<<"          archive master file = "<<filename<<std::endl;
        }
        else
        {
            my_notify(osg::NOTICE)<<"completed DataSet::writeDestination("<<filename<<")"<<std::endl;
        }

    }
    else
    {
        my_notify(osg::WARN)<<"Error: no scene graph to output, no file written."<<std::endl;
    }

    if (_archive.valid()) _archive->close();

    osgDB::Registry::instance()->setOptions(previous_options.get());

}


bool DataSet::addLayer(Source::Type type, osgTerrain::Layer* layer, unsigned layerNum)
{
    osgTerrain::HeightFieldLayer* hfl = dynamic_cast<osgTerrain::HeightFieldLayer*>(layer);
    if (hfl)
    {
        // need to read locator.
        std::cout<<"Elevation HeightFieldLayer supplied"<<hfl->getFileName()<<std::endl;
        vpb::Source* source = new vpb::Source(type, hfl->getFileName());
        source->setLayer(layerNum);

        if (layer->getLocator() && !layer->getLocator()->getDefinedInFile())
        {
            std::cout<<"Setting Coordinatesytem for "<<layer->getFileName()<<std::endl;
            source->setGeoTransformPolicy(vpb::Source::PREFER_CONFIG_SETTINGS_BUT_SCALE_BY_FILE_RESOLUTION);
            source->setGeoTransform(layer->getLocator()->getTransform());

            source->setCoordinateSystemPolicy(vpb::Source::PREFER_CONFIG_SETTINGS);
            source->setCoordinateSystem(layer->getLocator()->getCoordinateSystem());
        } 

        addSource(source);
        return true;
    }
    
    osgTerrain::ImageLayer* iml = dynamic_cast<osgTerrain::ImageLayer*>(layer);
    if (iml)
    {
        // need to read locator
        std::cout<<"ImageLayer supplied"<<iml->getFileName()<<std::endl;
        vpb::Source* source = new vpb::Source(type, iml->getFileName());
        source->setLayer(layerNum);

        if (layer->getLocator() && !layer->getLocator()->getDefinedInFile())
        {
            std::cout<<"Setting Coordinatesytem for "<<layer->getFileName()<<std::endl;

            source->setGeoTransformPolicy(vpb::Source::PREFER_CONFIG_SETTINGS_BUT_SCALE_BY_FILE_RESOLUTION);
            source->setGeoTransform(layer->getLocator()->getTransform());

            source->setCoordinateSystemPolicy(vpb::Source::PREFER_CONFIG_SETTINGS);
            source->setCoordinateSystem(layer->getLocator()->getCoordinateSystem());
        }

        addSource(source);
        return true;
    }
    
    osgTerrain::ProxyLayer* pl = dynamic_cast<osgTerrain::ProxyLayer*>(layer);
    if (pl)
    {
        std::cout<<"ProxyLayer supplied"<<pl->getFileName()<<std::endl;
        vpb::Source* source = new vpb::Source(type, pl->getFileName());
        source->setLayer(layerNum);

        if (layer->getLocator() && !layer->getLocator()->getDefinedInFile())
        {
            std::cout<<"Setting Coordinatesytem for "<<layer->getFileName()<<std::endl;

            source->setGeoTransformPolicy(vpb::Source::PREFER_CONFIG_SETTINGS_BUT_SCALE_BY_FILE_RESOLUTION);
            source->setGeoTransform(layer->getLocator()->getTransform());

            source->setCoordinateSystemPolicy(vpb::Source::PREFER_CONFIG_SETTINGS);
            source->setCoordinateSystem(layer->getLocator()->getCoordinateSystem());
        }

        addSource(source);
        return true;
    }

    osgTerrain::CompositeLayer* compositeLayer = dynamic_cast<osgTerrain::CompositeLayer*>(layer);
    {
        std::cout<<"CompositLayer supplied"<<std::endl;
        for(unsigned int i=0; i<compositeLayer->getNumLayers();++i)
        {
            if (compositeLayer->getLayer(i))
            {
                addLayer(type, compositeLayer->getLayer(i), layerNum);
            }
            else if (!compositeLayer->getFileName(i).empty())
            {
                vpb::Source* source = new vpb::Source(type, compositeLayer->getFileName(i));
                source->setLayer(layerNum);
                addSource(source);
            }
        }
        return true;
    }
    return false;
}

bool DataSet::addTerrain(osgTerrain::Terrain* terrain)
{
    if (terrain->getLocator())
    {
    }
 
    vpb::DatabaseBuilder* db = dynamic_cast<vpb::DatabaseBuilder*>(terrain->getTerrainTechnique());
    if (db)
    {
        setBuildOptions(*db);
    }

    if (terrain->getElevationLayer())
    {
        addLayer(vpb::Source::HEIGHT_FIELD, terrain->getElevationLayer(), 0);
    }

    for(unsigned int i=0; i<terrain->getNumColorLayers();++i)
    {
        osgTerrain::Layer* layer = terrain->getColorLayer(i);
        if (layer) 
        {
            addLayer(vpb::Source::IMAGE, layer, i);
        }
    }

    return true;
}

osgTerrain::Terrain* DataSet::createTerrainRepresentation() const
{
    osg::ref_ptr<osgTerrain::Terrain> terrain = new osgTerrain::Terrain;

    for(CompositeSource::source_iterator itr(_sourceGraph.get());itr.valid();++itr)
    {
        osg::ref_ptr<Source> source = (*itr);
#if 0
            osg::Locator* locator = new osg::Locator;
            osg::ref_ptr<osg::CoordinateSystemNode>     _cs;
            osg::Matrixd                                _geoTransform;
            GeospatialExtents                           _extents;
            DataType                                    _dataType;
            unsigned int                                _numValuesX;
            unsigned int                                _numValuesY;
            unsigned int                                _numValuesZ;
#endif
        unsigned int layerNum = source->getLayer();


        osg::ref_ptr<osg::Object> loadedObject = osgDB::readObjectFile(source->getFileName()+".gdal");
        osgTerrain::Layer* loadedLayer = dynamic_cast<osgTerrain::Layer*>(loadedObject.get());

        if (loadedLayer)
        {
            if (!loadedLayer->getLocator())
            {
                osgTerrain::Locator* locator = new osgTerrain::Locator;
                locator->setTransform(source->getGeoTransform());
                
                if (source->_cs.valid())
                {
                    locator->setCoordinateSystem(source->_cs->getCoordinateSystem());
                    locator->setFormat(source->_cs->getFormat());
                    locator->setEllipsoidModel(source->_cs->getEllipsoidModel());

                    switch(getCoordinateSystemType(source->_cs.get()))
                    {
                        case(GEOCENTRIC): locator->setCoordinateSystemType(osgTerrain::Locator::GEOCENTRIC); break;
                        case(GEOGRAPHIC): locator->setCoordinateSystemType(osgTerrain::Locator::GEOGRAPHIC); break;
                        case(PROJECTED): locator->setCoordinateSystemType(osgTerrain::Locator::PROJECTED); break;
                        case(LOCAL): locator->setCoordinateSystemType(osgTerrain::Locator::PROJECTED); break;
                    };
                }

                loadedLayer->setLocator(locator);
            }
        
            if (source->getType()==Source::IMAGE)
            {
                osgTerrain::Layer* existingLayer = (layerNum < terrain->getNumColorLayers()) ? terrain->getColorLayer(layerNum) : 0;
                osgTerrain::CompositeLayer* compositeLayer = dynamic_cast<osgTerrain::CompositeLayer*>(existingLayer);

                if (compositeLayer)
                {
                    compositeLayer->addLayer( loadedLayer );
                }
                else if (existingLayer)
                {
                    compositeLayer = new osgTerrain::CompositeLayer;
                    compositeLayer->addLayer( existingLayer );
                    compositeLayer->addLayer( loadedLayer );

                    terrain->setColorLayer(layerNum, compositeLayer);
                }
                else
                {
                    terrain->setColorLayer(layerNum, loadedLayer);
                }
            }
            else if (source->getType()==Source::HEIGHT_FIELD)
            {
                osgTerrain::Layer* existingLayer = terrain->getElevationLayer();
                osgTerrain::CompositeLayer* compositeLayer = dynamic_cast<osgTerrain::CompositeLayer*>(existingLayer);

                if (compositeLayer)
                {
                    compositeLayer->addLayer( loadedLayer );
                }
                else if (existingLayer)
                {
                    compositeLayer = new osgTerrain::CompositeLayer;
                    compositeLayer->addLayer( existingLayer );
                    compositeLayer->addLayer( loadedLayer );

                    terrain->setElevationLayer(compositeLayer);
                }
                else
                {
                    terrain->setElevationLayer(loadedLayer);
                }
            }
        }
    }
    
    osg::ref_ptr<DatabaseBuilder> builder = new DatabaseBuilder;
    builder->setBuildOptions(*this);
    terrain->setTerrainTechnique(builder.get());

    return terrain.release();
}

class MyGraphicsContext {
    public:
        MyGraphicsContext()
        {
            osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
            traits->x = 0;
            traits->y = 0;
            traits->width = 1;
            traits->height = 1;
            traits->windowDecoration = false;
            traits->doubleBuffer = false;
            traits->sharedContext = 0;
            traits->pbuffer = true;

            _gc = osg::GraphicsContext::createGraphicsContext(traits.get());

            if (!_gc)
            {
                osg::notify(osg::NOTICE)<<"Failed to create pbuffer, failing back to normal graphics window."<<std::endl;
                
                traits->pbuffer = false;
                _gc = osg::GraphicsContext::createGraphicsContext(traits.get());
            }

            if (_gc.valid()) 
            
            
            {
                _gc->realize();
                _gc->makeCurrent();
                std::cout<<"Realized window"<<std::endl;
            }
        }
        
        bool valid() const { return _gc.valid() && _gc->isRealized(); }
        
    private:
        osg::ref_ptr<osg::GraphicsContext> _gc;
};



int DataSet::run()
{
    // dummy Viewer to get round silly Windows autoregistration problem for GraphicsWindowWin32.cpp
    osgViewer::Viewer viewer;
    
    MyGraphicsContext context;
    if (!context.valid())
    {
        osg::notify(osg::NOTICE)<<"Error: Unable to create graphis context - cannot run osgdem"<<std::endl;
        return 1;
    }

    loadSources();

    createDestination(getMaximumNumOfLevels());

    writeDestination();        
}
