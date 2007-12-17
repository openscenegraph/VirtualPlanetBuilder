#ifndef EXTRUDEVISITOR_H_
#define EXTRUDEVISITOR_H_

#include <osg/NodeVisitor>
#include <osg/Geometry>

class ExtrudeVisitor : public osg::NodeVisitor
{
    public:

    	enum Mode
    	{
    		Replace,
    		Merge
    	};
    	
    	ExtrudeVisitor(Mode mode) : _mode(mode) {}
    	ExtrudeVisitor() : _mode(Merge) {}
    	
    	Mode getMode() { return _mode; }
    	void setMode(Mode mode) { _mode = mode; }
    	
        virtual void apply(osg::Geode& node);
        
    private:
        
        void extrude(osg::Geometry& geometry, osg::Vec3 & extrudeVector);
        osg::DrawElementsUInt * linkAsTriangleStrip(osg::UIntArray & originalIndexArray,
                                                    osg::UIntArray & extrudeIndexArray);
        Mode _mode;
};


#endif /*EXTRUDEVISITOR_H_*/
