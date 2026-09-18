package com.ember.launcher.build

import android.content.Context
import java.io.File
import java.io.FileOutputStream

class AppCompile(private val context: Context) {

    data class CompileResult(
        val success: Boolean,
        val isoFile: File?,
        val qemuBinary: File?,
        val message: String
    )

    private fun resolveAbi(): String {
        val supported = android.os.Build.SUPPORTED_ABIS
        for (abi in supported) {
            when (abi) {
                "arm64-v8a" -> return "arm64-v8a"
                "armeabi-v7a" -> return "armeabi-v7a"
                "x86_64" -> return "x86_64"
                "x86" -> return "x86"
            }
        }
        return supported.firstOrNull() ?: "unknown"
    }

    private fun resolveQemuBinaryName(): String {
        return "libqemu_system_i386.so"
    }

    private fun copyAssetToFile(assetName: String, outFile: File): Boolean {
        return try {
            context.assets.open(assetName).use { input ->
                FileOutputStream(outFile).use { output ->
                    input.copyTo(output)
                }
            }
            true
        } catch (e: Exception) {
            false
        }
    }

    private fun locateQemuBinary(): File? {
        val nativeDir = context.applicationInfo.nativeLibraryDir
        val binaryName = resolveQemuBinaryName()
        val binary = File(nativeDir, binaryName)
        return if (binary.exists()) binary else null
    }

    private fun prepareIso(): File? {
        val isoFile = File(context.filesDir, "ember.iso")

        if (isoFile.exists()) {
            return isoFile
        }

        val copied = copyAssetToFile("ember.iso", isoFile)
        return if (copied) isoFile else null
    }

    private fun verifyExecutablePermission(file: File): Boolean {
        if (!file.canExecute()) {
            return file.setExecutable(true)
        }
        return true
    }

    fun compile(): CompileResult {
        val abi = resolveAbi()

        if (abi == "unknown") {
            return CompileResult(
                success = false,
                isoFile = null,
                qemuBinary = null,
                message = "Unsupported device ABI"
            )
        }

        val qemuBinary = locateQemuBinary()
        if (qemuBinary == null) {
            return CompileResult(
                success = false,
                isoFile = null,
                qemuBinary = null,
                message = "QEMU binary not found for ABI: $abi"
            )
        }

        if (!verifyExecutablePermission(qemuBinary)) {
            return CompileResult(
                success = false,
                isoFile = null,
                qemuBinary = qemuBinary,
                message = "Failed to set executable permission on QEMU binary"
            )
        }

        val isoFile = prepareIso()
        if (isoFile == null) {
            return CompileResult(
                success = false,
                isoFile = null,
                qemuBinary = qemuBinary,
                message = "Failed to prepare ember.iso"
            )
        }

        return CompileResult(
            success = true,
            isoFile = isoFile,
            qemuBinary = qemuBinary,
            message = "Environment ready on $abi"
        )
    }

    fun cleanup() {
        val isoFile = File(context.filesDir, "ember.iso")
        if (isoFile.exists()) {
            isoFile.delete()
        }
    }
}
