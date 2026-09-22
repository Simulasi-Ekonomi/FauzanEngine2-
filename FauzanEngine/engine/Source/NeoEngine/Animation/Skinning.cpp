#include "Skinning.h"
#include <algorithm>

namespace NeoEngine {
static void TransformPoint(const Mat4& m, const float* p, float* out) {
    out[0]=m.m[0]*p[0]+m.m[4]*p[1]+m.m[8]*p[2]+m.m[12];
    out[1]=m.m[1]*p[0]+m.m[5]*p[1]+m.m[9]*p[2]+m.m[13];
    out[2]=m.m[2]*p[0]+m.m[6]*p[1]+m.m[10]*p[2]+m.m[14];
}
void Skinning::ApplySkinning(std::vector<float>& vertices,const std::vector<VertexWeight>& weights,const std::vector<Mat4>& boneMatrices) {
    if (vertices.size()%3!=0 || weights.size()!=vertices.size()/3) return;
    std::vector<float> source=vertices;
    for(size_t i=0;i<weights.size();++i){
        float out[3]={0,0,0}; float total=0;
        for(int j=0;j<4;++j){ const int b=weights[i].boneIDs[j]; const float w=weights[i].weights[j]; if(w<=0 || b<0 || static_cast<size_t>(b)>=boneMatrices.size()) continue; float p[3],t[3]; p[0]=source[i*3];p[1]=source[i*3+1];p[2]=source[i*3+2];TransformPoint(boneMatrices[b],p,t);out[0]+=t[0]*w;out[1]+=t[1]*w;out[2]+=t[2]*w;total+=w; }
        if(total>0){vertices[i*3]=out[0]/total;vertices[i*3+1]=out[1]/total;vertices[i*3+2]=out[2]/total;}
    }
}
}
