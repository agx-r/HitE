;; Example World Definition
(world
  (name "Example World")
  (fixed-delta-time 0.016666)
  (use-fixed-timestep #f)
  
  ;; Prefabs to instantiate
  (prefabs
    "camera"
    "torus")
  
  ;; Inline entities (without prefab)
  (entity "ground"
    (component "shape"
      (type "box")
      (position 0 0 0)
      (dimensions 20 0.1 20)
      (color 0.5 0.5 0.5 1.0)
      (visible #t))))
