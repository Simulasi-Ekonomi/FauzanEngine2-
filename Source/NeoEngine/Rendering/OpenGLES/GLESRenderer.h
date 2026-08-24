#pragma once
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <vector>
#include <cmath>
#include <cstdio>

namespace NeoEngine {

struct Vertex {
    float x,y,z, nx,ny,nz, u,v;
};

class GLESRenderer {
public:
    GLESRenderer() = default;
    ~GLESRenderer() { Shutdown(); }

    bool Initialize(int w, int h) {
        m_W=w; m_H=h;
        m_Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(m_Display, nullptr, nullptr);
        EGLint att[]={EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};
        EGLConfig cfg; EGLint n;
        eglChooseConfig(m_Display,att,&cfg,1,&n);
        EGLint ctxAtt[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
        m_Ctx = eglCreateContext(m_Display,cfg,EGL_NO_CONTEXT,ctxAtt);
        EGLint pb[]={EGL_WIDTH,w,EGL_HEIGHT,h,EGL_NONE};
        m_Surf = eglCreatePbufferSurface(m_Display,cfg,pb);
        eglMakeCurrent(m_Display,m_Surf,m_Surf,m_Ctx);
        glViewport(0,0,w,h); glEnable(GL_DEPTH_TEST);
        MakeShaders(); MakeAllGeom();
        m_Init=true; return true;
    }

    void Shutdown() {
        if(m_Display!=EGL_NO_DISPLAY) {
            eglMakeCurrent(m_Display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
            if(m_Ctx) eglDestroyContext(m_Display,m_Ctx);
            if(m_Surf) eglDestroySurface(m_Display,m_Surf);
            eglTerminate(m_Display);
        }
    }

    void Render(int shapeIdx) {
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        float c[]={0.2f,0.3f,0.4f,1.f}; glClearBufferfv(GL_COLOR,0,c);
        if(shapeIdx>=0 && shapeIdx<(int)m_Geom.size()) DrawGeo(m_Geom[shapeIdx]);
        glFinish();
    }

    void RenderAll(float) {
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        float c[]={0.15f,0.15f,0.2f,1.f}; glClearBufferfv(GL_COLOR,0,c);
        for(auto& g:m_Geom) DrawGeo(g);
        glFinish();
    }

private:
    struct Geo { GLuint vao,vbo,ebo; int idxCount; float px,py,pz; };
    GLuint m_Prog;
    EGLDisplay m_Display=EGL_NO_DISPLAY;
    EGLContext m_Ctx=EGL_NO_CONTEXT;
    EGLSurface m_Surf=EGL_NO_SURFACE;
    int m_W,m_H; bool m_Init=false;
    std::vector<Geo> m_Geom;

    void MakeShaders() {
        const char* vs=R"(#version 300 es
in vec3 aPos; in vec3 aNorm; in vec2 aTex;
uniform mat4 uMVP; out vec3 vNorm; out vec2 vTex;
void main(){gl_Position=uMVP*vec4(aPos,1.0);vNorm=aNorm;vTex=aTex;})";
        const char* fs=R"(#version 300 es
precision highp float;
in vec3 vNorm; in vec2 vTex; out vec4 fc;
void main(){
vec3 ld=normalize(vec3(1,2,3));
float diff=max(dot(normalize(vNorm),ld),0.0);
vec3 col=vec3(0.8,0.6,0.3)*(0.3+diff*0.7);
fc=vec4(col,1.0);})";
        GLuint vsid=glCreateShader(GL_VERTEX_SHADER); glShaderSource(vsid,1,&vs,0); glCompileShader(vsid);
        GLuint fsid=glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fsid,1,&fs,0); glCompileShader(fsid);
        m_Prog=glCreateProgram(); glAttachShader(m_Prog,vsid); glAttachShader(m_Prog,fsid); glLinkProgram(m_Prog);
        glDeleteShader(vsid); glDeleteShader(fsid);
    }

    void MakeAllGeom() {
        float pos[6][3]={{-2,0,0},{0,0,0},{2,0,0},{-2,0,3},{0,0,3},{2,0,3}};
        for(int i=0;i<6;i++){ Geo g; g.px=pos[i][0]; g.py=pos[i][1]; g.pz=pos[i][2];
            switch(i){case 0:Cube(g);break;case 1:Sphere(g,1.f,32);break;case 2:Torus(g,.5f,1.f,32,16);break;case 3:Pyramid(g,1.f);break;case 4:Cylinder(g,1.f,2.f,32);break;case 5:Cone(g,1.f,2.f,32);break;}
            m_Geom.push_back(g);
        }
    }

