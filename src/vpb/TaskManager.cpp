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
#include <vpb/FileSystem>

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/Math>

#include <osgDB/Input>
#include <osgDB/Output>
#include <osgDB/FileUtils>

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
        readSource(sourceName);
    }
    
    if (!_terrain) _terrain = new osgTerrain::Terrain;

    std::string terrainOutputName;
    while (arguments.read("--so",terrainOutputName)) {}

    int result = vpb::readSourceArguments(std::cout, arguments, _terrain.get());
    if (result) return result;

    DatabaseBuilder* db = dynamic_cast<DatabaseBuilder*>(_terrain->getTerrainTechnique());
    BuildOptions* bo = db->getBuildOptions();
    if (bo)
    {
        if (bo->getDistributedBuildSplitLevel()==0)
        {
            unsigned int maxLevel = bo->getMaximumNumOfLevels();
            unsigned int halfLevel = maxLevel / 2;
            if (halfLevel>=2)
            {
                bo->setDistributedBuildSplitLevel(osg::minimum(halfLevel,4u));
            }
        }
    }

    if (!terrainOutputName.empty())
    {
        if (_terrain.valid())
        {
            osgDB::writeNodeFile(*_terrain, terrainOutputName);
        }
        else
        {
            osg::notify(osg::NOTICE)<<"Error: unable to create terrain output \""<<terrainOutputName<<"\""<<std::endl;
        }
    }

    std::string machinePoolFileName;
    while (arguments.read("--machines",machinePoolFileName)) {}

    if (machinePoolFileName.empty()) machinePoolFileName = vpb::getMachineFileName();

    if (!machinePoolFileName.empty())
    {
        _machinePool->read(machinePoolFileName);
        
#if 0        
        _machinePool->write("test.machines");
#endif
    }
    
    

    std::string taskSetFileName;
    while (arguments.read("--tasks",taskSetFileName)) {}

    if (!taskSetFileName.empty())
    {
        readTasks(taskSetFileName);
#if 1        
        writeTasks("test.tasks");
#endif
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

void TaskManager::addTask(const std::string& taskFileName, const std::string& application)
{
    osg::ref_ptr<Task> taskFile = new Task(taskFileName,Task::READ);

    if (taskFile->valid())
    {
        taskFile->setProperty("application",application);

        taskFile->write();    

        addTask(taskFile.get());
    }
}

std::string TaskManager::createUniqueTaskFileName(const std::string application)
{
    return "taskfile.task";
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

bool TaskManager::generateTasksFromSource()
{
    bool result = false;
    
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

            result = dataset->generateTasks(this);

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
    
    return result;
}

bool TaskManager::run()
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
                case(Task::FAILED):
                {
                    // run the task
                    std::cout<<"Task previously failed attempting re-run: "<<task->getFileName()<<std::endl;
                    getMachinePool()->run(task);
                    break;
                }
                case(Task::PENDING):
                {
                    // run the task
                    getMachinePool()->run(task);
                    break;
                }
            }

        }

        // now need to wait till all dispatched tasks are complete.
        getMachinePool()->waitForCompletion();

        // tally up the tasks to see how we've done on this TasksSet
        unsigned int tasksPending = 0;        
        unsigned int tasksRunning = 0;        
        unsigned int tasksCompleted = 0;        
        unsigned int tasksFailed = 0;        
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
                    ++tasksPending;
                    break;
                }
                case(Task::COMPLETED):
                {
                    ++tasksCompleted;
                    break;
                }
                case(Task::FAILED):
                {
                    ++tasksFailed;
                    break;
                }
                case(Task::PENDING):
                {
                    ++tasksPending;
                    break;
                }
            }
        }
        std::cout<<"End of TaskSet: tasksPending="<<tasksPending<<" taskCompleted="<<tasksCompleted<<" taskRunning="<<tasksRunning<<" tasksFailed="<<tasksFailed<<std::endl;
        
    
        if (tasksFailed != 0) break;
        
    }
    
    // tally up the tasks to see how we've done overall
    unsigned int tasksPending = 0;        
    unsigned int tasksRunning = 0;        
    unsigned int tasksCompleted = 0;        
    unsigned int tasksFailed = 0;        
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
                    ++tasksPending;
                    break;
                }
                case(Task::COMPLETED):
                {
                    ++tasksCompleted;
                    break;
                }
                case(Task::FAILED):
                {
                    ++tasksFailed;
                    break;
                }
                case(Task::PENDING):
                {
                    ++tasksPending;
                    break;
                }
            }
        }
    }
    std::cout<<"End of run: tasksPending="<<tasksPending<<" taskCompleted="<<tasksCompleted<<" taskRunning="<<tasksRunning<<" tasksFailed="<<tasksFailed<<std::endl;

    if (tasksFailed==0) std::cout<<"Finished run successfully"<<std::endl;
    else std::cout<<"Finished run, but failed on "<<tasksFailed<<" tasks"<<std::endl;

    return tasksFailed != 0;
}


