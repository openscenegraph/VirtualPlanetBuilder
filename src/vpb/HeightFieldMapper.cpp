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


namespace vpb {

HeightFieldMapper::HeightFieldMapper(osg::HeightField & hf)
:     _mappingMode(PER_VERTEX),
    _hf(hf)
{
    _xMin = _hf.getOrigin().x();
    _yMin = _hf.getOrigin().y();
    _xMax = _xMin + _hf.getXInterval() * _hf.getNumColumns();
    _yMax = _yMin + _hf.getYInterval() * _hf.getNumRows();

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
        
        
        osg::UIntArray & _ia;
        osg::Vec3 _centroid;
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


bool HeightFieldMapper::getCentroid(osg::Geometry & geometry, osg::Vec3 & centroid) const
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
        osg::Vec3 centroid;
        if (getCentroid(geometry, centroid) == false) return false;
        
        // ** get z value to add to Z coordinates
        double zHeightField = getZfromXY(centroid.x(), centroid.y());
        if (zHeightField == FLT_MAX) return false;
        
        double z = centroid.z() - zHeightField;
        
        // add z value to z coordinates to all vertex 
        osgUtil::AddRangeFunctor arf;
        arf._vector = osg::Vec3(0,0,z);
        arf._begin = 0;
        arf._count = geometry.getVertexArray()->getNumElements();
        geometry.getVertexArray()->accept(arf);
        
        return true;
    }
    
    return false;
}


double HeightFieldMapper::getZfromXY(double x, double y) const
{
    if ((x > _xMax) || (x < _xMin) || (y > _yMax) || (y < _yMin)) return false;

    osg::Vec3 point(x,y,0.0f);
    
    /////////////////////////////////////////////////
    // ** search column and row containing this point
    
    point -= _hf.getOrigin();
    
    
    unsigned int column = static_cast<unsigned int>(point.x() / _hf.getXInterval());
    unsigned int row = static_cast<unsigned int>(point.y() / _hf.getYInterval());
    
    osg::Vec3 P00(_hf.getVertex(column, row) - _hf.getOrigin());
    osg::Vec3 P10(_hf.getVertex(column+1, row) - _hf.getOrigin());
    osg::Vec3 P01(_hf.getVertex(column, row+1) - _hf.getOrigin());
    
    float ratio00_10 = (point.x() - P00.x()) / (P10.x() - P00.x());
    float ratio00_01 = (point.y() - P00.y()) / (P01.y() - P00.y());
    
    float z = (ratio00_10 * (P10.z() - P00.z())) + (ratio00_01 * (P01.z() - P00.z()));
    
    return (P00.z() + z);  
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
