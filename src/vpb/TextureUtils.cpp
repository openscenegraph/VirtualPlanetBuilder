#include <vpb/TextureUtils>
#include <vpb/BuildLog>
#include <iostream>

#ifdef HAVE_NVTT
#include <nvtt/nvtt.h>
#include <string.h>

/// Error handler.
struct VPBErrorHandler : public nvtt::ErrorHandler
{

    // Signal error.
    virtual void error(nvtt::Error e)
    {
        switch (e)
        {
        case nvtt::Error_Unknown:
            vpb::log(osg::WARN," NVTT : unknown error");
            break;
        case nvtt::Error_InvalidInput:
            vpb::log(osg::WARN," NVTT : invalid input");
            break;
        case nvtt::Error_UnsupportedFeature:
            vpb::log(osg::WARN," NVTT : unsupported feature");
            break;
        case nvtt::Error_CudaError:
            vpb::log(osg::WARN," NVTT : cuda error");
            break;
          case nvtt::Error_FileOpen:
            vpb::log(osg::WARN," NVTT : file open error");
            break;
          case nvtt::Error_FileWrite:
            vpb::log(osg::WARN," NVTT : file write error");
            break;
        }
    }
};

/// Output handler.
struct OSGImageOutputHandler : public nvtt::OutputHandler
{
    typedef std::vector<unsigned char> MipMapData;

    std::vector<MipMapData*> _mipmaps;
    int _width;
    int _height;
    int _currentMipLevel;
    int _currentNumberOfWritenBytes;
    nvtt::Format _format;
    bool _discardAlpha;

    OSGImageOutputHandler(nvtt::Format format, bool discardAlpha)
        : _format(format), _discardAlpha(discardAlpha)
    {
    }
    virtual ~OSGImageOutputHandler()
    {
        for (unsigned int n=0; n<_mipmaps.size(); n++)
        {
            delete _mipmaps[n];
        }
        _mipmaps.clear();
    }

    // create the osg image from the given format
    osg::Image* createOSGImage()
    {
        // convert nvtt format to OpenGL pixel format
        GLint pixelFormat;
        switch (_format)
        {
        case nvtt::Format_RGBA:
            pixelFormat = _discardAlpha ? GL_RGB : GL_RGBA;
            break;
        case nvtt::Format_DXT1:
            pixelFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            break;
        case nvtt::Format_DXT1a:
            pixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;   
        case nvtt::Format_DXT3:
            pixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;   
        case nvtt::Format_DXT5:
            pixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
        default:
            vpb::log(osg::WARN," Invalid or not supported format");
            return 0;
        }

        osg::Image* image = new osg::Image();

        // Compute the total size and the mipmap offsets
        osg::Image::MipmapDataType mipmapOffsets(_mipmaps.size()-1);
        unsigned int totalSize = _mipmaps[0]->size();
        for (unsigned int n=1; n<_mipmaps.size(); n++)
        {
           mipmapOffsets[n-1] = totalSize;
           totalSize += _mipmaps[n]->size();
        }

        // Allocate data and copy it
        unsigned char* data = new unsigned char[ totalSize ];
        unsigned char* ptr = data;
        for (unsigned int n=0; n<_mipmaps.size(); n++)
        {
            memcpy( ptr, &(*_mipmaps[n])[0], _mipmaps[n]->size() );
            ptr += _mipmaps[n]->size();
        }

        image->setImage(_width,_height,1,pixelFormat,pixelFormat,GL_UNSIGNED_BYTE,data,osg::Image::USE_NEW_DELETE);
        image->setMipmapLevels(mipmapOffsets);
        return image;
    }
    
    /// Indicate the start of a new compressed image that's part of the final texture.
    virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel)
    {
        // store the new width/height of the texture
        if (miplevel == 0)
        {
            _width = width;
            _height = height;
        }
        // prepare to receive mipmap data
        if (miplevel >= static_cast<int>(_mipmaps.size()))
        {
            _mipmaps.resize(miplevel+1);
        }
        _mipmaps[miplevel] = new MipMapData(size);
        _currentMipLevel = miplevel;
        _currentNumberOfWritenBytes = 0;
    }
    
    /// Output data. Compressed data is output as soon as it's generated to minimize memory allocations.
    virtual bool writeData(const void * data, int size)
    {
        // Copy mipmap data
        std::vector<unsigned char>& dstData = *_mipmaps[_currentMipLevel];
        memcpy( &dstData[_currentNumberOfWritenBytes], data, size );
        _currentNumberOfWritenBytes += size;
        return true;
    }
};

// Convert RGBA to BGRA : nvtt only accepts BGRA pixel format
void convertRGBAToBGRA( std::vector<unsigned char>& outputData, const unsigned char* inputData )
{
    for (unsigned n=0; n<outputData.size(); n += 4)
    {
        outputData[n] = inputData[n+2];
        outputData[n+1] = inputData[n+1];
        outputData[n+2] = inputData[n];
        outputData[n+3] = inputData[n+3];
    }
}

// Convert RGB to BGRA : nvtt only accepts BGRA pixel format
void convertRGBToBGRA( std::vector<unsigned char>& outputData, const unsigned char* inputData )
{
    unsigned int numberOfPixels = outputData.size()/4;
    for (unsigned n=0; n<numberOfPixels; n++)
    {
        outputData[4*n] = inputData[3*n+2];
        outputData[4*n+1] = inputData[3*n+1];
        outputData[4*n+2] = inputData[3*n];
        outputData[4*n+3] = 255;
    }
}

