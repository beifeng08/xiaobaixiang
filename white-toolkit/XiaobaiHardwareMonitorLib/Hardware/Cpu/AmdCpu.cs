// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

namespace SysCore.Hardware.Cpu;

internal abstract class AmdCpu(int processorIndex, CpuId[][] cpuId, ISettings settings)
    : GenericCpu(processorIndex, cpuId, settings);
