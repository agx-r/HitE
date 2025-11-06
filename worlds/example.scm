(world
  (name "Example World")
  (fixed-delta-time 0.016666)
  (use-fixed-timestep #f)
  
  ; (entity (prefab "torus")
  ;   (component "shape"
  ;     (position 0 4 0)))

  ; (entity (prefab "torus")
  ;   (component "shape"
  ;     (position 0 8 0)))

  (entity (prefab "terrain"))
  
  (entity (prefab "camera")
          (component "camera"
                     ; (position 60 1382 58))))
                     (position -237 551 516))))
