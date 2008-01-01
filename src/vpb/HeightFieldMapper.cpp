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

#include <vpb/HeightFieldMapper>


#include <osg/Array>
#include <osg/Geometry>
#include <osg/Shape>
#include <osg/Geode>

#include <osgUtil/OperationArrayFunctor>
#include <osgUtil/EdgeCollector>
#include <osgUtil/ConvertVec>

namespace vpb {

HeightFieldMapper::HeightFieldMapper(osg::HeightField & hf)
:     _mappingMode(PER_VERTEX),
    _hf(hf)
{
    _xMin = _hf.getOrigin().x();
    _yMin = _hf.getOrigin().y();
    _xMax = _xMin + _hf.getXInterval() * double(_hf.getNumColumns()-1);
    _yMax = _yMin + _hf.getYInterval() * double(_hf.getNumRows()-1);

}
HeightFieldMapper::HeightFieldMapper(osg::HeightField & hf, double xMin, double xMax, double yMin, double yMax)
:     _mappingMode(PER_VERTEX),
    _hf(hf),
    _xMin(xMin),
    _yMin(yMin),
    _xMax(xMax),
    _yMax(yMax)
{    
}

HeightFieldMapper::~HeightFieldMapper()
{
}



class ComputeCentroidVisitor : public osg::ArrayVisitor
{
    public:
        
        ComputeCentroidVisitor(osg::UIntArray & ia) : _ia(ia) {}
        
        
        void apply(osg::Vec3Array& array) { computeCentroid<osg::Vec3Array>(array); }
        void apply(osg::Vec4Array& array) { computeCentroid<osg::Vec4Array>(array); }
        
        void apply(osg::Vec3dArray& array) { computeCentroid<osg::Vec3dArray>(array); }
        void apply(osg::Vec4dArray& array) { computeCentroid<osg::Vec4dArray>(array); }
    
#if 0        
        template <typename ArrayType>
        void computeCentroid(ArrayType & array)
        {
            double area = 0.0;
            double centroidTmp = 0.0;
            double centroidX = 0.0;
            double centroidY = 0.0;
            
            osg::UIntArray::iterator it, end = _ia.end() - 1;
            for (it = _ia.begin(); it != end; ++it)
            {
                typename ArrayType::ElementDataType & vec0 = array[*it];
                typename ArrayType::ElementDataType & vec1 = array[*(it + 1)];
                
                area += vec0.x() * vec1.y();
                area -= vec0.y() * vec1.x();
                
                centroidTmp = (vec0.x() * vec1.y() - vec1.x() * vec0.y());
                centroidX += (vec0.x() + vec1.x()) * centroidTmp;
                centroidY += (vec0.y() + vec1.y()) * centroidTmp;
            }
            
            typename ArrayType::ElementDataType & vec0 = array[_ia.back()];
            typename ArrayType::ElementDataType & vec1 = array[_ia.front()];
            
            area += vec0.x() * vec1.y();
            area -= vec0.y() * vec1.x();
            
            centroidTmp = (vec0.x() * vec1.y() - vec1.x() * vec0.y());
            centroidX += (vec0.x() + vec1.x()) * centroidTmp;
            centroidY += (vec0.y() + vec1.y()) * centroidTmp;
            
            area *= 0.5;
            area = 1 / (6 * area);
            _centroid.set(area * centroidX, area * centroidY, vec0.z());
        }
#else

        template <typename ArrayType>
        void computeCentroid(ArrayType & array)
        {
            if (_ia.empty()) return;
        
            osg::Vec3d total(0.0,0.0,0.0);
            
            for (osg::UIntArray::iterator it = _ia.begin(); it != _ia.end(); ++it)
            {
                typename ArrayType::ElementDataType & vec = array[*it];
                total.x() += vec.x();
                total.y() += vec.y();
                total.z() += vec.z();
            }
            
            _centroid = total / double(_ia.size()) ;
        }


#endif        
        
        osg::UIntArray & _ia;
        osg::Vec3d _centroid;
};

class HeightFieldMapperArrayVisitor : public osg::ArrayVisitor
{
    public:
        
        template <class T>
        struct HeightFieldMapperOperator
        {
            HeightFieldMapperOperator(const HeightFieldMapper & hfm) : _hfm(hfm) {}
            void operator ()(T & vec) { vec.z() = _hfm.getZfromXY(vec.x(), vec.y()); }

        private:
            
            const HeightFieldMapper _hfm;
        };
        
        
        HeightFieldMapperArrayVisitor(const HeightFieldMapper & hfm) : _hfm(hfm) {}
        
        virtual void apply(osg::Vec3Array & array)
        { std::for_each(array.begin(), array.end(), HeightFieldMapperOperator<osg::Vec3>(_hfm)); }
        virtual void apply(osg::Vec4Array & array)
          { std::for_each(array.begin(), array.end(), HeightFieldMapperOperator<osg::Vec4>(_hfm)); }
        virtual void apply(osg::Vec3dArray & array)
          { std::for_each(array.begin(), array.end(), HeightFieldMapperOperator<osg::Vec3d>(_hfm)); }
        virtual void apply(osg::Vec4dArray & array)
          { std::for_each(array.begin(), array.end(), HeightFieldMapperOperator<osg::Vec4d>(_hfm)); }
                
    private:
        
