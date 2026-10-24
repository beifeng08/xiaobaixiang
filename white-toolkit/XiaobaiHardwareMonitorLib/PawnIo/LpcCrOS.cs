// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;

namespace SysCore.PawnIo;

public class LpcCrOSEc
{
    private readonly PawnIo _pawnIO = PawnIo.LoadModuleFromResource(typeof(LpcCrOSEc).Assembly, $"{nameof(SysCore)}.Resources.PawnIo.LpcCrOSEC.bin");

    public byte[] EcCommand(int version, int command, int outsize, int insize, byte[] data)
    {
        long[] inArray = new long[38];
        inArray[0] = version;
        inArray[1] = command;
        inArray[2] = outsize;
        inArray[3] = insize;

        Buffer.BlockCopy(data, 0, inArray, 4 * 8, data.Length);

        long[] outArray = _pawnIO.Execute("ioctl_ec_command", inArray, 1 + (int)Math.Ceiling(insize / 8.0));
        if (outArray[0] < 0)
        {
            throw new Exception("EC returned error code " + -outArray[0]);
        }

        byte[] retArray = new byte[insize];
        
        Buffer.BlockCopy(outArray, 8, retArray, 0, insize);
        return retArray;
    }

    public byte[] ReadMemmap(byte offset, byte bytes)
    {
        long[] inArray = [offset, bytes];
        long[] outArray = _pawnIO.Execute("ioctl_ec_readmem", inArray, (int)Math.Ceiling(bytes / 8.0));
        byte[] retArray = new byte[bytes];
        Buffer.BlockCopy(outArray, 0, retArray, 0, bytes);
        return retArray;
    }

    public void Close() => _pawnIO.Close();
}
