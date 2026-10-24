// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (C) LibreHardwareMonitor and Contributors.
// Partial Copyright (C) Michael Möller <mmoeller@openhardwaremonitor.org> and Contributors.
// All Rights Reserved.

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
