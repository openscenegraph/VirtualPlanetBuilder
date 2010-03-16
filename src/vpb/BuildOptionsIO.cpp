/* -*-c++-*- VirtualPlanetBuilder - Copyright (C) 1998-2009 Robert Osfield
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

#include <vpb/BuildOptions>
#include <vpb/Serializer>

#include <iostream>
#include <string>
#include <map>
#include <set>

#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/io_utils>

#include <osgDB/ReadFile>
#include <osgDB/Registry>

using namespace vpb;

template<typename C>
class GeospatialExtentsSerializer : public vpb::Serializer
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
        
        return true;
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
        
        return true;
     }
     
     std::string        _fieldName;
     V                  _default;
     GetterFunctionType _getter;
     SetterFunctionType _setter;
};


template<typename C, typename T, typename Itr>
class SetSerializer : public vpb::Serializer
{
public:

     typedef const T& (C::*GetterFunctionType)() const;
     typedef void (C::*SetterFunctionType)(const T&);

     SetSerializer(const char* fieldName, const T& defaultValue, GetterFunctionType getter, SetterFunctionType setter):
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
            const T& value = (object.*_getter)();
            if (!value.empty())
            {
                fw.indent()<<_fieldName<<" {"<<std::endl;
                fw.moveIn();

                for(Itr itr = value.begin();
                    itr != value.end();
                    ++itr)
                {
                    fw.indent()<<*itr<<std::endl;
                }
                fw.moveOut();
                fw.indent()<<"}"<<std::endl;
            }
        }
        
        return true;
     }

    bool read(osgDB::Input& fr, osg::Object& obj, bool& itrAdvanced)
    {
        C& object = static_cast<C&>(obj);
        if (fr[0].matchWord(_fieldName.c_str()) && fr[1].isOpenBracket())
        {
            T value;

            int entry = fr[0].getNoNestedBrackets();

            fr += 2;

            while (!fr.eof() && fr[0].getNoNestedBrackets()>entry)
            {
                if (fr[0].isWord()) value.insert(fr[0].getStr());
                ++fr;
            }

            ++fr;
            
            (object.*_setter)(value);
            itrAdvanced = true;
        }
        
        return true;
     }
     
     std::string        _fieldName;
     T                  _default;
     GetterFunctionType _getter;
     SetterFunctionType _setter;
};


#define VPB_CREATE_ENUM_SERIALIZER(CLASS,PROPERTY,PROTOTYPE) \
    typedef vpb::EnumSerializer<CLASS, CLASS::PROPERTY> MySerializer;\
    osg::ref_ptr<MySerializer> serializer = new MySerializer(\
        #PROPERTY,\
        PROTOTYPE.get##PROPERTY(),\
        &CLASS::get##PROPERTY,\
        &CLASS::set##PROPERTY\
    )


#define VPB_CREATE_ENUM_SERIALIZER2(CLASS,NAME, PROPERTY,PROTOTYPE) \
    typedef vpb::EnumSerializer<CLASS, CLASS::PROPERTY> MySerializer;\
    osg::ref_ptr<MySerializer> serializer = new MySerializer(\
        #NAME,\
        PROTOTYPE.get##NAME(),\
        &CLASS::get##NAME,\
        &CLASS::set##NAME\
    )

    
#define VPB_ADD_ENUM_PROPERTY(PROPERTY) \
    VPB_CREATE_ENUM_SERIALIZER(BuildOptions, PROPERTY, prototype); \
    _serializerList.push_back(serializer.get())

#define VPB_ADD_ENUM_PROPERTY2(NAME, PROPERTY) \
    VPB_CREATE_ENUM_SERIALIZER2(BuildOptions, NAME, PROPERTY, prototype); \
    _serializerList.push_back(serializer.get())

#define VPB_ADD_STRING_PROPERTY(PROPERTY) _serializerList.push_back(VPB_CREATE_STRING_SERIALIZER(BuildOptions,PROPERTY,prototype))


#define VPB_ADD_UINT_PROPERTY(PROPERTY) _serializerList.push_back(VPB_CREATE_UINT_SERIALIZER(BuildOptions,PROPERTY,prototype))


#define VPB_ADD_INT_PROPERTY(PROPERTY) _serializerList.push_back(VPB_CREATE_INT_SERIALIZER(BuildOptions,PROPERTY,prototype))


#define VPB_ADD_FLOAT_PROPERTY(PROPERTY) _serializerList.push_back(VPB_CREATE_FLOAT_SERIALIZER(BuildOptions,PROPERTY,prototype))


#define VPB_ADD_DOUBLE_PROPERTY(PROPERTY) _serializerList.push_back(VPB_CREATE_DOUBLE_SERIALIZER(BuildOptions,PROPERTY,prototype))



#define VPB_ADD_VEC4_PROPERTY(PROPERTY) _serializerList.push_back(VPB_CREATE_VEC4_SERIALIZER(BuildOptions,PROPERTY,prototype))


#define VPB_ADD_BOOL_PROPERTY(PROPERTY) _serializerList.push_back(VPB_CREATE_BOOL_SERIALIZER(BuildOptions,PROPERTY,prototype))

#define VPB_ADD_ENUM_VALUE(VALUE) serializer->add(BuildOptions::VALUE, #VALUE)

#define VPB_ADD_ENUM_PROPERTY_TWO_VALUES(PROPERTY,VALUE1,VALUE2) \
    { \
        VPB_ADD_ENUM_PROPERTY(PROPERTY);\
        VPB_ADD_ENUM_VALUE(VALUE1);\
        VPB_ADD_ENUM_VALUE(VALUE2);\
    }

#define VPB_ADD_ENUM_PROPERTY_TWO_VALUES(PROPERTY,VALUE1,VALUE2) \
    { \
        VPB_ADD_ENUM_PROPERTY(PROPERTY);\
        VPB_ADD_ENUM_VALUE(VALUE1);\
        VPB_ADD_ENUM_VALUE(VALUE2);\
    }

#define VPB_ADD_ENUM_PROPERTY_THREE_VALUES(PROPERTY,VALUE1,VALUE2,VALUE3) \
    { \
        VPB_ADD_ENUM_PROPERTY(PROPERTY);\
        VPB_ADD_ENUM_VALUE(VALUE1);\
        VPB_ADD_ENUM_VALUE(VALUE2);\
        VPB_ADD_ENUM_VALUE(VALUE3);\
    }

#define VPB_AEV VPB_ADD_ENUM_VALUE
#define VPB_AEP VPB_ADD_ENUM_PROPERTY
#define VPB_AEP2 VPB_ADD_ENUM_PROPERTY2

#define VPB_SCOPED_AEV(SCOPE,VALUE) serializer->add(SCOPE::VALUE, #VALUE)


class BuildOptionsLookUps
{
public:

    typedef std::list< osg::ref_ptr<vpb::Serializer> > SerializerList;
    SerializerList _serializerList;

    BuildOptionsLookUps()
    {
        BuildOptions prototype;

        VPB_ADD_STRING_PROPERTY(Directory);
        VPB_ADD_BOOL_PROPERTY(OutputTaskDirectories);
        VPB_ADD_STRING_PROPERTY(DestinationTileBaseName);
        VPB_ADD_STRING_PROPERTY(DestinationTileExtension);
        VPB_ADD_STRING_PROPERTY(DestinationImageExtension);
        VPB_ADD_BOOL_PROPERTY(PowerOfTwoImages);
        VPB_ADD_STRING_PROPERTY(ArchiveName);
        VPB_ADD_STRING_PROPERTY(IntermediateBuildName);
        VPB_ADD_STRING_PROPERTY(LogFileName);
        VPB_ADD_STRING_PROPERTY(TaskFileName);
        VPB_ADD_STRING_PROPERTY(CommentString);

        VPB_ADD_ENUM_PROPERTY_TWO_VALUES(DatabaseType, LOD_DATABASE, PagedLOD_DATABASE)


        VPB_ADD_ENUM_PROPERTY_THREE_VALUES(GeometryType, HEIGHT_FIELD, POLYGONAL, TERRAIN)
        VPB_ADD_ENUM_PROPERTY_THREE_VALUES(MipMappingMode, NO_MIP_MAPPING, MIP_MAPPING_HARDWARE,MIP_MAPPING_IMAGERY)

        {
            VPB_AEP(TextureType);
            VPB_AEV(RGB_24);
            VPB_AEV(RGBA);
            VPB_AEV(RGB_16);
            VPB_AEV(RGBA_16);
            VPB_AEV(RGB_S3TC_DXT1);
            VPB_AEV(RGBA_S3TC_DXT1);
            VPB_AEV(RGBA_S3TC_DXT3);
            VPB_AEV(RGBA_S3TC_DXT5);
            VPB_AEV(ARB_COMPRESSED);
            VPB_AEV(COMPRESSED_TEXTURE);
            VPB_AEV(COMPRESSED_RGBA_TEXTURE);
        }

        VPB_ADD_UINT_PROPERTY(MaximumTileImageSize);
        VPB_ADD_UINT_PROPERTY(MaximumTileTerrainSize);

        VPB_ADD_FLOAT_PROPERTY(MaximumVisibleDistanceOfTopLevel);
        VPB_ADD_FLOAT_PROPERTY(RadiusToMaxVisibleDistanceRatio);
        VPB_ADD_FLOAT_PROPERTY(VerticalScale);
        VPB_ADD_FLOAT_PROPERTY(SkirtRatio);
        VPB_ADD_UINT_PROPERTY(ImageryQuantization);
        VPB_ADD_BOOL_PROPERTY(ImageryErrorDiffusion);
        VPB_ADD_FLOAT_PROPERTY(MaxAnisotropy);
        
        VPB_ADD_BOOL_PROPERTY(BuildOverlays);
        VPB_ADD_BOOL_PROPERTY(ReprojectSources);
        VPB_ADD_BOOL_PROPERTY(GenerateTiles);
        VPB_ADD_BOOL_PROPERTY(ConvertFromGeographicToGeocentric);
        VPB_ADD_BOOL_PROPERTY(UseLocalTileTransform);
        VPB_ADD_BOOL_PROPERTY(SimplifyTerrain);
        VPB_ADD_BOOL_PROPERTY(DecorateGeneratedSceneGraphWithCoordinateSystemNode);
        VPB_ADD_BOOL_PROPERTY(DecorateGeneratedSceneGraphWithMultiTextureControl);
        VPB_ADD_BOOL_PROPERTY(WriteNodeBeforeSimplification);

        VPB_ADD_VEC4_PROPERTY(DefaultColor);
        
        VPB_ADD_BOOL_PROPERTY(UseInterpolatedImagerySampling);
        VPB_ADD_BOOL_PROPERTY(UseInterpolatedTerrainSampling);
        
        VPB_ADD_STRING_PROPERTY(DestinationCoordinateSystem);
        VPB_ADD_STRING_PROPERTY(DestinationCoordinateSystemFormat);
        VPB_ADD_DOUBLE_PROPERTY(RadiusPolar);
        VPB_ADD_DOUBLE_PROPERTY(RadiusEquator);

        _serializerList.push_back(new GeospatialExtentsSerializer<BuildOptions>(
                "DestinationExtents",
                prototype.getDestinationExtents(),
                &BuildOptions::getDestinationExtents,
                &BuildOptions::setDestinationExtents));

        VPB_ADD_UINT_PROPERTY(MaximumNumOfLevels);
        
        VPB_ADD_UINT_PROPERTY(DistributedBuildSplitLevel);
        VPB_ADD_UINT_PROPERTY(DistributedBuildSecondarySplitLevel);
        VPB_ADD_BOOL_PROPERTY(RecordSubtileFileNamesOnLeafTile);
        VPB_ADD_BOOL_PROPERTY(GenerateSubtile);
        VPB_ADD_UINT_PROPERTY(SubtileLevel);
        VPB_ADD_UINT_PROPERTY(SubtileX);
        VPB_ADD_UINT_PROPERTY(SubtileY);

        { VPB_AEP(NotifyLevel); VPB_AEV(ALWAYS); VPB_AEV(FATAL); VPB_AEV(WARN); VPB_AEV(NOTICE); VPB_AEV(INFO); VPB_AEV(DEBUG_INFO); VPB_AEV(DEBUG_FP); }

        VPB_ADD_BOOL_PROPERTY(DisableWrites);
        
        VPB_ADD_FLOAT_PROPERTY(NumReadThreadsToCoresRatio);
        VPB_ADD_FLOAT_PROPERTY(NumWriteThreadsToCoresRatio);

        VPB_ADD_STRING_PROPERTY(BuildOptionsString);
        VPB_ADD_STRING_PROPERTY(WriteOptionsString);

        { VPB_AEP(LayerInheritance); VPB_AEV(INHERIT_LOWEST_AVAILABLE); VPB_AEV(INHERIT_NEAREST_AVAILABLE); VPB_AEV(NO_INHERITANCE); }

        VPB_ADD_BOOL_PROPERTY(AbortTaskOnError);
        VPB_ADD_BOOL_PROPERTY(AbortRunOnError);
        
        { VPB_AEP2(DefaultImageLayerOutputPolicy, LayerOutputPolicy); VPB_AEV(INLINE); VPB_AEV(EXTERNAL_LOCAL_DIRECTORY); VPB_AEV(EXTERNAL_SET_DIRECTORY); }
        { VPB_AEP2(DefaultElevationLayerOutputPolicy, LayerOutputPolicy); VPB_AEV(INLINE); VPB_AEV(EXTERNAL_LOCAL_DIRECTORY); VPB_AEV(EXTERNAL_SET_DIRECTORY); }
        
        { VPB_AEP2(OptionalImageLayerOutputPolicy, LayerOutputPolicy); VPB_AEV(INLINE); VPB_AEV(EXTERNAL_LOCAL_DIRECTORY); VPB_AEV(EXTERNAL_SET_DIRECTORY); }
        { VPB_AEP2(OptionalElevationLayerOutputPolicy, LayerOutputPolicy); VPB_AEV(INLINE); VPB_AEV(EXTERNAL_LOCAL_DIRECTORY); VPB_AEV(EXTERNAL_SET_DIRECTORY); }

        _serializerList.push_back(new SetSerializer<BuildOptions, BuildOptions::OptionalLayerSet, BuildOptions::OptionalLayerSet::const_iterator>(
                "OptionalLayerSet",
                prototype.getOptionalLayerSet(),
                &BuildOptions::getOptionalLayerSet,
                &BuildOptions::setOptionalLayerSet));

        VPB_ADD_UINT_PROPERTY(RevisionNumber);

        {
            VPB_AEP(BlendingPolicy);
            VPB_SCOPED_AEV(osgTerrain::TerrainTile,INHERIT);
            VPB_SCOPED_AEV(osgTerrain::TerrainTile,DO_NOT_SET_BLENDING);
            VPB_SCOPED_AEV(osgTerrain::TerrainTile,ENABLE_BLENDING);
            VPB_SCOPED_AEV(osgTerrain::TerrainTile,ENABLE_BLENDING_WHEN_ALPHA_PRESENT);
        }

    }

    bool read(osgDB::Input& fr, BuildOptions& db, bool& itrAdvanced)
    {
        for(SerializerList::iterator itr = _serializerList.begin();
            itr != _serializerList.end();
            ++itr)
        {
            (*itr)->read(fr,db, itrAdvanced);
        }

        return true;
    }

    bool write(osgDB::Output& fw, const BuildOptions& db)
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
//  BuildOptions IO support
//

bool BuildOptions_readLocalData(osg::Object &obj, osgDB::Input &fr);
bool BuildOptions_writeLocalData(const osg::Object &obj, osgDB::Output &fw);

osgDB::TemplateRegisterDotOsgWrapperProxy<BuildOptionsLookUps> BuildOptions_Proxy
(
    new vpb::BuildOptions,
    "BuildOptions",
    "BuildOptions Object",
    BuildOptions_readLocalData,
    BuildOptions_writeLocalData
);


bool BuildOptions_readLocalData(osg::Object& obj, osgDB::Input &fr)
{
    vpb::BuildOptions& gt = static_cast<vpb::BuildOptions&>(obj);
    bool itrAdvanced = false;
    
    BuildOptions_Proxy.read(fr, gt, itrAdvanced);
    
    return itrAdvanced;
}

bool BuildOptions_writeLocalData(const osg::Object& obj, osgDB::Output& fw)
{
    const vpb::BuildOptions& db = static_cast<const vpb::BuildOptions&>(obj);

    BuildOptions_Proxy.write(fw, db);

    return true;
}
