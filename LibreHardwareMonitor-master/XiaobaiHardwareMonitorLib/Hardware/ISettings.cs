// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware;

public interface ISettings
{
    
    bool Contains(string name);

    void SetValue(string name, string value);

    string GetValue(string name, string value);

    void Remove(string name);
}