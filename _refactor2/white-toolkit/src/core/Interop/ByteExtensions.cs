// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using Windows.Win32;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Interop;

#if NET8_0_OR_GREATER

internal static class ByteExtensions
{
    public static byte[] ToArray(this __byte_4 bytes)
    {
        return bytes.AsReadOnlySpan().ToArray();
    }
}

#endif
