// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;

namespace LibreHardwareMonitor.Hardware.Motherboard.Lpc;

internal interface IGigabyteController: IDisposable
{
    bool Enable(bool enabled);

    void Restore();
}
