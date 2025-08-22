#include "map2d_control.hpp"
using namespace godot;

void Map2DControl::_ready() {
    queue_redraw();;
}

void Map2DControl::_draw() {
    const int step = 64;
    Vector2 s = get_size();
    for (int x = 0; x < (int)s.x; x += step) {
        draw_line(Vector2(x,0), Vector2(x,s.y), Color(0.2,0.2,0.2), 1.0);
    }
    for (int y = 0; y < (int)s.y; y += step) {
        draw_line(Vector2(0,y), Vector2(s.x,y), Color(0.2,0.2,0.2), 1.0);
    }
}

void Map2DControl::_bind_methods() {}
