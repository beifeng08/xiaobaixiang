// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Diagnostics;
using System.Threading;
using LibreHardwareMonitor.PawnIo;

namespace LibreHardwareMonitor.Hardware.Motherboard.Lpc.EC;

public class WindowsEmbeddedControllerIO : IEmbeddedControllerIO
{
    private const int FailuresBeforeSkip = 20;
    private const int MaxRetries = 5;

    private const int WaitSpins = 50;

    private readonly LpcAcpiEc _pawnModule;
    private bool _disposed;

    private int _waitReadFailures;

    public WindowsEmbeddedControllerIO()
    {
        _pawnModule = new LpcAcpiEc();

        if (!Mutexes.WaitEc(10))
        {
            throw new BusMutexLockingFailedException();
        }
    }

    public void Read(ushort[] registers, byte[] data)
    {
        Trace.Assert(registers.Length <= data.Length,
                     "data buffer length has to be greater or equal to the registers array length");

        byte bank = 0;
        byte prevBank = SwitchBank(bank);

        Trace.WriteLineIf(prevBank != 0, "Concurrent access to the ACPI EC detected.\nRace condition possible.");

        for (int i = 0; i < registers.Length; i++)
        {
            byte regBank = (byte)(registers[i] >> 8);
            byte regIndex = (byte)(registers[i] & 0xFF);
            
            if (regBank > bank)
            {
                bank = SwitchBank(regBank);
            }

            data[i] = ReadByte(regIndex);
        }

        SwitchBank(prevBank);
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            _disposed = true;
            Mutexes.ReleaseEc();
            _pawnModule.Close();
        }
    }

    private byte ReadByte(byte register)
    {
        return ReadLoop<byte>(register, ReadByteOp);
    }

    private void WriteByte(byte register, byte value)
    {
        WriteLoop(register, value, WriteByteOp);
    }

    private byte SwitchBank(byte bank)
    {
        byte previous = ReadByte(0xFF);
        WriteByte(0xFF, bank);
        return previous;
    }

    private TResult ReadLoop<TResult>(byte register, ReadOp<TResult> op) where TResult : new()
    {
        TResult result = new();

        for (int i = 0; i < MaxRetries; i++)
        {
            if (op(register, out result))
            {
                return result;
            }
        }

        return result;
    }

    private void WriteLoop<TValue>(byte register, TValue value, WriteOp<TValue> op)
    {
        for (int i = 0; i < MaxRetries; i++)
        {
            if (op(register, value))
            {
                return;
            }
        }
    }

    private bool WaitForStatus(Status status, bool isSet)
    {
        for (int i = 0; i < WaitSpins; i++)
        {
            byte value = ReadIOPort(Port.Command);

            if (((byte)status & (!isSet ? value : (byte)~value)) == 0)
            {
                return true;
            }

            Thread.Sleep(1);
        }

        return false;
    }

    private bool WaitRead()
    {
        if (_waitReadFailures > FailuresBeforeSkip)
        {
            return true;
        }

        for (int i = 0; i < MaxRetries; i++)
        {
            byte status = ReadIOPort(Port.Command);
            if ((status & (byte)Status.OutputBufferFull) != 0)
            {
                _waitReadFailures = 0;
                return true;
            }

            Thread.Sleep(1);
        }

        for (int i = 0; i < WaitSpins; i++)
        {
            byte status = ReadIOPort(Port.Command);
            if ((status & (byte)Status.InputBufferFull) == 0)
            {
                _waitReadFailures = 0;
                return true;
            }

            Thread.Sleep(1);
        }

        _waitReadFailures++;
        return false;
    }

    private bool WaitWrite()
    {
        return WaitForStatus(Status.InputBufferFull, false);
    }

    private byte ReadIOPort(Port port)
    {
        return _pawnModule.ReadPort((byte)port);
    }

    private void WriteIOPort(Port port, byte datum)
    {
        _pawnModule.WritePort((byte)port, datum);
    }

    protected bool ReadByteOp(byte register, out byte value)
    {
        if (WaitWrite())
        {
            WriteIOPort(Port.Command, (byte)Command.Read);

            if (WaitWrite())
            {
                WriteIOPort(Port.Data, register);

                if (WaitWrite() && WaitRead())
                {
                    value = ReadIOPort(Port.Data);
                    return true;
                }
            }
        }

        value = 0;
        return false;
    }

    protected bool WriteByteOp(byte register, byte value)
    {
        if (WaitWrite())
        {
            WriteIOPort(Port.Command, (byte)Command.Write);
            if (WaitWrite())
            {
                WriteIOPort(Port.Data, register);
                if (WaitWrite())
                {
                    WriteIOPort(Port.Data, value);
                    return true;
                }
            }
        }

        return false;
    }

    public class BusMutexLockingFailedException : EmbeddedController.IOException
    {
        public BusMutexLockingFailedException() : base("could not lock ISA bus mutex")
        { }
    }

    private delegate bool ReadOp<TParam>(byte register, out TParam p);

    private delegate bool WriteOp<in TParam>(byte register, TParam p);

    private enum Port : byte
    {
        Command = 0x66,
        Data = 0x62
    }

    private enum Command : byte
    {
        Read = 0x80, 
        Write = 0x81, 
        BurstEnable = 0x82, 
        BurstDisable = 0x83, 
        Query = 0x84 
    }

    private enum Status : byte
    {
        OutputBufferFull = 0x01, 
        InputBufferFull = 0x02, 
        Command = 0x08, 
        BurstMode = 0x10, 
        SciEventPending = 0x20, 
        SmiEventPending = 0x40 
    }
}