        const HeightFieldMapper _hfm;
};


bool HeightFieldMapper::getCentroid(osg::Geometry & geometry, osg::Vec3d & centroid) const
{
    if (_mappingMode == PER_GEOMETRY)
    {
        osgUtil::EdgeCollector ec;
        ec.setGeometry(&geometry);
        if (ec._triangleSet.empty()) return false;
            
        // ** get IndexArray of each Edgeloop
        osgUtil::EdgeCollector::IndexArrayList indexArrayList;
        ec.getEdgeloopIndexList(indexArrayList);
        if (indexArrayList.empty()) return false;
        
        // ** compute centroid
        ComputeCentroidVisitor ccv(*indexArrayList.front());
        geometry.getVertexArray()->accept(ccv);
        
        centroid = ccv._centroid;
        return true;
    }
    
    return false;
}

bool HeightFieldMapper::map(osg::Geometry & geometry) const
{
    if (_mappingMode == PER_VERTEX)
    {
        HeightFieldMapperArrayVisitor hfmv(*this);
        geometry.getVertexArray()->accept(hfmv);
        
        return true;
    }
    
    if (_mappingMode == PER_GEOMETRY)
    {
        osg::Vec3d centroid(0.0,0.0,0.0);
        if (getCentroid(geometry, centroid) == false) return false;
        
        // ** get z value to add to Z coordinates
        double zHeightField = getZfromXY(centroid.x(), centroid.y());
        if (zHeightField == DBL_MAX) return false;
        
        double z = zHeightField - centroid.z();
        
        // add z value to z coordinates to all vertex 
        osgUtil::AddRangeFunctor arf;
        arf._vector = osg::Vec3d(0,0,z);
        arf._begin = 0;
        arf._count = geometry.getVertexArray()->getNumElements();
        geometry.getVertexArray()->accept(arf);
        
        return true;
    }
    
    return false;
}


double HeightFieldMapper::getZfromXY(double x, double y) const
{
#if 1
    return robert_getZfromXY(x,y);
#else
    double david_z = david_getZfromXY(x,y);
    double robert_z = robert_getZfromXY(x,y);
    if (david_z != robert_z)
    {
        osg::notify(osg::NOTICE)<<"Warning HeightFieldMapper::getZfromXY("<<x<<","<<y<<") david_z="<<david_z<<" robert_z="<<robert_z<<std::endl;
    }
    
    return robert_z;
#endif
}

double HeightFieldMapper::david_getZfromXY(double x, double y) const
{
    if ((x > _xMax) || (x < _xMin) || (y > _yMax) || (y < _yMin)) return DBL_MAX;

    osg::Vec3d point(x,y,0.0);
    
    // ** search column and row containing this point
    point = point - osg::Vec3d(_hf.getOrigin());
    
    unsigned int column = static_cast<unsigned int>(point.x() / (double)_hf.getXInterval());
    unsigned int row = static_cast<unsigned int>(point.y() / (double)_hf.getYInterval());
    
    // ** take 3 points, POO(0,0), P1O(1,0) and PO1(0,1)
    osg::Vec3d P00, P10, P01;
    
    osg::Vec3f vTmp(_hf.getVertex(column, row) - _hf.getOrigin());
    osgUtil::ConvertVec<osg::Vec3f, osg::Vec3d>::convert(vTmp, P00);
    
    vTmp.set(_hf.getVertex(column+1, row) - _hf.getOrigin());
    osgUtil::ConvertVec<osg::Vec3f, osg::Vec3d>::convert(vTmp, P10);
    
    vTmp.set(_hf.getVertex(column, row+1) - _hf.getOrigin());
    osgUtil::ConvertVec<osg::Vec3f, osg::Vec3d>::convert(vTmp, P01);
    
    // ** compute the ratio in X and Y direction
    double ratio00_10 = (point.x() - P00.x()) / (P10.x() - P00.x());
    double ratio00_01 = (point.y() - P00.y()) / (P01.y() - P00.y());
    
    // ** apply the ratio on z coordinates to find the distance between P00.z and point.z
    double z = (ratio00_10 * (P10.z() - P00.z())) + (ratio00_01 * (P01.z() - P00.z()));
    
    
    return (P00.z() + z);
}

double HeightFieldMapper::robert_getZfromXY(double x, double y) const
{
    if ((x > _xMax) || (x < _xMin) || (y > _yMax) || (y < _yMin)) return DBL_MAX;

    double dx_origin = x-_hf.getOrigin().x();
    double dy_origin = y-_hf.getOrigin().y();

    // compute the cell coordinates
    double cx = dx_origin / double(_hf.getXInterval());
    double cy = dy_origin / double(_hf.getYInterval());
    
    // compute the cell by taking the floor
    double fx = floor(cx);
    double fy = floor(cy);
    int c = static_cast<int>(fx);
    int r = static_cast<int>(fy);
    
    // compute the local cell ratio.
    double rx = cx-fx;
    double ry = cy-fy;
    
    double h00 = _hf.getHeight(c,r);
    double h01 = ((r+1) < _hf.getNumRows()) ? _hf.getHeight(c,r+1) : h00;
    double h10 = ((c) < _hf.getNumColumns()) ? _hf.getHeight(c+1,r) : h00;
    double h11 = ((c+1) < _hf.getNumColumns() && (r+1) < _hf.getNumRows()) ? _hf.getHeight(c+1,r+1) : h00;

    double z = _hf.getOrigin().z() + 
                h00*(1.0-rx)*(1.0-ry) + 
                h01*(1.0-rx)*(ry) + 
                h10*(rx)*(1.0-ry) + 
                h11*(rx)*(ry);
    
    return z;
}


void HeightFieldMapperVisitor::apply(osg::Geode& node)
{
    unsigned int numDrawable = node.getNumDrawables();
    
    for (unsigned int i = 0; i < numDrawable; ++i)
    {
        osg::Geometry * geo = dynamic_cast<osg::Geometry *>(node.getDrawable(i));
        
        if (geo)
        {
            _hfm.map(*geo);
        }
    }
}

} // end vpb namespace
