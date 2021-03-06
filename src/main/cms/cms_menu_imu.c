/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

// Menu contents for PID, RATES, RC preview, misc
// Should be part of the relevant .c file.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "platform.h"

#ifdef USE_CMS

#include "common/utils.h"

#include "cms/cms.h"
#include "cms/cms_types.h"
#include "cms/cms_menu_imu.h"

#include "common/axis.h"

#include "flight/pid.h"

#include "fc/config.h"
#include "fc/controlrate_profile.h"
#include "fc/rc_controls.h"

#include "navigation/navigation.h"

#include "sensors/gyro.h"


//
// PID
//
static uint8_t tmpProfileIndex;
static uint8_t profileIndex;
static char profileIndexString[] = " p";

static void cmsx_ReadPidToArray(uint8_t *dst, int pidIndex)
{
    dst[0] = pidBank()->pid[pidIndex].P;
    dst[1] = pidBank()->pid[pidIndex].I;
    dst[2] = pidBank()->pid[pidIndex].D;
}

static void cmsx_WritebackPidFromArray(uint8_t *src, int pidIndex)
{
    pidBankMutable()->pid[pidIndex].P = src[0];
    pidBankMutable()->pid[pidIndex].I = src[1];
    pidBankMutable()->pid[pidIndex].D = src[2];
}

static long cmsx_menuImu_onEnter(void)
{
    profileIndex = getConfigProfile();
    tmpProfileIndex = profileIndex + 1;
    profileIndexString[1] = '0' + tmpProfileIndex;

    return 0;
}

static long cmsx_menuImu_onExit(const OSD_Entry *self)
{
    UNUSED(self);

    setConfigProfile(profileIndex);

    return 0;
}

static long cmsx_profileIndexOnChange(displayPort_t *displayPort, const void *ptr)
{
    UNUSED(displayPort);
    UNUSED(ptr);

    profileIndex = tmpProfileIndex - 1;
    profileIndexString[1] = '0' + tmpProfileIndex;

    return 0;
}

static uint8_t cmsx_pidRoll[3];
static uint8_t cmsx_pidPitch[3];
static uint8_t cmsx_pidYaw[3];

static long cmsx_PidRead(void)
{
    cmsx_ReadPidToArray(cmsx_pidRoll, PID_ROLL);
    cmsx_ReadPidToArray(cmsx_pidPitch, PID_PITCH);
    cmsx_ReadPidToArray(cmsx_pidYaw, PID_YAW);

    return 0;
}

static long cmsx_PidOnEnter(void)
{
    profileIndexString[1] = '0' + tmpProfileIndex;
    cmsx_PidRead();

    return 0;
}

static long cmsx_PidWriteback(const OSD_Entry *self)
{
    UNUSED(self);

    cmsx_WritebackPidFromArray(cmsx_pidRoll, PID_ROLL);
    cmsx_WritebackPidFromArray(cmsx_pidPitch, PID_PITCH);
    cmsx_WritebackPidFromArray(cmsx_pidYaw, PID_YAW);

    schedulePidGainsUpdate();

    return 0;
}

static OSD_Entry cmsx_menuPidEntries[] =
{
    { "-- PID --", OME_Label, NULL, profileIndexString, 0},

    { "ROLL  P", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidRoll[0],  0, 200, 1 }, 0 },
    { "ROLL  I", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidRoll[1],  0, 200, 1 }, 0 },
    { "ROLL  D", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidRoll[2],  0, 200, 1 }, 0 },

    { "PITCH P", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidPitch[0], 0, 200, 1 }, 0 },
    { "PITCH I", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidPitch[1], 0, 200, 1 }, 0 },
    { "PITCH D", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidPitch[2], 0, 200, 1 }, 0 },

    { "YAW   P", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidYaw[0],   0, 200, 1 }, 0 },
    { "YAW   I", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidYaw[1],   0, 200, 1 }, 0 },
    { "YAW   D", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidYaw[2],   0, 200, 1 }, 0 },

    { "BACK", OME_Back, NULL, NULL, 0 },
    { NULL, OME_END, NULL, NULL, 0 }
};

static CMS_Menu cmsx_menuPid = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XPID",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_PidOnEnter,
    .onExit = cmsx_PidWriteback,
    .onGlobalExit = NULL,
    .entries = cmsx_menuPidEntries
};

static uint8_t cmsx_pidPosZ[3];
static uint8_t cmsx_pidVelZ[3];
static uint8_t cmsx_pidHead[3];

