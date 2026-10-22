// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace LibreHardwareMonitor.Hardware.Controller.MSI;

public enum MsiFanMode : byte
{
    Silent = 0,
    Bios = 1,
    Game = 2,
    Custom = 3,
    Unknown = 4,
    Smart = 5,
}
