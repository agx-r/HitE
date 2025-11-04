#include "src/renderer/raymarcher.h"
#include <stdio.h>

int main() {
    printf("sizeof(sdf_object_t) = %zu\n", sizeof(sdf_object_t));
    printf("sizeof(vec4_t) = %zu\n", sizeof(vec4_t));
    printf("sizeof(uint32_t) = %zu\n", sizeof(uint32_t));
    printf("sizeof(float) = %zu\n", sizeof(float));
    
    sdf_object_t obj = {0};
    printf("\nOffsets:\n");
    printf("position: %zu\n", (size_t)&obj.position - (size_t)&obj);
    printf("rotation: %zu\n", (size_t)&obj.rotation - (size_t)&obj);
    printf("dimensions: %zu\n", (size_t)&obj.dimensions - (size_t)&obj);
    printf("color: %zu\n", (size_t)&obj.color - (size_t)&obj);
    printf("type: %zu\n", (size_t)&obj.type - (size_t)&obj);
    printf("material_id: %zu\n", (size_t)&obj.material_id - (size_t)&obj);
    printf("smoothing: %zu\n", (size_t)&obj.smoothing - (size_t)&obj);
    printf("_padding: %zu\n", (size_t)&obj._padding - (size_t)&obj);
    
    return 0;
}
