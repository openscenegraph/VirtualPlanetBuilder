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

#include <vpb/MachinePool>
#include <vpb/Task>

#include <osg/GraphicsThread>
#include <osg/Timer>

#include <osgDB/Input>
#include <osgDB/Output>
#include <osgDB/FileUtils>

#include <unistd.h>

#include <iostream>

using namespace vpb;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MachineOperation
//
MachineOperation::MachineOperation(Task* task):
    osg::Operation(task->getFileName(), false),
    _task(task)
{
    _task->setStatus(Task::PENDING);
    _task->write();
}

void MachineOperation::operator () (osg::Object* object)
{
    Machine* machine = dynamic_cast<Machine*>(object);
    if (machine)
    {
        std::string application;
        if (_task->getProperty("application",application))
        {
            osg::Timer_t startTick = osg::Timer::instance()->tick();

            _task->setProperty("hostname",machine->getHostName());
            _task->setStatus(Task::RUNNING);
            _task->write();
            
            machine->startedTask(_task.get());

            int result = machine->exec(application);
            
            machine->endedTask(_task.get());

            // read any updates to the task written to file by the application.
            _task->read();
            
            double duration;
            if (!_task->getProperty("duration",duration))
            {
                duration = osg::Timer::instance()->delta_s(startTick, osg::Timer::instance()->tick());
            }

            if (result==0)
            {
                // success
                _task->setStatus(Task::COMPLETED);
                _task->write();
            }
            else
            {
                // failure
                _task->setStatus(Task::FAILED);
                _task->setProperty("error code",result);
                _task->write();
                
                // tell the machine about this task failure.
                machine->taskFailed(_task.get(), result);
            }
            
            std::cout<<machine->getHostName()<<" : completed in "<<duration<<" seconds : "<<application<<" result="<<result<<std::endl;
        }

    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BlockOperation
//
BlockOperation::BlockOperation():
    osg::Operation("Block", false)
{
}

void BlockOperation::release()
{
    Block::release();
}

void BlockOperation::operator () (osg::Object* object)
{
    Block::release();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Machine
//
Machine::Machine():
    _machinePool(0)
{
}

Machine::Machine(const Machine& m, const osg::CopyOp& copyop):
    osg::Object(m, copyop),
    _machinePool(m._machinePool),
    _hostname(m._hostname),
    _commandPrefix(m._commandPrefix),
    _commandPostfix(m._commandPostfix)
{
}

Machine::Machine(const std::string& hostname,const std::string& commandPrefix, const std::string& commandPostfix, int numThreads):
    _machinePool(0),
    _hostname(hostname),
    _commandPrefix(commandPrefix),
    _commandPostfix(commandPostfix)
{
    if (numThreads<0)
    {
        // autodetect
        numThreads = 1;
    }
    
    for(int i=0; i<numThreads; ++i)
    {
        osg::OperationThread* thread = new osg::OperationThread;
        thread->setParent(this);
        _threads.push_back(thread);
    }
}

Machine::~Machine()
{
    osg::notify(osg::INFO)<<"Machine::~Machine()"<<std::endl;
}

int Machine::exec(const std::string& application)
{
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));

    bool runningRemotely = getHostName()!=hostname;

    std::string executionString;

    if (!getCommandPrefix().empty())
    {
        executionString = getCommandPrefix() + std::string(" ") + application;
    }
    else if (runningRemotely)
    {
        executionString = std::string("ssh ") +
                          getHostName() +
                          std::string(" \"") +
                          application +
                          std::string("\"");
    }
    else
    {
        executionString = application;
    }

    if (!getCommandPostfix().empty())
    {
        executionString += std::string(" ") + getCommandPostfix();
    }

    std::cout<<getHostName()<<" : running "<<executionString<<std::endl;

    return system(executionString.c_str());
}

void Machine::startThreads()
{
    std::cout<<"Machine::startThreads() hostname="<<_hostname<<std::endl;
    for(Threads::iterator itr = _threads.begin();
        itr != _threads.end();
        ++itr)
    {
        std::cout<<"  Started thread"<<std::endl;
    
        (*itr)->startThread();
    }
}

void Machine::setOperationQueue(osg::OperationQueue* queue)
{
    for(Threads::iterator itr = _threads.begin();
        itr != _threads.end();
        ++itr)
    {
        (*itr)->setOperationQueue(queue);
    }
}

unsigned int Machine::getNumThreadsActive() const
{
    unsigned int numThreadsActive = 0;
    for(Threads::const_iterator itr = _threads.begin();
        itr != _threads.end();
        ++itr)
    {
        if ((*itr)->getCurrentOperation().valid())
        {        
            ++numThreadsActive;
        }
    }
    return numThreadsActive;
}

void Machine::startedTask(Task* task)
{
    _runningTasks.insert(task);
}

void Machine::endedTask(Task* task)
{
    _runningTasks.erase(task);
}

void Machine::taskFailed(Task* task, int result)
{
    osg::notify(osg::NOTICE)<<getHostName()<<"::taskFailed("<<result<<")"<<std::endl;
    if (_machinePool)
    {
        switch(_machinePool->getTaskFailureOperation())
        {
            case(MachinePool::IGNORE):
            {
                osg::notify(osg::NOTICE)<<"   IGNORE"<<std::endl;
                break;
            }
            case(MachinePool::BLACKLIST_MACHINE_AND_RESUBMIT_TASK):
            {
                osg::notify(osg::NOTICE)<<"Task "<<task->getFileName()<<" has failed, blacklisting machine "<<getHostName()<<" and resubmitting task"<<std::endl;
                setDone(true);
                setOperationQueue(0);
                _machinePool->run(task);
                break;
            }
            case(MachinePool::COMPLETE_RUNNING_TASKS_THEN_EXIT):
            {
                osg::notify(osg::NOTICE)<<"   COMPLETE_RUNNING_TASKS_THEN_EXIT"<<std::endl;
                break;
            }
            case(MachinePool::TERMINATE_RUNNING_TASKS_THEN_EXIT):
            {
                osg::notify(osg::NOTICE)<<"   TERMINATE_RUNNING_TASKS_THEN_EXIT"<<std::endl;
                break;
            }
        }
    }
}


void Machine::signal(int signal)
{
    osg::notify(osg::NOTICE)<<"Machine::signal("<<signal<<")"<<std::endl;
    RunningTasks tasks = _runningTasks;    
    for(RunningTasks::iterator itr = tasks.begin();
        itr != tasks.end();
        ++itr)
    {
        Task* task = *itr;
        task->read();
        std::string pid;
        if (task->getProperty("pid", pid))
        {
            std::stringstream signalcommand;
            signalcommand << "kill -" << signal<<" "<<pid;

            exec(signalcommand.str());
        }
    }
}

void Machine::setDone(bool done)
{
    for(Threads::const_iterator itr = _threads.begin();
        itr != _threads.end();
        ++itr)
    {
        (*itr)->setDone(done);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MachinePool
//

MachinePool::MachinePool():
    _taskFailureOperation(IGNORE)
{
    //_taskFailureOperation = IGNORE;
    _taskFailureOperation = BLACKLIST_MACHINE_AND_RESUBMIT_TASK;
    //_taskFailureOperation = COMPLETE_RUNNING_TASKS_THEN_EXIT;
    //_taskFailureOperation = TERMINATE_RUNNING_TASKS_THEN_EXIT;
            
    _operationQueue = new osg::OperationQueue;
    _blockOp = new BlockOperation;    
}

MachinePool::~MachinePool()
{
    osg::notify(osg::INFO)<<"MachinePool::~MachinePool()"<<std::endl;
}

void MachinePool::addMachine(const std::string& hostname,const std::string& commandPrefix, const std::string& commandPostfix, int numThreads)
{
    addMachine(new Machine(hostname, commandPrefix, commandPostfix, numThreads));
}

void MachinePool::addMachine(Machine* machine)
{
    machine->_machinePool = this;
    machine->setOperationQueue(_operationQueue.get());
    machine->startThreads();
    
    _machines.push_back(machine);
}

void MachinePool::run(Task* task)
{
    std::cout<<"Adding Task to MachinePool::OperationQueue "<<task->getFileName()<<std::endl;
    _operationQueue->add(new MachineOperation(task));
}

void MachinePool::waitForCompletion()
{
    // std::cout<<"MachinePool::waitForCompletion : Adding block to queue"<<std::endl;
    _blockOp->reset();
    
    // wait till the operaion queu has been flushed.
    //_operationQueue->add(_blockOp.get());
    
    std::cout<<"MachinePool::waitForCompletion : Waiting for block to complete"<<std::endl;
    //_blockOp->block();
    
    std::cout<<"MachinePool::waitForCompletion : Block completed"<<std::endl;

    // there can still be operations running though so need to double check.
    while(getNumThreadsActive()>0 && !done())
    {
        std::cout<<"MachinePool::waitForCompletion : Waiting for threads to complete = "<<getNumThreadsActive()<<std::endl;
        OpenThreads::Thread::microSleep(100000);
    }

    std::cout<<"MachinePool::waitForCompletion : finished"<<std::endl;
}

unsigned int MachinePool::getNumThreads() const
{
    unsigned int numThreads = 0;
    for(Machines::const_iterator itr = _machines.begin();
        itr != _machines.end();
        ++itr)
    {
        numThreads += (*itr)->getNumThreads();
    }
    return numThreads;
}

unsigned int MachinePool::getNumThreadsActive() const
{
    unsigned int numThreadsActive = 0;
    for(Machines::const_iterator itr = _machines.begin();
        itr != _machines.end();
        ++itr)
    {
        numThreadsActive += (*itr)->getNumThreadsActive();
    }
    return numThreadsActive;
}

void MachinePool::clear()
{
    _machines.clear();
}

bool MachinePool::read(const std::string& filename)
{
    std::string foundFile = osgDB::findDataFile(filename);
    if (foundFile.empty())
    {
        std::cout<<"Error: could not find machine specification file '"<<filename<<"'"<<std::endl;
        return false;
    }

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
                read(readFilename);
                ++itrAdvanced;
            }
        
            if (fr.matchSequence("Machine {"))
            {
                int local_entry = fr[0].getNoNestedBrackets();

                fr += 2;

                std::string hostname;
                std::string prefix;
                std::string postfix;
                int numThreads=-1;

                while (!fr.eof() && fr[0].getNoNestedBrackets()>local_entry)
                {
                    bool localAdvanced = false;

                    if (fr.read("hostname",hostname)) localAdvanced = true;
                    if (fr.read("prefix",prefix)) localAdvanced = true;
                    if (fr.read("postfix",postfix)) localAdvanced = true;
                    if (fr.read("threads",numThreads)) localAdvanced = true;
                    if (fr.read("processes",numThreads)) localAdvanced = true;

                    if (!localAdvanced) ++fr;
                }

                addMachine(hostname,prefix,postfix,numThreads);

                ++fr;

                itrAdvanced = true;
            }

            if (!itrAdvanced) ++fr;
        }        
    }
    
    return true;
}

