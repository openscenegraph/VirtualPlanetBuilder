#include <vpb/TextureUtils>
#include <vpb/BuildLog>
#include <iostream>

#ifdef HAVE_NVTT
#include <nvtt/nvtt.h>
#include <string.h>


class VPB_EXPORT NVTTProcessor : public osg::ImageProcessor
{
public:
    virtual void compress(osg::Image& image, osg::Texture::InternalFormatMode compressedFormat, bool generateMipMap, bool resizeToPowerOfTwo, CompressionMethod method, CompressionQuality quality);
    virtual void generateMipMap(osg::Image& image, bool resizeToPowerOfTwo, CompressionMethod method);

protected:

    void process( osg::Image& texture, nvtt::Format format, bool generateMipMap, bool resizeToPowerOfTwo, CompressionMethod method, CompressionQuality quality);

    struct VPBErrorHandler : public nvtt::ErrorHandler
    {
        virtual void error(nvtt::Error e);
    };

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

        OSGImageOutputHandler(nvtt::Format format, bool discardAlpha);
        virtual ~OSGImageOutputHandler();

        // create the osg image from the given format
        bool assignImage(osg::Image& image);

        /// Indicate the start of a new compressed image that's part of the final texture.
        virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel);

        /// Output data. Compressed data is output as soon as it's generated to minimize memory allocations.
        virtual bool writeData(const void * data, int size);
    };

    // Convert RGBA to BGRA : nvtt only accepts BGRA pixel format
    void convertRGBAToBGRA( std::vector<unsigned char>& outputData, const unsigned char* inputData );

    // Convert RGB to BGRA : nvtt only accepts BGRA pixel format
    void convertRGBToBGRA( std::vector<unsigned char>& outputData, const unsigned char* inputData );

};

/// Error handler.
void NVTTProcessor::VPBErrorHandler::error(nvtt::Error e)
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

/// Output handler.
NVTTProcessor::OSGImageOutputHandler::OSGImageOutputHandler(nvtt::Format format, bool discardAlpha)
    : _format(format), _discardAlpha(discardAlpha)
{
}

NVTTProcessor::OSGImageOutputHandler::~OSGImageOutputHandler()
{
    for (unsigned int n=0; n<_mipmaps.size(); n++)
    {
        delete _mipmaps[n];
    }
    _mipmaps.clear();
}

// create the osg image from the given format
bool NVTTProcessor::OSGImageOutputHandler::assignImage(osg::Image& image)
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
        return false;
    }

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

    image.setImage(_width,_height,1,pixelFormat,pixelFormat,GL_UNSIGNED_BYTE,data,osg::Image::USE_NEW_DELETE);
    image.setMipmapLevels(mipmapOffsets);

    return true;
}

/// Indicate the start of a new compressed image that's part of the final texture.
void NVTTProcessor::OSGImageOutputHandler::beginImage(int size, int width, int height, int depth, int face, int miplevel)
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
bool NVTTProcessor::OSGImageOutputHandler::writeData(const void * data, int size)
{
    // Copy mipmap data
    std::vector<unsigned char>& dstData = *_mipmaps[_currentMipLevel];
    memcpy( &dstData[_currentNumberOfWritenBytes], data, size );
    _currentNumberOfWritenBytes += size;
    return true;
}

// Convert RGBA to BGRA : nvtt only accepts BGRA pixel format
void NVTTProcessor::convertRGBAToBGRA( std::vector<unsigned char>& outputData, const unsigned char* inputData )
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
void NVTTProcessor::convertRGBToBGRA( std::vector<unsigned char>& outputData, const unsigned char* inputData )
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
void NVTTProcessor::process( osg::Image& image, nvtt::Format format, bool generateMipMap, bool resizeToPowerOfTwo, CompressionMethod method, CompressionQuality quality)
{
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
    switch(quality) 
    {
      case vpb::BuildOptions::FASTEST:
        compressionOptions.setQuality( nvtt::Quality_Fastest );
        break;
      case vpb::BuildOptions::NORMAL:
        compressionOptions.setQuality( nvtt::Quality_Normal );
        break;
      case vpb::BuildOptions::PRODUCTION:
        compressionOptions.setQuality( nvtt::Quality_Production);
        break;
      case vpb::BuildOptions::HIGHEST:
        compressionOptions.setQuality( nvtt::Quality_Highest);
        break;
    }    
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
    if(method == USE_GPU)
    {
      compressor.enableCudaAcceleration(true);
      if(!compressor.isCudaAccelerationEnabled()) {
        vpb::log(osg::WARN, "CUDA acceleration was enabled but it is not available. CPU will be used.");
      }
    } else {
      compressor.enableCudaAcceleration(false);
    }

    compressor.process(inputOptions,compressionOptions,outputOptions);

    outputHandler.assignImage(image);
}

