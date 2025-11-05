;; Camera prefab - entity with movement and rotation
(prefab
  (name "camera")
  (description "Camera entity with movement and rotation components")
  
  ;; Camera component (position, direction, fov)
  (component "camera"
    (position 0 0 0)
    (direction 0 -0.3 1)
    (up 0 1 0)
    (fov 70.0)
    (near-plane 0.1)
    (far-plane 1000.0)
    (active #t))
  
  ;; Camera movement component
  (component "camera_movement"
    (move-speed 5.0)
    (enabled #t))

  ;; Devcam
  (component "gui")
  (component "developer_overlay")
  
  ;; Camera rotation component
  (component "camera_rotation"
    (yaw -3.14159)
    (pitch -0.3)
    (look-sensitivity 0.003)
    (max-pitch 1.5)
    (min-pitch -1.5)
    (mouse-captured #t)
    (enabled #t)))
