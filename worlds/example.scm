(world
  (name "Example World")
  (fixed-delta-time 0.016666)
  (use-fixed-timestep #f)
  
  (entity (prefab "torus")
    (component "shape"
      (position 0 1.5 -3)))
  
  ;; Terrain
  (entity (prefab "terrain"))
  
  ;; Camera entity from prefab
  (entity (prefab "camera")))
