// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware.Controller.MSI;

public class MsiFanControl
{
    public MsiFan Fan1 { get; set; } = new();
    public MsiFan Fan2 { get; set; } = new();
    public MsiFan Fan3 { get; set; } = new();
    public MsiFan Fan4 { get; set; } = new();
    public MsiFan Fan5 { get; set; } = new();

    public int TemperatureInlet { get; set; }
    public int TemperatureOutlet { get; set; }

    public int TemperatureSensor1 { get; set; }
    public int TemperatureSensor2 { get; set; }
}
