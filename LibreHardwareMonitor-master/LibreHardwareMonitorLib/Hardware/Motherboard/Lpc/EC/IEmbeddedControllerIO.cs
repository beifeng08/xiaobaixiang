// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;

namespace LibreHardwareMonitor.Hardware.Motherboard.Lpc.EC;

public interface IEmbeddedControllerIO : IDisposable
{
    void Read(ushort[] registers, byte[] data);
}