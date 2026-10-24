// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Interop.PawnIo;

internal sealed class Nvidia
{
    public const int ThermalChannelCount = 6;

    private const uint ThermalChannelValid = 1u << 30;
    private const uint ThermalChannelValueMask = 0xFFFF;

    private readonly PawnIo _pawnIo = PawnIo.LoadModuleFromResource(typeof(Nvidia).Assembly, $"{nameof(SysCore)}.Resources.PawnIo.Nvidia.bin");

    public bool IsLoaded => _pawnIo.IsLoaded;

    public bool TryReadThermalChannels(uint bus, uint device, uint function, float?[] temperatures)
    {
        if (temperatures == null)
        {
            throw new ArgumentNullException(nameof(temperatures));
        }

        if (temperatures.Length != ThermalChannelCount)
        {
            throw new ArgumentException($"Exactly {ThermalChannelCount} output values are required.", nameof(temperatures));
        }

        Array.Clear(temperatures, 0, temperatures.Length);

        if (!_pawnIo.IsLoaded)
        {
            return false;
        }

        try
        {
            var rawTemperatures = _pawnIo.Execute("ioctl_read_thermal_registers", [bus, device, function], ThermalChannelCount);
            bool hasValidTemperature = false;

            for (int i = 0; i < ThermalChannelCount; i++)
            {
                uint rawTemperature = (uint)rawTemperatures[i];

                if ((rawTemperature & ThermalChannelValid) == 0)
                {
                    continue;
                }

                temperatures[i] = (rawTemperature & ThermalChannelValueMask) / 256.0f;
                hasValidTemperature = true;
            }

            return hasValidTemperature;
        }
        catch
        {
            return false;
        }
    }

    public void Close() => _pawnIo.Close();
}
