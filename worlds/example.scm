(world
  (name "Example World")
  (fixed-delta-time 0.016666)
  (use-fixed-timestep #f)
  
  (entity (prefab "torus")
    (component "shape"
      (position 0 6 0)))
  (entity (prefab "torus")
    (component "shape"
      (position 0 2 0)))
  
  ;; Camera entity from prefab
  (entity (prefab "camera")))