bool MachinePool::write(const std::string& filename) const
{
    osgDB::Output fout(filename.c_str());
    
    for(Machines::const_iterator itr = _machines.begin();
        itr != _machines.end();
        ++itr)
    {
        const Machine* machine = itr->get();
    
        if (itr != _machines.begin()) fout.indent()<<std::endl;

        fout.indent()<<"Machine {"<<std::endl;
        fout.moveIn();
        
        if (!machine->getHostName().empty()) fout.indent()<<"hostname "<<machine->getHostName()<<std::endl;
        if (!machine->getCommandPrefix().empty()) fout.indent()<<"prefix "<<machine->getCommandPrefix()<<std::endl;
        if (!machine->getCommandPostfix().empty()) fout.indent()<<"postfix "<<machine->getCommandPostfix()<<std::endl;
        if (machine->getNumThreads()>0) fout.indent()<<"processes "<<machine->getNumThreads()<<std::endl;
        
        fout.moveOut();
        fout.indent()<<"}"<<std::endl;
    }
    
    return true;
}

void MachinePool::removeAllOperations()
{
    _operationQueue->removeAllOperations();
}

void MachinePool::signal(int signal)
{
    osg::notify(osg::NOTICE)<<"MachinePool::signal("<<signal<<")"<<std::endl;
    for(Machines::iterator itr = _machines.begin();
        itr != _machines.end();
        ++itr)
    {
        (*itr)->signal(signal);
    }
}

void MachinePool::setDone(bool done)
{
    _done = done;

    if (_done) removeAllOperations();

    for(Machines::iterator itr = _machines.begin();
        itr != _machines.end();
        ++itr)
    {
        (*itr)->setDone(done);
    }
}

void MachinePool::release()
{
    if (_blockOp.valid()) _blockOp->release();
}