static long cmsx_menuPidAltMag_onEnter(void)
{
    cmsx_ReadPidToArray(cmsx_pidPosZ, PID_POS_Z);
    cmsx_ReadPidToArray(cmsx_pidVelZ, PID_VEL_Z);
    cmsx_pidHead[0] = pidBank()->pid[PID_HEADING].P;

    return 0;
}

static long cmsx_menuPidAltMag_onExit(const OSD_Entry *self)
{
    UNUSED(self);

    cmsx_WritebackPidFromArray(cmsx_pidPosZ, PID_POS_Z);
    cmsx_WritebackPidFromArray(cmsx_pidVelZ, PID_VEL_Z);
    pidBankMutable()->pid[PID_HEADING].P = cmsx_pidHead[0];

    navigationUsePIDs();

    return 0;
}

static OSD_Entry cmsx_menuPidAltMagEntries[] = {
    { "-- ALT&MAG --", OME_Label, NULL, profileIndexString, 0},

    { "ALT P", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidPosZ[0], 0, 255, 1 }, 0 },
    { "ALT I", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidPosZ[1], 0, 255, 1 }, 0 },
    { "ALT D", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidPosZ[2], 0, 255, 1 }, 0 },
    { "VEL P", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidVelZ[0], 0, 255, 1 }, 0 },
    { "VEL I", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidVelZ[1], 0, 255, 1 }, 0 },
    { "VEL D", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidVelZ[2], 0, 255, 1 }, 0 },
    { "MAG P", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidHead[0], 0, 255, 1 }, 0 },

    {"BACK", OME_Back, NULL, NULL, 0},
    {NULL, OME_END, NULL, NULL, 0}
};

static CMS_Menu cmsx_menuPidAltMag = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XALTMAG",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_menuPidAltMag_onEnter,
    .onExit = cmsx_menuPidAltMag_onExit,
    .onGlobalExit = NULL,
    .entries = cmsx_menuPidAltMagEntries,
};

static uint8_t cmsx_pidPosXY[3];
static uint8_t cmsx_pidVelXY[3];

static long cmsx_menuPidGpsnav_onEnter(void)
{
    cmsx_ReadPidToArray(cmsx_pidPosXY, PID_POS_XY);
    cmsx_ReadPidToArray(cmsx_pidVelXY, PID_VEL_XY);

    return 0;
}

static long cmsx_menuPidGpsnav_onExit(const OSD_Entry *self)
{
    UNUSED(self);

    cmsx_WritebackPidFromArray(cmsx_pidPosXY, PID_POS_XY);
    cmsx_WritebackPidFromArray(cmsx_pidVelXY, PID_VEL_XY);

    navigationUsePIDs();

    return 0;
}

static OSD_Entry cmsx_menuPidGpsnavEntries[] = {
    { "-- GPSNAV --", OME_Label, NULL, profileIndexString, 0},

    { "POS  P", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidPosXY[0],  0, 255, 1 }, 0 },
    { "POS  I", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidPosXY[1],  0, 255, 1 }, 0 },
    { "POSR P", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidVelXY[0], 0, 255, 1 }, 0 },
    { "POSR I", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidVelXY[1], 0, 255, 1 }, 0 },
    { "POSR D", OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_pidVelXY[2], 0, 255, 1 }, 0 },

    {"BACK", OME_Back, NULL, NULL, 0},
    {NULL, OME_END, NULL, NULL, 0}
};

static CMS_Menu cmsx_menuPidGpsnav = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XGPSNAV",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_menuPidGpsnav_onEnter,
    .onExit = cmsx_menuPidGpsnav_onExit,
    .onGlobalExit = NULL,
    .entries = cmsx_menuPidGpsnavEntries,
};

//
// MANUAL Rate & Expo
//
static OSD_Entry cmsx_menuManualRateProfileEntries[] =
{
    { "-- MANUAL RATE --", OME_Label, NULL, profileIndexString, 0 },

    OSD_SETTING_ENTRY("MANU ROLL RATE", SETTING_MANUAL_ROLL_RATE),
    OSD_SETTING_ENTRY("MANU PITCH RATE", SETTING_MANUAL_PITCH_RATE),
    OSD_SETTING_ENTRY("MANU YAW RATE", SETTING_MANUAL_YAW_RATE),

    OSD_SETTING_ENTRY("MANU RC EXPO", SETTING_MANUAL_RC_EXPO),
    OSD_SETTING_ENTRY("MANU RC YAW EXP", SETTING_MANUAL_RC_YAW_EXPO),

    { "BACK", OME_Back, NULL, NULL, 0 },
    { NULL, OME_END, NULL, NULL, 0 }
};

