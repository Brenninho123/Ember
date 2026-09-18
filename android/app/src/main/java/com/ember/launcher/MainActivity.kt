package com.ember.launcher

import android.os.Bundle
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.ember.launcher.build.AppCompile
import com.ember.launcher.ui.Input
import java.io.BufferedReader
import java.io.OutputStream

class MainActivity : AppCompatActivity(), Input.InputListener {

    private lateinit var statusView: TextView
    private lateinit var logView: TextView
    private lateinit var logScroll: ScrollView
    private lateinit var startButton: Button
    private lateinit var stopButton: Button
    private lateinit var restartButton: Button
    private lateinit var keyboardToggle: Button
    private lateinit var virtualKeyboardLayout: LinearLayout

    private lateinit var appCompile: AppCompile
    private lateinit var input: Input

    private var qemuProcess: Process? = null
    private var qemuStdin: OutputStream? = null
    private var isRunning = false
    private var launchThread: Thread? = null
    private var restartRequested = false

    private val maxLogLines = 500
    private val logLines = ArrayDeque<String>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        appCompile = AppCompile(this)
        input = Input(this)
        input.setListener(this)

        buildLayout()
    }

    private fun buildLayout() {
        val root = LinearLayout(this)
        root.orientation = LinearLayout.VERTICAL
        root.setPadding(24, 24, 24, 24)
        root.isFocusableInTouchMode = true

        statusView = TextView(this)
        statusView.text = "Ember Launcher ready"
        statusView.textSize = 16f
        root.addView(statusView)

        val buttonRow = LinearLayout(this)
        buttonRow.orientation = LinearLayout.HORIZONTAL

        startButton = Button(this)
        startButton.text = "Start"
        startButton.setOnClickListener { startEmber() }
        buttonRow.addView(startButton)

        stopButton = Button(this)
        stopButton.text = "Stop"
        stopButton.isEnabled = false
        stopButton.setOnClickListener { stopEmber() }
        buttonRow.addView(stopButton)

        restartButton = Button(this)
        restartButton.text = "Restart"
        restartButton.isEnabled = false
        restartButton.setOnClickListener { restartEmber() }
        buttonRow.addView(restartButton)

        keyboardToggle = Button(this)
        keyboardToggle.text = "Keyboard"
        keyboardToggle.setOnClickListener { toggleVirtualKeyboard() }
        buttonRow.addView(keyboardToggle)

        root.addView(buttonRow)

        virtualKeyboardLayout = LinearLayout(this)
        virtualKeyboardLayout.orientation = LinearLayout.VERTICAL
        virtualKeyboardLayout.visibility = View.GONE
        input.buildVirtualKeyboard(virtualKeyboardLayout)
        root.addView(virtualKeyboardLayout)

        logScroll = ScrollView(this)
        logView = TextView(this)
        logView.text = ""
        logView.setPadding(8, 8, 8, 8)
        logScroll.addView(logView)
        root.addView(logScroll)

        root.setOnTouchListener { _, event -> input.handleTouchEvent(event) }

        setContentView(root)
    }

    private fun appendLog(line: String) {
        runOnUiThread {
            logLines.addLast(line)
            if (logLines.size > maxLogLines) {
                logLines.removeFirst()
            }
            logView.text = logLines.joinToString("\n")
            logScroll.post { logScroll.fullScroll(View.FOCUS_DOWN) }
        }
    }

    private fun setStatus(text: String) {
        runOnUiThread {
            statusView.text = text
        }
    }

    private fun setRunningState(running: Boolean) {
        isRunning = running
        runOnUiThread {
            startButton.isEnabled = !running
            stopButton.isEnabled = running
            restartButton.isEnabled = running
        }
    }

    private fun toggleVirtualKeyboard() {
        virtualKeyboardLayout.visibility =
            if (virtualKeyboardLayout.visibility == View.VISIBLE) View.GONE else View.VISIBLE
    }

    private fun startEmber() {
        if (isRunning) {
            return
        }

        setStatus("Preparing environment")
        launchThread = Thread { launchEmber() }
        launchThread?.start()
    }

    private fun stopEmber() {
        val process = qemuProcess
        if (process != null && process.isAlive) {
            process.destroy()
            appendLog("QEMU process stopped")
        }
        qemuStdin = null
        input.release()
        setRunningState(false)
        setStatus("Stopped")
    }

    private fun restartEmber() {
        restartRequested = true
        stopEmber()
    }

    private fun launchEmber() {
        val result = appCompile.compile()

        if (!result.success) {
            setStatus("Failed: ${result.message}")
            appendLog(result.message)
            notifyError(result.message)
            return
        }

        setStatus(result.message)
        appendLog("ISO ready at ${result.isoFile?.absolutePath}")
        appendLog("QEMU binary at ${result.qemuBinary?.absolutePath}")

        try {
            val process = ProcessBuilder(
                result.qemuBinary!!.absolutePath,
                "-cdrom", result.isoFile!!.absolutePath,
                "-m", "128",
                "-display", "sdl",
                "-vga", "std",
                "-serial", "stdio"
            ).redirectErrorStream(true).start()

            qemuProcess = process
            qemuStdin = process.outputStream
            input.attachQemuStdin(process.outputStream)

            setRunningState(true)
            setStatus("Ember OS running")

            readProcessOutput(process.inputStream.bufferedReader())

            val exitCode = process.waitFor()
            appendLog("QEMU exited with code $exitCode")
            setRunningState(false)
            qemuStdin = null

            if (restartRequested) {
                restartRequested = false
                Thread.sleep(500)
                launchEmber()
                return
            }

            setStatus(if (exitCode == 0) "Stopped" else "Crashed (code $exitCode)")
        } catch (e: Exception) {
            appendLog("Launch failed: ${e.message}")
            setStatus("Launch failed")
            notifyError(e.message ?: "Unknown error")
            setRunningState(false)
        }
    }

    private fun readProcessOutput(reader: BufferedReader) {
        reader.forEachLine { line ->
            appendLog(line)
        }
    }

    private fun notifyError(message: String) {
        runOnUiThread {
            Toast.makeText(this, message, Toast.LENGTH_LONG).show()
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        if (isRunning && input.handleKeyEvent(keyCode, event)) {
            return true
        }
        return super.onKeyDown(keyCode, event)
    }

    override fun onKeyUp(keyCode: Int, event: KeyEvent): Boolean {
        if (isRunning && input.handleKeyEvent(keyCode, event)) {
            return true
        }
        return super.onKeyUp(keyCode, event)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        input.handleTouchEvent(event)
        return super.onTouchEvent(event)
    }

    override fun onKeyEvent(scancode: Int, pressed: Boolean) {
    }

    override fun onPointerEvent(x: Float, y: Float, pressed: Boolean) {
    }

    override fun onPause() {
        super.onPause()
        appendLog("Activity paused")
    }

    override fun onResume() {
        super.onResume()
        appendLog("Activity resumed")
    }

    override fun onDestroy() {
        super.onDestroy()
        val process = qemuProcess
        if (process != null && process.isAlive) {
            process.destroy()
        }
        input.release()
        appCompile.cleanup()
    }
}