    void Upload(Geo& g, const std::vector<Vertex>& v, const std::vector<unsigned>& idx) {
        glGenVertexArrays(1,&g.vao); glGenBuffers(1,&g.vbo); glGenBuffers(1,&g.ebo);
        glBindVertexArray(g.vao);
        glBindBuffer(GL_ARRAY_BUFFER,g.vbo); glBufferData(GL_ARRAY_BUFFER,v.size()*sizeof(Vertex),v.data(),GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,g.ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*sizeof(unsigned),idx.data(),GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)(3*sizeof(float)));
        glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)(6*sizeof(float)));
        g.idxCount = (int)idx.size();
    }

    void Cube(Geo& g) {
        std::vector<Vertex> v={/*Front*/{-1,-1,1,0,0,1,0,0},{1,-1,1,0,0,1,1,0},{1,1,1,0,0,1,1,1},{-1,1,1,0,0,1,0,1},
                              /*Back*/{1,-1,-1,0,0,-1,0,0},{-1,-1,-1,0,0,-1,1,0},{-1,1,-1,0,0,-1,1,1},{1,1,-1,0,0,-1,0,1},
                              /*Top*/{-1,1,1,0,1,0,0,0},{1,1,1,0,1,0,1,0},{1,1,-1,0,1,0,1,1},{-1,1,-1,0,1,0,0,1},
                              /*Bottom*/{-1,-1,-1,0,-1,0,0,0},{1,-1,-1,0,-1,0,1,0},{1,-1,1,0,-1,0,1,1},{-1,-1,1,0,-1,0,0,1},
                              /*Right*/{1,-1,1,1,0,0,0,0},{1,-1,-1,1,0,0,1,0},{1,1,-1,1,0,0,1,1},{1,1,1,1,0,0,0,1},
                              /*Left*/{-1,-1,-1,-1,0,0,0,0},{-1,-1,1,-1,0,0,1,0},{-1,1,1,-1,0,0,1,1},{-1,1,-1,-1,0,0,0,1}};
        std::vector<unsigned> idx={0,1,2,0,2,3,4,5,6,4,6,7,8,9,10,8,10,11,12,13,14,12,14,15,16,17,18,16,18,19,20,21,22,20,22,23};
        Upload(g,v,idx);
    }

    void Sphere(Geo& g, float r, int seg) {
        std::vector<Vertex> v; std::vector<unsigned> idx;
        for(int y=0;y<=seg;y++) for(int x=0;x<=seg;x++) {
            float xs=(float)x/seg, ys=(float)y/seg;
            float px=cos(xs*2*M_PI)*sin(ys*M_PI), py=cos(ys*M_PI), pz=sin(xs*2*M_PI)*sin(ys*M_PI);
            v.push_back({px*r,py*r,pz*r,px,py,pz,xs,ys});
        }
        for(int y=0;y<seg;y++) for(int x=0;x<seg;x++) {
            unsigned i0=y*(seg+1)+x, i1=i0+1, i2=i0+seg+1, i3=i2+1;
            idx.push_back(i0); idx.push_back(i1); idx.push_back(i2);
            idx.push_back(i1); idx.push_back(i3); idx.push_back(i2);
        }
        Upload(g,v,idx);
    }

    void Torus(Geo& g, float ir, float orr, int sides, int rings) {
        std::vector<Vertex> v; std::vector<unsigned> idx;
        for(int i=0;i<=rings;i++) {
            float th=i*2*M_PI/rings, ct=cos(th), st=sin(th);
            for(int j=0;j<=sides;j++) {
                float ph=j*2*M_PI/sides, cp=cos(ph), sp=sin(ph);
                float x=(orr+ir*cp)*ct, y=ir*sp, z=(orr+ir*cp)*st;
                v.push_back({x,y,z, cp*ct,sp,cp*st, (float)i/rings,(float)j/sides});
            }
        }
        for(int i=0;i<rings;i++) for(int j=0;j<sides;j++) {
            unsigned i0=i*(sides+1)+j, i1=i0+1, i2=i0+sides+1, i3=i2+1;
            idx.push_back(i0); idx.push_back(i1); idx.push_back(i2);
            idx.push_back(i1); idx.push_back(i3); idx.push_back(i2);
        }
        Upload(g,v,idx);
    }

    void Pyramid(Geo& g, float s) {
        std::vector<Vertex> v={/*base*/{-s,0,s,0,-1,0,0,0},{s,0,s,0,-1,0,1,0},{s,0,-s,0,-1,0,1,1},{-s,0,-s,0,-1,0,0,1},/*tip*/{0,s,0,0,0,1,0,0}};
        std::vector<unsigned> idx={0,1,2,0,2,3, 0,1,4,1,2,4,2,3,4,3,0,4};
        Upload(g,v,idx);
    }

    void Cylinder(Geo& g, float r, float h, int seg) {
        std::vector<Vertex> v; std::vector<unsigned> idx;
        for(int i=0;i<=seg;i++) {
            float ang=i*2*M_PI/seg; float x=cos(ang)*r, z=sin(ang)*r;
            v.push_back({x,-h/2,z,x,0,z,(float)i/seg,0});
            v.push_back({x, h/2,z,x,0,z,(float)i/seg,1});
        }
        for(int i=0;i<seg;i++) {
            unsigned i0=i*2, i1=i*2+1, i2=i0+2, i3=i0+3;
            idx.push_back(i0); idx.push_back(i1); idx.push_back(i2);
            idx.push_back(i1); idx.push_back(i3); idx.push_back(i2);
        }
        Upload(g,v,idx);
    }

    void Cone(Geo& g, float r, float h, int seg) {
        std::vector<Vertex> v={{0,h/2,0,0,1,0,0.5f,0.5f}};
        std::vector<unsigned> idx;
        for(int i=0;i<=seg;i++) {
            float ang=i*2*M_PI/seg; float x=cos(ang)*r, z=sin(ang)*r;
            v.push_back({x,-h/2,z,x,0,z,(float)i/seg,1});
        }
        for(int i=0;i<seg;i++) {
            idx.push_back(0); idx.push_back(i+1); idx.push_back(i+2);
        }
        Upload(g,v,idx);
    }

    void DrawGeo(Geo& g) {
        glUseProgram(m_Prog); glBindVertexArray(g.vao);
        glDrawElements(GL_TRIANGLES, g.idxCount, GL_UNSIGNED_INT, 0);
    }
};

} // namespace NeoEngine
