#include <plugin.h>
#include "common.h"
#include "CAutomobile.h"
#include "CTimer.h"
#include "CVehicle.h"
#include "RenderWare.h"
#include <unordered_map>
#include <cmath>
#include <algorithm>

using namespace plugin;

struct FrameBackup {
    RwMatrix m_matLF;
    RwMatrix m_matRF;
    bool     m_bValid = false;
};

struct WheelTracker {
    CVector     m_vecPrevForward = { 0.0f, 0.0f, 0.0f };
    float       m_fSmoothSteer   = 0.0f;
    bool        m_bInitialized   = false;
    FrameBackup m_backup;
};

static std::unordered_map<CVehicle*, WheelTracker> ms_Trackers;

// Rotate wheel strictly around its hub along the vehicle's vertical axis
static void RotateWheelAroundCenter(RwFrame* pFrame, const CVector& carUp, float fAngleDeg) {
    if (!pFrame || std::fabs(fAngleDeg) < 0.01f)
        return;

    RwMatrix* pMat = RwFrameGetMatrix(pFrame);
    if (!pMat)
        return;

    RwV3d center = pMat->pos;

    // 1. Translate wheel center to origin (0, 0, 0)
    RwV3d invCenter = { -center.x, -center.y, -center.z };
    RwMatrixTranslate(pMat, &invCenter, rwCOMBINEPOSTCONCAT);

    // 2. Rotate around chassis vertical axis (car Z)
    RwV3d axis = { carUp.x, carUp.y, carUp.z };
    RwMatrixRotate(pMat, &axis, fAngleDeg, rwCOMBINEPOSTCONCAT);

    // 3. Translate hub center back
    RwMatrixTranslate(pMat, &center, rwCOMBINEPOSTCONCAT);

    RwFrameUpdateObjects(pFrame);
}

static void OnBeforeVehicleRender(CVehicle* pVehicle) {
    if (!pVehicle || pVehicle->m_nVehicleSubClass != VEHICLE_AUTOMOBILE)
        return;

    auto* pAuto = reinterpret_cast<CAutomobile*>(pVehicle);

    if (pAuto->m_pDriver && pAuto->m_pDriver->IsPlayer()) {
        ms_Trackers.erase(pAuto);
        return;
    }

    if (pAuto->m_nStatus != STATUS_SIMPLE)
        return;

    RwFrame* pFrameLF = pAuto->m_aCarNodes[CAR_WHEEL_LF];
    RwFrame* pFrameRF = pAuto->m_aCarNodes[CAR_WHEEL_RF];

    if (!pFrameLF || !pFrameRF)
        return;

    RwMatrix* pMatLF = RwFrameGetMatrix(pFrameLF);
    RwMatrix* pMatRF = RwFrameGetMatrix(pFrameRF);

    if (!pMatLF || !pMatRF)
        return;

    auto& tracker = ms_Trackers[pAuto];

    // Backup original matrices
    tracker.m_backup.m_matLF = *pMatLF;
    tracker.m_backup.m_matRF = *pMatRF;
    tracker.m_backup.m_bValid = true;

    float fSpeed = pAuto->m_vecMoveSpeed.Magnitude();
    CVector curForward = pAuto->GetMatrix().up;

    if (!tracker.m_bInitialized) {
        tracker.m_vecPrevForward = curForward;
        tracker.m_bInitialized = true;
        return;
    }

    float prevAngle = std::atan2(tracker.m_vecPrevForward.y, tracker.m_vecPrevForward.x);
    float curAngle  = std::atan2(curForward.y, curForward.x);
    float deltaAngle = curAngle - prevAngle;

    while (deltaAngle > 3.14159265f)  deltaAngle -= 6.2831853f;
    while (deltaAngle < -3.14159265f) deltaAngle += 6.2831853f;

    tracker.m_vecPrevForward = curForward;

    float fDeltaTime = std::clamp(CTimer::ms_fTimeStep * 0.02f, 0.001f, 0.05f);
    float rawYawRate = deltaAngle / fDeltaTime;
    float fSpeedMps  = fSpeed * 50.0f;

    float targetSteer = 0.0f;
    if (fSpeedMps > 0.4f) {
        // Steering response multiplier
        targetSteer = std::atan2(rawYawRate * 2.6f, fSpeedMps);
    }
    targetSteer = std::clamp(targetSteer, -0.60f, 0.60f);

    // Smooth traffic node trajectory jitter
    float fInterpSpeed = 5.0f;
    float fBlend = 1.0f - std::exp(-fInterpSpeed * fDeltaTime);
    tracker.m_fSmoothSteer += (targetSteer - tracker.m_fSmoothSteer) * fBlend;

    float fAngleDeg = tracker.m_fSmoothSteer * (180.0f / 3.14159265f);
    CVector carUp = pAuto->GetMatrix().at;

    RotateWheelAroundCenter(pFrameLF, carUp, fAngleDeg);
    RotateWheelAroundCenter(pFrameRF, carUp, fAngleDeg);
}

static void OnAfterVehicleRender(CVehicle* pVehicle) {
    if (!pVehicle || pVehicle->m_nVehicleSubClass != VEHICLE_AUTOMOBILE)
        return;

    auto* pAuto = reinterpret_cast<CAutomobile*>(pVehicle);
    auto it = ms_Trackers.find(pAuto);

    if (it != ms_Trackers.end() && it->second.m_backup.m_bValid) {
        RwFrame* pFrameLF = pAuto->m_aCarNodes[CAR_WHEEL_LF];
        RwFrame* pFrameRF = pAuto->m_aCarNodes[CAR_WHEEL_RF];

        if (pFrameLF) {
            RwMatrix* pMatLF = RwFrameGetMatrix(pFrameLF);
            if (pMatLF) {
                *pMatLF = it->second.m_backup.m_matLF;
                RwFrameUpdateObjects(pFrameLF);
            }
        }

        if (pFrameRF) {
            RwMatrix* pMatRF = RwFrameGetMatrix(pFrameRF);
            if (pMatRF) {
                *pMatRF = it->second.m_backup.m_matRF;
                RwFrameUpdateObjects(pFrameRF);
            }
        }

        it->second.m_backup.m_bValid = false;
    }
}

struct TrafficSafeDirectFix {
    TrafficSafeDirectFix() {
        Events::vehicleRenderEvent.before += [](CVehicle* pVehicle) {
            OnBeforeVehicleRender(pVehicle);
        };

        Events::vehicleRenderEvent.after += [](CVehicle* pVehicle) {
            OnAfterVehicleRender(pVehicle);
        };

        Events::vehicleDtorEvent += [](CVehicle* pVehicle) {
            ms_Trackers.erase(pVehicle);
        };
    }
} g_trafficSafeDirectFix;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}
