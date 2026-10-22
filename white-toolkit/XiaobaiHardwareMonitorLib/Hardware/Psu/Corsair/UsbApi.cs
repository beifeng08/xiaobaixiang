// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Linq;
using System.Text;
using HidSharp;

namespace XiaobaiHardwareMonitor.Hardware.Psu.Corsair;

internal static class UsbApi
{
    public enum Command : byte
    {
        SELECT_RAIL = 0x00, 
        RAIL_VOLTS_HCRIT = 0x40, 
        RAIL_VOLTS_LCRIT = 0x44,
        RAIL_AMPS_HCRIT = 0x46,
        TEMP_HCRIT = 0x4F,
        IN_VOLTS = 0x88,
        IN_AMPS = 0x89,
        RAIL_VOLTS = 0x8B,
        RAIL_AMPS = 0x8C,
        TEMP0 = 0x8D,
        TEMP1 = 0x8E,
        FAN_RPM = 0x90,
        RAIL_WATTS = 0x96,
        VEND_STR = 0x99,
        PROD_STR = 0x9A,
        TOTAL_UPTIME = 0xD1,
        UPTIME = 0xD2,
        OCPMODE = 0xD8,
        TOTAL_WATTS = 0xEE,
        INIT = 0xFE
    }

    [Flags]
    public enum OptionalCommands
    {
        None = 0x0,
        InputCurrent = 0x1
    }

    public static void Init(HidStream stream)
    {
        
        SendCommand(stream, (byte)Command.INIT, (Command)3, 0, out _);
    }

    public static float? GetValue(HidStream stream, Command cmd, byte rail)
    {
        if (!Request(stream, cmd, rail, out byte[] data))
            return null;

        int tmp = BitConverter.ToInt32(data, 0); 
        return cmd switch
        {
            Command.RAIL_VOLTS_HCRIT or Command.RAIL_VOLTS_LCRIT or Command.RAIL_AMPS_HCRIT or Command.TEMP_HCRIT or Command.IN_VOLTS or Command.IN_AMPS or Command.RAIL_VOLTS or Command.RAIL_AMPS
                or Command.TEMP0 or Command.TEMP1 or Command.FAN_RPM or Command.RAIL_WATTS or Command.TOTAL_WATTS => Linear11ToFloat32((ushort)tmp), 
            Command.TOTAL_UPTIME or Command.UPTIME or Command.OCPMODE => tmp,
            _ => null
        };
    }

    public static Criticals GetCriticals(HidStream stream)
    {
        Criticals res = new();
        const byte tempCount = 2;
        res.TempMax = new float?[tempCount];

        for (byte rail = 0; rail < tempCount; rail++)
            res.TempMax[rail] = GetValue(stream, Command.TEMP_HCRIT, rail);

        const byte railCount = 3; 
        res.VoltageMin = new float?[railCount];
        res.VoltageMax = new float?[railCount];
        res.CurrentMax = new float?[railCount];

        for (byte rail = 0; rail < railCount; rail++)
        {
            res.VoltageMax[rail] = GetValue(stream, Command.RAIL_VOLTS_HCRIT, rail);
            res.VoltageMin[rail] = GetValue(stream, Command.RAIL_VOLTS_LCRIT, rail);
            res.CurrentMax[rail] = GetValue(stream, Command.RAIL_AMPS_HCRIT, rail);
        }

        return res;
    }

    public static OptionalCommands GetOptionalCommands(HidStream stream)
    {
        OptionalCommands res = OptionalCommands.None;

        if (GetValue(stream, Command.IN_AMPS, 0).HasValue)
            res |= OptionalCommands.InputCurrent;

        return res;
    }

    public static FirmwareInfo FwInfo(HidStream stream)
    {
        if (!SendCommand(stream, 3, Command.VEND_STR, 0, out byte[] vendorArr))
            throw new ProtocolError(stream.Device, "Can't read vendor string");

        if (!SendCommand(stream, 3, Command.PROD_STR, 0, out byte[] productArr))
            throw new ProtocolError(stream.Device, "Can't read product");

        static string ArrayToString(byte[] ar)
        {
            return Encoding.ASCII.GetString(ar.TakeWhile(x => x != 0).ToArray());
        }

        return new FirmwareInfo { Vendor = ArrayToString(vendorArr), Product = ArrayToString(productArr) };
    }

    private static bool SendCommand(HidStream stream, byte length, Command cmd, byte arg, out byte[] replyData)
    {
        
        const int cmdBufferSize = 64;
        const int replySize = 16;

        byte[] cmdBuffer = new byte[cmdBufferSize + 1];
        cmdBuffer[0] = 0; 
        cmdBuffer[1] = length;
        cmdBuffer[2] = (byte)cmd;
        cmdBuffer[3] = arg;

        stream.Write(cmdBuffer);
        byte[] reply = stream.Read();
        replyData = new byte[replySize];
        Array.Copy(reply, 3, replyData, 0, replySize);

        return reply[1] == cmdBuffer[1] && reply[2] == cmdBuffer[2];
    }

    private static bool Request(HidStream stream, Command cmd, byte rail, out byte[] data)
    {
        switch (cmd)
        {
            case Command.RAIL_VOLTS_HCRIT:
            case Command.RAIL_VOLTS_LCRIT:
            case Command.RAIL_AMPS_HCRIT:
            case Command.RAIL_VOLTS:
            case Command.RAIL_AMPS:
            case Command.RAIL_WATTS:
                if (!SendCommand(stream, 2, Command.SELECT_RAIL, rail, out _))
                {
                    data = null;
                    return false;
                }

                break;
        }

        return SendCommand(stream, 3, cmd, 0, out data);
    }

    private static float Linear11ToFloat32(ushort val)
    {
        int exp = (short)val >> 11;
        int mant = (short)(val & 0x7ff) << 5 >> 5;
        return mant * (float)Math.Pow(2, exp);
    }

    public struct FirmwareInfo
    {
        public string Vendor;
        public string Product;
    }

    public struct Criticals
    {
        public float?[] TempMax;

        public float?[] VoltageMin;
        public float?[] VoltageMax;

        public float?[] CurrentMax;
    }
}
