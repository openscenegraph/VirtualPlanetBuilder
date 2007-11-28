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


#include <vpb/Commandline>
#include <vpb/DataSet>
#include <vpb/DatabaseBuilder>
#include <vpb/System>
#include <vpb/Version>

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <iostream>

int main(int argc, char** argv)
{
    osg::Timer_t startTick = osg::Timer::instance()->tick();

    osg::ArgumentParser arguments(&argc,argv);

    // set up the usage document, in case we need to print out how to use this program.
    arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" application is utility tools which can be used to generate paged geospatial terrain databases.");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] filename ...");
    arguments.getApplicationUsage()->addCommandLineOption("-h or --help","Display this information");
    arguments.getApplicationUsage()->addCommandLineOption("--cache <filename>","Read the cache file to use a look up for locally cached files.");

    vpb::getSourceUsage(*arguments.getApplicationUsage());


    if (arguments.read("--version"))
    {
        std::cout<<"VirtualPlanetBuilder/osgdem version "<<vpbGetVersion()<<std::endl;
        return 0;
    }

    if (arguments.read("--version-number"))
    {
        std::cout<<vpbGetVersion()<<std::endl;
        return 0;
    }

    std::string runPath;
    if (arguments.read("--run-path",runPath))
    {
        chdir(runPath.c_str());
    }

    // if user request help write it out to cout.
    if (arguments.read("-h") || arguments.read("--help"))
    {
        arguments.getApplicationUsage()->write(std::cout,osg::ApplicationUsage::COMMAND_LINE_OPTION);
        return 1;
    }
    
    vpb::System::instance()->readArguments(arguments);

    std::string taskFileName;
    osg::ref_ptr<vpb::Task> taskFile;
    while (arguments.read("--task",taskFileName))
    {
        if (!taskFileName.empty())
        {
            taskFile = new vpb::Task(taskFileName);

            taskFile->read();

            taskFile->setStatus(vpb::Task::RUNNING);
            taskFile->setProperty("pid",vpb::getProcessID());
            taskFile->write();

        }
    }


    osg::ref_ptr<osgTerrain::Terrain> terrain = 0;


    //std::cout<<"PID="<<getpid()<<std::endl;

    std::string sourceName;
    while (arguments.read("-s",sourceName))
    {
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(sourceName);
        if (node.valid())
        {
            osgTerrain::Terrain* loaded_terrain = dynamic_cast<osgTerrain::Terrain*>(node.get());
            if (loaded_terrain) 
            {
                terrain = loaded_terrain;
            }
            else
            {
                osg::notify(osg::NOTICE)<<"Error: source file \""<<sourceName<<"\" not suitable terrain data."<<std::endl;
                return 1;
            }
        }
        else
        {
            osg::notify(osg::NOTICE)<<"Error: unable to load source file \""<<sourceName<<"\""<<std::endl;
            return 1;
        }
    }
    
    if (!terrain) terrain = new osgTerrain::Terrain;

    std::string terrainOutputName;
    while (arguments.read("--so",terrainOutputName)) {}

    bool report = false;
    while (arguments.read("--report")) { report = true; }

    int result = vpb::readSourceArguments(std::cout, arguments, terrain.get());
    if (result) return result;

    // any option left unread are converted into errors to write out later.
    arguments.reportRemainingOptionsAsUnrecognized();

    // report any errors if they have occured when parsing the program aguments.
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }
    
    if (!terrainOutputName.empty())
    {
        if (terrain.valid())
        {
            osgDB::writeNodeFile(*terrain, terrainOutputName);
        }
        else
        {
            osg::notify(osg::NOTICE)<<"Error: unable to create terrain output \""<<terrainOutputName<<"\""<<std::endl;
        }
        return 1;
    }


    double duration = 0.0;

    // generate the database
    if (terrain.valid())
    {
        try 
        {
            osg::ref_ptr<vpb::DataSet> dataset = new vpb::DataSet;

            vpb::DatabaseBuilder* db = dynamic_cast<vpb::DatabaseBuilder*>(terrain->getTerrainTechnique());
            vpb::BuildOptions* bo = db ? db->getBuildOptions() : 0;

            if (bo && !(bo->getLogFileName().empty()))
            {
                dataset->setBuildLog(new vpb::BuildLog);
            }

            if (taskFile.valid())
            {
                dataset->setTask(taskFile.get());
            }

            dataset->addTerrain(terrain.get());

            int result = dataset->run();

            if (dataset->getBuildLog() && report)
            {
                dataset->getBuildLog()->report(std::cout);
            }
            
            duration = osg::Timer::instance()->delta_s(startTick, osg::Timer::instance()->tick());
            
            dataset->log(osg::NOTICE,"Elapsed time = %f",duration);
            
        }
        catch(...)
        {
            printf("Caught exception.\n");
        }

    }

    if (duration==0) duration = osg::Timer::instance()->delta_s(startTick, osg::Timer::instance()->tick());

    if (taskFile.valid())
    {
        taskFile->setStatus(vpb::Task::COMPLETED);
        taskFile->setProperty("duration",duration);
        taskFile->write();
    }
    
    return 0;
}

