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
#include <unistd.h>
#include <signal.h>

using namespace vpb;

TaskManager::TaskManager()
{
    _done = false;
    _machinePool = new MachinePool;
    _buildName = "build";
    
    char str[2048]; 
    _runPath = getcwd ( str, sizeof(str));
}

TaskManager::~TaskManager()
{
    log(osg::INFO,"TaskManager::~TaskManager()");
}

void TaskManager::setBuildLog(BuildLog* bl)
{
    Logger::setBuildLog(bl);
    
    if (_machinePool.valid())  _machinePool->setBuildLog(bl);
}


void TaskManager::setRunPath(const std::string& runPath)
{
    _runPath = runPath;
    chdir(_runPath.c_str());
    
    log(osg::NOTICE,"setRunPath = %s",_runPath.c_str());
}

int TaskManager::read(osg::ArgumentParser& arguments)
{
    std::string logFileName;
    while (arguments.read("--master-log",logFileName))
    {
        setBuildLog(new BuildLog(logFileName));
    }


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
    
        
    while (arguments.read("--build-name",_buildName)) {}
    

    DatabaseBuilder* db = dynamic_cast<DatabaseBuilder*>(_terrain->getTerrainTechnique());
    BuildOptions* bo = db->getBuildOptions();
    if (bo)
    {
        if (bo->getDistributedBuildSplitLevel()==0)
        {
            unsigned int maxLevel = bo->getMaximumNumOfLevels();
            unsigned int halfLevel = (maxLevel / 2);
            if (halfLevel>=1)
            {
                bo->setDistributedBuildSplitLevel(osg::minimum(halfLevel,4u));
            }
        }
    }
    
    log(osg::NOTICE,"setDistributedBuildSplitLevel=%d",bo->getDistributedBuildSplitLevel());

    if (!terrainOutputName.empty())
    {
        if (_terrain.valid())
        {
            osgDB::writeNodeFile(*_terrain, terrainOutputName);
        }
        else
        {
            log(osg::NOTICE,"Error: unable to create terrain output \"%s\"",terrainOutputName.c_str());
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
    osg::ref_ptr<Task> taskFile = new Task(taskFileName);
    if (taskFile->valid()) addTask(taskFile.get());
}

void TaskManager::addTask(const std::string& taskFileName, const std::string& application)
{
    osg::ref_ptr<Task> taskFile = new Task(taskFileName);

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
    log(osg::NOTICE,"Begining run");
    
    for(TaskSetList::iterator tsItr = _taskSetList.begin();
        tsItr != _taskSetList.end() && !done();
        ++tsItr)
    {
        for(TaskSet::iterator itr = tsItr->begin();
            itr != tsItr->end() && !done();
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
                    log(osg::NOTICE,"Task claims still to be running: %s",task->getFileName().c_str());
                    break;
                }
                case(Task::COMPLETED):
                {
                    // task already completed so we can ignore it.
                    log(osg::NOTICE,"Task claims to have been completed: %s",task->getFileName().c_str());
                    break;
                }
                case(Task::FAILED):
                {
                    // run the task
                    log(osg::NOTICE,"Task previously failed attempting re-run: %s",task->getFileName().c_str());
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
        log(osg::NOTICE,"End of TaskSet: tasksPending=%d taskCompleted=%d taskRunning=%d tasksFailed=%d",tasksPending,tasksCompleted,tasksRunning,tasksFailed);
        
    
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
    log(osg::NOTICE,"End of run: tasksPending=%d taskCompleted=%d taskRunning=%d tasksFailed=%d",tasksPending,tasksCompleted,tasksRunning,tasksFailed);

    if (tasksFailed==0)
    {
        if (tasksPending==0) log(osg::NOTICE,"Finished run successfully.");
        else log(osg::NOTICE,"Finished run, but did not complete %d tasks.",tasksPending);
    }
    else log(osg::NOTICE,"Finished run, but failed on %d  tasks.",tasksFailed);

    return tasksFailed==0 && tasksPending==0;
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
            log(osg::WARN,"Error: source file \"%s\" not suitable terrain data.",filename.c_str());
            return false;
        }
    }
    else
    {
        log(osg::WARN,"Error: unable to load source file \"%s\" not suitable terrain data.",filename.c_str());
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
            osg::ref_ptr<Task> task = new Task(createUniqueTaskFileName(application));

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
        log(osg::WARN,"Error: could not find task file '%s'",filename.c_str());
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

void TaskManager::setDone(bool done)
{
    _done = done;
    if (_machinePool.valid()) _machinePool->setDone(done);
}

void TaskManager::signal(int signal)
{
    log(osg::NOTICE,"TaskManager::signal(%d)",signal);
    if (_machinePool.valid()) _machinePool->signal(signal);
}

void TaskManager::exit(int sig)
{
    //setDone(true);
    if (_machinePool.valid())
    {
        _machinePool->setTaskFailureOperation(MachinePool::IGNORE);
    
        if (sig==SIGHUP)
        {
            log(osg::NOTICE,"SIGHUP - exit on next frame");
            _done = true;
            _machinePool->removeAllOperations();
        }
        else
        {
            printf("  A: signal %d\n",sig);
            fflush(stdout);

            //log(osg::NOTICE,"Hard exit signal=%s",sig);
            _done = true;
            printf("  B: signal %d\n",sig);
            fflush(stdout);

            _machinePool->removeAllOperations();

            printf("  C: signal %d\n",sig);
            fflush(stdout);

            _machinePool->signal(sig);

            printf("  D: signal %d\n",sig);
            fflush(stdout);
        }
    }
}

