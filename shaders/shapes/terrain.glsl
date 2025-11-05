#ifndef SHAPE_TERRAIN_GLSL
#define SHAPE_TERRAIN_GLSL
#include "shape_common.glsl"

float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float hash3(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

vec3 repeat(vec3 p, vec3 c) {
    return p - c * round(p / c);
}

float sdf_box_local(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float create_mansion_cell(vec3 p, vec3 cell_id, float seed) {
    float h_size = hash3(cell_id + vec3(seed));
    float h1 = hash3(cell_id + vec3(seed * 1.1));
    float h2 = hash3(cell_id * 2.71 + vec3(seed * 1.41));
    float h3 = hash3(cell_id * 3.14 + vec3(seed * 2.71));
    float h4 = hash3(cell_id * 4.13 + vec3(seed * 3.14));
    float h6 = hash3(cell_id * 6.23 + vec3(seed * 5.23));
    float h7 = hash3(cell_id * 7.31 + vec3(seed * 6.31));
    
    float room_size = 4.0 + h_size * 3.0;
    float door_w = 1.5;
    float door_h = 2.5;
    float door_d = 1.0;
    float door_y = -room_size + door_h * 0.5;
    
    // Начинаем со сплошного материала
    float solid = -1e10;
    
    // Вырезаем комнату
    float room = sdf_box_local(p, vec3(room_size));
    solid = max(solid, -room);
    
    // Вырезаем дверные проходы
    if(h1 > 0.5) {
        float door = sdf_box_local(p - vec3(room_size, door_y, 0), vec3(door_d, door_h * 0.5, door_w * 0.5));
        solid = max(solid, -door);
    }
    if(h2 > 0.5) {
        float door = sdf_box_local(p - vec3(-room_size, door_y, 0), vec3(door_d, door_h * 0.5, door_w * 0.5));
        solid = max(solid, -door);
    }
    if(h3 > 0.5) {
        float door = sdf_box_local(p - vec3(0, door_y, room_size), vec3(door_w * 0.5, door_h * 0.5, door_d));
        solid = max(solid, -door);
    }
    if(h4 > 0.5) {
        float door = sdf_box_local(p - vec3(0, door_y, -room_size), vec3(door_w * 0.5, door_h * 0.5, door_d));
        solid = max(solid, -door);
    }
    
    // Вырезаем отверстия вверх/вниз
    if(h6 > 0.65) {
        float hole_up = sdf_box_local(p - vec3(0, room_size, 0), vec3(1.5, 1.0, 1.5));
        solid = max(solid, -hole_up);
    }
    
    if(h7 > 0.65) {
        float hole_down = sdf_box_local(p - vec3(0, -room_size, 0), vec3(1.5, 1.0, 1.5));
        solid = max(solid, -hole_down);
    }
    
    // Декоративные элементы - столбы в углах (оставляем сплошными, не вырезаем)
    for(float i = -1.0; i <= 1.0; i += 2.0) {
        for(float j = -1.0; j <= 1.0; j += 2.0) {
            float voxel_hash = hash3(vec3(i, j, 0) + cell_id * 11.37);
            if(voxel_hash > 0.7) {
                vec3 pillar_p = p - vec3(i * (room_size - 0.8), 0, j * (room_size - 0.8));
                float pillar = sdf_box_local(pillar_p, vec3(0.3, room_size - 0.2, 0.3));
                solid = min(solid, pillar);
            }
        }
    }
    
    return solid;
}

float shape_terrain_eval(vec3 p, float seed) {
    float spacing = 12.0;
    vec3 cell_id = round(p / spacing);
    vec3 local_p = repeat(p, vec3(spacing));
    
    return create_mansion_cell(local_p, cell_id, seed);
}

#endif
