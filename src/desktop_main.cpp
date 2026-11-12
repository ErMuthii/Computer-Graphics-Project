#define GLFW_INCLUDE_NONE
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>
#include "kitenge/studio.hpp"
#include "kitenge/ppm.hpp"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct App {
    kitenge::StudioState state;
    kitenge::StudioHooks hooks;
    bool dirty=true;
    bool exportRequested=false;
    std::filesystem::path exportPath="outputs/studio.ppm";
};
void key(GLFWwindow* w,int key,int,int action,int) {
    if(action!=GLFW_PRESS) return;
    auto& a=*static_cast<App*>(glfwGetWindowUserPointer(w));
    using kitenge::StudioAction;
    switch(key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w,GLFW_TRUE); return;
        case GLFW_KEY_M: kitenge::applyAction(a.state,StudioAction::NextMotif); break;
        case GLFW_KEY_P: kitenge::applyAction(a.state,StudioAction::NextPalette); break;
        case GLFW_KEY_R: kitenge::applyAction(a.state,StudioAction::Reset); break;
        case GLFW_KEY_S: a.exportRequested=true; break;
        // The animation owner connects Space/playback after supplying animate().
        default: return;
    }
    a.dirty=true;
}
GLuint shader(GLenum kind,const char* source) {
    GLuint id=glCreateShader(kind);
    glShaderSource(id,1,&source,nullptr); glCompileShader(id);
    GLint ok=0; glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
    if(!ok) {
        char log[4096]{};glGetShaderInfoLog(id,sizeof(log),nullptr,log);
        glDeleteShader(id);throw std::runtime_error(std::string("Shader compilation failed: ")+log);
    }
    return id;
}
class Presenter {
    GLuint program_=0,vao_=0,vbo_=0,texture_=0;
public:
    Presenter() {
        const char* vs=R"(#version 330 core
layout(location=0) in vec2 pos;
layout(location=1) in vec2 uv;
out vec2 texCoord;
void main(){ gl_Position=vec4(pos,0,1); texCoord=uv; }
)";
        const char* fs=R"(#version 330 core
in vec2 texCoord;
uniform sampler2D canvasTexture;
out vec4 color;
void main(){ color=texture(canvasTexture,texCoord); }
)";
        auto v=shader(GL_VERTEX_SHADER,vs);
        GLuint f=0;
        try {f=shader(GL_FRAGMENT_SHADER,fs);} catch(...) {glDeleteShader(v);throw;}
        program_=glCreateProgram();glAttachShader(program_,v);glAttachShader(program_,f);
        glLinkProgram(program_);glDeleteShader(v);glDeleteShader(f);
        GLint linked=0;glGetProgramiv(program_,GL_LINK_STATUS,&linked);
        if(!linked) {
            char log[4096]{};glGetProgramInfoLog(program_,sizeof(log),nullptr,log);
            glDeleteProgram(program_);program_=0;
            throw std::runtime_error(std::string("Shader link failed: ")+log);
        }
        // Top of the quad uses v=0 because Canvas stores its first row at the top.
        const float vertices[]={-1,-1,0,1, 1,-1,1,1, 1,1,1,0,
                                -1,-1,0,1, 1,1,1,0, -1,1,0,0};
        glGenVertexArrays(1,&vao_);glBindVertexArray(vao_);
        glGenBuffers(1,&vbo_);glBindBuffer(GL_ARRAY_BUFFER,vbo_);
        glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),nullptr);
        glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),reinterpret_cast<void*>(2*sizeof(float)));
        glGenTextures(1,&texture_);glBindTexture(GL_TEXTURE_2D,texture_);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,kitenge::studioWidth,kitenge::studioHeight,0,GL_RGB,GL_UNSIGNED_BYTE,nullptr);
        glUseProgram(program_);glUniform1i(glGetUniformLocation(program_,"canvasTexture"),0);
    }
    ~Presenter() {
        glDeleteTextures(1,&texture_);glDeleteBuffers(1,&vbo_);
        glDeleteVertexArrays(1,&vao_);glDeleteProgram(program_);
    }
    void upload(const kitenge::Canvas& canvas) {
        static_assert(sizeof(kitenge::Color)==3,"Canvas RGB upload requires packed RGB bytes");
        glBindTexture(GL_TEXTURE_2D,texture_);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,canvas.width(),canvas.height(),GL_RGB,GL_UNSIGNED_BYTE,canvas.pixels().data());
    }
    void draw(int w,int h) {
        glViewport(0,0,w,h);glClearColor(0.09f,0.10f,0.08f,1);glClear(GL_COLOR_BUFFER_BIT);
        const double scale=std::min(double(w)/kitenge::studioWidth,double(h)/kitenge::studioHeight);
        int vw=static_cast<int>(kitenge::studioWidth*scale),vh=static_cast<int>(kitenge::studioHeight*scale);
        glViewport((w-vw)/2,(h-vh)/2,vw,vh);
        glUseProgram(program_);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture_);
        glBindVertexArray(vao_);glDrawArrays(GL_TRIANGLES,0,6);
    }
};
void capture(const std::filesystem::path& path,int w,int h) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(w)*h*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadBuffer(GL_BACK);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    kitenge::Canvas result(w,h,{0,0,0});
    for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
        auto i=(static_cast<std::size_t>(h-1-y)*w+x)*3;
        result.setPixel(x,y,{pixels[i],pixels[i+1],pixels[i+2]});
    }
    kitenge::writePpm(result,path);
}
} // namespace
int main(int argc,char** argv) {
    bool smoke=false;
    std::filesystem::path capturePath;
    App app;
    for(int i=1;i<argc;++i) {
        std::string arg=argv[i];
        if(arg=="--smoke-test") smoke=true;
        else if((arg=="--capture"||arg=="--export")&&i+1<argc) {
            if(arg=="--capture") capturePath=argv[++i]; else app.exportPath=argv[++i];
        } else {std::cerr<<"Usage: kitenge_studio [--smoke-test] [--capture frame.ppm] [--export image.ppm]\n";return 1;}
    }
    glfwSetErrorCallback([](int code,const char* message){std::cerr<<"GLFW "<<code<<": "<<message<<'\n';});
    if(!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GLFW_TRUE);
#endif
    GLFWwindow* window=glfwCreateWindow(1200,760,"Kitenge Pattern Studio - desktop starter",nullptr,nullptr);
    if(!window){glfwTerminate();return 1;}
    glfwMakeContextCurrent(window);glfwSwapInterval(1);
    glfwSetWindowSizeLimits(window,720,456,GLFW_DONT_CARE,GLFW_DONT_CARE);
    glfwSetWindowUserPointer(window,&app);glfwSetKeyCallback(window,key);
    int result=0;
    try {
#ifndef __APPLE__
        glewExperimental=GL_TRUE;
        if(glewInit()!=GLEW_OK) throw std::runtime_error("Cannot initialize GLEW");
        while(glGetError()!=GL_NO_ERROR) {} // GLEW can set INVALID_ENUM on core contexts.
#endif
        std::cout<<"OpenGL "<<glGetString(GL_VERSION)<<"\nRenderer: "<<glGetString(GL_RENDERER)
                 <<"\nM motif / P palette / R reset / S export / Esc close\n"<<std::flush;
        Presenter presenter;
        kitenge::Canvas canvas(kitenge::studioWidth,kitenge::studioHeight,{0,0,0});
        double previous=glfwGetTime();int frames=0;
        while(!glfwWindowShouldClose(window)) {
            if(smoke&&frames==1) {
                key(window,GLFW_KEY_M,0,GLFW_PRESS,0);
                key(window,GLFW_KEY_P,0,GLFW_PRESS,0);
                if(app.state.motif!=1||app.state.palette!=1)
                    throw std::runtime_error("Selection callbacks failed");
            }
            if(smoke&&frames==3) {
                key(window,GLFW_KEY_R,0,GLFW_PRESS,0);
                if(app.state.motif!=0||app.state.palette!=0)
                    throw std::runtime_error("Reset callback failed");
                glfwSetWindowSize(window,960,700);
            }
            glfwPollEvents();double now=glfwGetTime();
            kitenge::advanceStudio(app.state,std::min(now-previous,0.1),app.hooks);previous=now;
            if(app.dirty||app.state.playing) {
                kitenge::renderStudio(canvas,app.state,app.hooks);presenter.upload(canvas);app.dirty=false;
            }
            if(app.exportRequested) {
                try {kitenge::writePpm(canvas,app.exportPath);std::cout<<"Exported "<<app.exportPath<<'\n';}
                catch(const std::exception& e){std::cerr<<"Export failed: "<<e.what()<<'\n';}
                app.exportRequested=false;
            }
            int w,h;glfwGetFramebufferSize(window,&w,&h);
            if(w>0&&h>0) {
                presenter.draw(w,h);
                if(!capturePath.empty()&&frames==(smoke?5:2)) capture(capturePath,w,h);
                if(auto error=glGetError();error!=GL_NO_ERROR)
                    throw std::runtime_error("OpenGL error "+std::to_string(error));
                glfwSwapBuffers(window);++frames;
            } else glfwWaitEventsTimeout(0.1);
            if(smoke&&frames>=6) break;
        }
        if(smoke) std::cout<<"Desktop smoke test passed: shaders, upload, selection/reset callbacks, resize and presentation.\n";
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';result=1;}
    glfwDestroyWindow(window);glfwTerminate();return result;
}
