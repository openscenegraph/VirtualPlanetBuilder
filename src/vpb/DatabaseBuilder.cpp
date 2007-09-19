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
#include <map>

#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/io_utils>

#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/Output>
#include <osgDB/ParameterOutput>

using namespace vpb;


class IntLookup
{
public:

    typedef int Value;

    IntLookup(Value defaultValue):
        _default(defaultValue) {}

    typedef std::map<std::string, Value> StringToValue;
    typedef std::map<Value, std::string> ValueToString;

    StringToValue   _stringToValue;
    ValueToString   _valueToString;
    
    void add(Value value, const char* str)
    {
        _stringToValue[str] = value;
        _valueToString[value] = str;
    }
    
    Value getValue(const char* str)
    {
        StringToValue::iterator itr = _stringToValue.find(str);
        if (itr==_stringToValue.end()) return _default;
        return itr->second;
    }
    
    const std::string& getString(Value value)
    {
        ValueToString::iterator itr = _valueToString.find(value);
        if (itr==_valueToString.end()) return _valueToString[_default];
        return itr->second;
    }
    
    Value _default;

};

class Serializer : public osg::Referenced
{
public:

     Serializer() {}
     
     virtual bool write(osgDB::Output&, const osg::Object&) = 0;
     virtual bool read(osgDB::Input&, osg::Object&, bool&) = 0;
};

template<typename C, typename P>
class EnumSerializer : public Serializer
{
public:

     typedef P (C::*GetterFunctionType)() const;
     typedef void (C::*SetterFunctionType)(P);

     EnumSerializer(const char* fieldName, P defaultValue, GetterFunctionType getter, SetterFunctionType setter):
        _fieldName(fieldName),
        _default(defaultValue),
        _getter(getter),
        _setter(setter),
        _lookup(defaultValue) {}
     
    void add(P value, const char* str)
    {
        _lookup.add(static_cast<IntLookup::Value>(value), str);
    }

    P getValue(const char* str)
    {
        return static_cast<P>(_lookup.getValue(str));
    }

    const std::string& getString(P value)
    {
        return _lookup.getString(static_cast<IntLookup::Value>(value));
    }

    bool write(osgDB::Output& fw, const osg::Object& obj)
    {
        const C& object = static_cast<const C&>(obj);
        if (fw.getWriteOutDefaultValues() ||
            _default != (object.*_getter)())
        {
            fw.indent()<<_fieldName<<" "<<getString((object.*_getter)())<<std::endl;
        }
    }

    bool read(osgDB::Input& fr, osg::Object& obj, bool& itrAdvanced)
    {
        C& object = static_cast<C&>(obj);
        if (fr[0].matchWord(_fieldName.c_str()) && fr[1].isWord())
        {
            (object.*_setter)(getValue(fr[1].getStr()));
            fr += 2;
            itrAdvanced = true;
        }
    }

    std::string        _fieldName;
    P                  _default;
    GetterFunctionType _getter;
    SetterFunctionType _setter;
    IntLookup          _lookup;
};


template<typename C>
class StringSerializer : public Serializer
{
public:

     typedef const std::string& P;
     typedef P (C::*GetterFunctionType)() const;
     typedef void (C::*SetterFunctionType)(P);

     StringSerializer(const char* fieldName, P defaultValue, GetterFunctionType getter, SetterFunctionType setter):
        _fieldName(fieldName),
        _default(defaultValue),
        _getter(getter),
        _setter(setter) {}
     
     bool write(osgDB::Output& fw, const osg::Object& obj)
     {
        const C& object = static_cast<const C&>(obj);
        if (fw.getWriteOutDefaultValues() ||
            _default != (object.*_getter)())
        {
            fw.indent()<<_fieldName<<" "<<fw.wrapString((object.*_getter)())<<std::endl;
        }
     }

