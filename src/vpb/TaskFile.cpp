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

#include <vpb/TaskFile>

#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>

using namespace vpb;

TaskFile::TaskFile(const std::string& filename, Type type):
    PropertyFile(filename,type),
    _argc(0),
    _argv(0)
{
}

TaskFile::~TaskFile()
{
}

void TaskFile::init(osg::ArgumentParser& arguments)
{
    std::string application(arguments[0]);
    std::string args;
    for(unsigned int i=1; i<arguments.argc(); ++i)
    {
        if (i>1) args += " ";
        args += arguments[i];
    }

    setProperty("application", application);
    setProperty("arguments", args);
    
    int pid = getpid();
    setProperty("pid",pid);
    
    char hostname[1024];
    if (gethostname(hostname, sizeof(hostname))==0)
    {
        std::string str(hostname);
        setProperty("hostname",str);
    }
    
    char domainname[1024];
    if (getdomainname(domainname, sizeof(domainname))==0)
    {
        std::string str(domainname);
        setProperty("domainname",str);
    }
    
    char* loginname = getlogin();
    if (loginname)
    {
        std::string str(loginname);
        setProperty("loginname",str);
    }
    
    char* usershell = getusershell();
    if (usershell)
    {
        std::string str(usershell);
        setProperty("usershell",str);
    }

    int tabelsize = getdtablesize();
    setProperty("tablesize",tabelsize);
}

void TaskFile::get(osg::ArgumentParser& arguments)
{
    std::string application;
    std::string args;
    if (getProperty("application",application))
    {
    }
    
    if (getProperty("arguments",args))
    {
    }
}

void TaskFile::invoke(bool runInBackground)
{
    std::string application;
    if (getProperty("application",application))
    {
        std::string args;
        if (getProperty("arguments",args))
        {
            application += std::string(" ") + args;
        }
        
        if (runInBackground)
        {
            application += std::string(" &");
        }

        system(application.c_str());
    }
}

void TaskFile::signal(int signal)
{
    std::string pid;
    if (getProperty("pid", pid))
    {
        std::stringstream signalcommand;
        signalcommand << "kill -" << signal<<" "<<pid;

        system(signalcommand.str().c_str());
    }
}

void TaskFileOperation::operator () (osg::Object*)
{
    _taskFile->sync();
    _taskFile->report(std::cout);
}

void SleepOperation::operator () (osg::Object*)
{
    OpenThreads::Thread::microSleep(_microSeconds);
}
