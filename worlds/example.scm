(world
  (name "Example World")
  (fixed-delta-time 0.016666)
  (use-fixed-timestep #f)
  
  ; (entity (prefab "torus")
  ;   (component "transform"
  ;     (position 0 4 0)))

  ; (entity (prefab "torus")
          ; (component "transform"
                     ; (position 0.0 -18.0 0.0)))

  ; (entity (prefab "terrain"))
  
  (entity (prefab "citadel"))
  
  (entity (prefab "player")
          (component "transform"
                     (position 0.0 0.0 0.0)))

  ; (entity (prefab "developer_camera")
          ; (component "transform"
                     ; (position 0 182 300)))
                     ; (position 253 172 94)))
)
