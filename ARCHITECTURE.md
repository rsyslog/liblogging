# Liblogging Architecture

## Overview

Liblogging is a lightweight, easy-to-use logging library for C applications. It aims to provide an enhanced replacement for the traditional `syslog(3)` API while maintaining simplicity and adding modern features like signal safety and multiple log destinations.

## Components

The repository is divided into two main components:

### 1. stdlog (Standard Logging)
This is the core, active component of the library. It is located in the `stdlog/` directory.

*   **Purpose**: To provide a modern logging API that separates the logging call from the log destination.
*   **Key Features**:
    *   **Signal-Safe**: Can be safely called from within signal handlers (when initialized with `STDLOG_SIGSAFE`).
    *   **Thread-Safe**: Supports multi-threaded applications.
    *   **Driver-Based**: Uses a driver layer to send logs to different destinations.

### 2. rfc3195 (Legacy)
Located in the `rfc3195/` directory. This is an implementation of the RFC 3195 standard (syslog over BEEP). It is considered a legacy component and is not recommended for new applications.

## stdlog Architecture

The `stdlog` component is designed around a **Separation of Concerns** principle:
*   **Application Developer**: Uses the `stdlog` API to emit log messages without worrying about where they go.
*   **System Administrator**: Configures the logging driver (via environment variables or application configuration) to direct logs to the appropriate destination.

### Drivers

`stdlog` uses a driver model to handle the actual transmission or storage of log messages. Supported drivers include:

*   **syslog**: Sends messages to the traditional system syslog daemon (e.g., via `/dev/log`).
*   **journal**: Writes directly to the systemd journal using its native API.
*   **file**: Writes messages to a specified file on the filesystem.
*   **uxsock**: Sends messages to a specified Unix domain socket.

### Data Flow

1.  **Initialization**: The application initializes the library (optional, but recommended for threading/signal safety).
2.  **Channel Creation**: A logging channel is opened via `stdlog_open()`. This selects the driver based on the provided connection string (or default).
3.  **Logging**: The application calls `stdlog_log()` or related functions.
4.  **Formatting**: The message is formatted (often into a syslog-like format) within the library.
5.  **Dispatch**: The active driver takes the formatted message and transmits it to the destination (file, socket, journal, etc.).

## Build System

The project uses GNU Autotools (`autoconf`, `automake`, `libtool`).
*   **configure.ac**: Main build configuration.
*   **Makefile.am**: Automake definitions.
