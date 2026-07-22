# External

> A high-performance native Android library built with C/C++ and the Android NDK, designed for JNI integration and low-level native Android development.

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Android-success?style=for-the-badge">
  <img src="https://img.shields.io/badge/Language-C%20%7C%20C++-blue?style=for-the-badge">
  <img src="https://img.shields.io/badge/Build-Android%20NDK-orange?style=for-the-badge">
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge">
</p>

---

# Overview

**External** is a native Android project developed with **C/C++**, **JNI**, and the **Android NDK**.

The project demonstrates a modular native architecture focused on high performance, portability, and clean code organization. It serves as a foundation for Android native development, JNI integration, and other low-level Android programming experiments.

The source code is designed to be:

- Modular
- Lightweight
- Fast
- Maintainable
- Cross-architecture compatible

---

# Features

- Native C implementation
- Native C++ implementation
- Android NDK support
- JNI integration
- Modular architecture
- Optimized native performance
- Multi-architecture support
- Lightweight binaries
- Efficient native utilities
- Clean project structure
- Low runtime overhead
- Easy to extend

---

# Repository Structure

```text
External/
│
├── jni/
│   ├── Android.mk
│   ├── Application.mk
│   ├── *.c
│   ├── *.cpp
│   └── *.h
│
├── obj/
│   └── local/
│       ├── arm64-v8a/
│       ├── armeabi-v7a/
│       └── x86_64/
│
├── compile.sh
├── LICENSE
└── README.md
```

---

# Supported Architectures

| ABI | Status |
|------|--------|
| arm64-v8a | ✅ Supported |
| armeabi-v7a | ✅ Supported |
| x86_64 | ✅ Supported |

---

# System Requirements

## Operating System

- Ubuntu 20.04+
- Ubuntu 22.04+
- Debian
- WSL2
- macOS (NDK supported)

## Development Tools

- Android NDK r26+
- Android SDK
- Java JDK 11+
- CMake 3.22+
- GNU Make
- Git

---

# Getting Started

Clone the repository:

```bash
git clone https://github.com/sudo-dava25/External.git

cd External
```

Configure your Android NDK:

```bash
export ANDROID_NDK_HOME=/path/to/android-ndk

export PATH=$ANDROID_NDK_HOME:$PATH
```

Grant execute permission:

```bash
chmod +x compile.sh
```

Compile:

```bash
./compile.sh
```

---

# Build Output

Compiled binaries are generated inside:

```text
obj/local/
```

Example:

```text
obj/local/
├── arm64-v8a/
├── armeabi-v7a/
└── x86_64/
```

---

# GitHub Actions

This repository supports automated compilation using **GitHub Actions**.

Each workflow automatically:

- Checks out the repository
- Configures the build environment
- Installs Android SDK
- Installs Android NDK
- Executes the build script
- Generates binaries
- Uploads artifacts

Workflow triggers:

- Push
- Pull Request
- Manual Dispatch

---

# JNI Integration

Load the native library inside your Android application:

```java
static {
    System.loadLibrary("external");
}
```

> Replace `"external"` with the actual generated library name if different.

---

# Native Components

The project includes modules for:

- JNI interface
- Native utilities
- Android NDK integration
- Architecture abstraction
- Common helper functions
- Performance-oriented native code

---

# Performance

The project is designed with performance as a primary objective.

Highlights:

- Optimized native execution
- Minimal JNI overhead
- Lightweight binaries
- Efficient memory usage
- Low CPU utilization
- Fast startup

---

# Supported Platforms

| Platform | Status |
|----------|--------|
| Android 5.0+ | ✅ |
| ARM64 | ✅ |
| ARMv7 | ✅ |
| x86_64 | ✅ |

---

# Development Principles

This project follows several core principles:

- Clean code
- Maintainable architecture
- High performance
- Readability
- Portability
- Scalability
- Modularity

---

# Contributing

Contributions are welcome.

1. Fork this repository

2. Create a feature branch

```bash
git checkout -b feature/my-feature
```

3. Commit your changes

```bash
git commit -m "Add new feature"
```

4. Push your branch

```bash
git push origin feature/my-feature
```

5. Open a Pull Request

---

# License

This project is distributed under the **MIT License**.

See the **LICENSE** file for additional information.

---

# Disclaimer

This repository is intended for **educational**, **research**, and **native Android development** purposes.

Users are responsible for ensuring compliance with:

- Local laws and regulations
- Platform policies
- Applicable terms of service

The maintainers assume no responsibility for misuse or any damages resulting from the use of this software.

---

# Support

If you encounter an issue:

- Open a GitHub Issue
- Include build logs
- Include Android version
- Include device information
- Include target ABI

Providing detailed information helps resolve issues more efficiently.

---

# Author

**sudo-dava25**

Android Native Development • C/C++ • Android NDK • JNI

GitHub: https://github.com/sudo-dava25

---

<p align="center">
⭐ If you find <b>External</b> useful, consider giving this repository a star.
</p>
