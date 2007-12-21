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

#include <vpb/ThreadPool>

using namespace vpb;

ThreadPool::ThreadPool(unsigned int numThreads)
{
    init(numThreads);
}

ThreadPool::ThreadPool(const ThreadPool& tp, const osg::CopyOp& copyop)
{
    init(tp._threads.size());
}

ThreadPool::~ThreadPool()
{
    stopThreads();
}


void ThreadPool::init(unsigned int numThreads)
{
    _numRunningOperations = 0;
    _done = false;

    _operationQueue = new osg::OperationQueue;
    _blockOp = new BlockOperation;    

    for(unsigned int i=0; i<numThreads; ++i)
    {
        osg::OperationThread* thread = new osg::OperationThread;
        thread->setOperationQueue(_operationQueue.get());
        thread->setParent(this);
        
        _threads.push_back(thread);
    }
}

void ThreadPool::startThreads()
{
    int numProcessors = OpenThreads::GetNumberOfProcessors();
    int processNum = 0;
    _done = false;
    for(Threads::iterator itr = _threads.begin();
        itr != _threads.end();
        ++itr, ++processNum)
    {
        if (!((*itr)->isRunning()))
        {            
            (*itr)->setProcessorAffinity(processNum % numProcessors);
            (*itr)->startThread();
        }
    }
}

void ThreadPool::stopThreads()
{
    _done = true;
    
    for(Threads::iterator itr = _threads.begin();
        itr != _threads.end();
        ++itr)
    {
        if ((*itr)->isRunning())
        {
            (*itr)->setDone(true);
        }
    }
}

void ThreadPool::run(BuildOperation* op)
{
    if (_done)
    {
        log(osg::NOTICE,"ThreadPool::run() Attempt to run BuilderOperation after ThreadPool has been suspended.");
        return;
    }
    
    _operationQueue->add(op);
}

unsigned int ThreadPool::getNumOperationsRunning() const
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    return _numRunningOperations;
}

void ThreadPool::waitForCompletion()
{
    _blockOp->reset();

    _operationQueue->add(_blockOp.get());

    // wait till block is complete i.e. the operation queue has been cleared up to the block    
    _blockOp->block();

    // there can still be operations running though so need to double check.
    while(getNumOperationsRunning()>0 && !done())
    {
        // log(osg::INFO, "MachinePool::waitForCompletion : Waiting for threads to complete = %d",getNumOperationsRunning());
        OpenThreads::Thread::YieldCurrentThread();
    }

}

void ThreadPool::runningOperation(BuildOperation* op)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    ++_numRunningOperations;
}

void ThreadPool::completedOperation(BuildOperation* op)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    --_numRunningOperations;
}
