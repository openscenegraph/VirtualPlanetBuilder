#ifndef REVERSEPRIMITIVEFUNCTOR_H_
#define REVERSEPRIMITIVEFUNCTOR_H_

#include <osg/PrimitiveSet>
#include <osg/Notify>

class ReversePrimitiveFunctor : public osg::PrimitiveIndexFunctor
{
public:

    virtual ~ReversePrimitiveFunctor() {}

    osg::PrimitiveSet * getReversedPrimitiveSet() { return _reversedPrimitiveSet.get(); }
    
    virtual void setVertexArray(unsigned int /*count*/,const osg::Vec2* /*vertices*/)
    { osg::notify(osg::WARN) << "ReversePrimitiveFunctor : not implemented " << std::endl; }

    virtual void setVertexArray(unsigned int /*count*/,const osg::Vec3* /*vertices*/)
    { osg::notify(osg::WARN) << "ReversePrimitiveFunctor : not implemented " << std::endl; }

    virtual void setVertexArray(unsigned int /*count*/,const osg::Vec4* /*vertices*/)
    { osg::notify(osg::WARN) << "ReversePrimitiveFunctor : not implemented " << std::endl; }

    virtual void setVertexArray(unsigned int /*count*/,const osg::Vec2d* /*vertices*/)
    { osg::notify(osg::WARN) << "ReversePrimitiveFunctor : not implemented " << std::endl; }

    virtual void setVertexArray(unsigned int /*count*/,const osg::Vec3d* /*vertices*/)
    { osg::notify(osg::WARN) << "ReversePrimitiveFunctor : not implemented " << std::endl; }

    virtual void setVertexArray(unsigned int /*count*/,const osg::Vec4d* /*vertices*/)
    { osg::notify(osg::WARN) << "ReversePrimitiveFunctor : not implemented " << std::endl; }

    virtual void drawArrays(GLenum /*mode*/,GLint /*first*/,GLsizei /*count*/)
    { osg::notify(osg::WARN) << "ReversePrimitiveFunctor : not implemented " << std::endl; }

    virtual void drawElements(GLenum mode,GLsizei count,const GLubyte* indices);

    virtual void drawElements(GLenum mode,GLsizei count,const GLushort* indices);

    virtual void drawElements(GLenum mode,GLsizei count,const GLuint* indices);
 

    /// Mimics the OpenGL \c glBegin() function.
    virtual void begin(GLenum /*mode*/)
    { osg::notify(osg::WARN) << "ReversePrimitiveFunctor : not implemented " << std::endl; }

    virtual void vertex(unsigned int /*pos*/)
    { osg::notify(osg::WARN) << "ReversePrimitiveFunctor : not implemented " << std::endl; }

    virtual void end()
    { osg::notify(osg::WARN) << "ReversePrimitiveFunctor : not implemented " << std::endl; }
    
    
    osg::ref_ptr<osg::PrimitiveSet> _reversedPrimitiveSet;
    
    
};





#endif /*REVERSEFACEVISITOR_H_*/
