//
// Created by Alex Little on 3/17/25.
//
#include "controllerinterface.h"

#include "Input/ds3_pad_handler.h"
#include "Input/ds4_pad_handler.h"
#include "Input/dualsense_pad_handler.h"
#include "Input/hid_pad_handler.h"
#include "Input/pad_thread.h"
#include "Input/virtual_pad_handler.h"
#include <Emu/Io/pad_config.h>
#include <jni.h>

#include "util/logs.hpp"
#include <android/log.h>

static std::mutex g_virtual_pad_mutex;
static std::shared_ptr<Pad> g_virtual_pad;

std::string g_input_config_override;
cfg_input_configurations g_cfg_input_configs;

LOG_CHANNEL(rpcs3_android_CI, "ANDROID_CI");

static bool initVirtualPad(const std::shared_ptr<Pad> &pad) {
    u32 pclass_profile = 0;
    pad->Init(CELL_PAD_STATUS_CONNECTED,
              CELL_PAD_CAPABILITY_PS3_CONFORMITY |
              CELL_PAD_CAPABILITY_PRESS_MODE |
              CELL_PAD_CAPABILITY_HP_ANALOG_STICK |
              CELL_PAD_CAPABILITY_ACTUATOR //| CELL_PAD_CAPABILITY_SENSOR_MODE
            ,
              CELL_PAD_DEV_TYPE_STANDARD, CELL_PAD_PCLASS_TYPE_STANDARD,
              pclass_profile, 0, 0, 50);

    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::set<u32>{},
                                CELL_PAD_CTRL_UP);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::set<u32>{},
                                CELL_PAD_CTRL_DOWN);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::set<u32>{},
                                CELL_PAD_CTRL_LEFT);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::set<u32>{},
                                CELL_PAD_CTRL_RIGHT);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::set<u32>{},
                                CELL_PAD_CTRL_CROSS);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::set<u32>{},
                                CELL_PAD_CTRL_SQUARE);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::set<u32>{},
                                CELL_PAD_CTRL_CIRCLE);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::set<u32>{},
                                CELL_PAD_CTRL_TRIANGLE);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::set<u32>{},
                                CELL_PAD_CTRL_L1);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::set<u32>{},
                                CELL_PAD_CTRL_L2);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::set<u32>{},
                                CELL_PAD_CTRL_L3);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::set<u32>{},
                                CELL_PAD_CTRL_R1);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::set<u32>{},
                                CELL_PAD_CTRL_R2);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::set<u32>{},
                                CELL_PAD_CTRL_R3);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::set<u32>{},
                                CELL_PAD_CTRL_START);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::set<u32>{},
                                CELL_PAD_CTRL_SELECT);
    pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::set<u32>{},
                                CELL_PAD_CTRL_PS);

    pad->m_sticks[0] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, {}, {});
    pad->m_sticks[1] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, {}, {});
    pad->m_sticks[2] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, {}, {});
    pad->m_sticks[3] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, {}, {});

    pad->m_sensors[0] =
            AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_X, 0, 0, 0, DEFAULT_MOTION_X);
    pad->m_sensors[1] =
            AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_Y, 0, 0, 0, DEFAULT_MOTION_Y);
    pad->m_sensors[2] =
            AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_Z, 0, 0, 0, DEFAULT_MOTION_Z);
    pad->m_sensors[3] =
            AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_G, 0, 0, 0, DEFAULT_MOTION_G);

    pad->m_vibrateMotors[0] = VibrateMotor(true, 0);
    pad->m_vibrateMotors[1] = VibrateMotor(false, 0);

    if (pad->m_player_id == 0) {
        std::lock_guard lock(g_virtual_pad_mutex);
        g_virtual_pad = pad;
    }
    return true;
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_overlayPadData(
        JNIEnv *env, jobject, jint digital1, jint digital2, jint leftStickX,
        jint leftStickY, jint rightStickX, jint rightStickY) {

    auto pad = [] {
        std::shared_ptr<Pad> result;
        std::lock_guard lock(g_virtual_pad_mutex);
        result = g_virtual_pad;
        return result;
    }();

    if (pad == nullptr) {
        return false;
    }

    for (auto &btn : pad->m_buttons) {
        if (btn.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1) {
            btn.m_pressed = (digital1 & btn.m_outKeyCode) != 0;
            btn.m_value = btn.m_pressed ? 127 : 0;
        } else if (btn.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2) {
            btn.m_pressed = (digital2 & btn.m_outKeyCode) != 0;
            btn.m_value = btn.m_pressed ? 127 : 0;
        }
    }

    pad->m_sticks[0].m_value = leftStickX;
    pad->m_sticks[1].m_value = leftStickY;
    pad->m_sticks[2].m_value = rightStickX;
    pad->m_sticks[3].m_value = rightStickY;
    return true;
}

