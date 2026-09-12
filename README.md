# GTA SA Traffic Steering Fix

An ASI plugin for **Grand Theft Auto: San Andreas (v1.0 US)** that enables dynamic front wheel steering animations for ambient traffic vehicles.

---

### The Problem
In vanilla GTA San Andreas, ambient traffic cars operate under `STATUS_SIMPLE` (kinematic on-rails movement via `CCarCtrl::UpdateCarOnRails`). In this state, the game completely ignores standard physics calculations and the `m_fSteerAngle` variable. As a result, ambient cars turn rigidly without turning their front wheels, breaking visual immersion.

Previous attempts to force `STATUS_PHYSICS` in `CAutomobile::PreRender` caused game crashes due to uninitialized damage/suspension structures in lightweight traffic objects. Direct manipulation of wheel matrices frequently caused "wobbly wheel / flat tire" artifacts due to rotation axis conflicts with wheel rolling mechanics.

---

### The Solution
This plugin bypasses the game's dormant steering logic and directly manipulates the RenderWare frames (`CAR_WHEEL_LF` and `CAR_WHEEL_RF`) right before rendering:

1. **Kinematic Steer Calculation:** Yaw rate is calculated dynamically from the vehicle matrix's forward vector delta:
   $$\Delta\theta = \text{atan2}(Y_t, X_t) - \text{atan2}(Y_{t-1}, X_{t-1})$$
   Target steering angle is derived using a bicycle kinematic model based on vehicle speed and wheel base.
2. **Exponential Low-Pass Filter:** Smooths out jagged orientation changes caused by segmented navigation path nodes.
3. **Axis-Correct Matrix Transformation:** 
   - Translates wheel origin to $(0, 0, 0)$.
   - Rotates around the vehicle's vertical axis (`car.at`), preserving natural forward wheel rolling.
   - Translates back to the original hub position, preventing wheel detachment.
4. **Zero-Drift Frame Restoration:** Original wheel matrices are backed up on `beforeRender` and restored on `afterRender`, eliminating floating-point accumulation errors.

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
