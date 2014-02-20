WorldInfo {
  info [
    "Cozmo test world. All rights reserved. Copyright (c) Anki, Inc."
    "Author: Hanns Tappeiner <hanns@anki.com>"
    "Author: Andrew Stein <andrew@anki.com>"
    "Date: 02 Aug 2013"
    ""
  ]
  physics "cozmo_physics"
  basicTimeStep 5
  contactProperties [
    DEF FLOOR_FRONTWHEEL ContactProperties {
      material1 "cmat_floor"
      material2 "highfriction"
      coulombFriction 50
      bounce 0.01
    }
    DEF RAMP_FRONTWHEEL ContactProperties {
      material1 "cmat_ramp"
      material2 "highfriction"
      coulombFriction 50
    }
    DEF BLOCK_FRONTWHEEL ContactProperties {
      material1 "cmat_block"
      material2 "highfriction"
      coulombFriction 50
      bounce 0.01
    }
    DEF BLOCK_REARWHEEL ContactProperties {
      material1 "cmat_block"
      material2 "nofriction"
      coulombFriction 0.0001
      bounce 0.001
    }
    DEF FLOOR_REARWHEEL ContactProperties {
      material1 "cmat_floor"
      material2 "nofriction"
      coulombFriction 0.0001
      bounce 0.001
    }
    DEF RAMP_REARWHEEL ContactProperties {
      material1 "cmat_ramp"
      material2 "nofriction"
      coulombFriction 0.0001
      bounce 0.001
    }
    DEF FLOOR_BLOCK ContactProperties {
      material1 "cmat_floor"
      material2 "cmat_block"
      coulombFriction 5000
      bounce 0.001
    }
    DEF BLOCK_BLOCK ContactProperties {
      material1 "cmat_block"
      material2 "cmat_block"
      coulombFriction 5000
      bounce 0.001
    }
    DEF RAMP_BLOCK ContactProperties {
      material1 "cmat_ramp"
      material2 "cmat_block"
      coulombFriction 5000
      bounce 0.001
    }
    DEF RAMP_RAMP ContactProperties {
      material1 "cmat_ramp"
      material2 "cmat_ramp"
      coulombFriction 5000
      bounce 0.001
    }
    DEF FLOOR_RAMP ContactProperties {
      material1 "cmat_ramp"
      material2 "cmat_block"
      coulombFriction 5000
      bounce 0.001
    }
  ]
} # WorldInfo

Viewpoint {
  orientation -0.118742 0.988082 0.097946 2.51612
  position 0.75138 0.342175 -1.26362
}

Background {
  skyColor [
    0.4 0.7 1
  ]
}

DirectionalLight {
  ambientIntensity 0.5
  direction 1 -2 1
  castShadows TRUE
}