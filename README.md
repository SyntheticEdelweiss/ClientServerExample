# Client-Server TCP Example

A pair of Qt applications demonstrating a TCP-based request/response. Client submits tasks with arbitrary data, and Server processes them in parallel and streams progress updates back to the Client, then returns a final result. Client configures tasks and displays results, featuring responsive non-blocking UI.

---

## Features

- Client-Server communication over TCP  
- Authentication and host-whitelist filtering on the Server  
- Multithreaded task execution using QThreadPool + QtConcurrent  
- Real-time progress updates streamed from Server to Client  
- Early task cancellation support  
- Server-side caching of completed request results  
- Bidirectional data validation  
- Configurable message format (JSON or binary) and endianness  
- Non-blocking Qt GUI for the Client (uses QtCharts for progress visualization)  
- Comprehensive logging on both sides  

---

## Requirements

- C++17  
- Qt 5.15 (Core, Network, Widgets, Charts)  
- CMake ≥ 3.16  
- Compatible C++ compiler

---

## CMake Configuration Options

You can toggle message format, byte-order, and other settings at build time.  

| Option                 | Description                                          | Values             | Default   |
|------------------------|------------------------------------------------------|--------------------|-----------|
| `MESSAGE_FORMAT`       | Send messages in either JSON or binary format        | JSON / BINARY      | JSON      |
| `ENDIANNESS`           | Byte order for request/reponse messages              | LITTLE / BIG       | LITTLE    |

---

## Build Instructions

   ```bash
   git clone https://github.com/SyntheticEdelweiss/ClientServerExample.git
   cd ./ClientServerExample
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-w
   cmake --build build --clean-first
   ```

   Depending on your system configuration, you might need to explicitly specify some extra stuff, e.g.
   ```bash
   cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-w -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/mingw81_64" -DCMAKE_C_COMPILER="C:\Qt\Tools\mingw810_64\bin\gcc.exe" -DCMAKE_CXX_COMPILER="C:\Qt\Tools\mingw810_64\bin\g++.exe"
   cmake --build build --clean-first
   ```