# TTI Launcher

A clean Qt 6 / C++ replication of the **Toontown Infinite** v1.2.0 launcher originally built with Qt 5.5.

The implementation follows the logic recovered from the original binary via IDA analysis.

## Building

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is written to `build/tti-launcher`. The game files will be located in the same directory as the launcher.

## Notes

- The manifest hashes are MD5 hex strings
- File updates are fetched as `<path>.bz2` and extracted in place.
- The user-agent is `TTI-Launcher/1.2.0 (live/win32)`.