static CMS_Menu cmsx_menuManualRateProfile = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "MENUMANURATE",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = NULL,
    .onExit = NULL,
    .onGlobalExit = NULL,
    .entries = cmsx_menuManualRateProfileEntries
};

//
// Rate & Expo
//
static OSD_Entry cmsx_menuRateProfileEntries[] =
{
    { "-- RATE --", OME_Label, NULL, profileIndexString, 0 },

#if 0
    { "RC RATE",     OME_FLOAT,  NULL, &(OSD_FLOAT_t){ &rateProfile.rcRate8,    0, 255, 1, 10 }, 0 },
    { "RC YAW RATE", OME_FLOAT,  NULL, &(OSD_FLOAT_t){ &rateProfile.rcYawRate8, 0, 255, 1, 10 }, 0 },
#endif

    OSD_SETTING_ENTRY_TYPE("ROLL RATE", SETTING_ROLL_RATE, CMS_DATA_TYPE_ANGULAR_RATE),
    OSD_SETTING_ENTRY_TYPE("PITCH RATE", SETTING_PITCH_RATE, CMS_DATA_TYPE_ANGULAR_RATE),
    OSD_SETTING_ENTRY_TYPE("YAW RATE", SETTING_YAW_RATE, CMS_DATA_TYPE_ANGULAR_RATE),

    OSD_SETTING_ENTRY("RC EXPO", SETTING_RC_EXPO),
    OSD_SETTING_ENTRY("RC YAW EXP", SETTING_RC_YAW_EXPO),

    OSD_SETTING_ENTRY("THR MID", SETTING_THR_MID),
    OSD_SETTING_ENTRY("THR EXPO", SETTING_THR_EXPO),

    OSD_SETTING_ENTRY("THRPID ATT", SETTING_TPA_RATE),
    OSD_SETTING_ENTRY_STEP("TPA BRKPT", SETTING_TPA_BREAKPOINT, 10),

    { "BACK", OME_Back, NULL, NULL, 0 },
    { NULL, OME_END, NULL, NULL, 0 }
};

static CMS_Menu cmsx_menuRateProfile = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "MENURATE",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = NULL,
    .onExit = NULL,
    .onGlobalExit = NULL,
    .entries = cmsx_menuRateProfileEntries
};

#ifdef NOT_YET
static uint8_t cmsx_dtermSetpointWeight;
static uint8_t cmsx_setpointRelaxRatio;
static uint8_t cmsx_angleStrength;
static uint8_t cmsx_horizonStrength;
static uint8_t cmsx_horizonTransition;

static long cmsx_profileOtherOnEnter(void)
{
    profileIndexString[1] = '0' + tmpProfileIndex;

    cmsx_dtermSetpointWeight = pidProfile()->dtermSetpointWeight;
    cmsx_setpointRelaxRatio  = pidProfile()->setpointRelaxRatio;

    cmsx_angleStrength       = pidProfile()[PIDLEVEL].P;
    cmsx_horizonStrength     = pidProfile()[PIDLEVEL].I;
    cmsx_horizonTransition   = pidProfile()[PIDLEVEL].D;

    return 0;
}

static long cmsx_profileOtherOnExit(const OSD_Entry *self)
{
    UNUSED(self);

    pidProfileMutable()->dtermSetpointWeight = cmsx_dtermSetpointWeight;
    pidProfileMutable()->setpointRelaxRatio  = cmsx_setpointRelaxRatio;

    pidProfileMutable()[PIDLEVEL].P        = cmsx_angleStrength;
    pidProfileMutable()[PIDLEVEL].I        = cmsx_horizonStrength;
    pidProfileMutable()[PIDLEVEL].D        = cmsx_horizonTransition;

    return 0;
}

static OSD_Entry cmsx_menuProfileOtherEntries[] = {
    { "-- OTHER PP --", OME_Label, NULL, profileIndexString, 0 },

    { "D SETPT WT",  OME_FLOAT, NULL, &(OSD_FLOAT_t){ &cmsx_dtermSetpointWeight, 0, 255, 1, 10 }, 0 },
    { "SETPT TRS",   OME_FLOAT, NULL, &(OSD_FLOAT_t){ &cmsx_setpointRelaxRatio,  0, 100, 1, 10 }, 0 },
    { "ANGLE STR",   OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_angleStrength,       0, 200, 1 }    , 0 },
    { "HORZN STR",   OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_horizonStrength,     0, 200, 1 }    , 0 },
    { "HORZN TRS",   OME_UINT8, NULL, &(OSD_UINT8_t){ &cmsx_horizonTransition,   0, 200, 1 }    , 0 },

    { "BACK", OME_Back, NULL, NULL, 0 },
    { NULL, OME_END, NULL, NULL, 0 }
};