void NVTTProcessor::compress(osg::Image& image, osg::Texture::InternalFormatMode compressedFormat, bool generateMipMap, bool resizeToPowerOfTwo, CompressionMethod method, CompressionQuality quality)
{
    nvtt::Format format;
    switch (compressedFormat)
    {
    case osg::Texture::USE_S3TC_DXT1_COMPRESSION:
        if (image.getPixelFormat() == GL_RGBA)
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

    process( image, format, generateMipMap, resizeToPowerOfTwo, method, quality );
}

void NVTTProcessor::generateMipMap(osg::Image& image, bool resizeToPowerOfTwo, CompressionMethod method)
{
    process( image, nvtt::Format_RGBA, true, resizeToPowerOfTwo, method, NORMAL);
}
#endif


void vpb::compress(osg::State& state, osg::Texture& texture, osg::Texture::InternalFormatMode compressedFormat, bool generateMipMap, bool resizeToPowerOfTwo, vpb::BuildOptions::CompressionMethod method, vpb::BuildOptions::CompressionQuality quality)
{
#ifdef HAVE_NVTT

  if(method != vpb::BuildOptions::GL_DRIVER) {

    NVTTProcessor::CompressionMethod cm = (method==vpb::BuildOptions::NVTT) ?  NVTTProcessor::USE_GPU : NVTTProcessor::USE_CPU;
    NVTTProcessor::CompressionQuality cq = NVTTProcessor::NORMAL;
    switch(quality)
    {
        case(vpb::BuildOptions::FASTEST): cq = NVTTProcessor::FASTEST;
        case(vpb::BuildOptions::NORMAL): cq = NVTTProcessor::NORMAL;
        case(vpb::BuildOptions::PRODUCTION): cq = NVTTProcessor::PRODUCTION;
        case(vpb::BuildOptions::HIGHEST): cq = NVTTProcessor::HIGHEST;
    }

    NVTTProcessor processor;
    processor.compress(*texture.getImage(0), compressedFormat, generateMipMap, resizeToPowerOfTwo, cm, cq);

    texture.setInternalFormatMode(osg::Texture::USE_IMAGE_DATA_FORMAT);
    texture.setResizeNonPowerOfTwoHint(resizeToPowerOfTwo);

  } else {

#endif

    if(method != vpb::BuildOptions::GL_DRIVER)
    {
      log(osg::WARN,"NVTT selected for texture processing but it is not available.");
    }

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

#ifdef HAVE_NVTT
  }
#endif

}

void vpb::generateMipMap(osg::State& state, osg::Texture& texture, bool resizeToPowerOfTwo, vpb::BuildOptions::CompressionMethod method)
{
#ifdef HAVE_NVTT

  if(method != vpb::BuildOptions::GL_DRIVER) {

    NVTTProcessor::CompressionMethod cm = (method==vpb::BuildOptions::NVTT) ?  NVTTProcessor::USE_GPU : NVTTProcessor::USE_CPU;

    NVTTProcessor processor;
    processor.generateMipMap(*texture.getImage(0), resizeToPowerOfTwo, cm);

    texture.setInternalFormatMode(osg::Texture::USE_IMAGE_DATA_FORMAT);
    texture.setResizeNonPowerOfTwoHint(resizeToPowerOfTwo);

} else {

#endif

    if(method != vpb::BuildOptions::GL_DRIVER)
    {
      log(osg::WARN,"NVTT selected for texture processing but it is not available.");
    }

    // make sure the OSG doesn't rescale images if it doesn't need to.
    texture.setResizeNonPowerOfTwoHint(resizeToPowerOfTwo);

    // get OpenGL driver to create texture from image.
    texture.apply(state);

    texture.getImage(0)->readImageFromCurrentTexture(0,true);

    texture.setInternalFormatMode(osg::Texture::USE_IMAGE_DATA_FORMAT);

    texture.dirtyTextureObject();

#ifdef HAVE_NVTT
  }
 #endif

}

