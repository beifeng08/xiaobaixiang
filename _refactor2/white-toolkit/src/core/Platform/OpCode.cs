// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
#if !NETFRAMEWORK
using Mono.Unix.Native;
#else
using System.Reflection;
#endif
using Windows.Win32;
using Windows.Win32.System.Memory;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Platform;

internal static class OpCode
{
    public static CpuidDelegate CpuId;
    public static RdtscDelegate Rdtsc;

    private static IntPtr _codeBuffer;
    private static ulong _size;

    private static readonly byte[] CpuId32 =
    [
        0x55, 
        0x8B,
        0xEC, 
        0x83,
        0xEC,
        0x10, 
        0x8B,
        0x45,
        0x08, 
        0x8B,
        0x4D,
        0x0C, 
        0x53, 
        0x0F,
        0xA2, 
        0x56, 
        0x8D,
        0x75,
        0xF0, 
        0x89,
        0x06, 
        0x8B,
        0x45,
        0x10, 
        0x89,
        0x5E,
        0x04, 
        0x89,
        0x4E,
        0x08, 
        0x89,
        0x56,
        0x0C, 
        0x8B,
        0x4D,
        0xF0, 
        0x89,
        0x08, 
        0x8B,
        0x45,
        0x14, 
        0x8B,
        0x4D,
        0xF4, 
        0x89,
        0x08, 
        0x8B,
        0x45,
        0x18, 
        0x8B,
        0x4D,
        0xF8, 
        0x89,
        0x08, 
        0x8B,
        0x45,
        0x1C, 
        0x8B,
        0x4D,
        0xFC, 
        0x5E, 
        0x89,
        0x08, 
        0x5B, 
        0xC9, 
        0xC2,
        0x18,
        0x00 
    ];

    private static readonly byte[] CpuId64Linux =
    [
        0x49,
        0x89,
        0xD2, 
        0x49,
        0x89,
        0xCB, 
        0x53, 
        0x89,
        0xF8, 
        0x89,
        0xF1, 
        0x0F,
        0xA2, 
        0x41,
        0x89,
        0x02, 
        0x41,
        0x89,
        0x1B, 
        0x41,
        0x89,
        0x08, 
        0x41,
        0x89,
        0x11, 
        0x5B, 
        0xC3 
    ];

    private static readonly byte[] CpuId64Windows =
    [
        0x48,
        0x89,
        0x5C,
        0x24,
        0x08, 
        0x8B,
        0xC1, 
        0x8B,
        0xCA, 
        0x0F,
        0xA2, 
        0x41,
        0x89,
        0x00, 
        0x48,
        0x8B,
        0x44,
        0x24,
        0x28, 
        0x41,
        0x89,
        0x19, 
        0x48,
        0x8B,
        0x5C,
        0x24,
        0x08, 
        0x89,
        0x08, 
        0x48,
        0x8B,
        0x44,
        0x24,
        0x30, 
        0x89,
        0x10, 
        0xC3 
    ];

    private static readonly byte[] Rdtsc32 =
    [
        0x0F,
        0x31, 
        0xC3 
    ];

    private static readonly byte[] Rdtsc64 =
    [
        0x0F,
        0x31, 
        0x48,
        0xC1,
        0xE2,
        0x20, 
        0x48,
        0x0B,
        0xC2, 
        0xC3 
    ];

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate bool CpuidDelegate(uint index, uint ecxValue, out uint eax, out uint ebx, out uint ecx, out uint edx);

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate ulong RdtscDelegate();

#if NET
    [RequiresUnreferencedCode("Dynamic code generation for OpCode delegates")]
#endif
    public static unsafe void Open()
    {
        byte[] rdTscCode;
        byte[] cpuidCode;
        if (IntPtr.Size == 4)
        {
            rdTscCode = Rdtsc32;
            cpuidCode = CpuId32;
        }
        else
        {
            rdTscCode = Rdtsc64;
            cpuidCode = SysCore.Platform.OperatingSystem.IsUnix ? CpuId64Linux : CpuId64Windows;
        }

        _size = (ulong)(rdTscCode.Length + cpuidCode.Length);

        if (SysCore.Platform.OperatingSystem.IsUnix)
        {
#if NETFRAMEWORK
            Assembly assembly = Assembly.Load("Mono.Posix, Version=2.0.0.0, Culture=neutral, " + "PublicKeyToken=0738eb9f132ed756");

            Type sysCall = assembly.GetType("Mono.Unix.Native.Syscall");
            MethodInfo mmap = sysCall.GetMethod("mmap");

            Type mmapProts = assembly.GetType("Mono.Unix.Native.MmapProts");
            object mmapProtsParam = Enum.ToObject(mmapProts,
                                                  (int)mmapProts.GetField("PROT_READ").GetValue(null) |
                                                  (int)mmapProts.GetField("PROT_WRITE").GetValue(null) |
                                                  (int)mmapProts.GetField("PROT_EXEC").GetValue(null));

            Type mmapFlags = assembly.GetType("Mono.Unix.Native.MmapFlags");
            object mmapFlagsParam = Enum.ToObject(mmapFlags,
                                                  (int)mmapFlags.GetField("MAP_ANONYMOUS").GetValue(null) |
                                                  (int)mmapFlags.GetField("MAP_PRIVATE").GetValue(null));

            if (mmap != null)
                _codeBuffer = (IntPtr)mmap.Invoke(null, [IntPtr.Zero, _size, mmapProtsParam, mmapFlagsParam, -1, 0]);
#else
            _codeBuffer = Syscall.mmap(IntPtr.Zero, _size,
                MmapProts.PROT_READ | MmapProts.PROT_WRITE | MmapProts.PROT_EXEC,
                MmapFlags.MAP_ANONYMOUS | MmapFlags.MAP_PRIVATE, -1, 0);
#endif
        }
        else
        {
            _codeBuffer = (IntPtr)PInvoke.VirtualAlloc(null,
                                                       (UIntPtr)_size,
                                                       VIRTUAL_ALLOCATION_TYPE.MEM_COMMIT | VIRTUAL_ALLOCATION_TYPE.MEM_RESERVE,
                                                       PAGE_PROTECTION_FLAGS.PAGE_EXECUTE_READWRITE);
        }

        Marshal.Copy(rdTscCode, 0, _codeBuffer, rdTscCode.Length);
        Rdtsc = Marshal.GetDelegateForFunctionPointer<RdtscDelegate>(_codeBuffer);
        IntPtr cpuidAddress = (IntPtr)((long)_codeBuffer + rdTscCode.Length);
        Marshal.Copy(cpuidCode, 0, cpuidAddress, cpuidCode.Length);
        CpuId = Marshal.GetDelegateForFunctionPointer<CpuidDelegate>(cpuidAddress);
    }

    public static unsafe void Close()
    {
        Rdtsc = null;
        CpuId = null;

        if (SysCore.Platform.OperatingSystem.IsUnix)
        {
#if NETFRAMEWORK
            Assembly assembly = Assembly.Load("Mono.Posix, Version=2.0.0.0, Culture=neutral, " + "PublicKeyToken=0738eb9f132ed756");

            Type sysCall = assembly.GetType("Mono.Unix.Native.Syscall");
            MethodInfo method = sysCall.GetMethod("munmap");
            method?.Invoke(null, [_codeBuffer, _size]);
#else
            Syscall.munmap(_codeBuffer, _size);
#endif
        }
        else
        {
            PInvoke.VirtualFree((void*)_codeBuffer, UIntPtr.Zero, VIRTUAL_FREE_TYPE.MEM_RELEASE);
        }
    }
}
