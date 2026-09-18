plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.ember.launcher"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.ember.launcher"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "0.1"

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    sourceSets {
        getByName("main") {
            jniLibs.srcDirs("src/main/jniLibs")
            assets.srcDirs("src/main/assets")
        }
    }
}

dependencies {
    implementation("androidx.appcompat:appcompat:1.7.0")
}
