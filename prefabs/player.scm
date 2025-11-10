(prefab
  (name "player")
  (description "Controllable player pawn with camera-driven movement and collider")

  (component "transform"
    (position 0 3 0)
    (rotation-euler 0 0 0))

  (component "player"
    (active #t))

  (component "camera"
    (fov 90.0)
    (near-plane 0.1)
    (background-color 0.33 0.23 0.12)
    (far-plane 1000.0)
    (active #t))

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
    (sun-direction 0.7 0.7 0.5)
    (sun-color 1.0 0.1 0.8)
    (ambient-strength 0.6)
    (diffuse-strength 0.25)
    (shadow-bias 0.1)
    (shadow-softness 1)
    (shadow-steps 48)
    (enabled #t))

  (component "player_collider"
    (radius 2.0)
    (height 4.2)
    (skin-width 0.04)
    (camera-height 4))

  (component "player_movement_controls"
    (enabled #t)
    (auto-emit #t))

  (component "player_movement"))
