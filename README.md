High-performance Vulkan raymarching engine with ECS architecture.

## Features

- **ECS (Entity Component System)** - Pure data-driven architecture
- **Vulkan Compute Raymarching** - GPU-accelerated SDF rendering
- **Scheme-based Configuration** - No custom DSL, use real Scheme!
- **Prefab System** - Unity-style prefabs defined in Scheme
- **Component-based Camera** - Separate movement and rotation components
- **Hot Reload** - Shaders can be reloaded at runtime

## Prefabs (`.scm` in `prefabs/`)

```scheme
;; Camera prefab example
(prefab
  (name "camera")
  (description "Camera with movement and rotation")
  
  (component "camera"
    (position 0 2 -5)
    (direction 0 -0.3 1)
    (fov 70.0)
    (active #t))
  
  (component "camera_movement"
    (move-speed 5.0)
    (enabled #t))
  
  (component "camera_rotation"
    (yaw -3.14159)
    (pitch -0.3)
    (enabled #t)))
```

## Worlds (`.scm` in `worlds/`)

```scheme
;; World definition example
(world
  (name "Example World")
  (fixed-delta-time 0.016666)
  (use-fixed-timestep #f)
  
  ;; Load prefabs
  (prefabs
    "camera"
    "torus")
  
  ;; Define inline entities
  (entity "ground"
    (component "shape"
      (type 'box)
      (position 0 -1 0)
      (dimensions 20 0.1 20)
      (color 0.5 0.5 0.5 1.0)
      (visible #t))))
```

## Building

```bash
nix build
./result/bin/hite
```

## Controls

- **WASD** - Move camera
- **Mouse** - Look around
- **Space** - Move up
- **Shift** - Move down
- **ESC** - Exit

## Adding Content

### New Prefab

Create `prefabs/my_entity.scm`:
```scheme
(prefab
  (name "my_entity")
  (component "shape"
    (type 'sphere)
    (position 0 0 0)
    (dimensions 1.0 0 0)
    (color 1.0 0.0 0.0 1.0)
    (visible #t)))
```

### New World

Create `worlds/my_world.scm`:
```scheme
(world
  (name "My World")
  (prefabs "camera" "my_entity"))
```

That's it! No build system changes needed.

## Architecture

- **Components are pure data** - no logic in components
- **Systems process components** - lifecycle functions (start, update, render)
- **Camera is an entity** - just like any other entity, composed of components
- **Declarative configuration** - Scheme files describe the world

## License

MIT
