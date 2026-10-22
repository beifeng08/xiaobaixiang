// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;

namespace LibreHardwareMonitor.Software;

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
