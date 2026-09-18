package com.ember.launcher.ui

import android.content.Context
import android.view.KeyEvent
import android.view.MotionEvent
import android.widget.Button
import android.widget.LinearLayout
import java.io.OutputStream

class Input(private val context: Context) {

    interface InputListener {
        fun onKeyEvent(scancode: Int, pressed: Boolean)
        fun onPointerEvent(x: Float, y: Float, pressed: Boolean)
    }

    private var listener: InputListener? = null
    private var qemuStdin: OutputStream? = null

    private val keycodeToScancode = mapOf(
        KeyEvent.KEYCODE_A to 0x1E,
        KeyEvent.KEYCODE_B to 0x30,
        KeyEvent.KEYCODE_C to 0x2E,
        KeyEvent.KEYCODE_D to 0x20,
        KeyEvent.KEYCODE_E to 0x12,
        KeyEvent.KEYCODE_F to 0x21,
        KeyEvent.KEYCODE_G to 0x22,
        KeyEvent.KEYCODE_H to 0x23,
        KeyEvent.KEYCODE_I to 0x17,
        KeyEvent.KEYCODE_J to 0x24,
        KeyEvent.KEYCODE_K to 0x25,
        KeyEvent.KEYCODE_L to 0x26,
        KeyEvent.KEYCODE_M to 0x32,
        KeyEvent.KEYCODE_N to 0x31,
        KeyEvent.KEYCODE_O to 0x18,
        KeyEvent.KEYCODE_P to 0x19,
        KeyEvent.KEYCODE_Q to 0x10,
        KeyEvent.KEYCODE_R to 0x13,
        KeyEvent.KEYCODE_S to 0x1F,
        KeyEvent.KEYCODE_T to 0x14,
        KeyEvent.KEYCODE_U to 0x16,
        KeyEvent.KEYCODE_V to 0x2F,
        KeyEvent.KEYCODE_W to 0x11,
        KeyEvent.KEYCODE_X to 0x2D,
        KeyEvent.KEYCODE_Y to 0x15,
        KeyEvent.KEYCODE_Z to 0x2C,
        KeyEvent.KEYCODE_0 to 0x0B,
        KeyEvent.KEYCODE_1 to 0x02,
        KeyEvent.KEYCODE_2 to 0x03,
        KeyEvent.KEYCODE_3 to 0x04,
        KeyEvent.KEYCODE_4 to 0x05,
        KeyEvent.KEYCODE_5 to 0x06,
        KeyEvent.KEYCODE_6 to 0x07,
        KeyEvent.KEYCODE_7 to 0x08,
        KeyEvent.KEYCODE_8 to 0x09,
        KeyEvent.KEYCODE_9 to 0x0A,
        KeyEvent.KEYCODE_SPACE to 0x39,
        KeyEvent.KEYCODE_ENTER to 0x1C,
        KeyEvent.KEYCODE_DEL to 0x0E,
        KeyEvent.KEYCODE_TAB to 0x0F,
        KeyEvent.KEYCODE_SHIFT_LEFT to 0x2A,
        KeyEvent.KEYCODE_SHIFT_RIGHT to 0x36,
        KeyEvent.KEYCODE_CTRL_LEFT to 0x1D,
        KeyEvent.KEYCODE_ALT_LEFT to 0x38,
        KeyEvent.KEYCODE_ESCAPE to 0x01,
        KeyEvent.KEYCODE_CAPS_LOCK to 0x3A,
        KeyEvent.KEYCODE_DPAD_UP to 0x48,
        KeyEvent.KEYCODE_DPAD_DOWN to 0x50,
        KeyEvent.KEYCODE_DPAD_LEFT to 0x4B,
        KeyEvent.KEYCODE_DPAD_RIGHT to 0x4D
    )

    fun setListener(inputListener: InputListener) {
        listener = inputListener
    }

    fun attachQemuStdin(stream: OutputStream) {
        qemuStdin = stream
    }

    fun handleKeyEvent(keyCode: Int, event: KeyEvent): Boolean {
        val scancode = keycodeToScancode[keyCode] ?: return false
        val pressed = event.action == KeyEvent.ACTION_DOWN

        listener?.onKeyEvent(scancode, pressed)
        sendScancodeToQemu(scancode, pressed)

        return true
    }

    fun handleTouchEvent(event: MotionEvent): Boolean {
        val x = event.x
        val y = event.y

        when (event.action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                listener?.onPointerEvent(x, y, true)
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                listener?.onPointerEvent(x, y, false)
            }
        }

        return true
    }

    private fun sendScancodeToQemu(scancode: Int, pressed: Boolean) {
        val stream = qemuStdin ?: return
        val code = if (pressed) scancode else (scancode or 0x80)

        try {
            stream.write(code)
            stream.flush()
        } catch (e: Exception) {
        }
    }

    fun buildVirtualKeyboard(parent: LinearLayout) {
        val row1 = LinearLayout(context)
        row1.orientation = LinearLayout.HORIZONTAL

        val escButton = Button(context)
        escButton.text = "Esc"
        escButton.setOnClickListener {
            listener?.onKeyEvent(0x01, true)
            sendScancodeToQemu(0x01, true)
            sendScancodeToQemu(0x01, false)
        }
        row1.addView(escButton)

        val tabButton = Button(context)
        tabButton.text = "Tab"
        tabButton.setOnClickListener {
            sendScancodeToQemu(0x0F, true)
            sendScancodeToQemu(0x0F, false)
        }
        row1.addView(tabButton)

        val ctrlButton = Button(context)
        ctrlButton.text = "Ctrl"
        ctrlButton.setOnClickListener {
            sendScancodeToQemu(0x1D, true)
            sendScancodeToQemu(0x1D, false)
        }
        row1.addView(ctrlButton)

        val enterButton = Button(context)
        enterButton.text = "Enter"
        enterButton.setOnClickListener {
            sendScancodeToQemu(0x1C, true)
            sendScancodeToQemu(0x1C, false)
        }
        row1.addView(enterButton)

        parent.addView(row1)
    }

    fun release() {
        qemuStdin = null
        listener = null
    }
}
