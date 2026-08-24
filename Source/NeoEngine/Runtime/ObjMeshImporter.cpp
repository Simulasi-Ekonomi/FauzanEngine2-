#include "ObjMeshImporter.h"

#include "ThirdParty/tiny_obj_loader.h"

#include <cmath>
#include <map>
#include <tuple>

namespace NeoEngine {
namespace {
bool Finite(float value){return std::isfinite(value);}
bool Read3(const std::vector<tinyobj::real_t>& values,int index,RenderPoint3& output){
    if(index<0||values.size()%3U!=0U||static_cast<size_t>(index)>=values.size()/3U)return false;
    const size_t base=static_cast<size_t>(index)*3U;output={static_cast<float>(values[base]),static_cast<float>(values[base+1U]),static_cast<float>(values[base+2U])};return Finite(output.x)&&Finite(output.y)&&Finite(output.z);
}
bool Read2(const std::vector<tinyobj::real_t>& values,int index,float& u,float& v){
    if(index<0||values.size()%2U!=0U||static_cast<size_t>(index)>=values.size()/2U)return false;
    const size_t base=static_cast<size_t>(index)*2U;u=static_cast<float>(values[base]);v=static_cast<float>(values[base+1U]);return Finite(u)&&Finite(v);
}
bool FlatNormal(const RenderPoint3& a,const RenderPoint3& b,const RenderPoint3& c,RenderPoint3& output){
    const float abx=b.x-a.x,aby=b.y-a.y,abz=b.z-a.z,acx=c.x-a.x,acy=c.y-a.y,acz=c.z-a.z;
    const float nx=aby*acz-abz*acy,ny=abz*acx-abx*acz,nz=abx*acy-aby*acx,lengthSquared=nx*nx+ny*ny+nz*nz;
    if(!Finite(lengthSquared)||lengthSquared<=0.000000000001F)return false;const float inverseLength=1.0F/std::sqrt(lengthSquared);output={nx*inverseLength,ny*inverseLength,nz*inverseLength};return Finite(output.x)&&Finite(output.y)&&Finite(output.z);
}
}
bool ObjMeshImporter::Import(std::string_view source,std::vector<MeshVertex>& vertices,std::vector<uint16_t>& indices,const ObjMeshImportOptions& options){
    if(source.empty()){lastError_=ObjMeshImportError::EmptySource;return false;}
    if(source.size()>kMaxSourceBytes){lastError_=ObjMeshImportError::SourceTooLarge;return false;}
    tinyobj::ObjReaderConfig config{};config.triangulate=true;tinyobj::ObjReader reader;
    if(!reader.ParseFromString(std::string(source),{},config)){lastError_=ObjMeshImportError::ParseFailed;return false;}
    const tinyobj::attrib_t& attributes=reader.GetAttrib();std::vector<MeshVertex> pendingVertices;std::vector<uint16_t> pendingIndices;std::map<std::tuple<int,int,int,size_t>,uint16_t> deduplicated;size_t faceOrdinal=0U;
    for(const tinyobj::shape_t& shape:reader.GetShapes()){
        if(!shape.lines.indices.empty()||!shape.points.indices.empty()){lastError_=ObjMeshImportError::UnsupportedPrimitive;return false;}
        size_t offset=0U;
        for(const unsigned char faceSize:shape.mesh.num_face_vertices){
            if(faceSize!=3U||offset>shape.mesh.indices.size()||shape.mesh.indices.size()-offset<3U){lastError_=ObjMeshImportError::ParseFailed;return false;}
            if(pendingIndices.size()>MeshRenderer::kMaxIndices-3U){lastError_=ObjMeshImportError::Capacity;return false;}
            const tinyobj::index_t& first=shape.mesh.indices[offset],&second=shape.mesh.indices[offset+1U],&third=shape.mesh.indices[offset+2U];const bool missingFirst=first.normal_index<0,missingSecond=second.normal_index<0,missingThird=third.normal_index<0;const bool flat=missingFirst||missingSecond||missingThird;
            if(flat&&(!missingFirst||!missingSecond||!missingThird)){lastError_=ObjMeshImportError::InvalidAttribute;return false;}
            if(flat&&!options.generateFlatNormals){lastError_=ObjMeshImportError::MissingNormal;return false;}
            RenderPoint3 generatedNormal{};if(flat){RenderPoint3 a{},b{},c{};if(!Read3(attributes.vertices,first.vertex_index,a)||!Read3(attributes.vertices,second.vertex_index,b)||!Read3(attributes.vertices,third.vertex_index,c)){lastError_=ObjMeshImportError::InvalidAttribute;return false;}if(!FlatNormal(a,b,c,generatedNormal)){lastError_=ObjMeshImportError::DegenerateTriangle;return false;}}
            for(size_t corner=0U;corner<3U;++corner){
                const tinyobj::index_t index=shape.mesh.indices[offset+corner];const auto key=std::make_tuple(index.vertex_index,index.normal_index,index.texcoord_index,flat?faceOrdinal:0U);auto found=deduplicated.find(key);uint16_t vertexIndex=0U;
                if(found==deduplicated.end()){
                    if(pendingVertices.size()>=MeshRenderer::kMaxVertices){lastError_=ObjMeshImportError::Capacity;return false;}
                    MeshVertex vertex{};if(!Read3(attributes.vertices,index.vertex_index,vertex.position)){lastError_=ObjMeshImportError::InvalidAttribute;return false;}
                    if(flat)vertex.normal=generatedNormal;else if(!Read3(attributes.normals,index.normal_index,vertex.normal)){lastError_=ObjMeshImportError::InvalidAttribute;return false;}
                    if(index.texcoord_index>=0&&!Read2(attributes.texcoords,index.texcoord_index,vertex.u,vertex.v)){lastError_=ObjMeshImportError::InvalidAttribute;return false;}
                    vertexIndex=static_cast<uint16_t>(pendingVertices.size());pendingVertices.push_back(vertex);deduplicated.emplace(key,vertexIndex);
                }else vertexIndex=found->second;
                pendingIndices.push_back(vertexIndex);
            }
            offset+=3U;++faceOrdinal;
        }
        if(offset!=shape.mesh.indices.size()){lastError_=ObjMeshImportError::ParseFailed;return false;}
    }
    if(pendingVertices.empty()||pendingIndices.empty()){lastError_=ObjMeshImportError::EmptyMesh;return false;}
    vertices=std::move(pendingVertices);indices=std::move(pendingIndices);lastError_=ObjMeshImportError::None;return true;
}
} // namespace NeoEngine
