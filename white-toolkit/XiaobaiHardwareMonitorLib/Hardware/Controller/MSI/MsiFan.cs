// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (C) Florian K. (Blacktempel)
// All Rights Reserved.

using System;
using System.Runtime.InteropServices;

namespace SysCore.Hardware.Controller.MSI;

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
