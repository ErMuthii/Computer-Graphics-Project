#include "kitenge/studio.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
bool same(kitenge::Color a,kitenge::Color b){return a.red==b.red&&a.green==b.green&&a.blue==b.blue;}
bool sameImage(const kitenge::Canvas& a,const kitenge::Canvas& b){
    for(int y=0;y<a.height();++y)for(int x=0;x<a.width();++x)
        if(!same(a.pixelAt(x,y),b.pixelAt(x,y)))return false;
    return true;
}
bool regionChanged(const kitenge::Canvas& a,const kitenge::Canvas& b,int x0,int y0,int x1,int y1){
    for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x)
        if(!same(a.pixelAt(x,y),b.pixelAt(x,y)))return true;
    return false;
}
}
int main(){
    try{
        using namespace kitenge;
        StudioState s;
        Canvas initial(studioWidth,studioHeight,{0,0,0}),updated(studioWidth,studioHeight,{0,0,0});
        renderStudio(initial,s);
        applyAction(s,StudioAction::NextMotif);renderStudio(updated,s);
        require(!sameImage(initial,updated),"Motif control must change the rendered scene");
        require(regionChanged(initial,updated,900,285,1000,370),"Motif must update the garment");
        applyAction(s,StudioAction::Reset);applyAction(s,StudioAction::NextPalette);renderStudio(updated,s);
        require(regionChanged(initial,updated,60,210,690,640),"Palette must update fabric");
        require(regionChanged(initial,updated,900,285,1000,370),"Palette must update garment");
        for(int i=0;i<9;++i){s.motif=i%3;s.palette=i/3;renderStudio(updated,s);}
        applyAction(s,StudioAction::Reset);renderStudio(updated,s);
        require(sameImage(initial,updated),"Reset must restore the initial image");
        advanceStudio(s,0.5);require(s.timeSeconds==0,"Paused clock must stay still");
        applyAction(s,StudioAction::TogglePlayback);
        StudioHooks hooks;bool called=false;
        hooks.animate=[&](StudioState&,double dt){called=dt==0.5;};
        advanceStudio(s,0.5,hooks);require(called&&s.timeSeconds==0.5,"Animation hook must receive time");
        hooks.fabric=[](Canvas& c,const StudioState&){c.setPixel(100,250,{1,2,3});};
        hooks.garment=[](Canvas& c,const StudioState&){c.setPixel(950,330,{3,2,1});};
        renderStudio(updated,s,hooks);
        require(same(updated.pixelAt(100,250),{1,2,3}),"Fabric hook must render");
        require(same(updated.pixelAt(950,330),{3,2,1}),"Garment hook must render");
        bool rejected=false;try{advanceStudio(s,std::numeric_limits<double>::quiet_NaN());}
        catch(const std::invalid_argument&){rejected=true;} require(rejected,"Invalid time must be rejected");
        std::cout<<"Studio scene, controls, reset and integration hooks passed.\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
