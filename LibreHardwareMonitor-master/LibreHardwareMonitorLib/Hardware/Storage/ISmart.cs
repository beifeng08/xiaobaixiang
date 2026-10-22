// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Collections.Generic;

namespace LibreHardwareMonitor.Hardware.Storage;

public interface ISmart
{
    
    IReadOnlyList<SmartAttribute> Attributes { get; }
}
