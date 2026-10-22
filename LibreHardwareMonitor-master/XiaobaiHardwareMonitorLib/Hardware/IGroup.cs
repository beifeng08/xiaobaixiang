// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Collections.Generic;

namespace XiaobaiHardwareMonitor.Hardware;

internal interface IGroup
{
    
    IReadOnlyList<IHardware> Hardware { get; }

    string GetReport();

    void Close();
}