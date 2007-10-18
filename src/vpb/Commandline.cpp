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

#include <vpb/Commandline>
#include <vpb/Source>
#include <vpb/BuildOptions>
#include <vpb/DatabaseBuilder>

#include <osg/Notify>
#include <osg/io_utils>

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

using namespace vpb;

static osg::Matrixd newComputeGeoTransForRange(double xMin, double xMax, double yMin, double yMax)
{
    osg::Matrixd matrix;
    matrix(0,0) = xMax-xMin;
    matrix(3,0) = xMin;

    matrix(1,1) = yMax-yMin;
    matrix(3,1) = yMin;
    
    return matrix;
}


static void processFile(const std::string& filename,
                 vpb::Source::Type type,
                 vpb::SpatialProperties::DataType dataType,
                 const std::string& currentCS, 
                 osg::Matrixd& geoTransform,
                 bool geoTransformSet,
                 bool geoTransformScale,
                 bool minmaxLevelSet, unsigned int min_level, unsigned int max_level,
                 unsigned int layerNum,
                 osgTerrain::Terrain* terrain)
{

    if (filename.empty()) return;
    
    osg::notify(osg::INFO)<<"processFile "<<filename<<" cs="<<currentCS<<std::endl;

    if (osgDB::fileType(filename) == osgDB::REGULAR_FILE)
    {

        osg::ref_ptr<osg::Object> loadedObject = osgDB::readObjectFile(filename+".gdal");
        osgTerrain::Layer* loadedLayer = dynamic_cast<osgTerrain::Layer*>(loadedObject.get());

        if (loadedLayer)
        {
            osgTerrain::Locator* locator = loadedLayer->getLocator();

            if (!loadedLayer->getLocator())
            {
                locator = new osgTerrain::Locator;
                loadedLayer->setLocator(locator);
            }

            if (!currentCS.empty())
            {
                osg::notify(osg::INFO)<<"locator->setCoordateSystem "<<currentCS<<std::endl;
                locator->setFormat("WKT");
                locator->setCoordinateSystem(currentCS);
                locator->setDefinedInFile(false);
            } 

            if (geoTransformSet)
            {
                osg::notify(osg::INFO)<<"locator->setTransform "<<geoTransform<<std::endl;
                locator->setTransform(geoTransform);
                locator->setDefinedInFile(false);
            }

            if (type==vpb::Source::IMAGE)
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
            else if (type==vpb::Source::HEIGHT_FIELD)
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


    } else if (osgDB::fileType(filename) == osgDB::DIRECTORY) {

        osgDB::DirectoryContents dirContents= osgDB::getDirectoryContents(filename);
        
        // loop through directory contents and call processFile
        std::vector<std::string>::iterator i;
        std::string fullfilename;
        for(i = dirContents.begin(); i != dirContents.end(); ++i) {
            if((*i != ".") && (*i != "..")) {
                fullfilename = filename + '/' + *i;
                processFile(fullfilename, type, dataType, currentCS, 
                            geoTransform, geoTransformSet, geoTransformScale, 
                            minmaxLevelSet, min_level, max_level,
                            layerNum,
                            terrain);
            }
        }
    }
}

void vpb::getSourceUsage(osg::ApplicationUsage& usage)
{
    usage.addCommandLineOption("-d <filename>","Specify the digital elevation map input file to process");
    usage.addCommandLineOption("-t <filename>","Specify the texture map input file to process");
    usage.addCommandLineOption("-a <archivename>","Specify the archive to place the generated database");
    usage.addCommandLineOption("--ibn <buildname>","Specify the intermediate build file name");
    usage.addCommandLineOption("-o <outputfile>","Specify the output master file to generate");
    usage.addCommandLineOption("-l <numOfLevels>","Specify the number of PagedLOD levels to generate");
    usage.addCommandLineOption("--image-ext <ext>","Specify the Image format to output to via its plugin name, i.e. rgb, dds, jp2, jpeg.");
    usage.addCommandLineOption("--levels <begin_level> <end_level>","Specify the range of lavels that the next source Texture or DEM will contribute to.");
    usage.addCommandLineOption("--layer <layer_num>","Specify the layer that the next source Texture will contribute to..");
    usage.addCommandLineOption("-e <x> <y> <w> <h>","Extents of the model to generate");
    usage.addCommandLineOption("--cs <coordinates system string>","Set the coordinates system of source imagery, DEM or destination database. The string may be any of the usual GDAL/OGR forms, complete WKT, PROJ.4, EPS");     
    usage.addCommandLineOption("--wkt <WKT string>","Set the coordinates system of source imagery, DEM or destination database in WellKownText form.");     
    usage.addCommandLineOption("--wkt-file <WKT file>","Set the coordinates system of source imagery, DEM or destination database by as file containing WellKownText definition.");     
    usage.addCommandLineOption("--skirt-ratio <float>","Set the ratio of skirt height to tile size");     
    usage.addCommandLineOption("--HEIGHT_FIELD","Create a height field database");     
    usage.addCommandLineOption("--POLYGONAL","Create a height field database");     
    usage.addCommandLineOption("--LOD","Create a LOD'd database");     
    usage.addCommandLineOption("--PagedLOD","Create a PagedLOD'd database");     
    usage.addCommandLineOption("-v","Set the vertical multiplier");     
    usage.addCommandLineOption("--compressed","Use OpenGL compression on RGB destination imagery");     
    usage.addCommandLineOption("--RGBA-compressed","Use OpenGL compression on RGBA destination imagery");     
    usage.addCommandLineOption("--RGB-16","Use 16bit RGB destination imagery");     
    usage.addCommandLineOption("--RGBA-24","Use 24bit RGB destination imagery");     
    usage.addCommandLineOption("--RGB-16","Use 16bit RGBA destination imagery");     
    usage.addCommandLineOption("--RGBA","Use 32bit RGBA destination imagery");     
    usage.addCommandLineOption("--vector","Interpret input as a vector data set");
    usage.addCommandLineOption("--raster","Interpret input as a raster data set (default)");
    usage.addCommandLineOption("--max-visible-distance-of-top-level","Set the maximum visible distance that the top most tile can be viewed at");     
    usage.addCommandLineOption("--no-terrain-simplification","Switch off terrain simplification.");
    usage.addCommandLineOption("--default-color <r,g,b,a>","Sets the default color of the terrain.");
    usage.addCommandLineOption("--radius-to-max-visible-distance-ratio","Set the maximum visible distance ratio for all tiles apart from the top most tile. The maximum visuble distance is computed from the ratio * tile radius.");     
    usage.addCommandLineOption("--no-mip-mapping","Disable mip mapping of textures");     
    usage.addCommandLineOption("--mip-mapping-hardware","Use mip mapped textures, and generate the mipmaps in hardware when available.");     
    usage.addCommandLineOption("--mip-mapping-imagery","Use mip mapped textures, and generate the mipmaps in imagery.");     
    usage.addCommandLineOption("--max-anisotropy","Max anisotropy level to use when texturing, defaults to 1.0.");
    usage.addCommandLineOption("--bluemarble-east","Set the coordinates system for next texture or dem to represent the eastern hemisphere of the earth.");     
    usage.addCommandLineOption("--bluemarble-west","Set the coordinates system for next texture or dem to represent the western hemisphere of the earth.");     
    usage.addCommandLineOption("--whole-globe","Set the coordinates system for next texture or dem to represent the whole hemisphere of the earth.");
    usage.addCommandLineOption("--geocentric","");
    usage.addCommandLineOption("--range","");     
    usage.addCommandLineOption("--xx","");     
    usage.addCommandLineOption("--xt","");     
    usage.addCommandLineOption("--yy","");     
    usage.addCommandLineOption("--yt","");     
    usage.addCommandLineOption("--zz","");     
    usage.addCommandLineOption("--zt","");     
    usage.addCommandLineOption("--version","Print out version");     
    usage.addCommandLineOption("--version-number","Print out version number only.");     
    usage.addCommandLineOption("--tile-image-size","Set the tile maximum image size");
    usage.addCommandLineOption("--tile-terrain-size","Set the tile maximum terrain size");
    usage.addCommandLineOption("--comment","Added a comment/description string to the top most node in the dataset");     
    usage.addCommandLineOption("-O","string option to pass to write plugins, use \" \" for multiple options");    
}

int vpb::readSourceArguments(std::ostream& fout, osg::ArgumentParser& arguments, osgTerrain::Terrain* terrain)
{
    vpb::DatabaseBuilder* databaseBuilder = dynamic_cast<vpb::DatabaseBuilder*>(terrain->getTerrainTechnique());
    if (!databaseBuilder) 
    {
        databaseBuilder = new vpb::DatabaseBuilder;
        terrain->setTerrainTechnique(databaseBuilder);
    }
    
    vpb::BuildOptions* buildOptions = databaseBuilder->getBuildOptions();
    if (!buildOptions)
    {
        buildOptions = new vpb::BuildOptions;
        databaseBuilder->setBuildOptions(buildOptions);
    }

    std::string logFilename;
    while(arguments.read("--log",logFilename)) { buildOptions->setLogFileName(logFilename); }


    float x,y,w,h;
    while (arguments.read("-e",x,y,w,h))
    {
        buildOptions->setDestinationExtents(vpb::GeospatialExtents(x,y,x+w,y+h,false)); // FIXME - need to check whether we a geographic extents of not
    }

    std::string buildname;    
    while (arguments.read("--ibn",buildname))
    {
        buildOptions->setIntermediateBuildName(buildname);
    }

    while (arguments.read("--HEIGHT_FIELD"))
    {
        buildOptions->setGeometryType(vpb::BuildOptions::HEIGHT_FIELD);
    }

    while (arguments.read("--POLYGONAL"))
    {
        buildOptions->setGeometryType(vpb::BuildOptions::POLYGONAL);
    }

    while (arguments.read("--LOD"))
    {
        buildOptions->setDatabaseType(vpb::BuildOptions::LOD_DATABASE);
    }
    
    while (arguments.read("--PagedLOD"))
    {
        buildOptions->setDatabaseType(vpb::BuildOptions::PagedLOD_DATABASE);
    }

    while (arguments.read("--compressed")) { buildOptions->setTextureType(vpb::BuildOptions::COMPRESSED_TEXTURE); }
    while (arguments.read("--RGBA-compressed")) { buildOptions->setTextureType(vpb::BuildOptions::COMPRESSED_RGBA_TEXTURE); }
    while (arguments.read("--RGB_16") || arguments.read("--RGB-16") ) { buildOptions->setTextureType(vpb::BuildOptions::RGB_16); }
    while (arguments.read("--RGBA_16") || arguments.read("--RGBA-16") ) { buildOptions->setTextureType(vpb::BuildOptions::RGBA_16); }
    while (arguments.read("--RGB_24") || arguments.read("--RGB-24") ) { buildOptions->setTextureType(vpb::BuildOptions::RGB_24); }
    while (arguments.read("--RGBA") || arguments.read("--RGBA") ) { buildOptions->setTextureType(vpb::BuildOptions::RGBA); }

    while (arguments.read("--no_mip_mapping") || arguments.read("--no-mip-mapping")) { buildOptions->setMipMappingMode(vpb::BuildOptions::NO_MIP_MAPPING); }
    while (arguments.read("--mip_mapping_hardware") || arguments.read("--mip-mapping-hardware")) { buildOptions->setMipMappingMode(vpb::BuildOptions::MIP_MAPPING_HARDWARE); }
    while (arguments.read("--mip_mapping_imagery") || arguments.read("--mip-mapping-imagery")) { buildOptions->setMipMappingMode(vpb::BuildOptions::MIP_MAPPING_IMAGERY); }

    float maxAnisotropy;
    while (arguments.read("--max_anisotropy",maxAnisotropy) || arguments.read("--max-anisotropy",maxAnisotropy))
    {
        buildOptions->setMaxAnisotropy(maxAnisotropy);
    }

    unsigned int image_size;
    while (arguments.read("--tile-image-size",image_size)) { buildOptions->setMaximumTileImageSize(image_size); }

    unsigned int terrain_size;
    while (arguments.read("--tile-terrain-size",terrain_size)) { buildOptions->setMaximumTileTerrainSize(terrain_size); }

    std::string comment;
    while (arguments.read("--comment",comment)) { buildOptions->setCommentString(comment); }

    std::string archiveName;
    while (arguments.read("-a",archiveName)) { buildOptions->setArchiveName(archiveName); }

    buildOptions->setDestinationName("output.ive");


    unsigned int numLevels = 10;
    while (arguments.read("-l",numLevels)) { buildOptions->setMaximumNumOfLevels(numLevels); }

    float verticalScale;
    while (arguments.read("-v",verticalScale))
    {
        buildOptions->setVerticalScale(verticalScale);
    }

    float skirtRatio;
    while (arguments.read("--skirt-ratio",skirtRatio))
    {
        buildOptions->setSkirtRatio(skirtRatio);
    }

    float maxVisibleDistanceOfTopLevel;
    while (arguments.read("--max_visible_distance_of_top_level",maxVisibleDistanceOfTopLevel) ||
          arguments.read("--max-visible-distance-of-top-level",maxVisibleDistanceOfTopLevel) )
    {
        buildOptions->setMaximumVisibleDistanceOfTopLevel(maxVisibleDistanceOfTopLevel);
    }

    float radiusToMaxVisibleDistanceRatio;
    while (arguments.read("--radius_to_max_visible_distance_ratio",radiusToMaxVisibleDistanceRatio) ||
           arguments.read("--radius-to-max-visible-distance-ratio",radiusToMaxVisibleDistanceRatio))
    {
        buildOptions->setRadiusToMaxVisibleDistanceRatio(radiusToMaxVisibleDistanceRatio);
    }

    while (arguments.read("--no_terrain_simplification") ||
           arguments.read("--no-terrain-simplification"))
    {
        buildOptions->setSimplifyTerrain(false);
    }

    std::string str;
    while (arguments.read("--default-color",str) ||
           arguments.read("--default_color",str))
    {
        osg::Vec4 defaultColor;
        if( sscanf( str.c_str(), "%f,%f,%f,%f",
                &defaultColor[0], &defaultColor[1], &defaultColor[2], &defaultColor[3] ) != 4 )
        {
            fout<<"Color argument format incorrect."<<std::endl;
            return 1;
        }
        buildOptions->setDefaultColor(defaultColor);
    }

    std::string image_ext;
    while (arguments.read("--image-ext",image_ext))
    {
        std::string::size_type dot = image_ext.find_last_of('.');
        if (dot!=std::string::npos) image_ext.erase(0,dot+1);

        osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(image_ext);
        if (rw) 
        {
            image_ext.insert(0,".");
            buildOptions->setDestinationImageExtension(image_ext);
        }
        else
        {
            fout<<"Error: can not find plugin to write out image with extension '"<<image_ext<<"'"<<std::endl;
            return 1;
        }
    }

    if (arguments.read("-O",str))
    {
        osgDB::ReaderWriter::Options* options = new osgDB::ReaderWriter::Options;
        options->setOptionString(str);
        osgDB::Registry::instance()->setOptions(options);
    }

    unsigned int maximumPossibleLevel = 30;


    // read the input data

    std::string filename;
    std::string currentCS;
    osg::Matrixd geoTransform;
    bool geoTransformSet = false; 
    bool geoTransformScale = false; 
    double xMin, xMax, yMin, yMax;
    bool minmaxLevelSet = false;
    unsigned int min_level=0, max_level=maximumPossibleLevel;
    unsigned int currentLayerNum = 0;
    vpb::SpatialProperties::DataType dataType = vpb::SpatialProperties::RASTER;
         
    int pos = 1;
    while(pos<arguments.argc())
    {
        std::string def;

        if (arguments.read(pos, "--cs",def))
        {
            currentCS = !def.empty() ? vpb::coordinateSystemStringToWTK(def) : "";
            fout<<"--cs \""<<def<<"\" converted to "<<currentCS<<std::endl;
        }
        else if (arguments.read(pos, "--wkt",def))
        {
            currentCS = def;
            fout<<"--wkt "<<currentCS<<std::endl;
        }
        else if (arguments.read(pos, "--wkt-file",def))
        {
            std::ifstream in(def.c_str());
            if (in)
            {   
                currentCS = "";
                while (!in.eof())
                {
                    std::string line;
                    in >> line;
                    currentCS += line;
                }
                fout<<"--wkt-file "<<currentCS<<std::endl;
            }
        }
        else if (arguments.read(pos, "--geocentric"))
        {
            buildOptions->setConvertFromGeographicToGeocentric(true);
            fout<<"--geocentric "<<currentCS<<std::endl;
        }

        else if (arguments.read(pos, "--bluemarble-east"))
        {
            currentCS = vpb::coordinateSystemStringToWTK("WGS84");
            geoTransformSet = true;
            geoTransformScale = true;
            geoTransform = newComputeGeoTransForRange(0.0, 180.0, -90.0, 90.0);
            
            fout<<"--bluemarble-east"<<currentCS<<" matrix="<<geoTransform<<std::endl;
        }

        else if (arguments.read(pos, "--bluemarble-west"))
        {
            currentCS = vpb::coordinateSystemStringToWTK("WGS84");
            geoTransformSet = true;
            geoTransformScale = true;
            geoTransform = newComputeGeoTransForRange(-180.0, 0.0, -90.0, 90.0);
            
            fout<<"--bluemarble-west "<<currentCS<<" matrix="<<geoTransform<<std::endl;
        }

        else if (arguments.read(pos, "--whole-globe"))
        {
            currentCS = vpb::coordinateSystemStringToWTK("WGS84");
            geoTransformSet = true;
            geoTransformScale = true;
            geoTransform = newComputeGeoTransForRange(-180.0, 180.0, -90.0, 90.0);
            
            fout<<"--whole-globe "<<currentCS<<" matrix="<<geoTransform<<std::endl;
        }

        else if (arguments.read(pos, "--range", xMin, xMax, yMin, yMax))
        {
            geoTransformSet = true;
            geoTransformScale = true;
            geoTransform = newComputeGeoTransForRange( xMin, xMax, yMin, yMax);
            
            fout<<"--range, matrix="<<geoTransform<<std::endl;
        }

        else if (arguments.read(pos, "--identity"))
        {
            geoTransformSet = false;
            geoTransform.makeIdentity();            
        }

        // x vector
        else if (arguments.read(pos, "--xx",geoTransform(0,0)))
        {
           geoTransformSet = true;
           geoTransformScale = false;
           fout<<"--xx "<<geoTransform(0,0)<<std::endl;
        }
        else if (arguments.read(pos, "--xy",geoTransform(1,0)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--xy "<<geoTransform(1,0)<<std::endl;
        }
        else if (arguments.read(pos, "--xz",geoTransform(2,0)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--xz "<<geoTransform(2,0)<<std::endl;
        }
        else if (arguments.read(pos, "--xt",geoTransform(3,0)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--xo "<<geoTransform(3,0)<<std::endl;
        }

        // y vector
        else if (arguments.read(pos, "--yx",geoTransform(0,1)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--yx "<<geoTransform(0,1)<<std::endl;
        }
        else if (arguments.read(pos, "--yy",geoTransform(1,1)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--yy "<<geoTransform(1,1)<<std::endl;
        }
        else if (arguments.read(pos, "--yz",geoTransform(2,1)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--yz "<<geoTransform(2,1)<<std::endl;
        }
        else if (arguments.read(pos, "--yt",geoTransform(3,1)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--yt "<<geoTransform(3,1)<<std::endl;
        }

        // z vector
        else if (arguments.read(pos, "--zx",geoTransform(0,2)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--zx "<<geoTransform(0,2)<<std::endl;
        }
        else if (arguments.read(pos, "--zy",geoTransform(1,2)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--zy "<<geoTransform(1,2)<<std::endl;
        }
        else if (arguments.read(pos, "--zz",geoTransform(2,2)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--zz "<<geoTransform(2,2)<<std::endl;
        }
        else if (arguments.read(pos, "--zt",geoTransform(3,2)))
        {
            geoTransformSet = true;
            geoTransformScale = false;
            fout<<"--zt "<<geoTransform(3,2)<<std::endl;
        }

        else if (arguments.read(pos, "--levels", min_level, max_level))
        {
            minmaxLevelSet = true;
            fout<<"--levels, min_level="<<min_level<<"  max_level="<<max_level<<std::endl;
        }
        
        else if (arguments.read(pos, "--layer", currentLayerNum))
        {
            fout<<"--layer layeNumber="<<currentLayerNum<<std::endl;
        }

        else if (arguments.read(pos, "--vector"))
        {
            dataType = vpb::SpatialProperties::VECTOR;
            fout<<"--vector input data"<<std::endl;
        }

        else if (arguments.read(pos, "--raster"))
        {
            dataType = vpb::SpatialProperties::RASTER;
            fout<<"--raster input data"<<std::endl;
        }

        else if (arguments.read(pos, "-d",filename))
        {
            fout<<"-d "<<filename<<std::endl;

            processFile(filename, vpb::Source::HEIGHT_FIELD, dataType, currentCS, 
                        geoTransform, geoTransformSet, geoTransformScale,
                        minmaxLevelSet, min_level, max_level,
                        currentLayerNum,
                        terrain);

            minmaxLevelSet = false;
            min_level=0; max_level=maximumPossibleLevel;
            currentLayerNum = 0;
            
            currentCS = "";
            geoTransformSet = false;
            geoTransformScale = false;
            geoTransform.makeIdentity();
            dataType = vpb::SpatialProperties::RASTER;
        }
        else if (arguments.read(pos, "-t",filename))
        {
            fout<<"-t "<<filename<<std::endl;

            processFile(filename, vpb::Source::IMAGE, dataType, currentCS, 
                        geoTransform, geoTransformSet, geoTransformScale, 
                        minmaxLevelSet, min_level, max_level, 
                        currentLayerNum,
                        terrain);

            minmaxLevelSet = false;
            min_level=0; max_level=maximumPossibleLevel;
            currentLayerNum = 0;
            
            currentCS = "";
            geoTransformSet = false;
            geoTransformScale = false;
            geoTransform.makeIdentity();            
            dataType = vpb::SpatialProperties::RASTER;
        }
        else if (arguments.read(pos, "-o",filename)) 
        {
            fout<<"-o "<<filename<<std::endl;
            buildOptions->setDestinationName(filename);
            
            if (!currentCS.empty()) buildOptions->setDestinationCoordinateSystem(currentCS);

            minmaxLevelSet = false;
            min_level=0; max_level=maximumPossibleLevel;
            
            currentCS = "";
            geoTransformSet = false;
            geoTransformScale = false;
            geoTransform.makeIdentity();            

        }
        else
        {
            // if no argument read advance to next argument.
            ++pos;
        }
    }
    return 0;    
}
