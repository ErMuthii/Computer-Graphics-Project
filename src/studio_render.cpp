#include "kitenge/studio.hpp"
#include "kitenge/pattern.hpp"
#include "kitenge/ppm.hpp"
#include <exception>
#include <iostream>
int main(int argc,char** argv) {
    if(argc>2) { std::cerr<<"Usage: studio_render [output.ppm]\n"; return 1; }
    try {
        kitenge::Canvas canvas(kitenge::studioWidth,kitenge::studioHeight,{0,0,0});
        kitenge::renderStudio(canvas,{},kitenge::makePatternHooks());
        const char* path=argc==2?argv[1]:"outputs/studio.ppm";
        kitenge::writePpm(canvas,path);
        std::cout<<"Saved "<<path<<'\n';
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
