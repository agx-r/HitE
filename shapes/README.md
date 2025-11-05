# Shapes Directory

This directory contains individual SDF (Signed Distance Field) implementations for various geometric primitives.

## Structure

Each shape file follows a strict unified structure:

1. **Must include only** `shape_interface.h`
2. **Must implement only** a single SDF function
3. **Must follow** naming convention: `sdf_<shapename>`
4. **Must use only** `vec3_t`, `vec2_t`, and `float` types
5. **Must not** include any other dependencies or functionality

## Available Shapes

- `sphere.c` - Sphere primitive
- `box.c` - Box/cube primitive
- `torus.c` - Torus/donut primitive
- `plane.c` - Infinite plane
- `cylinder.c` - Cylinder primitive
- `capsule.c` - Capsule (cylinder with rounded ends)
- `cone.c` - Cone primitive

## Adding New Shapes

To add a new shape:

1. Create `<shapename>.c` in this directory
2. Include only `shape_interface.h`
3. Implement `float sdf_<shapename>(vec3_t p, ...params)`
4. Add documentation comment explaining parameters
5. Include the file in `src/components/shape_component.c`

Example:

```c
/*
 * MyShape SDF Implementation
 * 
 * Parameters:
 *   p - Point in local space
 *   ... - Shape-specific parameters
 * 
 * Returns: Signed distance to shape surface
 */

#include "shape_interface.h"

float
sdf_myshape (vec3_t p, float param1, float param2)
{
  // Implementation here
  return distance;
}
```

## Do NOT

- ? Add component logic
- ? Add transformation code
- ? Add material/rendering code
- ? Include headers other than `shape_interface.h`
- ? Add multiple functions per file
- ? Add external dependencies

Keep shapes pure and simple!
