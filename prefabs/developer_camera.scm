(prefab
  (name "developer_camera")
  (description "Camera entity with movement, logger and rotation components")
  
  (component "camera"
    (position 0 0 0)
    (direction 0 -0.3 1)
    (up 0 1 0)
    (fov 190.0)
    (near-plane 2)
    (background-color 0.33 0.23 0.12)
    (far-plane 1000.0)
    (active #t))
  
  (component "camera_movement"
    (move-speed 8.0)
    (enabled #t))

  ; (component "developer_overlay")
  
  (component "camera_rotation"
    (yaw -3.14159)
    (pitch -0.3)
    (look-sensitivity 0.003)
    (max-pitch 1.5)
    (min-pitch -1.5)
    (mouse-captured #t)
    (enabled #t))
  
  (component "lighting"
    (sun-direction 0.7 0.7 0.5)
    (sun-color 1.0 0.1 0.8)
    (ambient-strength 0.6)
    (diffuse-strength 0.25)
    (shadow-bias 0.1)
    (shadow-softness 1)
    (shadow-steps 48)
    (enabled #t)))
