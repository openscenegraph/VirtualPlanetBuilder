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

#include <osgDB/Input>
#include <osgDB/Output>
#include <osgDB/FileUtils>

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

        std::cout<<"MachineOperation::operator() hostname="<<machine->getHostName()<<std::endl;

        std::string application;
        if (_task->getProperty("application",application))
        {
            _task->setProperty("hostname",machine->getHostName());
            _task->setStatus(Task::RUNNING);
            _task->write();

            std::string executionString = machine->getCommandPrefix() + std::string(" ") + application;

            std::string arguments;
            if (_task->getProperty("arguments",arguments))
            {
                executionString += std::string(" ") + arguments;
            }

            if (machine->getCommandPrefix().empty())
            {
                executionString += std::string(" ") + machine->getCommandPrefix();
            }
            
            std::cout<<"running "<<executionString<<std::endl;

            system(executionString.c_str());
            
            _task->setStatus(Task::COMPLETED);
            _task->write();

            std::cout<<"completed "<<executionString<<std::endl;
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
Machine::Machine()
{
}

Machine::Machine(const Machine& m, const osg::CopyOp& copyop):
    osg::Object(m, copyop),
    _hostname(m._hostname),
    _commandPrefix(m._commandPrefix),
    _commandPostfix(m._commandPostfix)
{
}

Machine::Machine(const std::string& hostname,const std::string& commandPrefix, const std::string& commandPostfix, int numThreads):
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MachinePool
//

MachinePool::MachinePool()
{
    _operationQueue = new osg::OperationQueue;
}

MachinePool::~MachinePool()
{
}

void MachinePool::addMachine(const std::string& hostname,const std::string& commandPrefix, const std::string& commandPostfix, int numThreads)
{
    addMachine(new Machine(hostname, commandPrefix, commandPostfix, numThreads));
}

void MachinePool::addMachine(Machine* machine)
{
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
    osg::ref_ptr<BlockOperation> blockOp = new BlockOperation;
    
    // std::cout<<"MachinePool::waitForCompletion : Adding block to queue"<<std::endl;
    
    // wait till the operaion queu has been flushed.
    _operationQueue->add(blockOp.get());
    
    std::cout<<"MachinePool::waitForCompletion : Waiting for block to complete"<<std::endl;
    blockOp->block();
    
    // std::cout<<"MachinePool::waitForCompletion : Block completed"<<std::endl;

    // there can still be operations running though so need to double check.
    while(getNumThreadsActive()>0)
    {
        // std::cout<<"MachinePool::waitForCompletion : Waiting for threads to complete"<<std::endl;
        OpenThreads::Thread::YieldCurrentThread();
    }

    // std::cout<<"MachinePool::waitForCompletion : finished"<<std::endl;
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
        _machines.clear();
    
        osgDB::Input fr;
        fr.attach(&fin);
        
        while(!fr.eof())
        {        
            bool itrAdvanced = false;
        
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
