package com.ember.launcher

import android.os.Bundle
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.ember.launcher.build.AppCompile
import java.io.BufferedReader

class MainActivity : AppCompatActivity() {

    private lateinit var statusView: TextView
    private lateinit var logView: TextView
    private lateinit var startButton: Button
    private lateinit var stopButton: Button
    private lateinit var restartButton: Button

    private lateinit var appCompile: AppCompile
    private var qemuProcess: Process? = null
    private var isRunning = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        appCompile = AppCompile(this)
        buildLayout()
    }

    private fun buildLayout() {
        val root = LinearLayout(this)
        root.orientation = LinearLayout.VERTICAL
        root.setPadding(24, 24, 24, 24)

        statusView = TextView(this)
        statusView.text = "Ember Launcher ready"
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

        root.addView(buttonRow)

        val scroll = ScrollView(this)
        logView = TextView(this)
        logView.text = ""
        scroll.addView(logView)
        root.addView(scroll)

        setContentView(root)
    }

    private fun appendLog(line: String) {
        runOnUiThread {
            logView.append(line + "\n")
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

    private fun startEmber() {
        if (isRunning) {
            return
        }

        setStatus("Preparing environment")
        Thread { launchEmber() }.start()
    }

    private fun stopEmber() {
        val process = qemuProcess
        if (process != null && process.isAlive) {
            process.destroy()
            appendLog("QEMU process stopped")
        }
        setRunningState(false)
        setStatus("Stopped")
    }

    private fun restartEmber() {
        stopEmber()
        Thread {
            Thread.sleep(500)
            launchEmber()
        }.start()
    }

    private fun launchEmber() {
        val result = appCompile.compile()

        if (!result.success) {
            setStatus("Failed: ${result.message}")
            appendLog(result.message)
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
                "-vga", "std"
            ).redirectErrorStream(true).start()

            qemuProcess = process
            setRunningState(true)
            setStatus("Ember OS running")

            readProcessOutput(process.inputStream.bufferedReader())

            val exitCode = process.waitFor()
            appendLog("QEMU exited with code $exitCode")
            setRunningState(false)
            setStatus(if (exitCode == 0) "Stopped" else "Crashed (code $exitCode)")
        } catch (e: Exception) {
            appendLog("Launch failed: ${e.message}")
            setStatus("Launch failed")
            setRunningState(false)
        }
    }

    private fun readProcessOutput(reader: BufferedReader) {
        reader.forEachLine { line ->
            appendLog(line)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        qemuProcess?.let {
            if (it.isAlive) {
                it.destroy()
            }
        }
        appCompile.cleanup()
    }
}
