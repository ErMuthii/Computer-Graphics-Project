#include "kitenge/studio.hpp"
#include "kitenge/lines.hpp"
#include "kitenge/mannequin.hpp"
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
void starterPattern(Canvas& c,const StudioState& state,const Polygon& boundary,Rect bounds) {
    // Fallback used only when no fabric/garment hook is registered. It places
    // untransformed catalog fills (strokes are left to the pattern engine).
    const Palette& p=catalogPalette(state.palette);
    const MotifShape& m=catalogMotif(state.motif);
    fillPolygon(c,boundary,p.background);
    for(int y=static_cast<int>(bounds.minY)-25;y<bounds.maxY+50;y+=70)
        for(int x=static_cast<int>(bounds.minX)-22;x<bounds.maxX+44;x+=64) {
            const bool swap=(x/64+y/70)%2!=0; // alternate primary and accent
            for(const MotifFill& f:m.fills) {
                Polygon placed;
                for(const Point2D& q:f.contour) placed.push_back({q.x+x,q.y+y});
                int role=f.role;
                if(swap&&role==kRolePrimary) role=kRoleAccent;
                else if(swap&&role==kRoleAccent) role=kRolePrimary;
                // Clipping uses the existing team's CP1 implementation.
                fillPolygon(c,clipPolygonToConvex(placed,boundary),paletteRoleColor(p,role));
            }
        }
}
void mannequin(Canvas& c,const StudioState& state,const StudioHooks& hooks) {
    // The figure and both convex dress panels come from the shared mannequin
    // module, so the outlines drawn here are the pattern's clipping boundaries.
    drawMannequinFigure(c,skin,brown);
    if(hooks.garment) hooks.garment(c,state);
    else {
        starterPattern(c,state,mannequinBodice(),polygonBounds(mannequinBodice()));
        starterPattern(c,state,mannequinSkirt(),polygonBounds(mannequinSkirt()));
    }
    drawGarmentTrim(c,brown);
}
} // namespace
StudioPalette studioPalette(int index) {
    // The palette data lives in the motif/palette catalog.
    const Palette& p=catalogPalette(index); // throws std::out_of_range
    return {p.background,p.primary,p.accent};
}
const Polygon& studioFabricArea() {
    static const Polygon area{{52,202},{696,202},{696,648},{52,648}};
    return area;
}
// Forwarders kept so existing callers keep working; the geometry itself lives
// once in the mannequin module.
const Polygon& studioBodice() { return mannequinBodice(); }
const Polygon& studioSkirt() { return mannequinSkirt(); }
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
    // Labels come from the catalog, so new motifs/palettes need no edits here.
    text(c,std::string("MOTIF: ")+catalogMotifLabel(s.motif),40,682,2,ink);
    text(c,"PALETTE: "+std::to_string(s.palette+1)+" "+catalogPaletteLabel(s.palette),390,682,2,ink);
    box(c,0,718,1200,42,brown);
    text(c,"M MOTIF   P PALETTE   R RESET   S EXPORT   ESC CLOSE",30,731,2,paper);
}
} // namespace kitenge
