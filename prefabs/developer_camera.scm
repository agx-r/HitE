(prefab
  (name "developer_camera")
  (description "Camera entity with movement, logger and rotation components")
  
  (component "camera"
    (position 0 3 0)
    (direction 0 -0.3 1)
    (up 0 1 0)
    (fov 90.0)
    (near-plane 0.1)
    (far-plane 1000.0)
    (active #t))
  
  (component "camera_movement"
    (move-speed 10.0)
    (enabled #t))

  (component "developer_overlay")
  
  (component "camera_rotation"
    (yaw -3.14159)
    (pitch -0.3)
    (look-sensitivity 0.003)
    (max-pitch 1.5)
    (min-pitch -1.5)
    (mouse-captured #t)
    (enabled #t))
  
  (component "lighting"
    (sun-direction 0.3 0.8 0.5)
    (sun-color 1.0 0.95 0.8)
    (ambient-strength 0.15)
    (diffuse-strength 0.85)
    (shadow-bias 0.05)
    (shadow-softness 0.3)
    (shadow-steps 48)
    (enabled #t)))
