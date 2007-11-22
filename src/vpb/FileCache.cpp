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

#include <vpb/FileCache>
#include <vpb/FileSystem>
#include <vpb/BuildLog>
#include <vpb/DataSet>

using namespace vpb;

FileCache::FileCache()
{
    _requiresWrite = false;
}


FileCache::FileCache(const FileCache& fc,const osg::CopyOp& copyop):
    osg::Object(fc, copyop)
{
    _requiresWrite = false;
}

FileCache::~FileCache()
{
}

bool FileCache::read(const std::string& filename)
{
    osg::notify(osg::NOTICE)<<"FileCache::read("<<filename<<")"<<std::endl;

    std::string foundFile = osgDB::findDataFile(filename);
    if (foundFile.empty())
    {
        log(osg::WARN,"Error: could not find cache file '%s'",filename.c_str());
        return false;
    }

    _filename = filename;

    std::ifstream fin(foundFile.c_str());
    
    bool emptyBefore = _variantMap.empty();
    
    if (fin)
    {
        osgDB::Input fr;
        fr.attach(&fin);
        
        std::string str;

        while(!fr.eof())
        {        
            bool itrAdvanced = false;

            if (fr.matchSequence("FileDetails {"))
            {
                osg::ref_ptr<FileDetails> fd = new FileDetails;

                int local_entry = fr[0].getNoNestedBrackets();

                fr += 2;

                while (!fr.eof() && fr[0].getNoNestedBrackets()>local_entry)
                {
                    bool localAdvanced = false;

                    if (fr.read("hostname",str))
                    {
                        fd->setHostName(str);
                        localAdvanced = true;
                    }

                    if (fr.read("original",str))
                    {
                        fd->setOriginalSourceFileName(str);
                        localAdvanced = true;
                    }

                    if (fr.read("file",str))
                    {
                        fd->setFileName(str);
                        localAdvanced = true;
                    }

                    if (fr.read("cs",str))
                    {
                        if (!fd->getSpatialProperties()._cs) fd->getSpatialProperties()._cs = new osg::CoordinateSystemNode;
                        
                        fd->getSpatialProperties()._cs->setCoordinateSystem(str); 
                        localAdvanced = true;
                    }

                    int sizeX, sizeY;
                    if (fr.read("size",sizeX, sizeY))
                    {
                        fd->getSpatialProperties()._numValuesX = sizeX;
                        fd->getSpatialProperties()._numValuesY = sizeY;
                        localAdvanced = true;
                    }

                    double minX, maxX, minY, maxY;
                    if (fr.read("extents",minX, minY, maxX, maxY))
                    {
                        fd->getSpatialProperties()._extents._min.set(minX,minY);
                        fd->getSpatialProperties()._extents._max.set(maxX,maxY);
                        localAdvanced = true;
                    }

                    if (!localAdvanced) ++fr;
                }

                ++fr;

                itrAdvanced = true;
                
                addFileDetails(fd.get());

            }
            
            if (!itrAdvanced) ++fr;
        }        
    }
    
    _requiresWrite = !emptyBefore;

    return false;
}
 
bool FileCache::open(const std::string& filename)
{
    std::string foundFile = osgDB::findDataFile(filename);
    if (foundFile.empty())
    {
        setFileName(filename);
        _requiresWrite = true;
        return false;
    }
    
    return read(foundFile);
}