    bool read(osgDB::Input& fr, osg::Object& obj, bool& itrAdvanced)
    {
        C& object = static_cast<C&>(obj);
        if (fr[0].matchWord(_fieldName.c_str()) && (fr[1].isWord() || fr[1].isString()))
        {
            (object.*_setter)(fr[1].getStr());
            fr += 2;
            itrAdvanced = true;
        }
     }
     
     std::string        _fieldName;
     std::string        _default;
     GetterFunctionType _getter;
     SetterFunctionType _setter;
};


template<typename C, typename P>
class TemplateSerializer : public Serializer
{
public:

     typedef P (C::*GetterFunctionType)() const;
     typedef void (C::*SetterFunctionType)(P);

     TemplateSerializer(const char* fieldName, P defaultValue, GetterFunctionType getter, SetterFunctionType setter):
        _fieldName(fieldName),
        _default(defaultValue),
        _getter(getter),
        _setter(setter) {}
     
     bool write(osgDB::Output& fw, const osg::Object& obj)
     {
        const C& object = static_cast<const C&>(obj);
        if (fw.getWriteOutDefaultValues() ||
            _default != (object.*_getter)())
        {
            fw.indent()<<_fieldName<<" "<<(object.*_getter)()<<std::endl;
        }
     }

    bool read(osgDB::Input& fr, osg::Object& obj, bool& itrAdvanced)
    {
        C& object = static_cast<C&>(obj);
        P value;
        if (fr.read(_fieldName.c_str(), value))
        {
            (object.*_setter)(value);
            itrAdvanced = true;
        }
     }
     
     std::string        _fieldName;
     P                  _default;
     GetterFunctionType _getter;
     SetterFunctionType _setter;
};

template<typename C>
class Vec4Serializer : public Serializer
{
public:

     typedef osg::Vec4 V;
     typedef const V& P;
     typedef P (C::*GetterFunctionType)() const;
     typedef void (C::*SetterFunctionType)(P);

     Vec4Serializer(const char* fieldName, P defaultValue, GetterFunctionType getter, SetterFunctionType setter):
        _fieldName(fieldName),
        _default(defaultValue),
        _getter(getter),
        _setter(setter) {}
     
     bool write(osgDB::Output& fw, const osg::Object& obj)
     {
        const C& object = static_cast<const C&>(obj);
        if (fw.getWriteOutDefaultValues() ||
            _default != (object.*_getter)())
        {
            fw.indent()<<_fieldName<<" "<<(object.*_getter)()<<std::endl;
        }
     }

    bool read(osgDB::Input& fr, osg::Object& obj, bool& itrAdvanced)
    {
        C& object = static_cast<C&>(obj);
        V value;
        if (fr.read(_fieldName.c_str(), value[0], value[1], value[2], value[3]))
        {
            (object.*_setter)(value);
            fr += 2;
            itrAdvanced = true;
        }
     }
     
     std::string        _fieldName;
     V                  _default;
     GetterFunctionType _getter;
     SetterFunctionType _setter;
};


template<typename C>
class BoolSerializer : public Serializer
{
public:

     typedef bool P;
     typedef P (C::*GetterFunctionType)() const;
     typedef void (C::*SetterFunctionType)(P);

     BoolSerializer(const char* fieldName, P defaultValue, GetterFunctionType getter, SetterFunctionType setter):
        _fieldName(fieldName),
        _default(defaultValue),
        _getter(getter),
        _setter(setter) {}
     
     bool write(osgDB::Output& fw, const osg::Object& obj)
     {
        const C& object = static_cast<const C&>(obj);
        if (fw.getWriteOutDefaultValues() ||
            _default != (object.*_getter)())
        {
            fw.indent()<<_fieldName<<" "<<((object.*_getter)() ? "TRUE" : "FALSE")<<std::endl;
        }
     }

    bool read(osgDB::Input& fr, osg::Object& obj, bool& itrAdvanced)
    {
        C& object = static_cast<C&>(obj);
        if (fr[0].matchWord(_fieldName.c_str()) && fr[1].isWord())
        {
            (object.*_setter)(fr[1].matchWord("TRUE") || fr[1].matchWord("True") || fr[1].matchWord("true"));
            fr += 2;
            itrAdvanced = true;
        }
     }
     
