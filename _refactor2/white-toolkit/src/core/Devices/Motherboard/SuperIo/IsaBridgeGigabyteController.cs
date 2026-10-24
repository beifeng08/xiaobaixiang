// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Threading;
using SysCore.Interop.PawnIo;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Devices.SuperIo;

internal class IsaBridgeGigabyteController : IGigabyteController
{
    private readonly IsaBridgeEc _isaBridgeEc;
    private readonly MMIOMapping _mmio;
    private bool? _enabled;
    private bool? _restoreEnabled;
    private const int ControllerEnableRegister = 0x47;
    private const uint ControllerFanControlArea = 0x900;

    private IsaBridgeGigabyteController(IsaBridgeEc isaBridgeEc, MMIOMapping mmio)
    {
        _isaBridgeEc = isaBridgeEc;
        _mmio = mmio;
    }

    public static bool TryCreate(out IsaBridgeGigabyteController isaBridgeGigabyteController)
    {
        isaBridgeGigabyteController = null;
        IsaBridgeEc _isaBridgeEc = new IsaBridgeEc();

        if (!_isaBridgeEc.FindSuperIoMMIO(out _, out MMIOMapping secondMmio))
        {
            _isaBridgeEc.Close();
            return false;
        }

        if (!_isaBridgeEc.GetOriginalState(out MMIOState originalState))
        {
            _isaBridgeEc.Close();
            return false;
        }

        if (!EnterMmio(_isaBridgeEc, originalState))
        {
            _isaBridgeEc.Close();
            return false;
        }

        if (!_isaBridgeEc.ReadMmio(
            superIoIndex: secondMmio.Index,
            offset: ControllerFanControlArea + ControllerEnableRegister,
            size: 1,
            value: out byte readvaluebyte) ||
            readvaluebyte == 0xFF)
        {
            _isaBridgeEc.Close();
            return false;
        }

        if (!ExitMmio(_isaBridgeEc))
        {
            _isaBridgeEc.Close();
            return false;
        }

        isaBridgeGigabyteController = new IsaBridgeGigabyteController(_isaBridgeEc, secondMmio);

        return true;
    }

    public bool Enable(bool enabled)
    {
        bool isEntered = false;

        try
        {
            
            if (_enabled is null)
            {
                isEntered = EnterMmio(_isaBridgeEc);
                if (!isEntered)
                {
                    return false;
                }

                if (!_isaBridgeEc.ReadMmio(
                      superIoIndex: _mmio.Index,
                      offset: ControllerFanControlArea + ControllerEnableRegister,
                      size: 1,
                      value: out byte readvaluebyte))
                {
                    return false;
                }

                bool readValue = Convert.ToBoolean(readvaluebyte);
                _restoreEnabled ??= readValue;
                _enabled = Convert.ToBoolean(readvaluebyte);
            }

            if (_enabled == enabled)
            {
                return true;
            }

            if (!isEntered)
            {
                
                isEntered = EnterMmio(_isaBridgeEc);
                if (!isEntered)
                {
                    return false;
                }
            }

            byte writeValue = Convert.ToByte(enabled);
            if (!_isaBridgeEc.WriteMmio(
                superIoIndex: _mmio.Index,
                offset: ControllerFanControlArea + ControllerEnableRegister,
                size: 1,
                value: writeValue))
            {
                return false;
            }

            Thread.Sleep(500);

            _enabled = enabled;

            return true;
        }
        finally
        {
            
            if (isEntered)
            {
                ExitMmio(_isaBridgeEc);
            }
        }
    }

    public void Restore()
    {
        if (_restoreEnabled is null)
        {
            return;
        }

        Enable(_restoreEnabled.Value);
    }

    public void Dispose()
    {
        Restore();
        _isaBridgeEc.Close();
    }

    private static bool EnterMmio(IsaBridgeEc isaBridgeEc, MMIOState? currentState = null)
    {
        if (!isaBridgeEc.Map())
        {
            return false;
        }

        if (currentState is null || (currentState != MMIOState.MMIO_Enabled4E && currentState != MMIOState.MMIO_EnabledBoth))
        {
            if (!isaBridgeEc.TrySetState(MMIOState.MMIO_Enabled4E))
            {
                isaBridgeEc.Unmap();
                return false;
            }
        }

        return true;
    }

    private static bool ExitMmio(IsaBridgeEc isaBridgeEc)
    {
        if (!isaBridgeEc.TrySetState(MMIOState.MMIO_Original))
        {
            return false;
        }

        if (!isaBridgeEc.Unmap())
        {
            return false;
        }

        return true;
    }
}
