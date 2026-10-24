// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;
using RAMSPDToolkit.I2CSMBus.Interop.PawnIO;
using RAMSPDToolkit.Windows.Driver.Interfaces;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Interop.PawnIo;

internal sealed class RAMSPDToolkitDriver : IPawnIODriver
{
    const string I801ModuleFilename = "SmbusI801.bin";
    const string Nct6793ModuleFilename = "SmbusNCT6793.bin";
    const string Piix4ModuleFilename = "SmbusPIIX4.bin";
    const string IntelSkylakeIMCModuleFileName = "SmbusIntelSkylakeIMC.bin";

    private readonly List<PawnIOModule> _pawnIOModules = new();

    public bool IsOpen => true;

    public bool Load()
    {
        
        return true;
    }

    public IPawnIOModule LoadModule(PawnIOSMBusIdentifier pawnIOSMBusIdentifier)
    {
        string moduleResourceFilename = pawnIOSMBusIdentifier switch
        {
            PawnIOSMBusIdentifier.I801 => I801ModuleFilename,
            PawnIOSMBusIdentifier.Piix4 => Piix4ModuleFilename,
            PawnIOSMBusIdentifier.NCT6793 => Nct6793ModuleFilename,
            PawnIOSMBusIdentifier.IntelSkylakeIMC => IntelSkylakeIMCModuleFileName,
            _ => null
        };

        if (moduleResourceFilename == null)
        {
            return null;
        }

        PawnIOModule pawnIOModule = null;

        try
        {
            var pawnIO = PawnIo.LoadModuleFromResource(typeof(RAMSPDToolkitDriver).Assembly, $"{nameof(SysCore)}.Resources.PawnIo.{moduleResourceFilename}");

            if (pawnIO.IsLoaded)
            {
                pawnIOModule = new PawnIOModule(pawnIO);
            }
        }
        catch
        {
            return null;
        }

        if (pawnIOModule != null)
        {
            _pawnIOModules.Add(pawnIOModule);
            return pawnIOModule;
        }

        return null;
    }

    public void Unload()
    {
        _pawnIOModules.ForEach(p => p.Dispose());
        _pawnIOModules.Clear();
    }

    internal class PawnIOModule : IPawnIOModule, IDisposable
    {
        private PawnIo _pawnIO;

        public PawnIOModule(PawnIo pawnIO)
        {
            _pawnIO = pawnIO;
        }

        public int Execute(string name, long[] inBuffer, uint inSize, long[] outBuffer, uint outSize, out uint returnSize)
            => _pawnIO.ExecuteHr(name, inBuffer, inSize, outBuffer, outSize, out returnSize);

        public void Dispose()
        {
            if (_pawnIO != null)
            {
                _pawnIO.Close();
                _pawnIO = null;
            }
        }
    }
}
