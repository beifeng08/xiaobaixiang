// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.Text;
using SysCore.Interop.PawnIo;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
using SysCore.Sensors;
namespace SysCore.Devices;

internal sealed class IntelCpu : GenericCpu
{
    private readonly Sensor _busClock;
    private readonly Sensor _coreAvg;
    private readonly Sensor[] _coreClocks;
    private readonly Sensor _coreMax;
    private readonly Sensor[] _coreTemperatures;
    private readonly Sensor[] _coreVIDs;
    private readonly Sensor _coreVoltage;
    private readonly Sensor[] _distToTjMaxTemperatures;

    private readonly string[] _coreNames;

    private readonly uint[] _energyStatusMsrs = { MSR_PKG_ENERGY_STATUS, MSR_PP0_ENERGY_STATUS, MSR_PP1_ENERGY_STATUS, MSR_DRAM_ENERGY_STATUS, MSR_PLATFORM_ENERGY_STATUS };
    private readonly uint[] _lastEnergyConsumed;
    private readonly DateTime[] _lastEnergyTime;

    private readonly MicroArchitecture _microArchitecture;
    private readonly Sensor _packageTemperature;
    private readonly Sensor[] _powerSensors;
    private readonly double _timeStampCounterMultiplier;

    private readonly IntelMsr _pawnModule;

