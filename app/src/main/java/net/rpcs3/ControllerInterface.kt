package net.rpcs3

import android.util.Log
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent

class ControllerHandler {

    fun captureMotionEvent(event: MotionEvent): Boolean {
        if (isGamepad(event.device)) {
            Log.d("ControllerHandler", event.toString())
            return true
        }
        return false
    }

    fun captureKeyEvent(event: KeyEvent): Boolean {
        if (isGamepad(event.device)) {
            Log.d("ControllerHandler", event.toString())
            return true
        }
        return false
    }

    fun printConnectedDevices() {
        // Get all device IDs connected to the system
        val deviceIds = InputDevice.getDeviceIds()

        // Loop through all device IDs
        for (deviceId in deviceIds) {
            // Retrieve the InputDevice for this ID
            val device = InputDevice.getDevice(deviceId)

            // Print some basic details about the device
            if (device != null) {

                if (!device.isVirtual) {
                    Log.d("ControllerHandler", "Device Dump: $device")
                }

            } else {
                Log.d("ControllerHandler", "Failed to retrieve device with ID: $deviceId")
            }
        }
    }

    fun isGamepad(device: InputDevice): Boolean{
        return device.sources and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD ||
                device.sources and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK
    }
}