// Main interface with NVTT
void nvttProcess( osg::Texture& texture, nvtt::Format format, bool generateMipMap, bool resizeToPowerOfTwo )
{
    const osg::Image& image = *texture.getImage(0);
    // Fill input options
    nvtt::InputOptions inputOptions;
    inputOptions.setTextureLayout(nvtt::TextureType_2D, image.s(), image.t() );
    inputOptions.setNormalMap(false);
    inputOptions.setConvertToNormalMap(false);
    inputOptions.setGamma(2.2f, 2.2f);
    inputOptions.setNormalizeMipmaps(false);
    inputOptions.setWrapMode(nvtt::WrapMode_Clamp);
    if (resizeToPowerOfTwo)
    {
        inputOptions.setRoundMode(nvtt::RoundMode_ToNearestPowerOfTwo);
    }
    inputOptions.setMipmapGeneration(generateMipMap);

    if (image.getPixelFormat() == GL_RGBA)
    {
        inputOptions.setAlphaMode( nvtt::AlphaMode_Transparency );
    }
    else
    {
        inputOptions.setAlphaMode( nvtt::AlphaMode_None );
    }
    std::vector<unsigned char> imageData( image.s() * image.t() * 4 );
    if (image.getPixelFormat() == GL_RGB)
    {
        convertRGBToBGRA( imageData, image.data() );
    }
    else
    {
        convertRGBAToBGRA( imageData, image.data() );
    }
    inputOptions.setMipmapData(&imageData[0],image.s(),image.t());

    // Fill compression options
    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setQuality( nvtt::Quality_Production );
    compressionOptions.setFormat( format );
    //compressionOptions.setQuantization(false,false,false);
    if (format == nvtt::Format_RGBA)
    {
        if (image.getPixelFormat() == GL_RGB)
        {
            compressionOptions.setPixelFormat(24,0xff,0xff00,0xff0000,0);
        }
        else
        {
            compressionOptions.setPixelFormat(32,0xff,0xff00,0xff0000,0xff000000);
        }    
    }

    // Handler
    OSGImageOutputHandler outputHandler(format,image.getPixelFormat() == GL_RGB);
    VPBErrorHandler errorHandler;

    // Fill output options
    nvtt::OutputOptions outputOptions;
    outputOptions.setOutputHandler(&outputHandler);
    outputOptions.setErrorHandler(&errorHandler);
    outputOptions.setOutputHeader(false);

    // Process the compression now
    nvtt::Compressor compressor;
    compressor.process(inputOptions,compressionOptions,outputOptions);

    texture.setImage( 0, outputHandler.createOSGImage() );
    texture.setInternalFormatMode(osg::Texture::USE_IMAGE_DATA_FORMAT);
    texture.setResizeNonPowerOfTwoHint(resizeToPowerOfTwo);
}
#endif

void vpb::compress(osg::State& state, osg::Texture& texture, osg::Texture::InternalFormatMode compressedFormat, bool generateMipMap, bool resizeToPowerOfTwo)
{
#ifdef HAVE_NVTT
    nvtt::Format format;
    switch (compressedFormat)
    {
    case osg::Texture::USE_S3TC_DXT1_COMPRESSION:
        if (texture.getImage(0)->getPixelFormat() == GL_RGBA)
            format = nvtt::Format_DXT1a;
        else
            format = nvtt::Format_DXT1;
        break;
    case osg::Texture::USE_S3TC_DXT3_COMPRESSION:
        format = nvtt::Format_DXT3;
        break;
    case osg::Texture::USE_S3TC_DXT5_COMPRESSION:
        format = nvtt::Format_DXT5;
        break;
    default:
        vpb::log(osg::WARN," Invalid or not supported compress format");
        return;
    }

    nvttProcess( texture, format, generateMipMap, resizeToPowerOfTwo );

#else
    texture.setInternalFormatMode(compressedFormat);

    // force the mip mapping off temporay if we intend the graphics hardware to do the mipmapping.
    osg::Texture::FilterMode filterMin = texture.getFilter(osg::Texture::MIN_FILTER);
    if (!generateMipMap)
    {
        log(osg::INFO,"   switching off MIP_MAPPING for compile");
        texture.setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);
    }

    // make sure the OSG doesn't rescale images if it doesn't need to.
    texture.setResizeNonPowerOfTwoHint(resizeToPowerOfTwo);

    // get OpenGL driver to create texture from image.
    texture.apply(state);

    texture.getImage(0)->readImageFromCurrentTexture(0,true);

    // restore the mip mapping mode.
    if (!generateMipMap)
    {
        texture.setFilter(osg::Texture::MIN_FILTER,filterMin);
    }
    texture.dirtyTextureObject();
    texture.setInternalFormatMode(osg::Texture::USE_IMAGE_DATA_FORMAT);
#endif

}

void vpb::generateMipMap(osg::State& state, osg::Texture& texture, bool resizeToPowerOfTwo)
{
#ifdef HAVE_NVTT
    nvttProcess( texture, nvtt::Format_RGBA, true, resizeToPowerOfTwo );
#else
    // make sure the OSG doesn't rescale images if it doesn't need to.
    texture.setResizeNonPowerOfTwoHint(resizeToPowerOfTwo);

    // get OpenGL driver to create texture from image.
    texture.apply(state);

    texture.getImage(0)->readImageFromCurrentTexture(0,true);

    texture.setInternalFormatMode(osg::Texture::USE_IMAGE_DATA_FORMAT);

    texture.dirtyTextureObject();
 #endif
}
