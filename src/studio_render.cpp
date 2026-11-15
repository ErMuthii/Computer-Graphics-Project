#include "kitenge/studio.hpp"
#include "kitenge/controls.hpp"
#include "kitenge/pattern.hpp"
#include "kitenge/ppm.hpp"
#include <exception>
#include <iostream>
#include <string>
int main(int argc,char** argv) {
    try {
        // Accepts the same scene options as kitenge_studio, so a headless image
        // can be compared with a desktop capture at equal state and settings.
        kitenge::PatternSceneOptions options;
        std::string path="outputs/studio.ppm";
        bool havePath=false;
        for(int i=1;i<argc;) {
            if(kitenge::parsePatternSceneOption(argc,argv,i,options)) continue;
            const std::string arg=argv[i];
            if(havePath||(!arg.empty()&&arg[0]=='-')) {
                std::cerr<<"Usage: studio_render [output.ppm] [options]\n"
                         <<kitenge::patternSceneOptionsUsage();
                return 1;
            }
            path=arg;havePath=true;++i;
        }
        // Same control state, hooks and header readout as the desktop window,
        // so a desktop capture and this image stay pixel-identical.
        const kitenge::ControlState controls=kitenge::controlStateFrom(options);
        kitenge::Canvas canvas(kitenge::studioWidth,kitenge::studioHeight,{0,0,0});
        kitenge::renderStudio(canvas,controls.studio,kitenge::makeControlledHooks(controls));
        kitenge::drawControlStatus(canvas,controls);
        kitenge::writePpm(canvas,path);
        std::cout<<"Saved "<<path<<'\n';
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
