/* OpenSceneGraph example, osgtexture3D.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

#include <osg/Node>
#include <osg/Geometry>
#include <osg/Notify>
#include <osg/Texture2D>
#include <osg/TexGen>
#include <osg/Geode>
#include <osg/ImageUtils>

#include <osgDB/ReadFile>

#include <osgText/Text>

#include <osgGA/TrackballManipulator>
#include <osgViewer/CompositeViewer>

#include <squish.h>

#include <iostream>

osg::Camera* createHUD(const std::string& label)
{
    // create a camera to set up the projection and model view matrices, and the subgraph to drawn in the HUD
    osg::Camera* camera = new osg::Camera;

    // set the projection matrix
    camera->setProjectionMatrix(osg::Matrix::ortho2D(0,1280,0,1024));

    // set the view matrix
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setViewMatrix(osg::Matrix::identity());

    // only clear the depth buffer
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);

    // draw subgraph after main camera view.
    camera->setRenderOrder(osg::Camera::POST_RENDER);

    // we don't want the camera to grab event focus from the viewers main camera(s).
    camera->setAllowEventFocus(false);

    // add to this camera a subgraph to render
    {

        osg::Geode* geode = new osg::Geode();

        std::string font("fonts/arial.ttf");

        // turn lighting off for the text and disable depth test to ensure its always ontop.
        osg::StateSet* stateset = geode->getOrCreateStateSet();
        stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);

        osg::Vec3 position(150.0f,150.0f,0.0f);

        osgText::Text* text = new  osgText::Text;
        geode->addDrawable( text );

        text->setFont(font);
        text->setPosition(position);
        text->setCharacterSize(100.0f);
        text->setText(label);

        camera->addChild(geode);
    }

    return camera;
}

osg::Node* creatQuad(const std::string& name,
                     osg::Image* image,
                     osg::Texture::InternalFormatMode formatMode,
                     osg::Texture::FilterMode minFilter)
{

    osg::Group* group = new osg::Group;

    {
        osg::Geode* geode = new osg::Geode;

        geode->addDrawable(createTexturedQuadGeometry(
                osg::Vec3(0.0f,0.0f,0.0f),
                osg::Vec3(float(image->s()),0.0f,0.0f),
                osg::Vec3(0.0f,0.0f,float(image->t()))));

        geode->setName(name);

        osg::StateSet* stateset = geode->getOrCreateStateSet();

        osg::Texture2D* texture = new osg::Texture2D(image);
        texture->setInternalFormatMode(formatMode);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, minFilter);
        stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

        group->addChild(geode);
    }

    {
        group->addChild(createHUD(name));
    }

    return group;
}

struct CopyPixelsOperator
{
    CopyPixelsOperator(squish::u8* data, int rowSize, int bits, bool errorDiffusion):
        _data(data),
        _rowSize(rowSize),
        _bits(bits),
        _errorDiffusion(errorDiffusion),
        _i(0)
    {
        _max = float((2 << (_bits-1))-1);
        _currentRow.resize(rowSize);
        _nextRow.resize(rowSize);
    }

    typedef std::vector<osg::Vec4> Errors;

    squish::u8 convert(float v, float max, int colour_index)
    {
        if (max<255.0)
        {
            //osg::notify(osg::NOTICE)<<"quantizing "<<max<<std::endl;

            if (_errorDiffusion)
            {
                float new_v = v - _currentRow[_i][colour_index];
                new_v = floorf(new_v*max+0.499999f) / max;
                if (new_v<0.0f) new_v=0.0f;
                if (new_v>1.0f) new_v=1.0f;

                float new_error = (new_v-v);
                v = new_v;

                if (_i+1<_rowSize)
                {
                    _currentRow[_i+1][colour_index] += new_error*0.33;
                    _nextRow[_i+1][colour_index] += new_error*0.16;
                }
                if (_i>0)
                {
                    _nextRow[_i-1][colour_index] += new_error*0.16;
                }
                _nextRow[_i][colour_index] += new_error*0.33;

            }
            else
            {
                v = floorf(v*max+0.499999f) / max;
            }
        }
        return squish::u8(floorf(v*255.0));
    }

    inline void colour(float r, float g, float b, float a)
    {
        *_data++ = convert(r, _max, 0);
        *_data++ = convert(g, _max, 1);
        *_data++ = convert(b, _max, 2);
        *_data++ = convert(a, _max, 3);

        ++_i;
        if (_i>=_rowSize)
        {
            std::swap(_currentRow, _nextRow);

            for(Errors::iterator itr = _nextRow.begin();
                itr != _nextRow.end();
                ++itr)
            {
                (*itr).set(0.0f,0.0f,0.0f,0.0f);
            }
            _i = 0;
        }
    }

    inline void luminance(float& l) { colour(l,l,l,1.0); }
    inline void alpha(float& a)  { colour(1.0,1.0,1.0,a);  }
    inline void luminance_alpha(float& l,float& a)  { colour(l,l,l,a);  }
    inline void rgb(float& r,float& g,float& b) { colour(r,g,b,1.0); }
    inline void rgba(float& r,float& g,float& b,float& a) { colour(r,g,b,a); }

    squish::u8*         _data;
    unsigned int        _rowSize;
    int                 _bits;
    bool                _errorDiffusion;
    float               _max;
    unsigned int        _i;
    Errors              _currentRow;
    Errors              _nextRow;
 };

osg::Image* squishImage(osg::Image* src_image, int flags, int bits, bool errorDiffusion)
{
    if (!src_image) return 0;

    int src_components = osg::Image::computeNumComponents(src_image->getPixelFormat());
    int width = src_image->s();
    int height = src_image->t();

    squish::u8* src_data = 0;
    if (src_components==4 && src_image->getDataType()==GL_UNSIGNED_BYTE)
    {
        src_data = src_image->data();

        if (flags==0) return src_image;
    }
    else
    {
        src_data = new squish::u8[4*width*height];
        CopyPixelsOperator copyop(src_data, width, bits, errorDiffusion);
        osg::readImage(src_image, copyop);

        osg::notify(osg::NOTICE)<<"Creating copy"<<std::endl;

        if (flags==0)
        {
            osg::ref_ptr<osg::Image> dst_image = new osg::Image;
            dst_image->setImage(width, height, 1,
                                GL_RGBA, GL_RGBA,
                                GL_UNSIGNED_BYTE,
                                (unsigned char*)src_data,
                                osg::Image::USE_NEW_DELETE);

            return dst_image.release();
        }
    }

    GLenum dst_pixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; // src_components==4 ?  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    if      (flags & squish::kDxt3) dst_pixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    else if (flags& squish::kDxt5) dst_pixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

    osg::notify(osg::NOTICE)<<"dst_pixelFormat = "<<dst_pixelFormat<<" flags = "<<flags<<std::endl;

    int dst_size = squish::GetStorageRequirements( width, height, flags );
    squish::u8* dest_data = new squish::u8[dst_size];


    osg::ref_ptr<osg::Image> dst_image = new osg::Image;
    dst_image->setImage(width, height, 1,
                        dst_pixelFormat, dst_pixelFormat,
                        GL_UNSIGNED_BYTE,
                        (unsigned char*)dest_data,
                        osg::Image::USE_NEW_DELETE);


    squish::CompressImage( src_data, width, height, dest_data, flags );

    if (src_data != src_image->data())
    {
        delete [] src_data;
    }

    return dst_image.release();
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc, argv);

    // construct the viewer.
    osgViewer::CompositeViewer viewer(arguments);
    
    osg::Texture::FilterMode minFilter = osg::Texture::LINEAR;
    while (arguments.read("--nearest")) { minFilter = osg::Texture::NEAREST; }
    while (arguments.read("--linear")) { minFilter = osg::Texture::LINEAR; }

    if (arguments.argc()<=1)
    {
        std::cout<<"Please supply an image filename on the commnand line."<<std::endl;
        return 1;
    }
    
    std::string filename = arguments[1];
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(filename);
    
    if (!image)
    {
        std::cout<<"Error: unable able to read image from "<<filename<<std::endl;
        return 1;
    }

    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (!wsi)
    {
        osg::notify(osg::NOTICE)<<"Error, no WindowSystemInterface available, cannot create windows."<<std::endl;
        return 1;
    }


    unsigned int width, height;
    wsi->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), width, height);

    // width = 1024;
    // height = 1024;

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0;
    traits->y = 0;
    traits->width = width;
    traits->height = height;
    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    if (!gc)
    {
        std::cout<<"Error: GraphicsWindow has not been created successfully."<<std::endl;
    }

    gc->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,1.0f));
    gc->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    osg::ref_ptr<osgGA::TrackballManipulator> trackball = new osgGA::TrackballManipulator;



    typedef std::vector< osg::ref_ptr<osg::Node> > Models;
    

    Models models;
    models.push_back(creatQuad("no compression", image.get(), osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));
#if 0
    models.push_back(creatQuad("ARB compression", image.get(), osg::Texture::USE_ARB_COMPRESSION, minFilter));
#endif

    models.push_back(creatQuad("DXT1 compression", image.get(), osg::Texture::USE_S3TC_DXT1_COMPRESSION, minFilter));
#if 0
    models.push_back(creatQuad("DXT3 compression", image.get(), osg::Texture::USE_S3TC_DXT3_COMPRESSION, minFilter));
    models.push_back(creatQuad("DXT5 compression", image.get(), osg::Texture::USE_S3TC_DXT5_COMPRESSION, minFilter));
#endif

    models.push_back(creatQuad("RGB, 8bit, no diff", squishImage(image.get(), 0, 8, false),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("RGB, 7bit, no diff", squishImage(image.get(), 0, 7, false),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("RBB, 6bit, no diff", squishImage(image.get(), 0, 6, false),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("RBB, 5bit, no diff", squishImage(image.get(), 0, 5, false),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("RGB, 8bit, diff", squishImage(image.get(), 0, 8, true),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("RGB, 7bit, diff", squishImage(image.get(), 0, 7, true),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("RGB, 6bit, diff", squishImage(image.get(), 0, 6, true),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("RGB, 5bit, diff", squishImage(image.get(), 0, 5, true),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));


    models.push_back(creatQuad("DXT1, 8bit, no diff", squishImage(image.get(), squish::kDxt1 | squish::kColourIterativeClusterFit, 8, false),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("DXT1, 7bit, no diff", squishImage(image.get(), squish::kDxt1 | squish::kColourIterativeClusterFit, 7, false),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("DXT1, 6bit, no diff", squishImage(image.get(), squish::kDxt1 | squish::kColourIterativeClusterFit, 6, false),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("DXT1, 5bit, no diff", squishImage(image.get(), squish::kDxt1 | squish::kColourIterativeClusterFit, 5, false),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));


    models.push_back(creatQuad("DXT1, 8bit, diff", squishImage(image.get(), squish::kDxt1 | squish::kColourIterativeClusterFit, 8, true),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("DXT1, 7bit, diff", squishImage(image.get(), squish::kDxt1 | squish::kColourIterativeClusterFit, 7, true),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("DXT1, 6bit, diff", squishImage(image.get(), squish::kDxt1 | squish::kColourIterativeClusterFit, 6, true),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("DXT1, 5bit, diff", squishImage(image.get(), squish::kDxt1 | squish::kColourIterativeClusterFit, 5, true),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));


#if 0
    models.push_back(creatQuad("DXT3 iterative cluster", squishImage(image.get(), squish::kDxt3 | squish::kColourIterativeClusterFit),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("DXT3 cluste fit", squishImage(image.get(), squish::kDxt3 | squish::kColourClusterFit),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("DXT3 range fit", squishImage(image.get(), squish::kDxt3 | squish::kColourRangeFit),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));


    models.push_back(creatQuad("DXT5 iterative cluster", squishImage(image.get(), squish::kDxt5 | squish::kColourIterativeClusterFit),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("DXT5 cluste fit", squishImage(image.get(), squish::kDxt5 | squish::kColourClusterFit),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("DXT5 range fit", squishImage(image.get(), squish::kDxt5 | squish::kColourRangeFit),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

    models.push_back(creatQuad("Convert to RGBA", squishImage(image.get(), 0),
                     osg::Texture::USE_IMAGE_DATA_FORMAT, minFilter));

#endif

    int numX = 1;
    int numY = 1;

    // compute the number of views up and across that are need
    {
        float aspectRatio = float(width)/float(height);
        float multiplier = sqrtf(float(models.size())/aspectRatio);;
        float multiplier_x = multiplier*aspectRatio;
        float multiplier_y = multiplier;


        if ((multiplier_x/ceilf(multiplier_x)) > (multiplier_y/ceilf(multiplier_y)))
        {
            numX = int(ceilf(multiplier_x));
            numY = int(ceilf(float(models.size())/float(numX)));
        }
        else
        {
            numY = int(ceilf(multiplier_y));
            numX = int(ceilf(float(models.size())/float(numY)));
        }
    }

    // populate the view with the required view to view each model.
    for(unsigned int i=0; i<models.size(); ++i)
    {
        osgViewer::View* view = new osgViewer::View;
        
        int xCell = i % numX;
        int yCell = i / numX;
    
        int vx = int((float(xCell)/float(numX)) * float(width));
        int vy = int((float(yCell)/float(numY)) * float(height));
        int vw =  int(float(width) / float(numX));
        int vh =  int(float(height) / float(numY));

        view->setSceneData(models[i].get());
        view->getCamera()->setProjectionMatrixAsPerspective(30.0, double(vw) / double(vh), 1.0, 1000.0);
        view->getCamera()->setViewport(new osg::Viewport(vx, vy, vw, vh));
        view->getCamera()->setGraphicsContext(gc.get());
        view->getCamera()->setClearMask(0);
        view->setCameraManipulator(trackball.get());

        viewer.addView(view);
    }

    return viewer.run();
}
