// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;

namespace SysCore.Software;

public static class OperatingSystem
{
    
    static OperatingSystem()
    {
        
        PlatformID platform = Environment.OSVersion.Platform;
        Version version = Environment.OSVersion.Version;
        IsUnix = platform is PlatformID.Unix or PlatformID.MacOSX;

        if (Environment.Is64BitOperatingSystem)
            Is64Bit = true;

        IsWindows8OrGreater = !IsUnix && ((version.Major == 6 && version.Minor >= 2) || version.Major > 6);
    }

    public static bool Is64Bit { get; }

    public static bool IsUnix { get; }

    public static bool IsWindows8OrGreater { get; }
}
