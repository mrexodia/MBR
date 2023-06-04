# MBR

PoC driver for the process isolation.

## Requirements

- Visual Studio 2022
- [WDK 10](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk) (didn't test with 11)
- CMake 3.15+

## Building

```
cmake -B build
cmake --build build --config Release
```

You can open `build\MBR.sln` in Visual Studio and work there.