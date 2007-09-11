/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield 
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

#include <vpb/SpatialProperties>

#include <osg/Notify>

#include <cpl_string.h>
#include <ogr_spatialref.h>

using namespace vpb;


CoordinateSystemType vpb::getCoordinateSystemType(const osg::CoordinateSystemNode* lhs)
{
    if (!lhs) return PROJECTED;

    // set up LHS SpatialReference
    char* projection_string = strdup(lhs->getCoordinateSystem().c_str());
    char* importString = projection_string;
    
    OGRSpatialReference lhsSR;
    lhsSR.importFromWkt(&importString);
    
     
    
    osg::notify(osg::INFO)<<"getCoordinateSystemType("<<projection_string<<")"<<std::endl;
    osg::notify(osg::INFO)<<"    lhsSR.IsGeographic()="<<lhsSR.IsGeographic()<<std::endl;
    osg::notify(osg::INFO)<<"    lhsSR.IsProjected()="<<lhsSR.IsProjected()<<std::endl;
    osg::notify(osg::INFO)<<"    lhsSR.IsLocal()="<<lhsSR.IsLocal()<<std::endl;

    free(projection_string);

    if (strcmp(lhsSR.GetRoot()->GetValue(),"GEOCCS")==0) osg::notify(osg::INFO)<<"    lhsSR. is GEOCENTRIC "<<std::endl;
    

    if (strcmp(lhsSR.GetRoot()->GetValue(),"GEOCCS")==0) return GEOCENTRIC;    
    if (lhsSR.IsGeographic()) return GEOGRAPHIC;
    if (lhsSR.IsProjected()) return PROJECTED;
    if (lhsSR.IsLocal()) return LOCAL;
    return PROJECTED;
}

std::string vpb::coordinateSystemStringToWTK(const std::string& coordinateSystem)
{
    std::string wtkString;

    CPLErrorReset();
    
    OGRSpatialReferenceH hSRS = OSRNewSpatialReference( NULL );
    if( OSRSetFromUserInput( hSRS, coordinateSystem.c_str() ) == OGRERR_NONE )
    {
        char *pszResult = NULL;
        OSRExportToWkt( hSRS, &pszResult );
        
        if (pszResult) wtkString = pszResult;

        CPLFree(pszResult);

    }
    else
    {
        osg::notify(osg::WARN)<<"Warning: coordinateSystem string not recognised."<<std::endl;
        
    }
    
    OSRDestroySpatialReference( hSRS );

    return wtkString;
}

double vpb::getLinearUnits(const osg::CoordinateSystemNode* lhs)
{
    // set up LHS SpatialReference
    char* projection_string = strdup(lhs->getCoordinateSystem().c_str());
    char* importString = projection_string;
    
    OGRSpatialReference lhsSR;
    lhsSR.importFromWkt(&importString);
    
    free(projection_string);

    char* str;
    double result = lhsSR.GetLinearUnits(&str);
    osg::notify(osg::INFO)<<"lhsSR.GetLinearUnits("<<str<<") "<<result<<std::endl;

    osg::notify(osg::INFO)<<"lhsSR.IsGeographic() "<<lhsSR.IsGeographic()<<std::endl;
    osg::notify(osg::INFO)<<"lhsSR.IsProjected() "<<lhsSR.IsProjected()<<std::endl;
    osg::notify(osg::INFO)<<"lhsSR.IsLocal() "<<lhsSR.IsLocal()<<std::endl;
    
    return result;
}

bool vpb::areCoordinateSystemEquivalent(const osg::CoordinateSystemNode* lhs,const osg::CoordinateSystemNode* rhs)
{
    // if ptr's equal the return true
    if (lhs == rhs) return true;
    
    // if one CS is NULL then true false
    if (!lhs || !rhs)
    {
        osg::notify(osg::INFO)<<"areCoordinateSystemEquivalent lhs="<<lhs<<"  rhs="<<rhs<<" return true"<<std::endl;
        return false;
    }
    
    osg::notify(osg::INFO)<<"areCoordinateSystemEquivalent lhs="<<lhs->getCoordinateSystem()<<"  rhs="<<rhs->getCoordinateSystem()<<std::endl;

    // use compare on ProjectionRef strings.
    if (lhs->getCoordinateSystem() == rhs->getCoordinateSystem()) return true;
    
    // set up LHS SpatialReference
    char* projection_string = strdup(lhs->getCoordinateSystem().c_str());
    char* importString = projection_string;
    
    OGRSpatialReference lhsSR;
    lhsSR.importFromWkt(&importString);
    
    free(projection_string);

    // set up RHS SpatialReference
    projection_string = strdup(rhs->getCoordinateSystem().c_str());
    importString = projection_string;

    OGRSpatialReference rhsSR;
    rhsSR.importFromWkt(&importString);

    free(projection_string);
    
    int result = lhsSR.IsSame(&rhsSR);

#if 0
    int result2 = lhsSR.IsSameGeogCS(&rhsSR);

     osg::notify(osg::INFO)<<"areCoordinateSystemEquivalent "<<std::endl
              <<"LHS = "<<lhs->getCoordinateSystem()<<std::endl
              <<"RHS = "<<rhs->getCoordinateSystem()<<std::endl
              <<"result = "<<result<<"  result2 = "<<result2<<std::endl;
#endif
         return result ? true : false;
}


void SpatialProperties::computeExtents()
{
    _extents.init();
    _extents.expandBy( osg::Vec3(0.0,0.0,0.0)*_geoTransform);

    // get correct extent if a vector format is used
    if (_dataType == VECTOR)
        _extents.expandBy( osg::Vec3(_numValuesX-1,_numValuesY-1,0.0)*_geoTransform);
    else
        _extents.expandBy( osg::Vec3(_numValuesX,_numValuesY,0.0)*_geoTransform);
    _extents._isGeographic = getCoordinateSystemType(_cs.get())==GEOGRAPHIC;

    osg::notify(osg::INFO)<<"DataSet::SpatialProperties::computeExtents() is geographic "<<_extents._isGeographic<<std::endl;
}


