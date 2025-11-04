# HitE - Developer Guide

A practical cookbook for building high-performance raymarched worlds with HitE.

## Table of Contents

[Quick Start](#quick-start)
[Creating Worlds](#creating-worlds)
[Working with Components](#working-with-components)
[SDF Shapes](#sdf-shapes)
[Physics System](#physics-system)
[Event System](#event-system)
[Resource Management](#resource-management)
[Custom Shaders](#custom-shaders)
[Performance Tips](#performance-tips)
[Advanced Patterns](#advanced-patterns)

---

## Quick Start

### Building

```bash
nix build
./result/bin/hite
```

### Development Environment

```bash
nix develop
cmake -B build -G Ninja
ninja -C build
./build/hite
```

### Your First World

```c
#include "core/world.h"
#include "components/shape_component.h"

result_t my_world_generator(ecs_world_t* world, void* user_data) {
    component_id_t shape_id = ecs_get_component_id(world, "shape");
    
    // Create a sphere
    entity_id_t sphere = ecs_entity_create(world);
    shape_component_t shape = shape_sphere_create(
        (vec3_t){0, 2, 0, 0},    // position
        1.0f,                     // radius
        (vec4_t){1, 0, 0, 1}     // red color
    );
    ecs_add_component(world, sphere, shape_id, &shape);
    
    return RESULT_SUCCESS;
}
```

---

## Creating Worlds

### World Definition Structure

```c
world_definition_t my_world = {
    .name = "My World",
    .fixed_delta_time = 1.0f / 60.0f,
    .use_fixed_timestep = false,
    .generator = my_world_generator,
    .generator_data = NULL,
    .generate_on_gpu = false
};
```

### Procedural Generation

```c
result_t procedural_forest(ecs_world_t* world, void* user_data) {
    component_id_t shape_id = ecs_get_component_id(world, "shape");
    
    // Generate 100 trees at random positions
    for (int i = 0; i < 100; i++) {
        float x = (rand() % 200) - 100.0f;
        float z = (rand() % 200) - 100.0f;
        
        entity_id_t tree = ecs_entity_create(world);
        shape_component_t trunk = shape_cylinder_create(
            (vec3_t){x, 1, z, 0},
            0.3f,  // radius
            2.0f,  // height
            (vec4_t){0.4f, 0.2f, 0.1f, 1}
        );
        ecs_add_component(world, tree, shape_id, &trunk);
    }
    
    return RESULT_SUCCESS;
}
```

### Loading Worlds at Runtime

```c
// Switch to a different world
result_t world_switch(world_manager_t* manager, const world_definition_t* new_world);

// Example: Level transition
world_definition_t level2 = {
    .name = "Level 2",
    .generator = level2_generator
};
world_switch(state.world_manager, &level2);
```

---

## Working with Components

### Creating Custom Components

```c
// 0. Define your component structure (must be aligned)
typedef struct {
    float health;
    float max_health;
    bool invulnerable;
    float _padding;  // Ensure 16-byte alignment
} ALIGN_16 health_component_t;

// 1. Define lifecycle functions
result_t health_start(ecs_world_t* world, entity_id_t entity, void* data) {
    health_component_t* health = (health_component_t*)data;
    if (health->max_health == 0) {
        health->max_health = 100.0f;
    }
    health->health = health->max_health;
    return RESULT_SUCCESS;
}

result_t health_update(ecs_world_t* world, entity_id_t entity, 
                       void* data, const time_info_t* time) {
    health_component_t* health = (health_component_t*)data;
    
    // Regenerate health over time
    if (health->health < health->max_health) {
        health->health += 10.0f * time->delta_time;
    }
    
    return RESULT_SUCCESS;
}

// 2. Register the component
void health_component_register(ecs_world_t* world) {
    component_descriptor_t desc = {
        .name = "health",
        .data_size = sizeof(health_component_t),
        .alignment = 16,
        .dependencies = NULL,
        .start = health_start,
        .update = health_update,
        .render = NULL,
        .destroy = NULL
    };
    
    component_id_t id;
    ecs_register_component(world, &desc, &id);
}
```

### Component Dependencies

```c
// Declare dependencies (component must have these)
static const char* ai_dependencies[] = {"shape", "health", NULL};

component_descriptor_t ai_desc = {
    .name = "ai",
    .dependencies = ai_dependencies,  // Will crash if missing
    // ...
};
```

### Accessing Other Components

```c
result_t damage_update(ecs_world_t* world, entity_id_t entity,
                      void* data, const time_info_t* time) {
    damage_component_t* damage = (damage_component_t*)data;
    
    // Get health component from same entity
    component_id_t health_id = ecs_get_component_id(world, "health");
    health_component_t* health = ecs_get_component(world, entity, health_id);
    
    if (health && damage->should_damage) {
        health->health -= damage->damage_per_second * time->delta_time;
    }
    
    return RESULT_SUCCESS;
}
```

---

## SDF Shapes

### Built-in Primitives

```c
// Sphere
shape_component_t sphere = shape_sphere_create(
    (vec3_t){x, y, z, 0},
    radius,
    (vec4_t){r, g, b, 1}
);

// Box
shape_component_t box = shape_box_create(
    (vec3_t){x, y, z, 0},
    (vec3_t){half_width, half_height, half_depth, 0},
    (vec4_t){r, g, b, 1}
);

// Torus
shape_component_t torus = shape_torus_create(
    (vec3_t){x, y, z, 0},
    major_radius,
    minor_radius,
    (vec4_t){r, g, b, 1}
);

// Cylinder, Capsule, Cone
shape_component_t cylinder = {
    .type = SHAPE_CYLINDER,
    .transform.position = {x, y, z, 0},
    .dimensions = {radius, height, 0, 0},
    .color = {r, g, b, 1}
};
```

### Custom SDF Functions

```c
// Define custom SDF
float my_custom_sdf(vec3_t p, const void* params) {
    // Example: twisted box
    float twist_amount = *(float*)params;
    float c = cosf(twist_amount * p.y);
    float s = sinf(twist_amount * p.y);
    
    vec3_t twisted = {
        c * p.x - s * p.z,
        p.y,
        s * p.x + c * p.z,
        0
    };
    
    return sdf_box(twisted, (vec3_t){1, 1, 1, 0});
}

// Use it
shape_component_t custom_shape = {
    .type = SHAPE_CUSTOM,
    .transform.position = {0, 2, 0, 0},
    .custom_sdf = my_custom_sdf,
    .custom_params = &twist_param,
    .custom_params_size = sizeof(float),
    .color = {1, 0.5f, 0, 1}
};
```

### Shape Operations

```c
// Smooth blending between shapes
shape_component_t blob = {
    .type = SHAPE_SPHERE,
    .operation = SHAPE_OP_SMOOTH_UNION,
    .smoothing = 0.5f,  // Blend radius
    // ...
};

// Subtraction (CSG)
shape_component_t hole = {
    .type = SHAPE_SPHERE,
    .operation = SHAPE_OP_SUBTRACTION,
    // ...
};
```

### Material Properties

```c
shape_component_t metallic_sphere = {
    .type = SHAPE_SPHERE,
    .color = {0.8f, 0.8f, 0.9f, 1},
    .roughness = 0.1f,   // 0 = mirror, 1 = matte
    .metallic = 0.9f,    // 0 = dielectric, 1 = metal
    // ...
};
```

---

## Physics System

### Basic Physics

```c
// Add physics to an entity
component_id_t physics_id = ecs_get_component_id(world, "physics");

physics_component_t physics = {
    .mass = 1.0f,
    .drag = 0.1f,
    .gravity_enabled = true,
    .velocity = {0, 0, 0, 0},
    .acceleration = {0, 0, 0, 0}
};

ecs_add_component(world, entity, physics_id, &physics);
```

### Applying Forces

```c
// In your update function
physics_component_t* physics = ecs_get_component(world, entity, physics_id);

// Apply impulse (instant velocity change)
vec3_t jump_impulse = {0, 10, 0, 0};
physics_apply_impulse(physics, jump_impulse);

// Apply force (gradual acceleration)
vec3_t wind_force = {5, 0, 0, 0};
physics_apply_force(physics, wind_force);
```

### Kinematic Objects

```c
// Objects that move but aren't affected by physics
physics_component_t platform_physics = {
    .kinematic = true,  // Ignores forces and gravity
    .mass = 1.0f
};
```

### Collision Detection

```c
// Check if entity is near a point
shape_component_t* shape = ecs_get_component(world, entity, shape_id);
vec3_t test_point = {x, y, z, 0};

float distance = shape_evaluate_sdf(shape, test_point);
if (distance < 0.5f) {
    // Collision detected!
}
```

---

## Event System

### Listening to Events

```c
void on_collision(const event_t* event, void* user_data) {
    printf("Collision at (%.1f, %.1f, %.1f)\n",
           event->data.collision.point.x,
           event->data.collision.point.y,
           event->data.collision.point.z);
    
    // Handle collision...
}

// Register listener
listener_id_t id = event_listen(
    event_system,
    EVENT_COLLISION,
    on_collision,
    NULL  // user_data
);
```

### Entity-Specific Events

```c
// Only listen to events from specific entity
listener_id_t id = event_listen_entity(
    event_system,
    EVENT_COLLISION,
    my_entity,
    on_entity_collision,
    NULL
);
```

### Emitting Events

```c
// Emit collision event
event_t collision = event_collision_create(
    entity_id,
    other_entity_id,
    (vec3_t){x, y, z, 0},      // collision point
    (vec3_t){nx, ny, nz, 0}    // collision normal
);
event_emit(event_system, &collision);

// Emit custom event
event_t custom = {
    .type = EVENT_CUSTOM_START + 1,
    .entity = my_entity,
    .timestamp = current_time
};
memcpy(custom.data.custom, my_data, sizeof(my_data));
event_emit(event_system, &custom);
```

### Input Events

```c
void on_key_press(const event_t* event, void* user_data) {
    if (event->data.key.key == GLFW_KEY_SPACE) {
        // Jump!
    }
}

event_listen(event_system, EVENT_KEY_PRESS, on_key_press, NULL);
```

---

## Resource Management

### Loading Resources

```c
resource_manager_t* rm = resource_manager_create();

// Load shader
resource_id_t shader_id;
result_t result = resource_load(
    rm,
    "shaders/my_shader.comp.spv",
    RESOURCE_SHADER,
    &shader_id
);

// Get resource data
resource_t* resource = resource_get(rm, shader_id);
void* shader_code = resource->data;
size_t shader_size = resource->size;
```

### Hot Reload

```c
// Check for file changes and reload automatically
resource_check_reload(resource_manager);

// Manual reload
resource_reload(resource_manager, shader_id);
```

### Resource Lifetime

```c
// Reference counting
resource_acquire(rm, resource_id);  // Increment ref count
resource_release(rm, resource_id);  // Decrement ref count

// Resources are freed when ref count reaches 0
```

---

## Custom Shaders

### Shader Structure

Your compute shader must follow this structure:

```glsl
#version 450
layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0) uniform Uniforms {
    mat4 view_matrix;
    mat4 projection_matrix;
    vec4 camera_position;
    vec4 camera_direction;
    vec2 resolution;
    float time;
} ubo;

layout(binding = 1, rgba8) uniform writeonly image2D output_image;

layout(binding = 2) buffer SDFObjects {
    // Your SDF object data
} sdf_objects;

void main() {
    ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
    
    // Your raymarching code here
    vec4 color = raymarch(...);
    
    imageStore(output_image, coords, color);
}
```

### Compiling Shaders

Shaders are automatically compiled by CMake:

```cmake
# CMakeLists.txt handles this automatically
# Place .comp files in shaders/ directory
# Compiled .spv files go to build/shaders/
```

### Loading Custom Shaders

```c
result_t raymarcher_load_shader(raymarcher_t* rm, const char* path);

// In render system init
raymarcher_load_shader(&system->raymarcher, "my_shader.comp.spv");
```

---

## Performance Tips

### ECS Best Practices

```c
// BAD: Accessing components in nested loops
for (each entity) {
    for (each other entity) {
        component_t* c = ecs_get_component(...);  // Slow lookup!
    }
}

// GOOD: Iterate components directly
void check_collisions(ecs_world_t* world, entity_id_t e, 
                      void* data, void* user_data) {
    // Process each component in cache-friendly order
}
ecs_iterate_components(world, component_id, check_collisions, NULL);
```

### GPU Buffer Updates

```c
// BAD: Upload every frame
for (each frame) {
    gpu_buffer_upload(...);  // Expensive!
}

// GOOD: Only upload when changed
if (shape->dirty) {
    gpu_buffer_upload(...);
    shape->dirty = false;
}
```

### SDF Optimization

```c
// Use bounding volumes
float cheap_bounds_check = sdf_sphere(p, large_radius);
if (cheap_bounds_check > 2.0f) {
    return cheap_bounds_check;  // Early out
}

// Now do expensive SDF calculation
return complex_sdf(p);
```

### Memory Alignment

```c
// CRITICAL: Align structures for SIMD/GPU
typedef struct {
    vec3_t position;      // 12 bytes
    float radius;         // 4 bytes
    // Total: 16 bytes - perfect!
} ALIGN_16 simple_data_t;

typedef struct {
    vec3_t position;      // 12 bytes
    float radius;         // 4 bytes
    vec3_t velocity;      // 12 bytes
    float _padding;       // 4 bytes needed!
    // Total: 32 bytes
} ALIGN_32 complex_data_t;
```

---

## Advanced Patterns

### State Machines with Components

```c
typedef enum {
    STATE_IDLE,
    STATE_WALKING,
    STATE_JUMPING
} entity_state_t;

typedef struct {
    entity_state_t current_state;
    entity_state_t previous_state;
    float state_time;
} ALIGN_16 state_machine_component_t;

result_t state_machine_update(ecs_world_t* world, entity_id_t entity,
                              void* data, const time_info_t* time) {
    state_machine_component_t* sm = data;
    sm->state_time += time->delta_time;
    
    switch (sm->current_state) {
        case STATE_IDLE:
            if (input_detected()) {
                sm->previous_state = STATE_IDLE;
                sm->current_state = STATE_WALKING;
                sm->state_time = 0;
            }
            break;
        // ...
    }
    
    return RESULT_SUCCESS;
}
```

### Object Pooling

```c
#define POOL_SIZE 1000

typedef struct {
    entity_id_t entities[POOL_SIZE];
    bool active[POOL_SIZE];
    size_t next_free;
} entity_pool_t;

entity_id_t pool_spawn(entity_pool_t* pool, ecs_world_t* world) {
    for (size_t i = 0; i < POOL_SIZE; i++) {
        size_t idx = (pool->next_free + i) % POOL_SIZE;
        if (!pool->active[idx]) {
            pool->active[idx] = true;
            pool->next_free = (idx + 1) % POOL_SIZE;
            return pool->entities[idx];
        }
    }
    return INVALID_ENTITY;
}
```

### Data-Driven Entities

```c
typedef struct {
    const char* shape_type;
    float shape_params[4];
    vec3_t position;
    vec4_t color;
    bool has_physics;
    float mass;
} entity_template_t;

// Load from JSON/file
entity_template_t templates[] = {
    {
        .shape_type = "sphere",
        .shape_params = {1.0f},  // radius
        .position = {0, 2, 0},
        .color = {1, 0, 0, 1},
        .has_physics = true,
        .mass = 1.0f
    },
    // ...
};

void spawn_from_template(ecs_world_t* world, entity_template_t* tmpl) {
    entity_id_t e = ecs_entity_create(world);
    
    // Create shape based on template
    component_id_t shape_id = ecs_get_component_id(world, "shape");
    shape_component_t shape = shape_sphere_create(
        tmpl->position,
        tmpl->shape_params[0],
        tmpl->color
    );
    ecs_add_component(world, e, shape_id, &shape);
    
    // Add physics if needed
    if (tmpl->has_physics) {
        component_id_t physics_id = ecs_get_component_id(world, "physics");
        physics_component_t physics = {.mass = tmpl->mass};
        ecs_add_component(world, e, physics_id, &physics);
    }
}
```

### GPU Generation

```c
// Generate on GPU (future feature)
world_definition_t gpu_world = {
    .name = "GPU Generated",
    .generate_on_gpu = true,
    .generator = NULL,  // Use compute shader instead
    .generator_data = &gpu_params
};
```

---

## Common Recipes

### Projectile System

```c
void spawn_projectile(ecs_world_t* world, vec3_t pos, vec3_t dir) {
    entity_id_t proj = ecs_entity_create(world);
    
    // Shape
    component_id_t shape_id = ecs_get_component_id(world, "shape");
    shape_component_t shape = shape_sphere_create(pos, 0.2f, 
                                                  (vec4_t){1, 1, 0, 1});
    ecs_add_component(world, proj, shape_id, &shape);
    
    // Physics
    component_id_t physics_id = ecs_get_component_id(world, "physics");
    physics_component_t physics = {
        .velocity = {dir.x * 20, dir.y * 20, dir.z * 20, 0},
        .mass = 0.1f,
        .drag = 0.01f,
        .gravity_enabled = false
    };
    ecs_add_component(world, proj, physics_id, &physics);
}
```

### Particle Effects

```c
void spawn_explosion(ecs_world_t* world, vec3_t pos) {
    for (int i = 0; i < 50; i++) {
        float angle = (float)i / 50.0f * 2.0f * M_PI;
        vec3_t dir = {cosf(angle), 0.5f, sinf(angle), 0};
        
        entity_id_t particle = ecs_entity_create(world);
        
        shape_component_t shape = shape_sphere_create(
            pos, 0.1f, (vec4_t){1, 0.5f, 0, 1}
        );
        ecs_add_component(world, particle, shape_id, &shape);
        
        physics_component_t physics = {
            .velocity = {dir.x * 5, dir.y * 5, dir.z * 5, 0},
            .mass = 0.01f,
            .gravity_enabled = true
        };
        ecs_add_component(world, particle, physics_id, &physics);
    }
}
```

### Camera Follow

```c
result_t camera_follow_update(ecs_world_t* world, entity_id_t entity,
                              void* data, const time_info_t* time) {
    camera_follow_t* follow = data;
    
    // Get target position
    shape_component_t* target = ecs_get_component(world, follow->target, 
                                                   shape_id);
    if (!target) return RESULT_SUCCESS;
    
    // Smooth follow
    vec3_t target_pos = target->transform.position;
    vec3_t offset = {0, 2, 5, 0};
    
    vec3_t desired = {
        target_pos.x + offset.x,
        target_pos.y + offset.y,
        target_pos.z + offset.z,
        0
    };
    
    // Lerp camera position
    float speed = 5.0f * time->delta_time;
    follow->camera_pos.x += (desired.x - follow->camera_pos.x) * speed;
    follow->camera_pos.y += (desired.y - follow->camera_pos.y) * speed;
    follow->camera_pos.z += (desired.z - follow->camera_pos.z) * speed;
    
    return RESULT_SUCCESS;
}
```

---

## Debugging

### Enable Validation Layers

Validation layers are enabled by default in debug builds.

### Print ECS State

```c
void debug_print_entities(ecs_world_t* world) {
    printf("=== ECS Debug ===\n");
    printf("Next entity ID: %u\n", world->next_entity_id);
    printf("Component types: %zu\n", world->component_count);
    
    for (size_t i = 0; i < world->component_count; i++) {
        component_array_t* array = &world->component_arrays[i];
        printf("  %s: %zu instances\n", array->descriptor.name, array->count);
    }
}
```

### Shader Debugging

Check shader compilation output:

```bash
# Manual compilation for debugging
glslangValidator -V shader.comp -o shader.spv --target-env vulkan1.2
```

---

## Building for Production

```bash
# Optimized build
nix build --option optimization-level 3

# Strip symbols
strip result/bin/hite
```

---

# Q&A

Q: When will X11 be supported?
A: Never. Use wayland.
