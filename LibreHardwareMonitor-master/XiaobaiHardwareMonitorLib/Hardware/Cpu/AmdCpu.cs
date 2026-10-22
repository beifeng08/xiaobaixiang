// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware.Cpu;

internal abstract class AmdCpu(int processorIndex, CpuId[][] cpuId, ISettings settings)
    : GenericCpu(processorIndex, cpuId, settings);