     std::string        _fieldName;
     P                  _default;
     GetterFunctionType _getter;
     SetterFunctionType _setter;
};

template<typename C>
class GeospatialExtentsSerializer : public Serializer
{
public:

     typedef GeospatialExtents V;
     typedef const V& P;
     typedef P (C::*GetterFunctionType)() const;
     typedef void (C::*SetterFunctionType)(P);

     GeospatialExtentsSerializer(const char* fieldName, P defaultValue, GetterFunctionType getter, SetterFunctionType setter):
        _fieldName(fieldName),
        _default(defaultValue),
        _getter(getter),
        _setter(setter) {}
     
     bool write(osgDB::Output& fw, const osg::Object& obj)
     {
        const C& object = static_cast<const C&>(obj);
        if (fw.getWriteOutDefaultValues() ||
            _default != (object.*_getter)())
        {
            P value = (object.*_getter)();
            fw.indent()<<_fieldName<<" "<<value._min[0]<<" "<<value._min[1]<<" "<<value._max[0]<<" "<<value._max[1]<<std::endl;
        }
     }

    bool read(osgDB::Input& fr, osg::Object& obj, bool& itrAdvanced)
    {
        C& object = static_cast<C&>(obj);
        V value;
        if (fr.read(_fieldName.c_str(), value._min[0], value._min[1], value._max[0], value._max[1]))
        {
            (object.*_setter)(value);
            itrAdvanced = true;
        }
     }
     
     std::string        _fieldName;
     V                  _default;
     GetterFunctionType _getter;
     SetterFunctionType _setter;
};


