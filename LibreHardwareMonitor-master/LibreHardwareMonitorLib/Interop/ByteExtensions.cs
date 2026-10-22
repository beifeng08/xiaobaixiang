// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using Windows.Win32;

namespace LibreHardwareMonitor.Interop;

#if NET8_0_OR_GREATER

internal static class ByteExtensions
{
    public static byte[] ToArray(this __byte_4 bytes)
    {
        return bytes.AsReadOnlySpan().ToArray();
    }
}

#endif