bool FileCache::write(const std::string& filename)
{
    osg::notify(osg::NOTICE)<<"FileCache::write("<<filename<<")"<<std::endl;

    _filename = filename;
    _requiresWrite = false;

    osgDB::Output fout(filename.c_str());

    fout.precision(15);

    for(VariantMap::iterator itr = _variantMap.begin();
        itr != _variantMap.end();
        ++itr)
    {
        Variants& variants = itr->second;
        for(Variants::iterator vitr = variants.begin();
            vitr != variants.end();
            ++vitr)
        {
            FileDetails* fd = vitr->get();
            
            fout.indent()<<"FileDetails {"<<std::endl;
            fout.moveIn();
            
            if (!fd->getHostName().empty())
            {
                fout.indent()<<"hostname "<<fout.wrapString(fd->getHostName())<<std::endl;
            }

            if (!fd->getOriginalSourceFileName().empty())
            {
                fout.indent()<<"original "<<fout.wrapString(fd->getOriginalSourceFileName())<<std::endl;
            }
            
            if (!fd->getFileName().empty())
            {
                fout.indent()<<"file "<<fout.wrapString(fd->getFileName())<<std::endl;
            }
            
            if (fd->getSpatialProperties()._cs.valid() && !fd->getSpatialProperties()._cs->getCoordinateSystem().empty())
            {
                fout.indent()<<"cs "<<fout.wrapString(fd->getSpatialProperties()._cs->getCoordinateSystem())<<std::endl;
            }

            if (fd->getSpatialProperties()._extents.valid())
            {
                const GeospatialExtents& extents = fd->getSpatialProperties()._extents;
                fout.indent()<<"extents "<<extents.xMin()<<" "<<extents.yMin()<<" "<<extents.xMax()<<" "<<extents.yMax()<<std::endl;
            }
            
            if (fd->getSpatialProperties()._numValuesX>0 || fd->getSpatialProperties()._numValuesY>0)
            {
                fout.indent()<<"size "<<fd->getSpatialProperties()._numValuesX<<" "<<fd->getSpatialProperties()._numValuesY<<std::endl;
            }
            
            fout.moveOut();
            fout.indent()<<"}"<<std::endl;
            
            
        }
    }

    return false;
}

void FileCache::addFileDetails(FileDetails* fd)
{
    _requiresWrite = true;
    _variantMap[fd->getOriginalSourceFileName()].push_back(fd);
}

void FileCache::removeFileDetails(FileDetails* fd)
{
    _requiresWrite = true;
    VariantMap::iterator itr = _variantMap.find(fd->getOriginalSourceFileName());
    if (itr==_variantMap.end()) return;

    Variants& variants = itr->second;
    for(Variants::iterator vitr = variants.begin();
        vitr != variants.end();
        ++vitr)
    {
        if (*vitr == fd)
        {
            variants.erase(vitr);
            return;
        }
    }
}

std::string FileCache::getOptimimumFile(const std::string& filename, const SpatialProperties& sp)
{
    VariantMap::iterator itr = _variantMap.find(filename);
    if (itr==_variantMap.end())
    {
        // osg::notify(osg::NOTICE)<<"FileCache::getOptimimumFile("<<filename<<") no variants found returning '"<<filename<<"'"<<std::endl;
        return filename;
    }
    
    Variants& variants = itr->second;

    FileDetails* fd_closest_below = 0;
    double res_closest_below = -DBL_MAX;
    
    FileDetails* fd_closest_above = 0;
    double res_closest_above = DBL_MAX;
    
    // osg::notify(osg::NOTICE)<<"FileCache::getOptimimumFile("<<filename<<") checking variats "<<itr->first<<std::endl;

    // first check cached files on 
    std::string hostname = getLocalHostName();
    for(Variants::iterator vitr = variants.begin();
        vitr != variants.end();
        ++vitr)
    {
        FileDetails* fd = vitr->get();
        const SpatialProperties& fd_sp = fd->getSpatialProperties();
        if (fd_sp.compatible(sp))
        {
            // osg::notify(osg::NOTICE)<<"  FileDetails("<<fd->getFileName()<<") is compatible "<<std::endl;

            double resolutionRatio = fd_sp.computeResolutionRatio(sp);
            if (resolutionRatio < 1.0)
            {
                if ( resolutionRatio > res_closest_below || 
                    (resolutionRatio==res_closest_below && fd->getHostName()==hostname))
                {
                    fd_closest_below = fd;
                    res_closest_below = resolutionRatio;
                }
            }
            else if (resolutionRatio>=1.0)
            {
                if (resolutionRatio < res_closest_above || 
                    (resolutionRatio==res_closest_above && fd->getHostName()==hostname))
                {
                    fd_closest_above = fd;
                    res_closest_above = resolutionRatio;
                }
            }
        }
        else
        {
            // osg::notify(osg::NOTICE)<<"  FileDetails("<<fd->getFileName()<<") not compatible "<<std::endl;
        }
    }
    
    if (fd_closest_above)
    {
        // osg::notify(osg::NOTICE)<<"FileCache::getOptimimumFile("<<filename<<") found closest_above variant '"<<fd_closest_above->getFileName()<<"'"<<std::endl;
        return fd_closest_above->getFileName();
    }
    
    if (fd_closest_below)
    {
        // osg::notify(osg::NOTICE)<<"FileCache::getOptimimumFile("<<filename<<") found closest_below variant '"<<fd_closest_below->getFileName()<<"'"<<std::endl;
        return fd_closest_below->getFileName();
    }

    // osg::notify(osg::NOTICE)<<"FileCache::getOptimimumFile("<<filename<<") no suitable variants found returning ''"<<std::endl;
    return std::string();
}

