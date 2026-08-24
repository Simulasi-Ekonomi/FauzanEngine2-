#include "Runtime/ObjMeshImporter.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

int main(){
    using namespace NeoEngine;
    const auto sameMesh=[](const std::vector<MeshVertex>& left,const std::vector<MeshVertex>& right,const std::vector<uint16_t>& leftIndices,const std::vector<uint16_t>& rightIndices){if(left.size()!=right.size()||leftIndices!=rightIndices)return false;for(size_t index=0U;index<left.size();++index){const MeshVertex& a=left[index];const MeshVertex& b=right[index];if(a.position.x!=b.position.x||a.position.y!=b.position.y||a.position.z!=b.position.z||a.normal.x!=b.normal.x||a.normal.y!=b.normal.y||a.normal.z!=b.normal.z||a.u!=b.u||a.v!=b.v)return false;}return true;};
    constexpr const char* quad="v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nvn 0 0 1\nf 1/1/1 2/2/1 3/3/1 4/4/1\n";
    ObjMeshImporter importer;std::vector<MeshVertex> vertices;std::vector<uint16_t> indices;
    if(!importer.Import(quad,vertices,indices)||vertices.size()!=4U||indices.size()!=6U)return 1;
    bool expectedUv=false;for(const MeshVertex& vertex:vertices){if(std::fabs(vertex.normal.z-1.0F)>0.0001F)return 1;if(std::fabs(vertex.u-1.0F)<=0.0001F&&std::fabs(vertex.v-1.0F)<=0.0001F)expectedUv=true;}if(!expectedUv)return 1;
    const std::vector<MeshVertex> preservedVertices=vertices;const std::vector<uint16_t> preservedIndices=indices;
    if(importer.Import("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 4\n",vertices,indices)||!sameMesh(vertices,preservedVertices,indices,preservedIndices))return 1;
    constexpr const char* missingNormal="v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    if(importer.Import(missingNormal,vertices,indices)||importer.LastError()!=ObjMeshImportError::MissingNormal||!sameMesh(vertices,preservedVertices,indices,preservedIndices))return 1;
    if(!importer.Import(missingNormal,vertices,indices,{true})||vertices.size()!=3U||indices.size()!=3U||std::fabs(vertices[0].normal.z-1.0F)>0.0001F)return 1;
    const std::vector<MeshVertex> flatVertices=vertices;const std::vector<uint16_t> flatIndices=indices;
    if(importer.Import("v 0 0 0\nv 1 0 0\nv 2 0 0\nf 1 2 3\n",vertices,indices,{true})||importer.LastError()!=ObjMeshImportError::DegenerateTriangle||!sameMesh(vertices,flatVertices,indices,flatIndices))return 1;
    vertices=preservedVertices;indices=preservedIndices;
    if(importer.Import("v 0 0 0\nv 1 0 0\nl 1 2\n",vertices,indices)||importer.LastError()!=ObjMeshImportError::UnsupportedPrimitive||!sameMesh(vertices,preservedVertices,indices,preservedIndices))return 1;
    if(importer.Import(std::string(ObjMeshImporter::kMaxSourceBytes+1U,' '),vertices,indices)||importer.LastError()!=ObjMeshImportError::SourceTooLarge||!sameMesh(vertices,preservedVertices,indices,preservedIndices))return 1;
    std::string capacity;for(uint32_t vertex=0U;vertex<2049U;++vertex)capacity+="v "+std::to_string(vertex)+" 0 0\n";capacity+="vn 0 0 1\n";for(uint32_t vertex=1U;vertex<=2049U;vertex+=3U)capacity+="f "+std::to_string(vertex)+"//1 "+std::to_string(vertex+1U)+"//1 "+std::to_string(vertex+2U)+"//1\n";
    if(importer.Import(capacity,vertices,indices)||importer.LastError()!=ObjMeshImportError::Capacity||!sameMesh(vertices,preservedVertices,indices,preservedIndices))return 1;
    std::printf("OBJ_MESH_IMPORTER_SMOKE_OK triangulated=2 vertices=%zu indices=%zu atomic=1\n",preservedVertices.size(),preservedIndices.size());
}
