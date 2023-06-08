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

## Resources

- https://github.com/OSRDrivers/kmexts
- https://www.triplefault.io/2017/09/enumerating-process-thread-and-image.html
- http://blog.deniable.org/posts/windows-callbacks/
- https://github.com/hfiref0x/WinObjEx64/blob/master/Docs/Callbacks.pdf
- https://www.tiraniddo.dev/2019/11/the-internals-of-applocker-part-2.html