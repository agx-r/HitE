;; Example World Definition
(world
  (name "Example World")
  (fixed-delta-time 0.016666)
  (use-fixed-timestep #f)
  
  (entity (prefab "torus")
    (component "shape"
      (position 0 1.5 -3)))
  
  ;; Ground plane
  (entity
    (component "shape"
      (type "box")
      (position 0 -0.5 0)
      (dimensions 20 0.1 20)
      (color 0.5 0.5 0.5 1.0)
      (visible #t)))
  
  ;; Red sphere
  ; (entity
  ;   (component "shape"
  ;     (type "sphere")
  ;     (position 2 1 -2)
  ;     (dimensions 0.5 0.5 0.5)
  ;     (color 1.0 0.2 0.2 1.0)
  ;     (visible #t)))
  
  ;; Camera entity from prefab
  (entity (prefab "camera")))
