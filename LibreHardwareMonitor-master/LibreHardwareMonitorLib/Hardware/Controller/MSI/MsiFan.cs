// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Runtime.InteropServices;

namespace LibreHardwareMonitor.Hardware.Controller.MSI;

public class MsiFan
{
    
    public int Speed { get; set; }
    public int Duty { get; set; }

    public MsiFanConfigure ConfigureDuty;

    public MsiFanConfigure ConfigureTemp;
}

public struct MsiFanConfigure
{
    public MsiFanConfigure()
    {
        if (Marshal.SizeOf<MsiFanConfigure>() != 8)
        {
            throw new InvalidOperationException($"{nameof(MsiFanConfigure)} struct size is invalid.");
        }
    }

    public MsiFanMode Mode = MsiFanMode.Unknown;
    public byte Item0;
    public byte Item1;
    public byte Item2;
    public byte Item3;
    public byte Item4;
    public byte Item5;
    public byte Item6;
}
