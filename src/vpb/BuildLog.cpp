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

#include <vpb/BuildLog>
#include <vpb/BuildOperation>

using namespace vpb;

OperationLog::OperationLog(BuildLog* buildLog):
    Object(true),
    _buildLog(buildLog),
    _startPendingTime(-1.0),
    _startRunningTime(-1.0),
    _endRunningTime(-1.0)
{
}

OperationLog::OperationLog(BuildLog* buildLog, const std::string& name):
    Object(true),
    _buildLog(buildLog),
    _startPendingTime(-1.0),
    _startRunningTime(-1.0),
    _endRunningTime(-1.0)
{
    setName(name);
}


OperationLog::OperationLog(const OperationLog& log, const osg::CopyOp& copyop):
    osg::Object(log,copyop),
    _buildLog(log._buildLog),
    _startPendingTime(log._startPendingTime),
    _startRunningTime(log._startRunningTime),
    _endRunningTime(log._endRunningTime)
{
}

OperationLog::~OperationLog()
{
    for(Messages::iterator itr = _messages.begin();
        itr != _messages.end();
        ++itr)
    {
        if (itr->second)
        {
            delete itr->second;
            itr->second = 0;
        }
    }
}


std::ostream& OperationLog::operator() (osg::NotifySeverity level)
{
    _messages.push_back(MessagePair(_buildLog->getCurrentTime(), new std::ostringstream));
    return *(_messages.back().second);
}

BuildLog::BuildLog():
    osg::Object(true)
{
    initStartTime();
}

BuildLog::BuildLog(const BuildLog& bl, const osg::CopyOp& copyop):
    osg::Object(bl,copyop)
{
}

void BuildLog::initStartTime()
{
    _timer.setStartTick(_timer.tick());
}

void BuildLog::pendingOperation(BuildOperation* operation)
{
    OperationLog* log = operation->getOperationLog();
    
    if (log)
    {
        log->setStartPendingTime(getCurrentTime());

        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_pendingOperationsMutex);
        _pendingOperations.push_back(log);
    }
}

void BuildLog::runningOperation(BuildOperation* operation)
{
    OperationLog* log = operation->getOperationLog();
    
    if (log)
    {
        log->setStartRunningTime(getCurrentTime());

        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_runningOperationsMutex);
            _runningOperations.push_back(log);
        }
        
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_pendingOperationsMutex);
            remove(_pendingOperations, log);
        }
        
    }
}


void BuildLog::completedOperation(BuildOperation* operation)
{
    OperationLog* log = operation->getOperationLog();
    
    if (log)
    {
        log->setEndRunningTime(getCurrentTime());

        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_completedOperationsMutex);
            _completedOperations.push_back(log);
        }
        
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_runningOperationsMutex);
            remove(_runningOperations, log);
        }
    }
}


void BuildLog::remove(OperationLogs& logs, OperationLog* log)
{
    OperationLogs::iterator itr = std::find(logs.begin(), logs.end(), log);
    if (itr != logs.end())
    {
        logs.erase(itr);
    }
}


bool BuildLog::isComplete() const
{
    unsigned int numOutstandingOperations = 0;

    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_pendingOperationsMutex);
        numOutstandingOperations += _pendingOperations.size();
    }

    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_runningOperationsMutex);
        numOutstandingOperations += _runningOperations.size();
    }

    return numOutstandingOperations==0;
}

void BuildLog::waitForCompletion() const
{
    while(!isComplete())
    {
        OpenThreads::Thread::YieldCurrentThread();
    }
}


void BuildLog::report(std::ostream& out)
{
    out<<"BuildLog::report"<<std::endl;
    out<<"================"<<std::endl<<std::endl;
    out<<"Pending Operations   "<<_pendingOperations.size()<<std::endl;
    out<<"Runnning Operations  "<<_runningOperations.size()<<std::endl;
    out<<"Completed Operations "<<_completedOperations.size()<<std::endl;
}


