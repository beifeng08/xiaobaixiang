// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (C) LibreHardwareMonitor and Contributors.
// All Rights Reserved.

namespace SysCore.Hardware.Gpu;

public abstract class GenericGpu : Hardware
{
    
    protected GenericGpu(string name, Identifier identifier, ISettings settings) : base(name, identifier, settings)
    {
    }

    public abstract string DeviceId { get; }

    public override void Close()
    {
        base.Close();
    }
}
