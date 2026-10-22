// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using LibreHardwareMonitor.Hardware;

namespace YanJi.Hardware;

internal static class LhmComputerAccess
{
    const string FailureKey = "YanJi.Lhm.ComputerFailure";

    public static void Run(Computer computer, Action<Computer> read, CancellationToken token)
    {
        using var gate = new Semaphore(1, 1, $"Local\\YanJi.Lhm.Computer.{Environment.ProcessId}");
        if (WaitHandle.WaitAny([gate, token.WaitHandle]) == 1)
            token.ThrowIfCancellationRequested();
        try
        {
            token.ThrowIfCancellationRequested();
            if (AppContext.GetData(FailureKey) is string failure)
                throw new InvalidOperationException("LHM 初始化或释放曾失败，请重启程序后重试：" + failure);

            bool opened = false;
            try
            {
                try
                {
                    computer.Open();
                    opened = true;
                }
                catch (Exception ex)
                {
                    
                    AppContext.SetData(FailureKey, ex.Message);
                    throw;
                }
                token.ThrowIfCancellationRequested();
                read(computer);
            }
            finally
            {
                try { computer.Close(); }
                catch (Exception ex)
                {
                    AppContext.SetData(FailureKey, ex.Message);
                    if (opened) throw;
                }
            }
        }
        finally { gate.Release(); }
    }
}