#define CREATE_STRING_SERIALIZER(CLASS,PROPERTY,PROTOTYPE) \
    new StringSerializer<CLASS>( \
    #PROPERTY, \
    PROTOTYPE.get##PROPERTY(), \
    &CLASS::get##PROPERTY, \
    &CLASS::set##PROPERTY)

#define ADD_STRING_PROPERTY(PROPERTY) _serializerList.push_back(CREATE_STRING_SERIALIZER(DatabaseBuilder,PROPERTY,prototype))


#define CREATE_UINT_SERIALIZER(CLASS,PROPERTY,PROTOTYPE) \
    new TemplateSerializer<CLASS,unsigned int>( \
    #PROPERTY, \
    PROTOTYPE.get##PROPERTY(), \
    &CLASS::get##PROPERTY, \
    &CLASS::set##PROPERTY)

#define ADD_UINT_PROPERTY(PROPERTY) _serializerList.push_back(CREATE_UINT_SERIALIZER(DatabaseBuilder,PROPERTY,prototype))


#define CREATE_INT_SERIALIZER(CLASS,PROPERTY,PROTOTYPE) \
    new TemplateSerializer<CLASS, int>( \
    #PROPERTY, \
    PROTOTYPE.get##PROPERTY(), \
    &CLASS::get##PROPERTY, \
    &CLASS::set##PROPERTY)

#define ADD_INT_PROPERTY(PROPERTY) _serializerList.push_back(CREATE_INT_SERIALIZER(DatabaseBuilder,PROPERTY,prototype))


#define CREATE_FLOAT_SERIALIZER(CLASS,PROPERTY,PROTOTYPE) \
    new TemplateSerializer<CLASS,float>( \
    #PROPERTY, \
    PROTOTYPE.get##PROPERTY(), \
    &CLASS::get##PROPERTY, \
    &CLASS::set##PROPERTY)

#define ADD_FLOAT_PROPERTY(PROPERTY) _serializerList.push_back(CREATE_FLOAT_SERIALIZER(DatabaseBuilder,PROPERTY,prototype))

#define CREATE_DOUBLE_SERIALIZER(CLASS,PROPERTY,PROTOTYPE) \
    new TemplateSerializer<CLASS, double>( \
    #PROPERTY, \
    PROTOTYPE.get##PROPERTY(), \
    &CLASS::get##PROPERTY, \
    &CLASS::set##PROPERTY)

#define ADD_DOUBLE_PROPERTY(PROPERTY) _serializerList.push_back(CREATE_DOUBLE_SERIALIZER(DatabaseBuilder,PROPERTY,prototype))


#define CREATE_VEC4_SERIALIZER(CLASS,PROPERTY,PROTOTYPE) \
    new Vec4Serializer<CLASS>( \
    #PROPERTY, \
    PROTOTYPE.get##PROPERTY(), \
    &CLASS::get##PROPERTY, \
    &CLASS::set##PROPERTY)

#define ADD_VEC4_PROPERTY(PROPERTY) _serializerList.push_back(CREATE_VEC4_SERIALIZER(DatabaseBuilder,PROPERTY,prototype))


#define CREATE_BOOL_SERIALIZER(CLASS,PROPERTY,PROTOTYPE) \
    new BoolSerializer<CLASS>( \
    #PROPERTY, \
    PROTOTYPE.get##PROPERTY(), \
    &CLASS::get##PROPERTY, \
    &CLASS::set##PROPERTY)

#define ADD_BOOL_PROPERTY(PROPERTY) _serializerList.push_back(CREATE_BOOL_SERIALIZER(DatabaseBuilder,PROPERTY,prototype))


#define CREATE_ENUM_SERIALIZER(CLASS,PROPERTY,PROTOTYPE) \
    typedef EnumSerializer<DatabaseBuilder, DatabaseBuilder::PROPERTY> MySerializer;\
    osg::ref_ptr<MySerializer> serializer = new MySerializer(\
        #PROPERTY,\
        PROTOTYPE.get##PROPERTY(),\
        &CLASS::get##PROPERTY,\
        &CLASS::set##PROPERTY\
    )
    

#define ADD_ENUM_PROPERTY(PROPERTY) \
    CREATE_ENUM_SERIALIZER(DatabaseBuilder, PROPERTY, prototype); \
    _serializerList.push_back(serializer.get())

#define ADD_ENUM_VALUE(VALUE) serializer->add(DatabaseBuilder::VALUE, #VALUE)

#define ADD_ENUM_PROPERTY_TWO_VALUES(PROPERTY,VALUE1,VALUE2) \
    { \
        ADD_ENUM_PROPERTY(PROPERTY);\
        ADD_ENUM_VALUE(VALUE1);\
        ADD_ENUM_VALUE(VALUE2);\
    }

#define ADD_ENUM_PROPERTY_TWO_VALUES(PROPERTY,VALUE1,VALUE2) \
    { \
        ADD_ENUM_PROPERTY(PROPERTY);\
        ADD_ENUM_VALUE(VALUE1);\
        ADD_ENUM_VALUE(VALUE2);\
    }

#define ADD_ENUM_PROPERTY_THREE_VALUES(PROPERTY,VALUE1,VALUE2,VALUE3) \
    { \
        ADD_ENUM_PROPERTY(PROPERTY);\
        ADD_ENUM_VALUE(VALUE1);\
        ADD_ENUM_VALUE(VALUE2);\
        ADD_ENUM_VALUE(VALUE3);\
    }

#define AEV ADD_ENUM_VALUE
#define AEP ADD_ENUM_PROPERTY

class DatabaseBuilderLookUps
{
public:

    typedef std::list< osg::ref_ptr<Serializer> > SerializerList;
    SerializerList _serializerList;

    DatabaseBuilderLookUps()
    {
        DatabaseBuilder prototype;

        ADD_STRING_PROPERTY(Directory);
        ADD_STRING_PROPERTY(DestinationTileBaseName);
        ADD_STRING_PROPERTY(DestinationTileExtension);
        ADD_STRING_PROPERTY(DestinationImageExtension);
        ADD_STRING_PROPERTY(ArchiveName);
        ADD_STRING_PROPERTY(CommentString);
        
        ADD_ENUM_PROPERTY_TWO_VALUES(DatabaseType, LOD_DATABASE, PagedLOD_DATABASE)
        ADD_ENUM_PROPERTY_TWO_VALUES(GeometryType, HEIGHT_FIELD, POLYGONAL)
        ADD_ENUM_PROPERTY_THREE_VALUES(MipMappingMode, NO_MIP_MAPPING, MIP_MAPPING_HARDWARE,MIP_MAPPING_IMAGERY)

        { AEP(TextureType); AEV(RGB_24); AEV(RGBA);AEV(RGB_16);AEV(RGBA_16);AEV(COMPRESSED_TEXTURE);AEV(COMPRESSED_RGBA_TEXTURE); }

        ADD_UINT_PROPERTY(MaximumTileImageSize);
        ADD_UINT_PROPERTY(MaximumTileTerrainSize);

        ADD_FLOAT_PROPERTY(MaximumVisibleDistanceOfTopLevel);
        ADD_FLOAT_PROPERTY(RadiusToMaxVisibleDistanceRatio);
        ADD_FLOAT_PROPERTY(VerticalScale);
        ADD_FLOAT_PROPERTY(SkirtRatio);
        ADD_FLOAT_PROPERTY(MaxAnisotropy);
        
        ADD_BOOL_PROPERTY(ConvertFromGeographicToGeocentric);
        ADD_BOOL_PROPERTY(UseLocalTileTransform);
        ADD_BOOL_PROPERTY(SimplifyTerrain);
        ADD_BOOL_PROPERTY(DecorateGeneratedSceneGraphWithCoordinateSystemNode);
        ADD_BOOL_PROPERTY(DecorateGeneratedSceneGraphWithMultiTextureControl);
        ADD_BOOL_PROPERTY(WriteNodeBeforeSimplification);

        ADD_VEC4_PROPERTY(DefaultColor);
        
        ADD_STRING_PROPERTY(DestinationCoordinateSystem);


        _serializerList.push_back(new GeospatialExtentsSerializer<DatabaseBuilder>(
                "DestinationExtents", 
                prototype.getDestinationExtents(), 
                &DatabaseBuilder::getDestinationExtents,
                &DatabaseBuilder::setDestinationExtents));

        ADD_UINT_PROPERTY(MaximumNumOfLevels);
        
    }
    
    bool read(osgDB::Input& fr, DatabaseBuilder& db, bool& itrAdvanced)
    {
        for(SerializerList::iterator itr = _serializerList.begin();
            itr != _serializerList.end();
            ++itr)
        {
            (*itr)->read(fr,db, itrAdvanced);
        }
        return true;
    }

    bool write(osgDB::Output& fw, const DatabaseBuilder& db)
    {
        bool result = false;
        for(SerializerList::iterator itr = _serializerList.begin();
            itr != _serializerList.end();
            ++itr)
        {
            if ((*itr)->write(fw,db)) result = true;
        }
        return result;
    }

};

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DatabaseBuilder IO support
//

bool DatabaseBuilder_readLocalData(osg::Object &obj, osgDB::Input &fr);
bool DatabaseBuilder_writeLocalData(const osg::Object &obj, osgDB::Output &fw);

osgDB::TemplateRegisterDotOsgWrapperProxy<DatabaseBuilderLookUps> DatabaseBuilder_Proxy
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
    
    DatabaseBuilder_Proxy.read(fr, gt, itrAdvanced);
    
    return itrAdvanced;
}

bool DatabaseBuilder_writeLocalData(const osg::Object& obj, osgDB::Output& fw)
{
    const vpb::DatabaseBuilder& db = static_cast<const vpb::DatabaseBuilder&>(obj);

    DatabaseBuilder_Proxy.write(fw, db);

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

