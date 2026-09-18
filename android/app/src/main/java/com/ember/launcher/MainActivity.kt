package com.ember.launcher

import android.os.Bundle
import android.widget.TextView
import android.widget.LinearLayout
import androidx.appcompat.app.AppCompatActivity
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity() {

    private lateinit var statusView: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this)
        layout.orientation = LinearLayout.VERTICAL

        statusView = TextView(this)
        statusView.text = "Preparing Ember OS"
        layout.addView(statusView)

        setContentView(layout)

        Thread { launchEmber() }.start()
    }

    private fun copyAssetToFile(assetName: String, outFile: File) {
        assets.open(assetName).use { input ->
            FileOutputStream(outFile).use { output ->
                input.copyTo(output)
            }
        }
    }

    private fun launchEmber() {
        val isoFile = File(filesDir, "ember.iso")
        copyAssetToFile("ember.iso", isoFile)

        val nativeDir = applicationInfo.nativeLibraryDir
        val qemuBinary = File(nativeDir, "libqemu_system_i386.so")

        if (!qemuBinary.exists()) {
            runOnUiThread { statusView.text = "QEMU binary not found" }
            return
        }

        val process = ProcessBuilder(
            qemuBinary.absolutePath,
            "-cdrom", isoFile.absolutePath,
            "-m", "128",
            "-display", "sdl",
            "-vga", "std"
        ).redirectErrorStream(true).start()

        runOnUiThread { statusView.text = "Ember OS running" }

        process.inputStream.bufferedReader().forEachLine { }
    }
}
