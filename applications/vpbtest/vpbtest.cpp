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

    osg::ref_ptr<vpb::DataSet> dataset = new vpb::DataSet;

    std::string outputFilename;
    while(arguments.read("-o",outputFilename)) { dataset->setDestinationName(outputFilename); }

    std::string archiveName;
    while (arguments.read("-a",archiveName)) { dataset->setArchiveName(archiveName); }

    // input data.
    osg::ref_ptr<osgTerrain::Terrain> terrain;

    std::string filename;
    while(arguments.read("-s",filename))
    {
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filename);
        osg::ref_ptr<osgTerrain::Terrain> cast_terrain = dynamic_cast<osgTerrain::Terrain*>(node.get());
        if (cast_terrain.valid())
        {
            terrain = cast_terrain;
            std::cout<<"Read terrain : "<<filename<<std::endl;
        } 
        else
        {
            std::cout<<"Model is not a terrain data : "<<filename<<std::endl;
        }
    }
    
    if (terrain.valid() && !outputFilename.empty())
    {
    
        // create DataSet.
        
        dataset->addTerrain(terrain.get());
        
        dataset->loadSources();
        
        unsigned int numLevels = 5;

        dataset->createDestination((unsigned int)numLevels);

        dataset->writeDestination();        
    }

    return 0;
}