bool TaskManager::writeSource(const std::string& filename)
{
    if (_terrain.valid())
    {
        _sourceFileName = filename;
    
        osgDB::writeNodeFile(*_terrain, _sourceFileName);
        return true;
    }
    else
    {
        return false;
    }
}

bool TaskManager::readSource(const std::string& filename)
{
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filename);
    if (node.valid())
    {
        osgTerrain::Terrain* loaded_terrain = dynamic_cast<osgTerrain::Terrain*>(node.get());
        if (loaded_terrain) 
        {
            _sourceFileName = filename;
            _terrain = loaded_terrain;
            return true;
        }
        else
        {
            osg::notify(osg::NOTICE)<<"Error: source file \""<<filename<<"\" not suitable terrain data."<<std::endl;
            return false;
        }
    }
    else
    {
        osg::notify(osg::NOTICE)<<"Error: unable to load source file \""<<filename<<"\""<<std::endl;
        return false;
    }
    
}

void TaskManager::clearTaskSetList()
{
    _taskSetList.clear();
}

Task* TaskManager::readTask(osgDB::Input& fr, bool& itrAdvanced)
{
    if (fr.matchSequence("exec {"))
    {
        int local_entry = fr[0].getNoNestedBrackets();

        fr += 2;

        std::string application;
 
        while (!fr.eof() && fr[0].getNoNestedBrackets()>local_entry)
        {
            if (fr[0].getStr())
            {
                if (application.empty())
                {
                    // first entry is the application
                    application = fr[0].getStr();
                }
                else
                {
                    // subsequent entries and appended to arguments
                    application += std::string(" ") + std::string(fr[0].getStr());
                }
            }
            ++fr;
        }

        if (!application.empty())
        {
            osg::ref_ptr<Task> task = new Task(createUniqueTaskFileName(application),Task::READ);

            if (task->valid())
            {
                task->setProperty("application",application);

                task->write();

                return task.release();
            }
        }

        ++fr;
        
        itrAdvanced = true;

    }
    
    return 0;
}

bool TaskManager::readTasks(const std::string& filename)
{
    std::string foundFile = osgDB::findDataFile(filename);
    if (foundFile.empty())
    {
        std::cout<<"Error: could not find task file '"<<filename<<"'"<<std::endl;
        return false;
    }

    _tasksFileName = filename;

    std::ifstream fin(foundFile.c_str());
    
    if (fin)
    {
        osgDB::Input fr;
        fr.attach(&fin);
        
        while(!fr.eof())
        {        
            bool itrAdvanced = false;
        
            std::string readFilename;
            if (fr.read("file",readFilename))
            {
                nextTaskSet();
                readTasks(readFilename);
                ++itrAdvanced;
            }

            Task* task = readTask(fr, itrAdvanced);
            if (task)
            {
                nextTaskSet();
                addTask(task);
            }
            
            if (fr.matchSequence("Tasks {"))
            {
                nextTaskSet();

                int local_entry = fr[0].getNoNestedBrackets();

                fr += 2;

                while (!fr.eof() && fr[0].getNoNestedBrackets()>local_entry)
                {
                    bool localAdvanced = false;

                    Task* task = readTask(fr, localAdvanced);
                    if (task)
                    {
                        addTask(task);
                    }

                    if (!localAdvanced) ++fr;
                }

                ++fr;

                itrAdvanced = true;

            }
            
            if (!itrAdvanced) ++fr;
        }        
    }
    
    return false;
}

bool TaskManager::writeTask(osgDB::Output& fout, const Task* task) const
{
    std::string application;
    std::string arguments;
    if (task->getProperty("application",application))
    {
        fout.indent()<<"exec { "<<application<<" }"<<std::endl;
    }
    return true;
}

bool TaskManager::writeTasks(const std::string& filename)
{
    _tasksFileName = filename;
    
    osgDB::Output fout(filename.c_str());

    for(TaskSetList::const_iterator tsItr = _taskSetList.begin();
        tsItr != _taskSetList.end();
        ++tsItr)
    {
        const TaskSet& taskSet = *tsItr;

        if (taskSet.size()==1)
        {
            writeTask(fout,taskSet.front().get());
        }
        else if (taskSet.size()>1)
        {        
            fout.indent()<<"Tasks {"<<std::endl;
            fout.moveIn();

            for(TaskSet::const_iterator itr = taskSet.begin();
                itr != taskSet.end();
                ++itr)
            {
                writeTask(fout,itr->get());
            }

            fout.moveOut();
            fout.indent()<<"}"<<std::endl;
        }
    }
    

    return true;
}

BuildOptions* TaskManager::getBuildOptions()
{
    vpb::DatabaseBuilder* db = dynamic_cast<vpb::DatabaseBuilder*>(_terrain->getTerrainTechnique());
    return db ? db->getBuildOptions() : 0;
}
