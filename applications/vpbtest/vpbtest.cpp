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
#include <vpb/Task>

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgTerrain/Terrain>

#include <iostream>

#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

#include <ext/stdio_filebuf.h>

struct LoadOperation : public vpb::BuildOperation
{
    LoadOperation(vpb::BuildLog* buildLog, const std::string& filename):
        vpb::BuildOperation(buildLog,"LoadOperation",false),
        _filename(filename) {}
    
    virtual void build()
    {
        std::cout<<"PID="<<getpid()<<std::endl;
        // log("loading %s",_filename.c_str());
        osg::ref_ptr<osg::Object> object = osgDB::readObjectFile(_filename+".gdal");
        if (object.valid())
        {
            // log("succeded in loading %s",_filename.c_str());
        }
        else
        {
            // log("failed loading %s",_filename.c_str());
        }
    }
    
    std::string _filename;
};

int main( int argc, char **argv )
{

    std::cout<<"Result = "<<system("vpbinfo --number-cores")<<std::endl;

    osg::ArgumentParser arguments(&argc,argv);
    
    int signal=9;
    std::string filename;
    if (arguments.read("-k",filename, signal) || arguments.read("-k",filename))
    {
        osg::ref_ptr<vpb::Task> taskFile = new vpb::Task(filename);
        taskFile->read();
        taskFile->signal(signal);
    }

    bool background = false;
    if (arguments.read("-i",filename, background) || arguments.read("-i",filename))
    {
        osg::ref_ptr<vpb::Task> taskFile = new vpb::Task(filename);
        taskFile->read();
        taskFile->invoke(background);
    }

    if (arguments.read("-r",filename))
    {
        osg::ref_ptr<vpb::Task> taskFile = new vpb::Task(filename);

        osg::ref_ptr<osg::OperationThread> thread = new osg::OperationThread;
        thread->add(new vpb::TaskOperation(taskFile.get()));
        thread->add(new vpb::SleepOperation(1000000));
        thread->startThread();
        
        sleep(10);
    }

    if (arguments.read("-w",filename))
    {

        osg::ref_ptr<vpb::Task> taskFile = new vpb::Task(filename);

        taskFile->init(arguments);

        osg::ref_ptr<osg::OperationThread> thread = new osg::OperationThread;
        thread->add(new vpb::TaskOperation(taskFile.get()));
        thread->add(new vpb::SleepOperation(1000000));
        thread->startThread();
        
        unsigned int count = 0;
        for(;;)
        {
            taskFile->setProperty("count",count);

            usleep(100000);

            ++count;
        }        

    }

    return 0;
}