    public IntelCpu(int processorIndex, CpuId[][] cpuId, ISettings settings) : base(processorIndex, cpuId, settings)
    {
        if (SysCore.Platform.OperatingSystem.IsUnix)
            return;

        _pawnModule = new IntelMsr();

        uint eax;

        float[] tjMax;
        switch (_family)
        {
            case 0x06:
                {
                    switch (_model)
                    {
                        case 0x0F: 
                            _microArchitecture = MicroArchitecture.Core;
                            tjMax = _stepping switch
                            {
                                
                                0x06 => _coreCount switch
                                {
                                    2 => Floats(80 + 10),
                                    4 => Floats(90 + 10),
                                    _ => Floats(85 + 10)
                                },
                                
                                0x0B => Floats(90 + 10),
                                
                                0x0D => Floats(85 + 10),
                                _ => Floats(85 + 10)
                            };
                            break;

                        case 0x17: 
                            _microArchitecture = MicroArchitecture.Core;
                            tjMax = Floats(100);
                            break;

                        case 0x1C: 
                            _microArchitecture = MicroArchitecture.Atom;
                            tjMax = _stepping switch
                            {
                                
                                0x02 => Floats(90),
                                
                                0x0A => Floats(100),
                                _ => Floats(90)
                            };
                            break;

                        case 0x1A: 
                        case 0x1E: 
                        case 0x1F: 
                        case 0x25: 
                        case 0x2C: 
                        case 0x2E: 
                        case 0x2F: 
                            _microArchitecture = MicroArchitecture.Nehalem;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x2A: 
                        case 0x2D: 
                            _microArchitecture = MicroArchitecture.SandyBridge;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x3A: 
                        case 0x3E: 
                            _microArchitecture = MicroArchitecture.IvyBridge;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x3C: 
                        case 0x3F: 
                        
                        case 0x45: 
                        case 0x46:
                            _microArchitecture = MicroArchitecture.Haswell;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x3D: 
                        case 0x47: 
                        case 0x4F: 
                        case 0x56: 
                            _microArchitecture = MicroArchitecture.Broadwell;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x36: 
                            _microArchitecture = MicroArchitecture.Atom;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x37: 
                        case 0x4A:
                        case 0x4D: 
                        case 0x5A:
                        case 0x5D:
                            _microArchitecture = MicroArchitecture.Silvermont;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x4E:
                        case 0x5E: 
                        case 0x55: 
                            _microArchitecture = MicroArchitecture.Skylake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x4C: 
                            _microArchitecture = MicroArchitecture.Airmont;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x8E: 
                        case 0x9E:
                            _microArchitecture = MicroArchitecture.KabyLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x5C: 
                        case 0x5F: 
                            _microArchitecture = MicroArchitecture.Goldmont;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x7A: 
                            _microArchitecture = MicroArchitecture.GoldmontPlus;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x66: 
                            _microArchitecture = MicroArchitecture.CannonLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x7D: 
                        case 0x7E:
                        case 0x6A: 
                        case 0x6C:
                            _microArchitecture = MicroArchitecture.IceLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0xA5:
                        case 0xA6: 
                            _microArchitecture = MicroArchitecture.CometLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x86: 
                            _microArchitecture = MicroArchitecture.Tremont;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x8C: 
                        case 0x8D:
                            _microArchitecture = MicroArchitecture.TigerLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x97: 
                        case 0x9A: 
                        case 0xBE: 
                            _microArchitecture = MicroArchitecture.AlderLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0xB7: 
                        case 0xBA: 
                        case 0xBF: 
                            _microArchitecture = MicroArchitecture.RaptorLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0xAC: 
                        case 0xAA: 
                            _microArchitecture = MicroArchitecture.MeteorLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x9C: 
                            _microArchitecture = MicroArchitecture.JasperLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0xA7: 
                            _microArchitecture = MicroArchitecture.RocketLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0xB5: 
                        case 0xC5: 
                        case 0xC6: 
                            _microArchitecture = MicroArchitecture.ArrowLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0xBD: 
                            _microArchitecture = MicroArchitecture.LunarLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x8F: 
                            _microArchitecture = MicroArchitecture.SapphireRapids;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0x96: 
                            _microArchitecture = MicroArchitecture.ElkhartLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        case 0xCC: 
                            _microArchitecture = MicroArchitecture.PantherLake;
                            tjMax = GetTjMaxFromMsr();
                            break;

                        default:
                            _microArchitecture = MicroArchitecture.Unknown;
                            tjMax = Floats(100);
                            break;
                    }
                }

                break;
            case 0x0F:
                switch (_model)
                {
                    case 0x00: 
                    case 0x01: 
                    case 0x02: 
                    case 0x03: 
                    case 0x04: 
                    case 0x06: 
                        _microArchitecture = MicroArchitecture.NetBurst;
                        tjMax = Floats(100);
                        break;

                    default:
                        _microArchitecture = MicroArchitecture.Unknown;
                        tjMax = Floats(100);
                        break;
                }

                break;
            default:
                _microArchitecture = MicroArchitecture.Unknown;
                tjMax = Floats(100);
                break;
        }

        switch (_microArchitecture)
        {
            case MicroArchitecture.Atom:
            case MicroArchitecture.Core:
            case MicroArchitecture.NetBurst:
                if (_pawnModule.ReadMsr(IA32_PERF_STATUS, out uint _, out uint edx))
                    _timeStampCounterMultiplier = ((edx >> 8) & 0x1f) + (0.5 * ((edx >> 14) & 1));

                break;
            case MicroArchitecture.Airmont:
            case MicroArchitecture.AlderLake:
            case MicroArchitecture.ArrowLake:
            case MicroArchitecture.Broadwell:
            case MicroArchitecture.CannonLake:
            case MicroArchitecture.CometLake:
            case MicroArchitecture.Goldmont:
            case MicroArchitecture.GoldmontPlus:
            case MicroArchitecture.Haswell:
            case MicroArchitecture.IceLake:
            case MicroArchitecture.IvyBridge:
            case MicroArchitecture.JasperLake:
            case MicroArchitecture.KabyLake:
            case MicroArchitecture.LunarLake:
            case MicroArchitecture.Nehalem:
            case MicroArchitecture.MeteorLake:
            case MicroArchitecture.PantherLake:
            case MicroArchitecture.RaptorLake:
            case MicroArchitecture.RocketLake:
            case MicroArchitecture.SandyBridge:
            case MicroArchitecture.Silvermont:
            case MicroArchitecture.Skylake:
            case MicroArchitecture.TigerLake:
            case MicroArchitecture.SapphireRapids:
            case MicroArchitecture.ElkhartLake:
            case MicroArchitecture.Tremont:
                if (_pawnModule.ReadMsr(MSR_PLATFORM_INFO, out eax, out uint _))
                    _timeStampCounterMultiplier = (eax >> 8) & 0xff;

                break;
            default:
                _timeStampCounterMultiplier = 0;
                break;
        }

        _coreNames = new string[_coreCount];
        int pCoreIndex = 1;
        int eCoreIndex = 1;

        for (int i = 0; i < _coreCount; i++)
        {
            if (_cpuId.Length > i && _cpuId[i].Length > 0)
            {
                CoreType coreType = _cpuId[i][0].CoreType;

                _coreNames[i] = coreType switch
                {
                    CoreType.Performance => $"P-Core #{pCoreIndex++}",
                    CoreType.Efficient => $"E-Core #{eCoreIndex++}",
                    _ => CoreString(i)
                };
            }
            else
            {
                _coreNames[i] = CoreString(i);
            }
        }

        int coreSensorId = 0;

        if (cpuId[0][0].Data.GetLength(0) > 6 && (cpuId[0][0].Data[6, 0] & 0x40) != 0 && _microArchitecture != MicroArchitecture.Unknown && _coreCount > 1)
        {
            _coreMax = new Sensor("Core Max", coreSensorId, SensorType.Temperature, this, settings);
            ActivateSensor(_coreMax);
            coreSensorId++;

            _coreAvg = new Sensor("Core Average", coreSensorId, SensorType.Temperature, this, settings);
            ActivateSensor(_coreAvg);
            coreSensorId++;
        }
        else
        {
            _coreMax = null;
            _coreAvg = null;
        }

        if (cpuId[0][0].Data.GetLength(0) > 6 && (cpuId[0][0].Data[6, 0] & 1) != 0 && _microArchitecture != MicroArchitecture.Unknown)
        {
            _coreTemperatures = new Sensor[_coreCount];
            for (int i = 0; i < _coreTemperatures.Length; i++)
            {
                _coreTemperatures[i] = new Sensor(_coreNames[i],
                    coreSensorId,
                    SensorType.Temperature,
                    this,
                    [
                        new ParameterDescription("TjMax [°C]", "TjMax temperature of the core sensor.\n" + "Temperature = TjMax - TSlope * Value.", tjMax[i]),
                        new ParameterDescription("TSlope [°C]", "Temperature slope of the digital thermal sensor.\n" + "Temperature = TjMax - TSlope * Value.", 1)
                    ],
                    settings);

                ActivateSensor(_coreTemperatures[i]);
                coreSensorId++;
            }
        }
        else
        {
            _coreTemperatures = [];
        }

        if (cpuId[0][0].Data.GetLength(0) > 6 && (cpuId[0][0].Data[6, 0] & 0x40) != 0 && _microArchitecture != MicroArchitecture.Unknown)
        {
            _packageTemperature = new Sensor("CPU Package",
                                             coreSensorId,
                                             SensorType.Temperature,
                                             this,
                                             [
                                                 new ParameterDescription("TjMax [°C]", "TjMax temperature of the package sensor.\n" + "Temperature = TjMax - TSlope * Value.", tjMax[0]),
                                                 new ParameterDescription("TSlope [°C]", "Temperature slope of the digital thermal sensor.\n" + "Temperature = TjMax - TSlope * Value.", 1)
                                             ],
                                             settings);

            ActivateSensor(_packageTemperature);
            coreSensorId++;
        }

        if (cpuId[0][0].Data.GetLength(0) > 6 && (cpuId[0][0].Data[6, 0] & 1) != 0 && _microArchitecture != MicroArchitecture.Unknown)
        {
            _distToTjMaxTemperatures = new Sensor[_coreCount];
            for (int i = 0; i < _distToTjMaxTemperatures.Length; i++)
            {
                _distToTjMaxTemperatures[i] = new Sensor(_coreNames[i] + " Distance to TjMax", coreSensorId, SensorType.Temperature, this, settings);
                ActivateSensor(_distToTjMaxTemperatures[i]);
                coreSensorId++;
            }
        }
        else
        {
            _distToTjMaxTemperatures = [];
        }

        _busClock = new Sensor("Bus Speed", 0, SensorType.Clock, this, settings);
        _coreClocks = new Sensor[_coreCount];
        for (int i = 0; i < _coreClocks.Length; i++)
        {
            _coreClocks[i] = new Sensor(_coreNames[i], i + 1, SensorType.Clock, this, settings);
            if (HasTimeStampCounter && _microArchitecture != MicroArchitecture.Unknown)
                ActivateSensor(_coreClocks[i]);
        }

        if (_microArchitecture is MicroArchitecture.Airmont or
            MicroArchitecture.AlderLake or
            MicroArchitecture.ArrowLake or
            MicroArchitecture.Broadwell or
            MicroArchitecture.CannonLake or
            MicroArchitecture.CometLake or
            MicroArchitecture.Goldmont or
            MicroArchitecture.GoldmontPlus or
            MicroArchitecture.Haswell or
            MicroArchitecture.IceLake or
            MicroArchitecture.IvyBridge or
            MicroArchitecture.JasperLake or
            MicroArchitecture.KabyLake or
            MicroArchitecture.LunarLake or
            MicroArchitecture.MeteorLake or
            MicroArchitecture.PantherLake or
            MicroArchitecture.RaptorLake or
            MicroArchitecture.RocketLake or
            MicroArchitecture.SandyBridge or
            MicroArchitecture.Silvermont or
            MicroArchitecture.Skylake or
            MicroArchitecture.TigerLake or
            MicroArchitecture.SapphireRapids or
            MicroArchitecture.ElkhartLake or
            MicroArchitecture.Tremont)
        {
            _powerSensors = new Sensor[_energyStatusMsrs.Length];
            _lastEnergyTime = new DateTime[_energyStatusMsrs.Length];
            _lastEnergyConsumed = new uint[_energyStatusMsrs.Length];

            if (_pawnModule.ReadMsr(MSR_RAPL_POWER_UNIT, out eax, out uint _))
            {
                EnergyUnitsMultiplier = _microArchitecture switch
                {
                    MicroArchitecture.Silvermont or MicroArchitecture.Airmont => 1.0e-6f * (1 << (int)((eax >> 8) & 0x1F)),
                    _ => 1.0f / (1 << (int)((eax >> 8) & 0x1F))
                };
            }

            if (EnergyUnitsMultiplier != 0)
            {
                string[] powerSensorLabels = ["CPU Package", "CPU Cores", "CPU Graphics", "CPU Memory", "CPU Platform"];

                for (int i = 0; i < _energyStatusMsrs.Length; i++)
                {
                    if (!_pawnModule.ReadMsr(_energyStatusMsrs[i], out eax, out uint _))
                        continue;

                    if (i == 2 && !SysCore.Platform.OperatingSystem.IsUnix)
                        continue;

                    _lastEnergyTime[i] = DateTime.UtcNow;
                    _lastEnergyConsumed[i] = eax;
                    _powerSensors[i] = new Sensor(powerSensorLabels[i],
                                                  i,
                                                  SensorType.Power,
                                                  this,
                                                  settings);

                    ActivateSensor(_powerSensors[i]);
                }
            }
        }

        if (_pawnModule.ReadMsr(IA32_PERF_STATUS, out eax, out uint _) && ((eax >> 32) & 0xFFFF) > 0)
        {
            _coreVoltage = new Sensor("CPU Core", 0, SensorType.Voltage, this, settings);
            ActivateSensor(_coreVoltage);
        }

        _coreVIDs = new Sensor[_coreCount];
        for (int i = 0; i < _coreVIDs.Length; i++)
        {
            _coreVIDs[i] = new Sensor(_coreNames[i], i + 1, SensorType.Voltage, this, settings);
            ActivateSensor(_coreVIDs[i]);
        }

        Update();
    }