static CMS_Menu cmsx_menuProfileOther = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XPROFOTHER",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_profileOtherOnEnter,
    .onExit = cmsx_profileOtherOnExit,
    .onGlobalExit = NULL,
    .entries = cmsx_menuProfileOtherEntries,
};
#endif // NOT_YET

//
// Per profile filters
//
static OSD_Entry cmsx_menuFilterPerProfileEntries[] =
{
    { "-- FILTER PP  --", OME_Label, NULL, profileIndexString, 0 },

    OSD_SETTING_ENTRY("DTERM LPF", SETTING_DTERM_LPF_HZ),
    OSD_SETTING_ENTRY("GYRO SLPF", SETTING_GYRO_LPF_HZ),
    OSD_SETTING_ENTRY("YAW P LIM", SETTING_YAW_P_LIMIT),
    OSD_SETTING_ENTRY("YAW LPF", SETTING_YAW_LPF_HZ),

    { "BACK", OME_Back, NULL, NULL, 0 },
    { NULL, OME_END, NULL, NULL, 0 }
};

static CMS_Menu cmsx_menuFilterPerProfile = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XFLTPP",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = NULL,
    .onExit = NULL,
    .onGlobalExit = NULL,
    .entries = cmsx_menuFilterPerProfileEntries,
};

static OSD_Entry cmsx_menuGyroEntries[] =
{
    { "-- GYRO GLB --", OME_Label, NULL, profileIndexString, 0},

    OSD_SETTING_ENTRY("GYRO SYNC", SETTING_GYRO_SYNC),
    OSD_SETTING_ENTRY("GYRO DENOM", SETTING_GYRO_SYNC_DENOM),
    OSD_SETTING_ENTRY("GYRO LPF", SETTING_GYRO_HARDWARE_LPF),

    {"BACK", OME_Back, NULL, NULL, 0},
    {NULL, OME_END, NULL, NULL, 0}
};

static CMS_Menu cmsx_menuGyro = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XGYROGLB",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = NULL,
    .onExit = NULL,
    .onGlobalExit = NULL,
    .entries = cmsx_menuGyroEntries,
};

static OSD_Entry cmsx_menuImuEntries[] =
{
    { "-- PID TUNING --", OME_Label, NULL, NULL, 0},

    // Profile dependent
    {"PID PROF",   OME_UINT8,   cmsx_profileIndexOnChange,     &(OSD_UINT8_t){ &tmpProfileIndex, 1, MAX_PROFILE_COUNT, 1}, 0},
    {"PID",        OME_Submenu, cmsMenuChange,                 &cmsx_menuPid,                                              0},
    {"PID ALTMAG", OME_Submenu, cmsMenuChange,                 &cmsx_menuPidAltMag,                                        0},
    {"PID GPSNAV", OME_Submenu, cmsMenuChange,                 &cmsx_menuPidGpsnav,                                        0},
    {"FILT PP",   OME_Submenu, cmsMenuChange,                 &cmsx_menuFilterPerProfile,                                  0},

    // Rate profile dependent
    {"RATE PROF", OME_UINT8,   cmsx_profileIndexOnChange, &(OSD_UINT8_t){ &tmpProfileIndex, 1, MAX_CONTROL_RATE_PROFILE_COUNT, 1}, 0},
    {"RATE",      OME_Submenu, cmsMenuChange,                 &cmsx_menuRateProfile,                                       0},
    {"MANU RATE", OME_Submenu, cmsMenuChange,                 &cmsx_menuManualRateProfile,                                 0},

    // Global
    {"GYRO GLB",  OME_Submenu, cmsMenuChange,                 &cmsx_menuGyro,                                              0},

#ifdef NOT_YET
    {"OTHER PP",  OME_Submenu, cmsMenuChange,                 &cmsx_menuProfileOther,                                      0},
    // Profile independent
    {"FILT GLB",  OME_Submenu, cmsMenuChange,                 &cmsx_menuFilterGlobal,                                      0},
#endif

    {"BACK", OME_Back, NULL, NULL, 0},
    {NULL, OME_END, NULL, NULL, 0}
};

CMS_Menu cmsx_menuImu = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "XIMU",
    .GUARD_type = OME_MENU,
#endif
    .onEnter = cmsx_menuImu_onEnter,
    .onExit = cmsx_menuImu_onExit,
    .onGlobalExit = NULL,
    .entries = cmsx_menuImuEntries,
};
#endif // CMS
