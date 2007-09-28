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

#include <osgDB/ReadFile>
#include <osgTerrain/Terrain>

#include <iostream>

struct OperationOne : public vpb::BuildOperation
{
    OperationOne(vpb::BuildLog* buildLog, unsigned int i):
        vpb::BuildOperation(buildLog,"Operation One",false),
        _i(i) {}
    
    virtual void build()
    {
        log() << "one: "<<_i<<std::endl;
    }
    
    unsigned int _i;

};

struct OperationTwo : public vpb::BuildOperation
{
    OperationTwo(vpb::BuildLog* buildLog, unsigned int i):
        vpb::BuildOperation(buildLog,"Operation Two",false),
        _i(i) {}
    
    virtual void build()
    {
        log() << "two: "<<_i<<std::endl;
    }
    unsigned int _i;

};

int main( int argc, char **argv )
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);


    osg::ref_ptr<vpb::BuildLog> buildLog = new vpb::BuildLog;
    
    typedef std::list< osg::ref_ptr<osg::OperationThread> > Threads;
    Threads threads;
    
    osg::ref_ptr<osg::OperationQueue> operationQueue = new osg::OperationQueue;
    
    unsigned int numThreads=1;
    while(arguments.read("-t",numThreads)) {}
    
    for(unsigned int i=0; i<numThreads; ++i)
    {
        osg::ref_ptr<osg::OperationThread> thread = new osg::OperationThread;
        thread->setOperationQueue(operationQueue.get());
        threads.push_back(thread.get());
        
        thread->startThread();
    }
    
    
    unsigned int numOneOps=100;
    while(arguments.read("-1",numOneOps)) {}

    unsigned int numTwoOps=100;
    while(arguments.read("-2",numTwoOps)) {}
    
    unsigned int i = 0;
    unsigned int j = 0;
    for(unsigned int i=0; 
        i<numOneOps || j<numTwoOps;
        ++i, ++j)
    {
        if (i<numOneOps) operationQueue->add(new OperationOne(buildLog.get(),i));
        if (j<numOneOps) operationQueue->add(new OperationTwo(buildLog.get(),j));
    }
    
    osg::ref_ptr<osg::Operation> operation;
    while ((operation=operationQueue->getNextOperation()).valid())
    {
        (*operation)(0);
    }

    return 0;
}