    public float EnergyUnitsMultiplier { get; }

    private float[] Floats(float f)
    {
        float[] result = new float[_coreCount];
        for (int i = 0; i < _coreCount; i++)
            result[i] = f;

        return result;
    }

    private float[] GetTjMaxFromMsr()
    {
        float[] result = new float[_coreCount];
        for (int i = 0; i < _coreCount; i++)
        {
            if (_pawnModule.ReadMsr(IA32_TEMPERATURE_TARGET, out uint eax, out uint _, _cpuId[i][0].Affinity))
                result[i] = (eax >> 16) & 0xFF;
            else
                result[i] = 100;
        }

        return result;
    }

    public override string GetReport()
    {
        StringBuilder r = new();
        r.Append(base.GetReport());
        r.Append("MicroArchitecture: ");
        r.AppendLine(_microArchitecture.ToString());
        r.Append("Time Stamp Counter Multiplier: ");
        r.AppendLine(_timeStampCounterMultiplier.ToString(CultureInfo.InvariantCulture));
        r.AppendLine();
        return r.ToString();
    }

    public override void Close()
    {
        base.Close();
        _pawnModule.Close();
    }

    public override void Update()
    {
        base.Update();

        float coreMax = float.MinValue;
        float coreAvg = 0;
        uint eax;

        for (int i = 0; i < _coreTemperatures?.Length; i++)
        {
            
            if (_pawnModule.ReadMsr(IA32_THERM_STATUS_MSR, out eax, out _, _cpuId[i][0].Affinity) && (eax & 0x80000000) != 0)
            {
                
                float deltaT = (eax & 0x007F0000) >> 16;
                float tjMax = _coreTemperatures[i].Parameters[0].Value;
                float tSlope = _coreTemperatures[i].Parameters[1].Value;
                _coreTemperatures[i].Value = tjMax - (tSlope * deltaT);

                coreAvg += (float)_coreTemperatures[i].Value;
                if (coreMax < _coreTemperatures[i].Value)
                    coreMax = (float)_coreTemperatures[i].Value;

                _distToTjMaxTemperatures[i].Value = deltaT;
            }
            else
            {
                _coreTemperatures[i].Value = null;
                _distToTjMaxTemperatures[i].Value = null;
            }
        }

        if (_coreMax != null && coreMax != float.MinValue)
        {
            _coreMax.Value = coreMax;
            coreAvg /= _coreTemperatures.Length;
            _coreAvg.Value = coreAvg;
        }

        if (_packageTemperature != null)
        {
            
            if (_pawnModule.ReadMsr(IA32_PACKAGE_THERM_STATUS, out eax, out _, _cpuId[0][0].Affinity) && (eax & 0x80000000) != 0)
            {
                
                float deltaT = (eax & 0x007F0000) >> 16;
                float tjMax = _packageTemperature.Parameters[0].Value;
                float tSlope = _packageTemperature.Parameters[1].Value;
                _packageTemperature.Value = tjMax - (tSlope * deltaT);
            }
            else
            {
                _packageTemperature.Value = null;
            }
        }

        if (HasTimeStampCounter && _timeStampCounterMultiplier > 0)
        {
            double newBusClock = 0;
            for (int i = 0; i < _coreClocks.Length; i++)
            {
                System.Threading.Thread.Sleep(1);
                if (_pawnModule.ReadMsr(IA32_PERF_STATUS, out eax, out _, _cpuId[i][0].Affinity))
                {
                    newBusClock = TimeStampCounterFrequency / _timeStampCounterMultiplier;
                    switch (_microArchitecture)
                    {
                        case MicroArchitecture.Nehalem:
                            _coreClocks[i].Value = (float)((eax & 0xff) * newBusClock);
                            break;
                        case MicroArchitecture.Airmont:
                        case MicroArchitecture.AlderLake:
                        case MicroArchitecture.ArrowLake:
                        case MicroArchitecture.Broadwell:
                        case MicroArchitecture.CannonLake:
                        case MicroArchitecture.CometLake:
                        case MicroArchitecture.Goldmont:
                        case MicroArchitecture.GoldmontPlus:
                        case MicroArchitecture.Haswell:
                        case MicroArchitecture.IceLake:
                        case MicroArchitecture.IvyBridge:
                        case MicroArchitecture.JasperLake:
                        case MicroArchitecture.KabyLake:
                        case MicroArchitecture.LunarLake:
                        case MicroArchitecture.MeteorLake:
                        case MicroArchitecture.PantherLake:
                        case MicroArchitecture.RaptorLake:
                        case MicroArchitecture.RocketLake:
                        case MicroArchitecture.SandyBridge:
                        case MicroArchitecture.Silvermont:
                        case MicroArchitecture.Skylake:
                        case MicroArchitecture.TigerLake:
                        case MicroArchitecture.SapphireRapids:
                        case MicroArchitecture.ElkhartLake:
                        case MicroArchitecture.Tremont:
                            _coreClocks[i].Value = (float)(((eax >> 8) & 0xff) * newBusClock);
                            break;
                        default:
                            _coreClocks[i].Value = (float)((((eax >> 8) & 0x1f) + (0.5 * ((eax >> 14) & 1))) * newBusClock);
                            break;
                    }
                }
                else
                {
                    
                    _coreClocks[i].Value = (float)TimeStampCounterFrequency;
                }
            }

            if (newBusClock > 0)
            {
                _busClock.Value = (float)newBusClock;
                ActivateSensor(_busClock);
            }
        }

        if (_powerSensors != null)
        {
            foreach (Sensor sensor in _powerSensors)
            {
                if (sensor == null)
                    continue;

                if (!_pawnModule.ReadMsr(_energyStatusMsrs[sensor.Index], out eax, out _))
                    continue;

                DateTime time = DateTime.UtcNow;
                uint energyConsumed = eax;
                float deltaTime = (float)(time - _lastEnergyTime[sensor.Index]).TotalSeconds;
                if (deltaTime < 0.01)
                    continue;

                sensor.Value = EnergyUnitsMultiplier * unchecked(energyConsumed - _lastEnergyConsumed[sensor.Index]) / deltaTime;
                _lastEnergyTime[sensor.Index] = time;
                _lastEnergyConsumed[sensor.Index] = energyConsumed;
            }
        }

        if (_coreVoltage != null && _pawnModule.ReadMsr(IA32_PERF_STATUS, out eax, out uint edx))
        {
            uint vidBits = edx & 0xFFFF;        
            if (vidBits == 0)
                vidBits = edx & 0xFFFF;
        
            if (vidBits > 0)
            {
                _coreVoltage.Value = vidBits / 8192.0f;
            }
            else
            {
                _coreVoltage.Value = 0;
            }
        }

        for (int i = 0; i < _coreVIDs?.Length; i++)
        {

            if (_pawnModule.ReadMsr(IA32_PERF_STATUS, out eax, out edx, _cpuId[i][0].Affinity))
            {
                uint vidBitsLoop = edx & 0xFFFF;        
                if (vidBitsLoop == 0)
                    vidBitsLoop = eax & 0xFFFF;
        
                if (vidBitsLoop > 0)
                {
                    _coreVIDs[i].Value = vidBitsLoop / 8192.0f;
                    ActivateSensor(_coreVIDs[i]);
                }
                else
                {
                    DeactivateSensor(_coreVIDs[i]);
                }
            }
            else
            {
                DeactivateSensor(_coreVIDs[i]);
            }
        }
    }

