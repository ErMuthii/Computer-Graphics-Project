#include "kitenge/studio.hpp"
#include "kitenge/circles.hpp"
#include "kitenge/lines.hpp"
#include "kitenge/polygon_fill.hpp"
#include "kitenge/polygon_clip.hpp"
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

namespace kitenge {
namespace {
constexpr Color paper{246,240,227}, ink{49,57,37}, brown{65,42,35};
constexpr Color muted{111,105,89}, skin{187,156,121}, gold{238,174,46};
constexpr std::array<StudioPalette,studioPaletteCount> palettes{{
    {{242,221,175},{174,65,32},{48,81,57}},
    {{228,231,216},{40,96,110},{190,113,43}},
    {{234,214,207},{110,56,86},{55,76,77}}
}};
// Small built-in bitmap alphabet keeps both desktop and headless renders free
// of font files, platform text APIs and runtime asset paths.
const char* glyph(char c) {
    static constexpr const char* letters[] = {
        "01110100011000111111100011000110001", "11110100011000111110100011000111110",
        "01111100001000010000100001000001111", "11110100011000110001100011000111110",
        "11111100001000011110100001000011111", "11111100001000011110100001000010000",
        "01111100001000010111100011000101111", "10001100011000111111100011000110001",
        "11111001000010000100001000010011111", "00111000100001000010100101001001100",
        "10001100101010011000101001001010001", "10000100001000010000100001000011111",
        "10001110111010110101100011000110001", "10001110011010110011100011000110001",
        "01110100011000110001100011000101110", "11110100011000111110100001000010000",
        "01110100011000110001101011001001101", "11110100011000111110101001001010001",
        "01111100001000001110000010000111110", "11111001000010000100001000010000100",
        "10001100011000110001100011000101110", "10001100011000110001100010101000100",
        "10001100011000110101101011101110001", "10001100010101000100010101000110001",
        "10001100010101000100001000010000100", "11111000010001000100010001000011111"
    };
    static constexpr const char* digits[] = {
        "01110100011001110101110011000101110", "00100011000010000100001000010001110",
        "01110100010000100010001000100011111", "11110000010000101110000010000111110",
        "00010001100101010010111110001000010", "11111100001000011110000010000111110",
        "01110100001000011110100011000101110", "11111000010001000100010000100001000",
        "01110100011000101110100011000101110", "01110100011000101111000010000101110"
    };
    if(c >= 'A' && c <= 'Z') return letters[c-'A'];
    if(c >= '0' && c <= '9') return digits[c-'0'];
    if(c == '-') return "00000000000000011111000000000000000";
    if(c == ':') return "00000001000010000000001000010000000";
    if(c == '/') return "00001000100001000100010000100010000";
    if(c == '+') return "00000001000010011111001000010000000";
    return "00000000000000000000000000000000000";
}
void box(Canvas& c, int x, int y, int w, int h, Color color) {
    for(int j=y;j<y+h;++j) for(int i=x;i<x+w;++i) c.setPixel(i,j,color);
}
void text(Canvas& c, const std::string& s, int x, int y, int scale, Color color) {
    for(char ch:s) {
        const char* bits = glyph(ch);
        for(int j=0;j<7;++j) for(int i=0;i<5;++i)
            if(bits[j*5+i]=='1') box(c,x+i*scale,y+j*scale,scale,scale,color);
        x+=6*scale;
    }
}
void outline(Canvas& c,const Polygon& p,Color color) {
    for(std::size_t i=0;i<p.size();++i) {
        auto a=p[i],b=p[(i+1)%p.size()];
        drawLine(c,{static_cast<int>(std::lround(a.x)),static_cast<int>(std::lround(a.y))},
                   {static_cast<int>(std::lround(b.x)),static_cast<int>(std::lround(b.y))},color);
    }
}
Polygon motif(int kind,double x,double y) {
    // Fixed starter shapes, not the team's future motif editor or matrix system.
    if(kind==1) return {{x,y-22},{x+21,y+19},{x-21,y+19}};
    if(kind==2) return {{x-22,y-10},{x-8,y-10},{x-8,y-23},{x+8,y-23},
                        {x+8,y-10},{x+22,y-10},{x+22,y+10},{x+8,y+10},
                        {x+8,y+23},{x-8,y+23},{x-8,y+10},{x-22,y+10}};
    return {{x,y-25},{x+22,y},{x,y+25},{x-22,y}};
}
void starterPattern(Canvas& c,const StudioState& state,const Polygon& boundary,Rect bounds) {
    const auto& p=palettes.at(static_cast<std::size_t>(state.palette));
    fillPolygon(c,boundary,p.background);
    for(int y=static_cast<int>(bounds.minY)-25;y<bounds.maxY+50;y+=70)
        for(int x=static_cast<int>(bounds.minX)-22;x<bounds.maxX+44;x+=64) {
            // Clipping uses the existing team's CP1 implementation.
            auto clipped=clipPolygonToConvex(motif(state.motif,x,y),boundary);
            fillPolygon(c,clipped,((x/64+y/70)%2==0)?p.primary:p.secondary);
        }
}
void mannequin(Canvas& c,const StudioState& state,const StudioHooks& hooks) {
    // Static scaffold: two convex dress panels make clipping explicit.
    fillPolygon(c,{{937,241},{963,241},{963,278},{937,278}},skin);
    fillPolygon(c,{{887,288},{900,302},{864,420},{847,412}},skin);
    fillPolygon(c,{{1000,302},{1013,288},{1053,412},{1036,420}},skin);
    fillPolygon(c,{{917,561},{943,561},{939,640},{920,640}},skin);
    fillPolygon(c,{{957,561},{983,561},{980,640},{961,640}},skin);
    for(int y=-34;y<=34;++y) {
        const int half=static_cast<int>(29*std::sqrt(1.0-double(y*y)/(35*35)));
        drawLine(c,{950-half,214+y},{950+half,214+y},skin);
    }
    drawEllipseMidpoint(c,{950,214},30,35,brown);
    box(c,910,640,31,9,brown); box(c,960,640,31,9,brown);
    const Polygon& bodice=studioBodice();
    const Polygon& skirt=studioSkirt();
    if(hooks.garment) hooks.garment(c,state);
    else {
        starterPattern(c,state,bodice,{897,278,1003,377});
        starterPattern(c,state,skirt,{859,377,1041,574});
    }
    outline(c,bodice,brown); outline(c,skirt,brown);
    box(c,914,374,72,8,brown);
}
} // namespace
StudioPalette studioPalette(int index) {
    if(index<0||index>=studioPaletteCount) throw std::out_of_range("Unknown studio palette");
    return palettes[static_cast<std::size_t>(index)];
}
const Polygon& studioFabricArea() {
    static const Polygon area{{52,202},{696,202},{696,648},{52,648}};
    return area;
}
const Polygon& studioBodice() {
    static const Polygon bodice{{897,278},{1003,278},{985,377},{915,377}};
    return bodice;
}
const Polygon& studioSkirt() {
    static const Polygon skirt{{915,377},{985,377},{1041,574},{859,574}};
    return skirt;
}
void applyAction(StudioState& s,StudioAction action) {
    switch(action) {
        case StudioAction::NextMotif: s.motif=(s.motif+1)%studioMotifCount; break;
        case StudioAction::NextPalette: s.palette=(s.palette+1)%studioPaletteCount; break;
        case StudioAction::TogglePlayback: s.playing=!s.playing; break;
        case StudioAction::Reset: s=StudioState{}; break;
    }
}
void advanceStudio(StudioState& s,double dt,const StudioHooks& hooks) {
    if(!std::isfinite(dt)||dt<0) throw std::invalid_argument("Delta time must be finite and non-negative");
    if(!s.playing) return;
    s.timeSeconds+=dt;
    if(hooks.animate) hooks.animate(s,dt);
}
void renderStudio(Canvas& c,const StudioState& s,const StudioHooks& hooks) {
    if(c.width()!=studioWidth||c.height()!=studioHeight)
        throw std::invalid_argument("Studio canvas must be 1200 x 760");
    if(s.motif<0||s.motif>=studioMotifCount||s.palette<0||s.palette>=studioPaletteCount)
        throw std::invalid_argument("Unknown starter motif or palette");
    c.clear(paper);
    box(c,0,0,studioWidth,110,brown);
    box(c,28,27,7,52,gold);
    text(c,"KITENGE PATTERN STUDIO",52,28,4,paper);
    text(c,"DESKTOP STARTER / PURE C++ CANVAS + OPENGL DISPLAY",54,75,2,gold);
    text(c,"01  FABRIC PREVIEW",40,141,3,ink);
    text(c,"02  MODEL PREVIEW",762,141,3,ink);
    box(c,40,190,668,470,{226,213,192});
    const Polygon& fabric=studioFabricArea();
    if(hooks.fabric) hooks.fabric(c,s); else starterPattern(c,s,fabric,{52,202,696,648});
    outline(c,fabric,brown);
    drawLine(c,{735,142},{735,661},{209,199,178});
    mannequin(c,s,hooks);
    text(c,"STATIC 2D MANNEQUIN",802,674,2,muted);
    const char* names[]={"DIAMOND","TRIANGLE","CROSS"};
    text(c,std::string("MOTIF: ")+names[s.motif],40,682,2,ink);
    text(c,"PALETTE: "+std::to_string(s.palette+1),390,682,2,ink);
    box(c,0,718,1200,42,brown);
    text(c,"M MOTIF   P PALETTE   R RESET   S EXPORT   ESC CLOSE",30,731,2,paper);
}
} // namespace kitenge
