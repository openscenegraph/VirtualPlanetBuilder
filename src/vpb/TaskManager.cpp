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

#include <vpb/TaskManager>
#include <vpb/Commandline>
#include <vpb/DatabaseBuilder>
#include <osgDB/ReadFile>

#include <iostream>

using namespace vpb;

TaskManager::TaskManager()
{
    _machinePool = new MachinePool;
}

TaskManager::~TaskManager()
{
}

int TaskManager::read(osg::ArgumentParser& arguments)
{
    std::string sourceName;
    while (arguments.read("-s",sourceName))
    {
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(sourceName);
        if (node.valid())
        {
            osgTerrain::Terrain* loaded_terrain = dynamic_cast<osgTerrain::Terrain*>(node.get());
            if (loaded_terrain) 
            {
                _terrain = loaded_terrain;
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
    
    if (!_terrain) _terrain = new osgTerrain::Terrain;

    std::string terrainOutputName;
    while (arguments.read("--so",terrainOutputName)) {}

    int result = vpb::readSourceArguments(std::cout, arguments, _terrain.get());
    if (result) return result;

    std::string machinePoolFileName;
    while (arguments.read("--machines",machinePoolFileName)) {}

    if (!machinePoolFileName.empty())
    {
        _machinePool->read(machinePoolFileName);
    }

    std::string taskSetFileName;
    while (arguments.read("--tasks",taskSetFileName)) {}

    if (!taskSetFileName.empty())
    {
        read(taskSetFileName);
    }

    return 0;
}

void TaskManager::setSource(osgTerrain::Terrain* terrain)
{
    _terrain = terrain;
}

osgTerrain::Terrain* TaskManager::getSource()
{
    return _terrain.get();
}

void TaskManager::setMachinePool(MachinePool* machinePool)
{
    _machinePool = machinePool;
}

MachinePool* TaskManager::getMachinePool()
{
    return _machinePool.get();
}

void TaskManager::nextTaskSet()
{
    // don't need to add a new task set if last task set is still empty.
    if (!_taskSetList.empty() && _taskSetList.back().empty()) return;
    
    _taskSetList.push_back(TaskSet());
}

void TaskManager::addTask(Task* task)
{
    if (!task) return;

    if (_taskSetList.empty()) _taskSetList.push_back(TaskSet());
    _taskSetList.back().push_back(task);
        
}

void TaskManager::addTask(const std::string& taskFileName)
{
    osg::ref_ptr<Task> taskFile = new Task(taskFileName,Task::READ);
    if (taskFile->valid()) addTask(taskFile.get());
}

void TaskManager::addTask(const std::string& taskFileName, const std::string& application, const std::string& arguments)
{
    osg::ref_ptr<Task> taskFile = new Task(taskFileName,Task::READ);

    if (taskFile->valid())
    {
        taskFile->setProperty("application",application);
        taskFile->setProperty("arguments",arguments);

        taskFile->write();    

        addTask(taskFile.get());
    }
}

void TaskManager::buildWithoutSlaves()
{

    if (_terrain.valid())
    {
        try 
        {
            osg::ref_ptr<vpb::DataSet> dataset = new vpb::DataSet;

            vpb::DatabaseBuilder* db = dynamic_cast<vpb::DatabaseBuilder*>(_terrain->getTerrainTechnique());
            vpb::BuildOptions* bo = db ? db->getBuildOptions() : 0;

            if (bo && !(bo->getLogFileName().empty()))
            {
                dataset->setBuildLog(new vpb::BuildLog);
            }

            if (_taskFile.valid())
            {
                dataset->setTask(_taskFile.get());
            }

            dataset->addTerrain(_terrain.get());

            int result = dataset->run();

            if (dataset->getBuildLog())
            {
                dataset->getBuildLog()->report(std::cout);
            }
            
        }
        catch(...)
        {
            printf("Caught exception.\n");
        }

    }
}


void TaskManager::run()
{
    std::cout<<"Begining run"<<std::endl;
    for(TaskSetList::iterator tsItr = _taskSetList.begin();
        tsItr != _taskSetList.end();
        ++tsItr)
    {
        for(TaskSet::iterator itr = tsItr->begin();
            itr != tsItr->end();
            ++itr)
        {
            Task* task = itr->get();
            Task::Status status = task->getStatus();
            switch(status)
            {
                case(Task::RUNNING):
                {
                    // do we check to see if this process is still running?
                    // do we kill this process?
                    std::cout<<"Task claims still to be running: "<<task->getFileName()<<std::endl;
                    break;
                }
                case(Task::COMPLETED):
                {
                    // task already completed so we can ignore it.
                    std::cout<<"Task claims to have been completed: "<<task->getFileName()<<std::endl;
                    break;
                }
                default:
                {
                    // do we check to see if this process is still running?
                    // do we kill this process?
                    getMachinePool()->run(task);
                    break;
                }
            }
            // now need to wait till all dispatched tasks are complete.
            getMachinePool()->waitForCompletion();
        }
    }
    std::cout<<"Finished run"<<std::endl;
}

bool TaskManager::read(const std::string& filename)
{
    std::cout<<"TaskManager::read() still in developement."<<std::endl;
    return false;
}

bool TaskManager::write(const std::string& filename)
{
    std::cout<<"TaskManager::write() still in developement."<<std::endl;
    return false;
}