    [SuppressMessage("ReSharper", "IdentifierTypo")]
    private enum MicroArchitecture
    {
        Airmont,
        AlderLake,
        Atom,
        ArrowLake, 
        Broadwell,
        CannonLake,
        CometLake,
        Core,
        Goldmont,
        GoldmontPlus,
        Haswell,
        IceLake,
        IvyBridge,
        JasperLake,
        KabyLake,
        LunarLake,
        Nehalem,
        NetBurst,
        MeteorLake,
        PantherLake,
        RocketLake,
        SandyBridge,
        Silvermont,
        Skylake,
        TigerLake,
        Tremont,
        RaptorLake,
        SapphireRapids,
        ElkhartLake,
        Unknown
    }

    private const uint IA32_PACKAGE_THERM_STATUS = 0x1B1;
    private const uint IA32_PERF_STATUS = 0x0198;
    private const uint IA32_TEMPERATURE_TARGET = 0x01A2;
    private const uint IA32_THERM_STATUS_MSR = 0x019C;

    private const uint MSR_DRAM_ENERGY_STATUS = 0x619;
    private const uint MSR_PKG_ENERGY_STATUS = 0x611;
    private const uint MSR_PLATFORM_INFO = 0xCE;
    private const uint MSR_PP0_ENERGY_STATUS = 0x639;
    private const uint MSR_PP1_ENERGY_STATUS = 0x641;
    private const uint MSR_PLATFORM_ENERGY_STATUS = 0x64D;

    private const uint MSR_RAPL_POWER_UNIT = 0x606;
    
}
