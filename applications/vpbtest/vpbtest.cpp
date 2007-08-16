/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield 
 *
 * This application is open source and may be redistributed and/or modified   
 * freely and without restriction, both in commericial and non commericial applications,
 * as long as this copyright notice is maintained.
 * 
 * This application is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <vpb/DataSet>
#include <vpb/Version>

#include <osgDB/ReadFile>
#include <osgTerrain/Terrain>

#include <iostream>

int main( int argc, char **argv )
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    std::string outputFilename;
    while(arguments.read("-o",outputFilename)) {}

    std::string archiveName;
    while (arguments.read("-a",archiveName)) {}

    // input data.
    osg::ref_ptr<osgTerrain::Terrain> sourceGraph;

    std::string filename;
    while(arguments.read("-s",filename))
    {
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filename);
        osg::ref_ptr<osgTerrain::Terrain> terrain = dynamic_cast<osgTerrain::Terrain*>(node.get());
        if (terrain.valid())
        {
            sourceGraph = terrain;
            std::cout<<"Read terrain : "<<filename<<std::endl;
        } 
        else
        {
            std::cout<<"Model is not a terrain data : "<<filename<<std::endl;
        }
    }
    
    if (sourceGraph.valid())
    {
        if (sourceGraph->getElevationLayer())
        {
            osgTerrain::HeightFieldLayer* hfl = dynamic_cast<osgTerrain::HeightFieldLayer*>(sourceGraph->getElevationLayer());
            osgTerrain::ImageLayer* iml = dynamic_cast<osgTerrain::ImageLayer*>(sourceGraph->getElevationLayer());
            osgTerrain::CompositeLayer* cl = dynamic_cast<osgTerrain::CompositeLayer*>(sourceGraph->getElevationLayer());
            if (hfl) std::cout<<"Elevation HeightFieldLayer supplied"<<std::endl;
            if (iml) std::cout<<"Elevation ImageLayer supplied"<<std::endl;
            if (cl)
            {
                std::cout<<"ElevationLayer CompositLayer supplied"<<std::endl;
            }
            
        }
        for(unsigned int i=0; i<sourceGraph->getNumColorLayers();++i)
        {
            osgTerrain::Layer* layer = sourceGraph->getColorLayer(i);
            if (layer)
            {
                osgTerrain::HeightFieldLayer* hfl = dynamic_cast<osgTerrain::HeightFieldLayer*>(sourceGraph->getColorLayer(i));
                osgTerrain::ImageLayer* iml = dynamic_cast<osgTerrain::ImageLayer*>(sourceGraph->getColorLayer(i));
                osgTerrain::CompositeLayer* cl = dynamic_cast<osgTerrain::CompositeLayer*>(sourceGraph->getColorLayer(i));
                if (hfl) std::cout<<"ColorLayer "<<i<<" HeightFieldLayer supplied"<<std::endl;
                if (iml) std::cout<<"ColorLayer "<<i<<" ImageLayer supplied"<<std::endl;
                if (cl)
                {
                    std::cout<<"ColorLayer "<<i<<" CompositLayer supplied"<<std::endl;
                    for(unsigned int j=0; j<cl->getNumLayers(); ++j)
                    {
                        std::cout<<"   Layer "<<cl->getFileName(j)<<std::endl;
                    }
                }
            }
        }
    }

    return 0;
}

