# GTA SA Traffic Steering Fix

An ASI plugin for **Grand Theft Auto: San Andreas (v1.0 US)** that enables dynamic front wheel steering animations for ambient traffic vehicles.

---

### The Problem
In vanilla GTA San Andreas, ambient traffic cars operate under `STATUS_SIMPLE` (kinematic on-rails movement via `CCarCtrl::UpdateCarOnRails`). In this state, the game completely ignores standard physics calculations and the `m_fSteerAngle` variable. As a result, ambient cars turn rigidly without turning their front wheels, breaking visual immersion.

Previous attempts to force `STATUS_PHYSICS` in `CAutomobile::PreRender` caused game crashes due to uninitialized damage/suspension structures in lightweight traffic objects. Direct manipulation of wheel matrices frequently caused "wobbly wheel / flat tire" artifacts due to rotation axis conflicts with wheel rolling mechanics.

---

### The Solution & Technical Deep Dive

To achieve smooth and crash-free steering for traffic vehicles without altering their core physics status, this plugin implements a non-intrusive RenderWare matrix injection pipeline operating strictly during the render phase.

#### 1. Why `STATUS_PHYSICS` Fails
In vanilla San Andreas, ambient cars use `STATUS_SIMPLE`, which completely bypasses standard handling equations, collision mesh updates, and wheel turning logic. 
* **The Crash Vector:** Forcing `STATUS_PHYSICS` on lightweight traffic models inside `CAutomobile::PreRender` causes hard engine crashes. Traffic objects lack initialized damage structures, suspension spring limits, and proper wheel inertia tensors required by the physics engine.
* **The Matrix Artifacts:** Directly modifying wheel matrices without handling local coordinate spaces results in "wobbly wheels" or flat-tire tearing because rotations conflict with wheel rolling mechanics.

#### 2. The RenderWare Matrix Pipeline (`RwFrame`)
Instead of forcing physics, the plugin hooks into `vehicleRenderEvent.before` and intercepts the local frames of the front wheels (`CAR_WHEEL_LF` and `CAR_WHEEL_RF`).

* **Kinematic Steer Calculation:** 
  Since `m_fSteerAngle` is dead for `STATUS_SIMPLE` cars, the plugin calculates the vehicle's yaw rate dynamically by comparing the delta of the forward vector across frames:
  $$\Delta\theta = \text{atan2}(Y_t, X_t) - \text{atan2}(Y_{t-1}, X_{t-1})$$
  The target steering angle is derived using a bicycle kinematic model scaled by vehicle speed and a steering response multiplier ($2.6f$).

* **Exponential Low-Pass Filtering:**
  Ambient traffic follows segmented node paths in world space, causing sudden, jagged orientation updates. An exponential smoothing filter:
  $$\text{Steer}_{smooth} += (\text{Target} - \text{Steer}_{smooth}) \cdot (1 - e^{-5.0 \cdot dt})$$
  eliminates twitching and ensures smooth visual transitions.

* **Axis-Correct Local Matrix Transformation:**
  To rotate the wheel around its hub without detaching it from the axle or breaking rolling animations, a 3-step matrix operation is applied:
  1. Translate the wheel matrix origin to local $(0, 0, 0)$.
  2. Rotate around the vehicle's vertical axis (`car.at`).
  3. Translate the matrix back to the original hub position (`pMat->pos`).
  
  ```cpp
  RwV3d center = pMat->pos;
  RwV3d invCenter = { -center.x, -center.y, -center.z };
  RwMatrixTranslate(pMat, &invCenter, rwCOMBINEPOSTCONCAT);
  RwMatrixRotate(pMat, &axis, fAngleDeg, rwCOMBINEPOSTCONCAT);
  RwMatrixTranslate(pMat, &center, rwCOMBINEPOSTCONCAT);
  RwFrameUpdateObjects(pFrame);

---

### Requirements
- GTA San Andreas v1.0 US (Compact / HOODLUM)
- An ASI Loader (e.g., Ultimate ASI Loader, Silent's ASI Loader)

---

### Installation
Move `TrafficFix.asi` into your GTA San Andreas root directory or `scripts/` folder.

---

### Building from Source
- Visual Studio 2022 / 2026 (MSVC v143+)
- CMake 3.20+
- [Plugin-SDK](https://github.com/DK22Pac/plugin-sdk)

```bash
git clone [https://github.com/bobbybonya/gta-sa-traffic-steer-fix.git](https://github.com/bobbybonya/gta-sa-traffic-steer-fix.git)
cd gta-sa-traffic-steer-fix
mkdir build && cd build
cmake .. -A Win32
cmake --build . --config Release



---

### Credits
- **Author:** bobbybonya
- **Assisted by:** AI (Claude / Gemini) for mathematical transformations and RenderWare matrix pipeline prototyping
- **Libraries:** Plugin-SDK by DK22Pac
- **Testing & Debugging:** Reverse-engineered with x32dbg
