// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Runtime.InteropServices;

namespace XiaobaiHardwareMonitor.Interop;

internal class LibC
{
    private const string DllName = "libc";

    [DllImport(DllName)]
    internal static extern int sched_getaffinity(int pid, IntPtr maskSize, ref ulong mask);

    [DllImport(DllName)]
    internal static extern int sched_setaffinity(int pid, IntPtr maskSize, ref ulong mask);
}