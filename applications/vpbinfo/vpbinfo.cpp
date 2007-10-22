/* -*-c++-*- VirtualPlanetBuilder - Copyright (C) 1998-2007 Robert Osfield 
 *
 * This application is open source and may be redistributed and/or modified   
 * freely and without restriction, both in commericial and non commericial applications,
 * as long as this copyright notice is maintained.
 * 
 * This application is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <vpb/BuildOperation>
#include <osg/CoordinateSystemNode>

#include <osgDB/ReadFile>
#include <osgTerrain/Terrain>

#include <iostream>


int main( int argc, char **argv )
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    if (arguments.read("--machine-details"))
    {
        osg::notify(osg::NOTICE)<<"processors : "<<OpenThreads::GetNumberOfProcessors()<<std::endl;
        return 0;
    }

    return 0;
}

