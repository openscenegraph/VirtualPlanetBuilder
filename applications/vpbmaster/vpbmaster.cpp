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
#include <vpb/TaskManager>

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <iostream>

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc,argv);

    // set up the usage document, in case we need to print out how to use this program.
    arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" application is utility tools which can be used to generate paged geospatial terrain databases.");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] filename ...");
    arguments.getApplicationUsage()->addCommandLineOption("-h or --help","Display this information");

    std::string runPath;
    if (arguments.read("--run-path",runPath))
    {
        chdir(runPath.c_str());
    }

    osg::ref_ptr<vpb::TaskManager> taskManager = new vpb::TaskManager;
    
    taskManager->read(arguments);

    bool buildWithoutSlaves = false;
    while (arguments.read("--build")) { buildWithoutSlaves=true; } 
    
    std::string tasksOutputFileName;
    while (arguments.read("--to",tasksOutputFileName));

    // any option left unread are converted into errors to write out later.
    arguments.reportRemainingOptionsAsUnrecognized();

    // report any errors if they have occured when parsing the program aguments.
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }
    
    if (!tasksOutputFileName.empty())
    {
        std::string sourceFileName = taskManager->getBuildName() + std::string("_master.source");
        taskManager->writeSource(tasksOutputFileName);

        taskManager->generateTasksFromSource();
        taskManager->writeTasks(tasksOutputFileName);
        return 1;
    }

    if (buildWithoutSlaves)
    {
        taskManager->buildWithoutSlaves();
    }
    else
    {
        if (!taskManager->hasTasks())
        {
            std::string sourceFileName = taskManager->getBuildName() + std::string("_master.source");
            tasksOutputFileName = taskManager->getBuildName() + std::string("_master.tasks");

            taskManager->writeSource(sourceFileName);

            taskManager->generateTasksFromSource();
            taskManager->writeTasks(tasksOutputFileName);
            
            std::cout<<"Generated tasks file = "<<tasksOutputFileName<<std::endl;
        }
    
        if (taskManager->hasMachines())
        {
            taskManager->run();
        }
        else
        {
            std::cout<<"Cannot run build without machines assigned, please pass in a machines definiation file via --machines <file>."<<std::endl;
        }
    }
    
    return 0;
}

