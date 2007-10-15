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

bool vpb::Parameter::getString(std::string& str)
{
    switch(_type)
    {
        case(BOOL_PARAMETER):
        {
            str = *_value._bool ? "True" : "False";
            break;
        }
        case(FLOAT_PARAMETER):
        {
            std::stringstream sstr;
            sstr << *_value._float;
            str = sstr.str();
            break;
        }
        case(DOUBLE_PARAMETER):
        {
            std::stringstream sstr;
            sstr << *_value._double;
            str = sstr.str();
            break;
        }
        case(INT_PARAMETER):
        {
            std::stringstream sstr;
            sstr << *_value._int;
            str = sstr.str();
            break;
        }
        case(UNSIGNED_INT_PARAMETER):
        {
            std::stringstream sstr;
            sstr << *_value._uint;
            str = sstr.str();
            break;
        }
        case(STRING_PARAMETER):
        {
            str = *(_value._string);
            break;
        }
    }
}


TaskFile::TaskFile(const std::string& filename, Type type):
    _type(type),
    _fileID(0),
    _syncCount(0),
    _propertiesModified(false),
    _previousSize(0),
    _previousData(0),
    _currentSize(0),
    _currentData(0),
    _argc(0),
    _argv(0)
{
    if (access(filename.c_str(), F_OK)==0)
    {
        _fileID = open (filename.c_str(), O_RDWR);
    }
    else
    {
        FILE* file = fopen(filename.c_str(), "wr");
        fclose(file);

        _fileID = open (filename.c_str(), O_RDWR);
    }
}

TaskFile::~TaskFile()
{
    if (_fileID) close(_fileID);

    if (_previousData) delete [] _previousData;
    if (_currentData) delete [] _currentData;
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

void TaskFile::setProperty(const std::string& property, Parameter value)
{
    std::string originalValue = _propertyMap[property];
    value.getString(_propertyMap[property]);
    
    if (_propertyMap[property] != originalValue) _propertiesModified=true;
}

bool TaskFile::getProperty(const std::string& property, Parameter value)
{
    PropertyMap::iterator itr = _propertyMap.find(property);
    if (itr != _propertyMap.end())
    {
        const std::string field = _propertyMap[property];
        if (value.valid(field.c_str()))
        {
            value.assign(field.c_str());
            return true;
        }
    }
    return false;
}

bool TaskFile::sync()
{
    if (_type==READ) return read();
    else return write();
}

bool TaskFile::read()
{
    int status = 0;

    lseek(_fileID, 0, SEEK_SET);

    status = lockf(_fileID, F_LOCK, 0);
    if (status!=0) perror("lock error");
    
    std::swap(_currentSize, _previousSize);
    std::swap(_currentData, _previousData);
    
    int size = lseek(_fileID, 0, SEEK_END);

    if (_currentSize!=size) 
    {
        if (_currentData) delete [] _currentData;
        _currentData = new char[size];
    }
    
    char* data = _currentData;
    
    status = lseek(_fileID, 0, SEEK_SET);

    _currentSize = ::read(_fileID, data, size);

    lseek(_fileID, 0, SEEK_SET);

    status = lockf(_fileID, F_ULOCK, 0);
    if (status!=0) perror("file unlock error");
    

    bool dataChanged = (_currentSize != _previousSize) ||
                       memcmp(_currentData, _previousData, _currentSize)!=0;

    if (dataChanged)
    {
        _propertyMap.clear();
    
        char* end = data + _currentSize;
        char* curr = data;

        while(curr < end)
        {
            // skip over proceeding spaces
            while(*curr==' ' && curr<end) ++curr;

            char* end_of_line = curr;
            while (end_of_line<end && (*end_of_line != '\n')) ++end_of_line;

            // wipe trailing spaces/control characters
            char* back = end_of_line-1;
            while(back>curr && *back<=' ') --back;
            ++back;

            char* colon = strchr(curr,':');
            if (colon)
            {
                char* endName = colon-1;
                while(endName>curr && *endName==' ') --endName;

                char* startValue = colon+1;
                while(startValue<end && *startValue<=' ') ++startValue;

                if (startValue<end_of_line)
                {
                    _propertyMap[std::string(curr, endName+1)] = std::string(startValue, end_of_line); 
                }
                else
                {
                    _propertyMap[std::string(curr, endName+1)] = ""; 
                }
            }
            else
            {
                _propertyMap[""] = std::string(curr, end_of_line);
            }

            curr = end_of_line+1;
        }

        _propertiesModified = false;

        return true;
    }
    else
    {
        return false;
    }
}

bool TaskFile::write()
{
    if (!_propertiesModified)
    {
        return false;
    }

    int status = 0;
    
    lseek(_fileID, 0, SEEK_SET);

    status = lockf(_fileID, F_LOCK, 0);
    if (status!=0) perror("file lock error");

    for(PropertyMap::iterator itr = _propertyMap.begin();
        itr != _propertyMap.end();
        ++itr)
    {
        ::write(_fileID, itr->first.c_str(), itr->first.length());
        ::write(_fileID, " : ", 3);
        ::write(_fileID, itr->second.c_str(), itr->second.length());
        ::write(_fileID, "\n", 1);
    }
    
    
    ftruncate(_fileID, lseek(_fileID, 0, SEEK_CUR) );

    lseek(_fileID, 0, SEEK_SET);

    status = lockf(_fileID, F_ULOCK, 0);
    if (status!=0) perror("file unlock error");
    
    _propertiesModified = false;
}

void TaskFile::report(std::ostream& out)
{
    out<<"Properties:"<<std::endl;
    for(PropertyMap::iterator itr = _propertyMap.begin();
        itr != _propertyMap.end();
        ++itr)
    {
        out<<itr->first<<":"<<itr->second<<std::endl;
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
