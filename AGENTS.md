# Instructions for AI Agents

This file provides context and instructions for AI agents working on the `liblogging` repository.

## Project Overview
`liblogging` is a C library for logging, offering an enhanced replacement for `syslog(3)`.

*   **Primary Component**: `stdlog` (Standard Logging) - Focus your efforts here.
*   **Legacy Component**: `rfc3195` - Only touch if explicitly requested.

## Build Instructions

The project uses GNU Autotools.

### Prerequisites
*   gcc
*   make
*   autoconf, automake, libtool
*   pkg-config
*   rst2man (optional, for man pages)

### Building form Source
1.  **Generate build scripts**:
    ```bash
    autoreconf -fi
    ```
2.  **Configure**:
    ```bash
    ./configure
    # If rst2man is missing:
    ./configure --disable-man-pages
    ```
3.  **Compile**:
    ```bash
    make
    ```

## Directory Structure

*   `stdlog/`: Source code for the main library.
    *   `stdlog.c/h`: Main API implementation.
    *   `*drvr.c`: Driver implementations (e.g., `file.c`, `uxsock.c`).
    *   `tester.c`: Basic test utility.
*   `rfc3195/`: Legacy RFC 3195 implementation.
*   `m4/`: Autotools macros.

## Testing

*   **stdlog**: A basic tester binary is built in `stdlog/tester`.
    *   Usage: `stdlog/tester -p "driver:specification"`
    *   Example: `stdlog/tester -p "file:/tmp/test.log"`

## Code Guidelines

*   **Language**: C (C99/C11 compatible).
*   **Style**: Follow the existing coding style (K&R-ish).
*   **Safety**: Pay special attention to signal safety and thread safety in `stdlog`. Avoid using non-signal-safe functions (like `malloc`, `printf`) in critical paths if `STDLOG_SIGSAFE` is enabled.
*   **Documentation**: Update `.rst` files in `stdlog/` when changing the API.

## Known Issues
*   The `journalemu` component mentioned in older documentation does not exist in the codebase.