void FileCache::clear()
{
    _requiresWrite = true;
    
    _variantMap.clear();
    
    osg::notify(osg::NOTICE)<<"FileCache::clear()"<<std::endl;
}

void FileCache::addSource(osgTerrain::Terrain* source)
{
    if (!source) return;

    _requiresWrite = true;
    
    osg::ref_ptr<DataSet> dataset = new DataSet;
    dataset->addTerrain(source);

    for(CompositeSource::source_iterator itr(dataset->getSourceGraph());itr.valid();++itr)
    {
        (*itr)->loadSourceData();
        Source* source = itr->get();
        SourceData* sd = (*itr)->getSourceData();
        
        
        FileDetails* fd = new FileDetails;
        fd->setOriginalSourceFileName(source->getFileName());
        fd->setFileName(source->getFileName());
        fd->setSpatialProperties(*sd);
        
        addFileDetails(fd);        
    }
    
    osg::notify(osg::NOTICE)<<"FileCache::addSource()"<<std::endl;
}

void FileCache::buildMipmaps()
{
    _requiresWrite = true;

    osg::notify(osg::NOTICE)<<"FileCache::buildMipmaps()"<<std::endl;
}

void FileCache::mirror(Machine* machine, const std::string& directory)
{
    _requiresWrite = true;

    osg::notify(osg::NOTICE)<<"FileCache::mirror("<<machine->getHostName()<<", "<<directory<<")"<<std::endl;
}

void FileCache::report(std::ostream& out)
{
    for(VariantMap::iterator itr = _variantMap.begin();
        itr != _variantMap.end();
        ++itr)
    {
        out<<"Variants of "<<itr->first<<" {"<<std::endl;
        Variants& variants = itr->second;
        for(Variants::iterator vitr = variants.begin();
            vitr != variants.end();
            ++vitr)
        {
            FileDetails* fd = vitr->get();
            
            out<<"  FileDetails {"<<std::endl;
            
            if (!fd->getHostName().empty())
            {
                out<<"    hostname "<<fd->getHostName()<<std::endl;
            }

            if (!fd->getOriginalSourceFileName().empty())
            {
                out<<"    original "<<fd->getOriginalSourceFileName()<<std::endl;
            }
            
            if (!fd->getFileName().empty())
            {
                out<<"    file "<<fd->getFileName()<<std::endl;
            }
            
            if (fd->getSpatialProperties()._cs.valid() && !fd->getSpatialProperties()._cs->getCoordinateSystem().empty())
            {
                out<<"    cs "<<fd->getSpatialProperties()._cs->getCoordinateSystem()<<std::endl;
            }

            if (fd->getSpatialProperties()._extents.valid())
            {
                const GeospatialExtents& extents = fd->getSpatialProperties()._extents;
                out<<"    extents "<<extents.xMin()<<" "<<extents.yMin()<<" "<<extents.xMax()<<" "<<extents.yMax()<<std::endl;
            }
            
            if (fd->getSpatialProperties()._numValuesX>0 || fd->getSpatialProperties()._numValuesY>0)
            {
                out<<"    size "<<fd->getSpatialProperties()._numValuesX<<" "<<fd->getSpatialProperties()._numValuesY<<std::endl;
            }
            
            out<<"  }"<<std::endl;
                        
        }
        out<<"}"<<std::endl;
    }
}

