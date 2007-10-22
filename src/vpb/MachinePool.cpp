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

using namespace vpb;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MachineOperation
//
MachineOperation::MachineOperation(TaskFile* taskFile):
    osg::Operation(taskFile->getFileName(), false),
    _taskFile(taskFile)
{
}

void MachineOperation::operator () (osg::Object* object)
{
    Machine* machine = dynamic_cast<Machine*>(object);
    if (machine)
    {

        std::string application;
        if (_taskFile->getProperty("application",application))
        {
            _taskFile->setProperty("hostname",machine->getHostName());
            
            _taskFile->write();

            std::string executionString = machine->getCommandPrefix() + std::string(" ") + application;

            std::string arguments;
            if (_taskFile->getProperty("arguments",arguments))
            {
                executionString += std::string(" ") + arguments;
            }

            if (machine->getCommandPrefix().empty())
            {
                executionString += std::string(" ") + machine->getCommandPrefix();
            }

            system(executionString.c_str());
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Machine
//
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

void Machine::setOperationQueue(osg::OperationQueue* queue)
{
    for(Threads::iterator itr = _threads.begin();
        itr != _threads.end();
        ++itr)
    {
        (*itr)->setOperationQueue(queue);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MachinePool
//
