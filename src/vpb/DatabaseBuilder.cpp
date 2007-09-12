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

#include <vpb/DatabaseBuilder>

#include <iostream>
#include <string>

#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/io_utils>

#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/Output>
#include <osgDB/ParameterOutput>

using namespace vpb;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DatabaseBuilder IO support
//

bool DatabaseBuilder_readLocalData(osg::Object &obj, osgDB::Input &fr);
bool DatabaseBuilder_writeLocalData(const osg::Object &obj, osgDB::Output &fw);

osgDB::RegisterDotOsgWrapperProxy DatabaseBuilder_Proxy
(
    new vpb::DatabaseBuilder,
    "DatabaseBuilder",
    "DatabaseBuilder Object",
    DatabaseBuilder_readLocalData,
    DatabaseBuilder_writeLocalData
);


bool DatabaseBuilder_readLocalData(osg::Object& obj, osgDB::Input &fr)
{
    vpb::DatabaseBuilder& gt = static_cast<vpb::DatabaseBuilder&>(obj);
    bool itrAdvanced = false;
    return itrAdvanced;
}

bool DatabaseBuilder_writeLocalData(const osg::Object& obj, osgDB::Output& fw)
{
    const vpb::DatabaseBuilder& gt = static_cast<const vpb::DatabaseBuilder&>(obj);
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DatabaseBuilder implementation
//
DatabaseBuilder::DatabaseBuilder()
{
}

DatabaseBuilder::DatabaseBuilder(const DatabaseBuilder& db,const osg::CopyOp& copyop):
    osgTerrain::TerrainTechnique(db, copyop),
    BuildOptions(db)
{
}

void DatabaseBuilder::init()
{
}

void DatabaseBuilder::update(osgUtil::UpdateVisitor*)
{
}

void DatabaseBuilder::cull(osgUtil::CullVisitor*)
{
}

