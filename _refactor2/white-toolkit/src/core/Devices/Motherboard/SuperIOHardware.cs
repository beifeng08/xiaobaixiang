// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using System.Threading;
using SysCore.Devices.SuperIo;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
using SysCore.Sensors;
namespace SysCore.Devices;

internal sealed class SuperIOHardware : Hardware
{
    private readonly List<Sensor> _controls = [];
    private readonly List<Sensor> _fans = [];
    private readonly Motherboard _motherboard;

    private readonly UpdateDelegate _postUpdate;
    private readonly ReadValueDelegate _readControl;
    private readonly ReadValueDelegate _readFan;
    private readonly ReadValueDelegate _readTemperature;
    private readonly ReadValueDelegate _readVoltage;

    private readonly ISuperIO _superIO;
    private readonly List<Sensor> _temperatures = [];
    private readonly List<Sensor> _voltages = [];

    public SuperIOHardware(Motherboard motherboard, ISuperIO superIO, Manufacturer manufacturer, Model model, ISettings settings, int index)
        : base(ChipName.GetName(superIO.Chip), new Identifier("lpc", superIO.Chip.ToString().ToLowerInvariant(), index.ToString()), settings)
    {
        _motherboard = motherboard;
        _superIO = superIO;

        GetBoardSpecificConfiguration(superIO,
                                      manufacturer,
                                      model,
                                      index,
                                      out IList<Voltage> v,
                                      out IList<Temperature> t,
                                      out IList<Fan> f,
                                      out IList<ControlChannel> c,
                                      out _readVoltage,
                                      out _readTemperature,
                                      out _readFan,
                                      out _readControl,
                                      out _postUpdate,
                                      out _);

        CreateVoltageSensors(superIO, settings, v);
        CreateTemperatureSensors(superIO, settings, t);
        CreateFanSensors(superIO, settings, f);
        CreateControlSensors(superIO, settings, c);
    }

    public override HardwareType HardwareType
    {
        get { return HardwareType.SuperIO; }
    }

    public override IHardware Parent
    {
        get { return _motherboard; }
    }

    private void CreateControlSensors(ISuperIO superIO, ISettings settings, IList<ControlChannel> c)
    {
        foreach (ControlChannel ctrl in c)
        {
            int index = ctrl.Index;
            if (index < superIO.Controls.Length)
            {
                Sensor sensor = new(ctrl.Name, index, SensorType.Control, this, settings);
                SysCore.Model.Control control = new(sensor, settings, 0, 100);
                control.ControlModeChanged += cc =>
                {
                    switch (cc.ControlMode)
                    {
                        case ControlMode.Undefined:
                            return;
                        case ControlMode.Default:
                            superIO.SetControl(index, null);
                            break;
                        case ControlMode.Software:
                            superIO.SetControl(index, GetSoftwareValueAsByte(cc));
                            break;
                        default:
                            return;
                    }
                };

                control.SoftwareControlValueChanged += cc =>
                {
                    if (cc.ControlMode == ControlMode.Software)
                        superIO.SetControl(index, GetSoftwareValueAsByte(cc));
                };

                switch (control.ControlMode)
                {
                    case ControlMode.Undefined:
                        break;
                    case ControlMode.Default:
                        superIO.SetControl(index, null);
                        break;
                    case ControlMode.Software:
                        superIO.SetControl(index, GetSoftwareValueAsByte(control));
                        break;
                }

                sensor.Control = control;
                _controls.Add(sensor);
                ActivateSensor(sensor);
            }
        }
    }

    private static byte GetSoftwareValueAsByte(SysCore.Model.Control control)
    {
        const float percentToByteRatio = 2.55f;
        float value = control.SoftwareValue * percentToByteRatio;
        return (byte)value;
    }

    private void CreateFanSensors(ISuperIO superIO, ISettings settings, IList<Fan> f)
    {
        foreach (Fan fan in f)
        {
            if (fan.Index < superIO.Fans.Length)
            {
                Sensor sensor = new(fan.Name, fan.Index, SensorType.Fan, this, settings);
                _fans.Add(sensor);
            }
        }
    }

    private void CreateTemperatureSensors(ISuperIO superIO, ISettings settings, IList<Temperature> t)
    {
        foreach (Temperature temperature in t)
        {
            if (temperature.Index < superIO.Temperatures.Length)
            {
                Sensor sensor = new(temperature.Name,
                                    temperature.Index,
                                    SensorType.Temperature,
                                    this,
                                    [new ParameterDescription("Offset [°C]", "Temperature offset.", 0)],
                                    settings);

                _temperatures.Add(sensor);
            }
        }
    }

    private void CreateVoltageSensors(ISuperIO superIO, ISettings settings, IList<Voltage> v)
    {
        const string formula = "Voltage = value + (value - Vf) * Ri / Rf.";
        foreach (Voltage voltage in v)
        {
            if (voltage.Index < superIO.Voltages.Length)
            {
                Sensor sensor = new(voltage.Name,
                                    voltage.Index,
                                    voltage.Hidden,
                                    SensorType.Voltage,
                                    this,
                                    [
                                        new ParameterDescription("Ri [kΩ]", "Input resistance.\n" + formula, voltage.Ri),
                                        new ParameterDescription("Rf [kΩ]", "Reference resistance.\n" + formula, voltage.Rf),
                                        new ParameterDescription("Vf [V]", "Reference voltage.\n" + formula, voltage.Vf)
                                    ],
                                    settings);

                _voltages.Add(sensor);
            }
        }
    }

    private static void GetBoardSpecificConfiguration
    (
        ISuperIO superIO,
        Manufacturer manufacturer,
        Model model,
        int superIOIndex,
        out IList<Voltage> v,
        out IList<Temperature> t,
        out IList<Fan> f,
        out IList<ControlChannel> c,
        out ReadValueDelegate readVoltage,
        out ReadValueDelegate readTemperature,
        out ReadValueDelegate readFan,
        out ReadValueDelegate readControl,
        out UpdateDelegate postUpdate,
        out Mutex mutex)
    {
        readVoltage = index => superIO.Voltages[index];
        readTemperature = index => superIO.Temperatures[index];
        readFan = index => superIO.Fans[index];
        readControl = index => superIO.Controls[index];

        postUpdate = () => { };
        mutex = null;

        v = new List<Voltage>();
        t = new List<Temperature>();
        f = new List<Fan>();
        c = new List<ControlChannel>();

        switch (superIO.Chip)
        {
            case Chip.IT8705F:
            case Chip.IT8712F:
            case Chip.IT8716F:
            case Chip.IT8718F:
            case Chip.IT8720F:
            case Chip.IT8726F:
                GetIteConfigurationsA(superIO, manufacturer, model, v, t, f, c, ref readFan, ref postUpdate, ref mutex);
                break;

            case Chip.IT8613E:
            case Chip.IT8620E:
            case Chip.IT8625E:
            case Chip.IT8628E:
            case Chip.IT8631E:
            case Chip.IT8638E:
            case Chip.IT8655E:
            case Chip.IT8665E:
            case Chip.IT8686E:
            case Chip.IT8688E:
            case Chip.IT8689E:
            case Chip.IT8721F:
            case Chip.IT8728F:
            case Chip.IT8771E:
            case Chip.IT8772E:
            case Chip.IT8696E:
                GetIteConfigurationsB(superIO, manufacturer, model, v, t, f, c);
                break;

            case Chip.IT87952E:
            case Chip.IT8792E:
            case Chip.IT8790E:
                GetIteConfigurationsC(superIO, manufacturer, model, v, t, f, c);
                break;

            case Chip.F71858:
                v.Add(new Voltage("VCC3V", 0, 150, 150));
                v.Add(new Voltage("VSB3V", 1, 150, 150));
                v.Add(new Voltage("Battery", 2, 150, 150));

                for (int i = 0; i < superIO.Temperatures.Length; i++)
                    t.Add(new Temperature("Temperature #" + (i + 1), i));

                for (int i = 0; i < superIO.Fans.Length; i++)
                    f.Add(new Fan("Fan #" + (i + 1), i));

                break;

            case Chip.F71808E:
            case Chip.F71862:
            case Chip.F71869:
            case Chip.F71869A:
            case Chip.F71882:
            case Chip.F71889AD:
            case Chip.F71889ED:
            case Chip.F71889F:
                GetFintekConfiguration(superIO, manufacturer, model, v, t, f, c);
                break;

            case Chip.W83627EHF:
                GetWinbondConfigurationEhf(manufacturer, model, v, t, f, c);
                break;

            case Chip.W83627DHG:
            case Chip.W83627DHGP:
            case Chip.W83667HG:
            case Chip.W83667HGB:
                GetWinbondConfigurationHg(manufacturer, model, v, t, f, c);
                break;

            case Chip.W83627HF:
                v.Add(new Voltage("Vcore", 0));
                v.Add(new Voltage("Voltage #2", 1, true));
                v.Add(new Voltage("Voltage #3", 2, true));
                v.Add(new Voltage("AVCC", 3, 34, 51));
                v.Add(new Voltage("Voltage #5", 4, true));
                v.Add(new Voltage("+5VSB", 5, 34, 51));
                v.Add(new Voltage("CMOS Battery", 6));
                t.Add(new Temperature("CPU", 0));
                t.Add(new Temperature("Auxiliary", 1));
                t.Add(new Temperature("System", 2));
                f.Add(new Fan("System Fan", 0));
                f.Add(new Fan("CPU Fan", 1));
                f.Add(new Fan("Auxiliary Fan", 2));
                c.Add(new ControlChannel("Fan 1", 0));
                c.Add(new ControlChannel("Fan 2", 1));
                break;

            case Chip.W83627THF:
            case Chip.W83687THF:
                v.Add(new Voltage("Vcore", 0));
                v.Add(new Voltage("Voltage #2", 1, true));
                v.Add(new Voltage("Voltage #3", 2, true));
                v.Add(new Voltage("AVCC", 3, 34, 51));
                v.Add(new Voltage("Voltage #5", 4, true));
                v.Add(new Voltage("+5VSB", 5, 34, 51));
                v.Add(new Voltage("CMOS Battery", 6));
                t.Add(new Temperature("CPU", 0));
                t.Add(new Temperature("Auxiliary", 1));
                t.Add(new Temperature("System", 2));
                f.Add(new Fan("System Fan", 0));
                f.Add(new Fan("CPU Fan", 1));
                f.Add(new Fan("Auxiliary Fan", 2));
                c.Add(new ControlChannel("System Fan", 0));
                c.Add(new ControlChannel("CPU Fan", 1));
                c.Add(new ControlChannel("Auxiliary Fan", 2));
                break;

            case Chip.NCT5585D:
                switch (manufacturer)
                {
                    case Manufacturer.ASRock when model == Model.X870E_NOVA_WIFI:
                        
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DRAM", 1));
                        v.Add(new Voltage("+3.3V (AVCC)", 2, 34, 34));
                        v.Add(new Voltage("+3.3V (3VCC)", 3, 34, 34));
                        v.Add(new Voltage("VDD Misc", 4));
                        v.Add(new Voltage("3VSB", 7, 34, 34));
                        v.Add(new Voltage("VBat", 8, 34, 34));
                        v.Add(new Voltage("Voltage #2", 12));
                        v.Add(new Voltage("Voltage #3", 13));
                        v.Add(new Voltage("Voltage #7", 14));
                        v.Add(new Voltage("Voltage #9", 15));

                        t.Add(new Temperature("MOS", 1));
                        t.Add(new Temperature("CPU (PECI)", 2));
                        t.Add(new Temperature("Auxiliary 3", 3)); 

                        f.Add(new Fan("MOS Fan", 1));

                        c.Add(new ControlChannel("MOS Fan", 1));
                        break;
                }
                break;

            case Chip.NCT6771F:
            case Chip.NCT6776F:
                GetNuvotonConfigurationF(superIO, manufacturer, model, v, t, f, c);
                break;

            case Chip.NCT610XD:
                switch (manufacturer)
                {
                    case Manufacturer.Supermicro when model == Model.X11SWN_E:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+12V", 1, 94, 10.25f));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("3VCC", 3, 34, 34));
                        v.Add(new Voltage("+5V", 4, 14, 8.2f));
                        v.Add(new Voltage("VDIMM", 5));
                        v.Add(new Voltage("3VSB", 6, 34, 34));
                        v.Add(new Voltage("VBat", 7, 34, 34));
                        t.Add(new Temperature("System", 1));
                        t.Add(new Temperature("Peripheral", 2));
                        t.Add(new Temperature("CPU Core", 4));

                        f.Add(new Fan("Fan #1", 1));

                        c.Add(new ControlChannel("Fan #1", 1));

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #0", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #1", 4, true));
                        v.Add(new Voltage("Voltage #2", 5, true));
                        v.Add(new Voltage("Reserved", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("Voltage #10", 9, true));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Auxiliary", 1));
                        t.Add(new Temperature("System0", 2));
                        t.Add(new Temperature("System1", 3));
                        t.Add(new Temperature("System2", 4));
                        t.Add(new Temperature("System3", 5));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;

            case Chip.NCT6779D:
            case Chip.NCT6791D:
            case Chip.NCT6792D:
            case Chip.NCT6792DA:
            case Chip.NCT6793D:
            case Chip.NCT6795D:
            case Chip.NCT6796D:
            case Chip.NCT6796DR:
            case Chip.NCT6796DS:
            case Chip.NCT6797D:
            case Chip.NCT6798D:
            case Chip.NCT6799D:
            case Chip.NCT6701D:
            case Chip.NCT6683D:
                GetNuvotonConfigurationD(superIO, manufacturer, model, superIOIndex, v, t, f, c);
                break;

            case Chip.NCT6686D:
            case Chip.NCT6687D:
                switch (manufacturer)
                {
                    case Manufacturer.ASRock when model == Model.Z790_Taichi:
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 1));
                        t.Add(new Temperature("MOS", 2));

                        f.Add(new Fan("CPU Fan #1", 0));
                        f.Add(new Fan("Chassis Fan #4", 1));
                        f.Add(new Fan("CPU Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #2", 3));
                        f.Add(new Fan("Chassis Fan #1", 4));
                        f.Add(new Fan("MOS Fan #1", 5));

                        c.Add(new ControlChannel("CPU Fan #1", 0));
                        c.Add(new ControlChannel("Chassis Fan #4", 1));
                        c.Add(new ControlChannel("CPU Fan #2", 2));
                        c.Add(new ControlChannel("Chassis Fan #2", 3));
                        c.Add(new ControlChannel("Chassis Fan #1", 4));
                        c.Add(new ControlChannel("MOS Fan #1", 5));
                        break;
                    case Manufacturer.ASRock when model == Model.X870E_TAICHI:
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("VRM MOS", 2));

                        f.Add(new Fan("Water Pump", 0)); 
                        f.Add(new Fan("Chassis Fan #3", 1)); 
                        f.Add(new Fan("Chassis Fan #4", 2)); 

                        c.Add(new ControlChannel("Water Pump", 0)); 
                        c.Add(new ControlChannel("Chassis Fan #3", 1)); 
                        c.Add(new ControlChannel("Chassis Fan #4", 2)); 
                        break;
                    case Manufacturer.MSI when model == Model.B550A_PRO:
                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("CPU Northbridge/SoC", 2));
                        v.Add(new Voltage("DIMM", 3, 1, 1));
                        v.Add(new Voltage("Vcore", 4, -1, 2));
                        v.Add(new Voltage("Chipset", 5));
                        v.Add(new Voltage("CPU System Agent", 6));
                        v.Add(new Voltage("+3.3V", 8));
                        v.Add(new Voltage("+1.8V", 9));
                        v.Add(new Voltage("CPU VDDP", 10));
                        v.Add(new Voltage("+3V Standby", 11));
                        v.Add(new Voltage("AVSB", 12));

                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("System", 1));
                        t.Add(new Temperature("VRM MOS", 2));
                        t.Add(new Temperature("Chipset", 3));
                        t.Add(new Temperature("CPU Socket", 4));
                        t.Add(new Temperature("PCIe x1", 5));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Pump Fan", 1));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        f.Add(new Fan("System Fan #3", 4));
                        f.Add(new Fan("System Fan #4", 5));
                        f.Add(new Fan("System Fan #5", 6));
                        f.Add(new Fan("System Fan #6", 7));

                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("Pump Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 2));
                        c.Add(new ControlChannel("System Fan #2", 3));
                        c.Add(new ControlChannel("System Fan #3", 4));
                        c.Add(new ControlChannel("System Fan #4", 5));
                        c.Add(new ControlChannel("System Fan #5", 6));
                        c.Add(new ControlChannel("System Fan #6", 7));

                        break;

                    case Manufacturer.MSI when model == Model.B650M_Gaming_Plus_Wifi: 
                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("CPU Northbridge/SoC", 2));
                        v.Add(new Voltage("CPU VDDIO", 3, 1, 1));
                        v.Add(new Voltage("Vcore", 4, -1, 2));
                        v.Add(new Voltage("+3.3V", 8));
                        v.Add(new Voltage("+3V Standby", 11));
                        v.Add(new Voltage("AVSB", 12));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("CMOS Battery", 13));

                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("System", 1));
                        t.Add(new Temperature("VRM MOS", 2));
                        t.Add(new Temperature("Chipset", 3));
                        t.Add(new Temperature("CPU Socket", 4));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("CPU Pump Fan", 1));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        f.Add(new Fan("System Fan #3", 4));

                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("CPU Pump Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 2));
                        c.Add(new ControlChannel("System Fan #2", 3));
                        c.Add(new ControlChannel("System Fan #3", 4));

                        break;

                    case Manufacturer.MSI when model == Model.Z790_GODLIKE_MAX: 
                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("Vcore", 2));
                        v.Add(new Voltage("Voltage #1", 3));
                        v.Add(new Voltage("DIMM", 4));
                        v.Add(new Voltage("CPU I/O", 5));
                        v.Add(new Voltage("CPU System Agent", 6));
                        v.Add(new Voltage("Voltage #2", 7));
                        v.Add(new Voltage("AVCC3", 8));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("VRef", 10));
                        v.Add(new Voltage("VSB", 11));
                        v.Add(new Voltage("AVSB", 12));
                        v.Add(new Voltage("CMOS Battery", 13));

                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("System", 1));
                        t.Add(new Temperature("VRM MOS", 2));
                        t.Add(new Temperature("PCH", 3));
                        t.Add(new Temperature("CPU Socket", 4));
                        t.Add(new Temperature("Thermistor Sensor 1", 5));
                        t.Add(new Temperature("Thermistor Sensor 2", 6));
                        t.Add(new Temperature("PCIe #1", 7));
                        t.Add(new Temperature("PCIe #2", 8));
                        t.Add(new Temperature("M2 #1", 9));
                        t.Add(new Temperature("M2 #4", 10));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Pump Fan #1", 1));
                        f.Add(new Fan("Pump Fan #2", 8));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        f.Add(new Fan("System Fan #3", 4));
                        f.Add(new Fan("System Fan #4", 5));
                        f.Add(new Fan("System Fan #5", 6));
                        f.Add(new Fan("System Fan #6", 7));
                        f.Add(new Fan("System Fan #7", 16));
                        f.Add(new Fan("EZ-Connect Fan", 9));

                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("Pump Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #1", 2));
                        c.Add(new ControlChannel("System Fan #2", 3));
                        c.Add(new ControlChannel("System Fan #3", 4));
                        c.Add(new ControlChannel("System Fan #4", 5));
                        c.Add(new ControlChannel("System Fan #5", 6));
                        c.Add(new ControlChannel("System Fan #6", 7));

                        break;

                    default:
                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("Vcore", 2));
                        v.Add(new Voltage("Voltage #1", 3));
                        v.Add(new Voltage("DIMM", 4));
                        v.Add(new Voltage("CPU I/O", 5));
                        v.Add(new Voltage("CPU System Agent", 6));
                        v.Add(new Voltage("Voltage #2", 7));
                        v.Add(new Voltage("AVCC3", 8));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("VRef", 10));
                        v.Add(new Voltage("VSB", 11));
                        v.Add(new Voltage("AVSB", 12));
                        v.Add(new Voltage("CMOS Battery", 13));

                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("System", 1));
                        t.Add(new Temperature("VRM MOS", 2));
                        t.Add(new Temperature("PCH", 3));
                        t.Add(new Temperature("CPU Socket", 4));
                        t.Add(new Temperature("PCIe x1", 5));
                        t.Add(new Temperature("M2 #1", 6));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Pump Fan", 1));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        f.Add(new Fan("System Fan #3", 4));
                        f.Add(new Fan("System Fan #4", 5));
                        f.Add(new Fan("System Fan #5", 6));
                        f.Add(new Fan("System Fan #6", 7));

                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("Pump Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 2));
                        c.Add(new ControlChannel("System Fan #2", 3));
                        c.Add(new ControlChannel("System Fan #3", 4));
                        c.Add(new ControlChannel("System Fan #4", 5));
                        c.Add(new ControlChannel("System Fan #5", 6));
                        c.Add(new ControlChannel("System Fan #6", 7));

                        break;
                }

                break;

            case Chip.NCT6687DR:

                t.Add(new Temperature("CPU Core", 0));
                t.Add(new Temperature("System", 1));
                t.Add(new Temperature("VRM MOS", 2));
                t.Add(new Temperature("Chipset", 3));

                f.Add(new Fan("CPU Fan", 0));
                f.Add(new Fan("Pump Fan #1", 1));
                f.Add(new Fan("Chipset Fan", 2));
                f.Add(new Fan("System Fan #1", 10));
                f.Add(new Fan("System Fan #2", 11));
                f.Add(new Fan("System Fan #3", 12));
                f.Add(new Fan("System Fan #4", 13));
                f.Add(new Fan("System Fan #5", 14));
                f.Add(new Fan("System Fan #6", 15));
                f.Add(new Fan("EZ-Connect Fan", 3));

                c.Add(new ControlChannel("CPU Fan", 0));
                c.Add(new ControlChannel("Pump Fan", 1));
                c.Add(new ControlChannel("Chipset Fan", 2));
                c.Add(new ControlChannel("System Fan #1", 10));
                c.Add(new ControlChannel("System Fan #2", 11));
                c.Add(new ControlChannel("System Fan #3", 12));
                c.Add(new ControlChannel("System Fan #4", 13));
                c.Add(new ControlChannel("System Fan #5", 14));
                c.Add(new ControlChannel("System Fan #6", 15));
                c.Add(new ControlChannel("EZ-Connect Fan", 3));

                switch (model)
                {
                    case Model.X870E_TOMAHAWK_WIFI:
                    case Model.X870E_TOMAHAWK_MAX_WIFI_PZ:
                    case Model.X870E_EDGE_TI_WIFI:
                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("CPU Northbridge/SoC", 2));
                        v.Add(new Voltage("DIMM", 3, 1, 1));
                        v.Add(new Voltage("Vcore", 4, -1, 2));
                        v.Add(new Voltage("Chipset", 5));
                        v.Add(new Voltage("CPU System Agent", 6));
                        
                        v.Add(new Voltage("+3.3V", 8));
                        v.Add(new Voltage("VREF", 9));
                        v.Add(new Voltage("+1.8V", 10));
                        v.Add(new Voltage("+3V Standby", 11));
                        v.Add(new Voltage("AVSB", 12));
                        v.Add(new Voltage("CMOS Battery", 13));

                        t.Add(new Temperature("CPU Socket", 4));

                        break;

                    case Model.X870E_ACE_MAX:
                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("CPU Northbridge/SoC", 2));
                        v.Add(new Voltage("DIMM", 3, 1, 1));
                        v.Add(new Voltage("Vcore", 4, -1, 2));
                        v.Add(new Voltage("Chipset", 5));
                        v.Add(new Voltage("CPU System Agent", 6));
                        
                        v.Add(new Voltage("+3.3V", 8));
                        v.Add(new Voltage("VREF", 9));
                        v.Add(new Voltage("+1.8V", 10));
                        v.Add(new Voltage("+3V Standby", 11));
                        v.Add(new Voltage("AVSB", 12));
                        v.Add(new Voltage("CMOS Battery", 13));

                        t.Add(new Temperature("Chipset #2", 5));
                        t.Add(new Temperature("Thermistor Sensor #1", 6));
                        t.Add(new Temperature("Thermistor Sensor #2", 4));

                        break;

                    case Model.X870E_CARBON_WIFI:
                    case Model.X870E_GODLIKE:
                        f.Add(new Fan("System Fan #7", 9));
                        c.Add(new ControlChannel("System Fan #7", 9));

                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("CPU Northbridge/SoC", 2));
                        v.Add(new Voltage("DIMM", 3, 1, 1));
                        v.Add(new Voltage("Vcore", 4, -1, 2));
                        v.Add(new Voltage("Chipset", 5));
                        v.Add(new Voltage("CPU System Agent", 6));
                        
                        v.Add(new Voltage("+3.3V", 8));
                        v.Add(new Voltage("VREF", 9));
                        v.Add(new Voltage("+1.8V", 10));
                        v.Add(new Voltage("+3V Standby", 11));
                        v.Add(new Voltage("AVSB", 12));
                        v.Add(new Voltage("CMOS Battery", 13));

                        t.Add(new Temperature("PCIe x1", 5));
                        t.Add(new Temperature("Thermistor Sensor #1", 6));
                        t.Add(new Temperature("Thermistor Sensor #2", 4));

                        break;

                    case Model.Z890_CARBON_WIFI:
                    case Model.Z890_TOMAHAWK_WIFI:
                    case Model.Z890_ACE:
                    case Model.Z890_UNIFY_X:
                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("Vcore", 2));
                        v.Add(new Voltage("VIN5", 3));
                        v.Add(new Voltage("VDIMM", 4));
                        v.Add(new Voltage("Chipset", 5));
                        
                        v.Add(new Voltage("VIN7", 7));
                        v.Add(new Voltage("+3.3V", 8));
                        v.Add(new Voltage("VTT", 9));
                        v.Add(new Voltage("+1.8V", 10));
                        v.Add(new Voltage("+3V Standby", 11));
                        v.Add(new Voltage("AVSB", 12));
                        v.Add(new Voltage("CMOS Battery", 13));

                        t.Add(new Temperature("CPU Socket", 4));
                        t.Add(new Temperature("PCIe x1", 5));
                        t.Add(new Temperature("M2 #1", 6));

                        break;

                    case Model.Z890_EDGE_TI_WIFI:
                    case Model.Z890I_EDGE_TI_WIFI:
                    case Model.Z890P_PRO_WIFI:
                    case Model.Z890A_PRO_WIFI:
                    case Model.Z890S_PRO_WIFI:
                    case Model.Z890_GAMING_PLUS_WIFI:
                    case Model.Z890S_PRO_WIFI_PROJECT_ZERO:
                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("Vcore", 2));
                        v.Add(new Voltage("VIN5", 3));
                        v.Add(new Voltage("VDIMM", 4));
                        v.Add(new Voltage("Chipset", 5));
                        
                        v.Add(new Voltage("Unknown_3", 7));
                        v.Add(new Voltage("+3.3V", 8));
                        v.Add(new Voltage("VTT", 9));
                        v.Add(new Voltage("+1.8V", 10));
                        v.Add(new Voltage("+3V Standby", 11));
                        v.Add(new Voltage("AVSB", 12));
                        v.Add(new Voltage("CMOS Battery", 13));

                        break;

                    default:
                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("CPU Northbridge/SoC", 2));
                        v.Add(new Voltage("DIMM", 3, 1, 1));
                        v.Add(new Voltage("Vcore", 4, -1, 2));
                        v.Add(new Voltage("Chipset", 5));
                        v.Add(new Voltage("CPU System Agent", 6));
                        
                        v.Add(new Voltage("+3.3V", 8));
                        v.Add(new Voltage("VREF", 9));
                        v.Add(new Voltage("+1.8V", 10));
                        v.Add(new Voltage("+3V Standby", 11));
                        v.Add(new Voltage("AVSB", 12));
                        v.Add(new Voltage("CMOS Battery", 13));

                        break;
                }

                break;

            case Chip.IPMI:
                Ipmi ipmi = (Ipmi)superIO;

                foreach (Temperature temperature in ipmi.GetTemperatures())
                    t.Add(temperature);

                foreach (Fan fan in ipmi.GetFans())
                    f.Add(fan);

                foreach (Voltage voltage in ipmi.GetVoltages())
                    v.Add(voltage);

                foreach (ControlChannel control in ipmi.GetControls())
                    c.Add(control);

                break;

            default:
                GetDefaultConfiguration(superIO, v, t, f, c);
                break;
        }
    }

    private static void GetDefaultConfiguration(ISuperIO superIO, ICollection<Voltage> v, ICollection<Temperature> t, ICollection<Fan> f, ICollection<ControlChannel> c)
    {
        for (int i = 0; i < superIO.Voltages.Length; i++)
            v.Add(new Voltage("Voltage #" + (i + 1), i, true));

        for (int i = 0; i < superIO.Temperatures.Length; i++)
            t.Add(new Temperature("Temperature #" + (i + 1), i));

        for (int i = 0; i < superIO.Fans.Length; i++)
            f.Add(new Fan("Fan #" + (i + 1), i));

        for (int i = 0; i < superIO.Controls.Length; i++)
            c.Add(new ControlChannel("Fan #" + (i + 1), i));
    }

    private static void GetIteConfigurationsA
    (
        ISuperIO superIO,
        Manufacturer manufacturer,
        Model model,
        IList<Voltage> v,
        IList<Temperature> t,
        IList<Fan> f,
        ICollection<ControlChannel> c,
        ref ReadValueDelegate readFan,
        ref UpdateDelegate postUpdate,
        ref Mutex mutex)
    {
        switch (manufacturer)
        {
            case Manufacturer.ASUS:
                switch (model)
                {
                    case Model.CROSSHAIR_III_FORMULA: 
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("CPU", 0));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        break;

                    case Model.M2N_SLI_Deluxe:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 4, 30, 10));
                        v.Add(new Voltage("+5VSB", 7, 6.8f, 10));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 1));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Chassis Fan #1", 1));
                        f.Add(new Fan("Power Fan", 2));

                        break;

                    case Model.M4A79XTD_EVO: 
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 1));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Chassis Fan #1", 1));
                        f.Add(new Fan("Chassis Fan #2", 2));

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("Voltage #3", 2, true));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("Voltage #8", 7, true));
                        v.Add(new Voltage("CMOS Battery", 8));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.ASRock:
                switch (model)
                {
                    case Model.P55_Deluxe: 
                        GetASRockConfiguration(superIO,
                                               v,
                                               t,
                                               f,
                                               ref readFan,
                                               ref postUpdate,
                                               out mutex);

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("Voltage #3", 2, true));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("Voltage #8", 7, true));
                        v.Add(new Voltage("CMOS Battery", 8));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.DFI:
                switch (model)
                {
                    case Model.LP_BI_P45_T2RS_Elite: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("CPU Termination", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 4, 30, 10));
                        v.Add(new Voltage("Northbridge Core", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+5VSB", 7, 6.8f, 10));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("System", 1));
                        t.Add(new Temperature("Chipset", 2));
                        f.Add(new Fan("Fan #1", 0));
                        f.Add(new Fan("Fan #2", 1));
                        f.Add(new Fan("Fan #3", 2));

                        break;

                    case Model.LP_DK_P55_T3EH9: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("CPU Termination", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 4, 30, 10));
                        v.Add(new Voltage("Phase Locked Loop", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+5VSB", 7, 6.8f, 10));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("Chipset", 0));
                        t.Add(new Temperature("CPU PWM", 1));
                        t.Add(new Temperature("CPU", 2));
                        f.Add(new Fan("Fan #1", 0));
                        f.Add(new Fan("Fan #2", 1));
                        f.Add(new Fan("Fan #3", 2));

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("CPU Termination", 1, true));
                        v.Add(new Voltage("+3.3V", 2, true));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10, 0, true));
                        v.Add(new Voltage("+12V", 4, 30, 10, 0, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("DIMM", 6, true));
                        v.Add(new Voltage("+5VSB", 7, 6.8f, 10, 0, true));
                        v.Add(new Voltage("CMOS Battery", 8));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.Gigabyte:
                switch (model)
                {
                    case Model._965P_S3: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 7, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));

                        break;

                    case Model.EP45_DS3R: 
                    case Model.EP45_UD3R:
                    case Model.X38_DS5:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 7, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #2", 1));
                        f.Add(new Fan("Power Fan", 2));
                        f.Add(new Fan("System Fan #1", 3));

                        break;

                    case Model.EX58_EXTREME: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Northbridge", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #2", 1));
                        f.Add(new Fan("Power Fan", 2));
                        f.Add(new Fan("System Fan #1", 3));

                        break;

                    case Model.P35_DS3: 
                    case Model.P35_DS3L: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 7, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("Power Fan", 3));

                        break;

                    case Model.P55_UD4: 
                    case Model.P55A_UD3: 
                    case Model.P55M_UD4: 
                    case Model.H55_USB3: 
                    case Model.EX58_UD3R: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 5, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #2", 1));
                        f.Add(new Fan("Power Fan", 2));
                        f.Add(new Fan("System Fan #1", 3));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #2", 1));

                        break;

                    case Model.H55N_USB3: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 5, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));

                        break;

                    case Model.G41M_COMBO: 
                    case Model.G41MT_S2: 
                    case Model.G41MT_S2P: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 7, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("CPU", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));

                        break;

                    case Model._970A_UD3: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 4, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("Power Fan", 4));
                        c.Add(new ControlChannel("PWM #1", 0));
                        c.Add(new ControlChannel("PWM #2", 1));
                        c.Add(new ControlChannel("PWM #3", 2));

                        break;

                    case Model.MA770T_UD3: 
                    case Model.MA770T_UD3P: 
                    case Model.MA790X_UD3P: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 4, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("Power Fan", 3));

                        break;

                    case Model.MA78LM_S2H: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 4, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("VRM", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("Power Fan", 3));

                        break;

                    case Model.MA790X_DS4: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 4, 24.3f, 8.2f));
                        v.Add(new Voltage("VBat", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("Power Fan", 4));
                        
                        c.Add(new ControlChannel("CPU Fan DC Control", 0));
                        c.Add(new ControlChannel("CPU Fan PWM Control", 2));

                        c.Add(new ControlChannel("System Fan #1 Control", 1));

                        break;

                    case Model.MA785GM_US2H: 
                    case Model.MA785GMT_UD2H: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 4, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));
                        f.Add(new Fan("Northbridge Fan", 2));

                        break;

                    case Model.X58A_UD3R: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+3.3V", 2));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10));
                        v.Add(new Voltage("+12V", 5, 24.3f, 8.2f));
                        v.Add(new Voltage("CMOS Battery", 8));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Northbridge", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #2", 1));
                        f.Add(new Fan("Power Fan", 2));
                        f.Add(new Fan("System Fan #1", 3));

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1, true));
                        v.Add(new Voltage("+3.3V", 2, true));
                        v.Add(new Voltage("+5V", 3, 6.8f, 10, 0, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("Voltage #8", 7, true));
                        v.Add(new Voltage("CMOS Battery", 8));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;

            default:
                v.Add(new Voltage("Vcore", 0));
                v.Add(new Voltage("Voltage #2", 1, true));
                v.Add(new Voltage("Voltage #3", 2, true));
                v.Add(new Voltage("Voltage #4", 3, true));
                v.Add(new Voltage("Voltage #5", 4, true));
                v.Add(new Voltage("Voltage #6", 5, true));
                v.Add(new Voltage("Voltage #7", 6, true));
                v.Add(new Voltage("Voltage #8", 7, true));
                v.Add(new Voltage("CMOS Battery", 8));

                for (int i = 0; i < superIO.Temperatures.Length; i++)
                    t.Add(new Temperature("Temperature #" + (i + 1), i));

                for (int i = 0; i < superIO.Fans.Length; i++)
                    f.Add(new Fan("Fan #" + (i + 1), i));

                for (int i = 0; i < superIO.Controls.Length; i++)
                    c.Add(new ControlChannel("Fan #" + (i + 1), i));

                break;
        }
    }

    private static void GetASRockConfiguration
    (
        ISuperIO superIO,
        IList<Voltage> v,
        IList<Temperature> t,
        IList<Fan> f,
        ref ReadValueDelegate readFan,
        ref UpdateDelegate postUpdate,
        out Mutex mutex)
    {
        v.Add(new Voltage("Vcore", 0));
        v.Add(new Voltage("+3.3V", 2));
        v.Add(new Voltage("+12V", 4, 30, 10));
        v.Add(new Voltage("+5V", 5, 6.8f, 10));
        v.Add(new Voltage("CMOS Battery", 8));
        t.Add(new Temperature("CPU", 0));
        t.Add(new Temperature("Motherboard", 1));
        f.Add(new Fan("CPU Fan", 0));
        f.Add(new Fan("Chassis Fan #1", 1));

        mutex = new Mutex(false, "ASRockOCMark");

        bool exclusiveAccess = false;
        try
        {
            exclusiveAccess = mutex.WaitOne(10, false);
        }
        catch (AbandonedMutexException)
        { }
        catch (InvalidOperationException)
        { }

        if (exclusiveAccess)
        {
            f.Add(new Fan("Chassis Fan #2", 2));
            f.Add(new Fan("Chassis Fan #3", 3));
            f.Add(new Fan("Power Fan", 4));

            readFan = index =>
            {
                if (index < 2)
                {
                    return superIO.Fans[index];
                }

                byte? gpio = superIO.ReadGpio(7);
                if (!gpio.HasValue)
                    return null;

                int[] masks = [0x05, 0x03, 0x06];
                return ((gpio.Value >> 3) & 0x07) == masks[index - 2] ? superIO.Fans[2] : null;
            };

            int fanIndex = 0;

            postUpdate = () =>
            {
                
                byte? gpio = superIO.ReadGpio(7);
                if (!gpio.HasValue)
                    return;

                int[] masks = [0x05, 0x03, 0x06];
                superIO.WriteGpio(7, (byte)((gpio.Value & 0xC7) | (masks[fanIndex] << 3)));
                fanIndex = (fanIndex + 1) % 3;
            };
        }
    }

    private static void GetIteConfigurationsB(ISuperIO superIO, Manufacturer manufacturer, Model model, IList<Voltage> v, IList<Temperature> t, IList<Fan> f, IList<ControlChannel> c)
    {
        switch (manufacturer)
        {
            case Manufacturer.ASUS:
                switch (model)
                {
                    case Model.PRIME_X370_PRO: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Southbridge 2.5V", 1));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("Voltage #4", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3.3V", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        v.Add(new Voltage("Voltage #10", 9, true));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 1));
                        t.Add(new Temperature("VRM", 2));

                        for (int i = 3; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Chassis Fan #1", 1));
                        f.Add(new Fan("Chassis Fan #2", 2));
                        f.Add(new Fan("AIO Pump", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        f.Add(new Fan("Water Pump", 5));

                        for (int i = 6; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.TUF_X470_PLUS_GAMING: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Southbridge 2.5V", 1));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("Voltage #4", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3.3V", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        v.Add(new Voltage("Voltage #10", 9, true));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 1));
                        t.Add(new Temperature("PCH", 2));

                        for (int i = 3; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        f.Add(new Fan("CPU Fan", 0));

                        for (int i = 1; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.ROG_ZENITH_EXTREME: 
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        v.Add(new Voltage("DIMM A/B", 1, 10, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("Southbridge 1.05V", 4, 10, 10));
                        v.Add(new Voltage("DIMM C/D", 5, 10, 10));
                        v.Add(new Voltage("Phase Locked Loop", 6, 10, 10));
                        v.Add(new Voltage("+3.3V", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 1));
                        t.Add(new Temperature("CPU Socket", 2));
                        t.Add(new Temperature("Temperature #4", 3));
                        t.Add(new Temperature("Temperature #5", 4));
                        t.Add(new Temperature("VRM", 5));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Chassis Fan #1", 1));
                        f.Add(new Fan("Chassis Fan #2", 2));
                        f.Add(new Fan("High Amp Fan", 3));
                        f.Add(new Fan("Fan 5", 4));
                        f.Add(new Fan("Fan 6", 5));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.ROG_STRIX_X470_I: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Southbridge 2.5V", 1));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("+3.3V", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 1));
                        t.Add(new Temperature("Thermistor Sensor", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM", 4));
                        t.Add(new Temperature("Temperature #6", 5));

                        f.Add(new Fan("CPU Fan", 0));

                        f.Add(new Fan("Chassis Fan #1", 1));
                        f.Add(new Fan("Chassis Fan #2", 2));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + i, i));

                        break;

                    case Model.TUF_GAMING_B450_PLUS_II: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Vccp2", 1));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("3VSB", 7, 10, 10));
                        v.Add(new Voltage("VBat", 8, 10, 10));
                        
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 1));
                        t.Add(new Temperature("Temperature #3", 2));
                        t.Add(new Temperature("Temperature #4", 3));
                        t.Add(new Temperature("Temperature #5", 4));
                        t.Add(new Temperature("Temperature #6", 5));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Chassis Fan #1", 1));
                        f.Add(new Fan("Chassis Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #3", 3));
                        
                        f.Add(new Fan("AIO Pump", 5));

                        c.Add(new ControlChannel("CPU Fan Control", 0));
                        c.Add(new ControlChannel("Chassis Fan #1 Control", 1));
                        c.Add(new ControlChannel("Chassis Fan #2 Control", 2));
                        c.Add(new ControlChannel("Chassis Fan #3 Control", 3));
                        
                        c.Add(new ControlChannel("AIO Pump Control", 5));

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("Voltage #3", 2, true));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("Voltage #8", 7, true));
                        v.Add(new Voltage("CMOS Battery", 8));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.ECS:
                switch (model)
                {
                    case Model.A890GXM_A: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("Northbridge", 2));
                        v.Add(new Voltage("AVCC", 3, 10, 10));
                        
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("System", 1));
                        t.Add(new Temperature("Northbridge", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));
                        f.Add(new Fan("Power Fan", 2));

                        break;

                    default:
                        v.Add(new Voltage("Voltage #1", 0, true));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("Voltage #3", 2, true));
                        v.Add(new Voltage("AVCC", 3, 10, 10, 0, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10, 0, true));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.Gigabyte:
                switch (model)
                {
                    case Model.H61M_DS2_REV_1_2: 
                    case Model.H61M_USB3_B3_REV_2_0: 
                        v.Add(new Voltage("CPU Termination", 0));
                        v.Add(new Voltage("+12V", 2, 30.9f, 10));
                        v.Add(new Voltage("Vcore", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));

                        break;

                    case Model.H67A_UD3H_B3: 
                    case Model.H67A_USB3_B3: 
                        v.Add(new Voltage("CPU Termination", 0));
                        v.Add(new Voltage("+5V", 1, 15, 10));
                        v.Add(new Voltage("+12V", 2, 30.9f, 10));
                        v.Add(new Voltage("Vcore", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("Power Fan", 2));
                        f.Add(new Fan("System Fan #2", 3));

                        break;

                    case Model.B75M_D3H: 
                        v.Add(new Voltage("CPU Termination", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.49f, 10));
                        v.Add(new Voltage("+5V", 3, 15, 10));
                        v.Add(new Voltage("+12V", 2, 10, 2));
                        v.Add(new Voltage("iGPU VAXG", 4));
                        v.Add(new Voltage("Vcore", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));
                        c.Add(new ControlChannel("CPU Fan", 2));
                        c.Add(new ControlChannel("System Fan", 1));

                        break;

                    case Model._970A_DS3P: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("+3.3V", 4, 6.5f, 10));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU Package", 1));
                        t.Add(new Temperature("CPU Cores", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("Power Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));

                        break;

                    case Model.H81M_HD3: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.5f, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("iGPU", 4));
                        v.Add(new Voltage("CPU Input Auxiliary", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("System", 0));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan", 1));

                        break;

                    case Model.H97_D3H: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.5f, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("iGPU", 4));
                        v.Add(new Voltage("CPU Input Auxiliary", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));

                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("System", 0));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("CPU Optional Fan", 1));
                        f.Add(new Fan("System Fan #1", 4));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));

                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("CPU Optional Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 4));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));

                        break;

                    case Model.Z170N_WIFI: 
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        
                        v.Add(new Voltage("iGPU VAXG", 5, 0, 1));
                        v.Add(new Voltage("DIMM A/B", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        v.Add(new Voltage("AVCC3", 9, 54, 10));

                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM", 4));
                        t.Add(new Temperature("System #2", 5));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));

                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan", 1));

                        break;

                    case Model.AX370_Gaming_K7: 
                    case Model.AX370_Gaming_5:
                    case Model.AB350_Gaming_3: 
                        
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 0.65f, 1));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("VSOC", 4));
                        v.Add(new Voltage("VDDP", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        v.Add(new Voltage("AVCC3", 9, 7.53f, 1));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.X399_AORUS_Gaming_7: 
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("DIMM C/D", 4, 0, 1));
                        v.Add(new Voltage("Vcore SoC", 5, 0, 1));
                        v.Add(new Voltage("DIMM A/B", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        v.Add(new Voltage("AVCC3", 9, 54, 10));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM", 4));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.B450_AORUS_PRO:
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("Vcore SoC", 4, 0, 1));
                        v.Add(new Voltage("VDDP", 5, 0, 1));
                        v.Add(new Voltage("DIMM", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("VSoC MOS", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.B450_GAMING_X:
                    case Model.B450_AORUS_ELITE:
                    case Model.B450M_AORUS_ELITE:
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("Vcore SoC", 4, 0, 1));
                        v.Add(new Voltage("VDDP", 5, 0, 1));
                        v.Add(new Voltage("DIMM", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("VSoC MOS", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));

                        break;

                    case Model.B450M_GAMING: 
                    case Model.B450_AORUS_M:
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("Vcore SoC", 4, 0, 1));
                        v.Add(new Voltage("VDDP", 5, 0, 1));
                        v.Add(new Voltage("DIMM", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("VSoC MOS", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));

                        break;

                    case Model.B450_I_AORUS_PRO_WIFI:
                    case Model.B450M_DS3H: 
                    case Model.B450M_S2H:
                    case Model.B450M_H:
                    case Model.B450M_K:
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("Vcore SoC", 4, 0, 1));
                        v.Add(new Voltage("VDDP", 5, 0, 1));
                        v.Add(new Voltage("DIMM", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("VSoC MOS", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan", 1));

                        break;

                    case Model.X470_AORUS_GAMING_7_WIFI: 
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("Vcore SoC", 4, 0, 1));
                        v.Add(new Voltage("VDDP", 5, 0, 1));
                        v.Add(new Voltage("DIMM A/B", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        v.Add(new Voltage("AVCC3", 9, 54, 10));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM", 4));
                        t.Add(new Temperature("Sensor #1", 5));
                        t.Add(new Temperature("Sensor #2", 6));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.B560M_AORUS_ELITE: 
                    case Model.B560M_AORUS_PRO:
                    case Model.B560M_AORUS_PRO_AX:
                    case Model.B560I_AORUS_PRO_AX:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 29.4f, 45.3f));
                        v.Add(new Voltage("+12V", 2, 10f, 2f));
                        v.Add(new Voltage("+5V", 3, 15f, 10f));
                        v.Add(new Voltage("iGPU VAGX", 4));
                        v.Add(new Voltage("CPU System Agent", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10f, 10f));
                        v.Add(new Voltage("CMOS Battery", 8, 10f, 10f));
                        v.Add(new Voltage("AVCC3", 9, 59.9f, 9.8f));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("System #2", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.B650_EAGLE_AX: 
                    case Model.B650_AORUS_ELITE: 
                    case Model.B650_AORUS_ELITE_AX: 
                    case Model.B650_AORUS_ELITE_V2: 
                    case Model.B650_AORUS_ELITE_AX_V2: 
                    case Model.B650_AORUS_ELITE_AX_ICE: 
                    case Model.B650_GAMING_X_AX: 
                    case Model.B650E_AORUS_ELITE_AX_ICE: 
                    case Model.B650M_AORUS_PRO: 
                    case Model.B650M_AORUS_PRO_AX:
                    case Model.B650M_AORUS_ELITE:
                    case Model.B650M_AORUS_ELITE_AX:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 29.4f, 45.3f));
                        v.Add(new Voltage("+12V", 2, 10f, 2f));
                        v.Add(new Voltage("+5V", 3, 15f, 10f));
                        v.Add(new Voltage("Vcore SoC", 4));
                        v.Add(new Voltage("Vcore Misc", 5));
                        v.Add(new Voltage("Dual DDR5 5V", 6, 1.5f, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10f, 10f));
                        v.Add(new Voltage("CMOS Battery", 8, 10f, 10f));
                        v.Add(new Voltage("AVCC3", 9, 10f, 10f));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("VSoC MOS", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));

                        if (model == Model.B650_EAGLE_AX)
                        {
                            f.Add(new Fan("CPU Optional Fan", 4));
                        }
                        else
                        {
                            f.Add(new Fan("System Fan #4 / Pump", 4));
                            f.Add(new Fan("CPU Optional Fan", 5));
                        }

                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));

                        if (model == Model.B650_EAGLE_AX)
                        {
                            c.Add(new ControlChannel("CPU Optional Fan", 4));
                        }
                        else
                        {
                            c.Add(new ControlChannel("System Fan #4 / Pump", 4));
                            c.Add(new ControlChannel("CPU Optional Fan", 5));
                        }

                        break;

                    case Model.B650I_AX: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 29.4f, 45.3f));
                        v.Add(new Voltage("+12V", 2, 10f, 2f));
                        v.Add(new Voltage("+5V", 3, 15f, 10f));
                        v.Add(new Voltage("Vcore SoC", 4));
                        v.Add(new Voltage("Vcore Misc", 5));

                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("VSoC MOS", 5));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));

                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));

                        break;

                    case Model.A320M_S2H_CF: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 29.4f, 45.3f));
                        v.Add(new Voltage("+12V", 2, 10f, 2f));
                        v.Add(new Voltage("+5V", 3, 15f, 10f));
                        v.Add(new Voltage("CPU Vcore SoC", 4));
                        v.Add(new Voltage("CPU VDDP", 5));
                        v.Add(new Voltage("DRAM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 1, 1));
                        v.Add(new Voltage("CMOS Battery", 8, 1, 1));
                        v.Add(new Voltage("AVCC3", 9, 1, 1));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("VSoC MOS", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan", 1));

                        break;

                    case Model.B360M_H: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 29.4f, 45.3f));
                        v.Add(new Voltage("+12V", 2, 10f, 2f));
                        v.Add(new Voltage("+5V", 3, 15f, 10f));
                        v.Add(new Voltage("VCCSA", 5));
                        v.Add(new Voltage("DIMM A/B", 6));
                        v.Add(new Voltage("+3V Standby", 7, 1, 1));
                        v.Add(new Voltage("CMOS Battery", 8, 1, 1));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("VRM MOS", 4));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan", 1));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan", 1));

                        break;

                    case Model.B360_AORUS_GAMING_3_WIFI_CF: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 29.4f, 45.3f));
                        v.Add(new Voltage("+12V", 2, 10f, 2f));
                        v.Add(new Voltage("+5V", 3, 15f, 10f));
                        v.Add(new Voltage("CPU Vcore", 4, 0, 1));
                        v.Add(new Voltage("CPU System Agent", 5, 0, 1));
                        v.Add(new Voltage("DIMM A/B", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 1, 1));
                        v.Add(new Voltage("CMOS Battery", 8, 1, 1));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("Sensor #1", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("PCH", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("PCH Fan", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("PCH Fan", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.X570_AORUS_MASTER: 
                    case Model.X570_AORUS_ULTRA:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 29.4f, 45.3f));
                        v.Add(new Voltage("+12V", 2, 10f, 2f));
                        v.Add(new Voltage("+5V", 3, 15f, 10f));
                        v.Add(new Voltage("Vcore SoC", 4));
                        v.Add(new Voltage("VDDP", 5));
                        v.Add(new Voltage("DIMM A/B", 6));
                        v.Add(new Voltage("+3V Standby", 7, 1f, 10f));
                        v.Add(new Voltage("CMOS Battery", 8, 1f, 10f));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("Sensor #1", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("PCH", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("PCH Fan", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("PCH Fan", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.X570_AORUS_PRO: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 29.4f, 45.3f));
                        v.Add(new Voltage("+12V", 2, 10f, 2f));
                        v.Add(new Voltage("+5V", 3, 15f, 10f));
                        v.Add(new Voltage("Vcore SoC", 4));
                        v.Add(new Voltage("VDDP", 5));
                        v.Add(new Voltage("DIMM A/B", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10f, 10f));
                        v.Add(new Voltage("CMOS Battery", 8, 10f, 10f));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("External #1", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("PCH", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("PCH Fan", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("PCH Fan", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.X570_GAMING_X: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 29.4f, 45.3f));
                        v.Add(new Voltage("+12V", 2, 10f, 2f));
                        v.Add(new Voltage("+5V", 3, 15f, 10f));
                        v.Add(new Voltage("Vcore SoC", 4));
                        v.Add(new Voltage("VDDP", 5));
                        v.Add(new Voltage("DIMM A/B", 6));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("System #2", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("PCH", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("PCH Fan", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("PCH Fan", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.Z390_M_GAMING: 
                    case Model.Z390_AORUS_ULTRA:
                    case Model.Z390_UD:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.49f, 10));
                        v.Add(new Voltage("+12V", 2, 5f, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("CPU VCCGT", 4));
                        v.Add(new Voltage("CPU System Agent", 5));
                        v.Add(new Voltage("VDDQ", 6));
                        v.Add(new Voltage("DDRVTT", 7));
                        v.Add(new Voltage("PCHCore", 8));
                        v.Add(new Voltage("CPU VCCIO", 9));
                        v.Add(new Voltage("DDRVPP", 10));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("System #2", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));

                        break;

                    case Model.Z390_AORUS_PRO:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.49f, 10));
                        v.Add(new Voltage("+12V", 2, 5f, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("CPU VCCGT", 4));
                        v.Add(new Voltage("CPU System Agent", 5));
                        v.Add(new Voltage("DDR", 6));
                        v.Add(new Voltage("Voltage #7", 7, true));
                        v.Add(new Voltage("+3V Standby", 8, 1f, 1f, -0.312f));
                        v.Add(new Voltage("CMOS Battery", 9, 6f, 1f, 0.01f));
                        v.Add(new Voltage("AVCC3", 10, 6f, 1f, 0.048f));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("Sensor #1/System #2", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.Z790_UD: 
                    case Model.Z790_UD_AC: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.49f, 10));
                        v.Add(new Voltage("+12V", 2, 5f, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("iGPU", 4));
                        v.Add(new Voltage("CPU Input Auxiliary", 5));
                        v.Add(new Voltage("Dual DDR5 5V", 6, 1.5f, 1));
                        v.Add(new Voltage("+3V Standby", 7, 1, 1));
                        v.Add(new Voltage("CMOS Battery", 8, 1, 1));
                        v.Add(new Voltage("AVCC3", 9, true));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("System #2", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3 / Pump", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3 / Pump", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));
                        break;

                    case Model.Z790_GAMING_X: 
                    case Model.Z790_GAMING_X_AX: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.49f, 10));
                        v.Add(new Voltage("+12V", 2, 5f, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("iGPU", 4));
                        v.Add(new Voltage("CPU Input Auxiliary", 5));
                        v.Add(new Voltage("Dual DDR5 5V", 6, 1.5f, 1));
                        v.Add(new Voltage("+3V Standby", 7, 1, 1));
                        v.Add(new Voltage("CMOS Battery", 8, 1, 1));
                        v.Add(new Voltage("AVCC3", 9, true));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("System #2", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("System Fan #4 / Pump", 4));
                        f.Add(new Fan("CPU Optional Fan", 5));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));
                        c.Add(new ControlChannel("System Fan #4 / Pump", 4));
                        c.Add(new ControlChannel("CPU Optional Fan", 5));
                        break;

                    case Model.Z790_AORUS_PRO_X: 
                    case Model.Z690_AORUS_PRO:
                    case Model.Z690_AORUS_ULTRA: 
                    case Model.Z690_AORUS_MASTER: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.49f, 10));
                        v.Add(new Voltage("+12V", 2, 5f, 1));
                        v.Add(new Voltage("+5V", 3, 1.5f, 1));
                        v.Add(new Voltage("iGPU VAXG", 4));
                        v.Add(new Voltage("CPU Input Auxiliary", 5));
                        v.Add(new Voltage("Dual DDR5 5V", 6, 1.5f, 1));
                        v.Add(new Voltage("+3V Standby", 7, 1f, 1f));
                        v.Add(new Voltage("CMOS Battery", 8, 1f, 1f));
                        v.Add(new Voltage("AVCC3", 9, true));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("External #1", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3 / Pump", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3 / Pump", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));
                        break;

                    case Model.X870_AORUS_ELITE_WIFI7: 
                    case Model.X870_AORUS_ELITE_WIFI7_ICE: 
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("EC", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));
                        break;

                    case Model.Z690_GAMING_X_DDR4:
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("System #2", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        f.Add(new Fan("System Fan #4 / Pump", 5));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("System Fan #4 / Pump", 5));
                        break;

                    case Model.Z68A_D3H_B3: 
                        v.Add(new Voltage("CPU Termination", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.49f, 10));
                        v.Add(new Voltage("+12V", 2, 30.9f, 10));
                        v.Add(new Voltage("+5V", 3, 7.15f, 10));
                        v.Add(new Voltage("Vcore", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("Power Fan", 2));
                        f.Add(new Fan("System Fan #2", 3));

                        break;

                    case Model.P67A_UD3_B3: 
                    case Model.P67A_UD3R_B3: 
                    case Model.P67A_UD4_B3: 
                    case Model.Z68AP_D3: 
                    case Model.Z68X_UD3H_B3: 
                    case Model.Z68XP_UD3R: 
                        v.Add(new Voltage("CPU Termination", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.49f, 10));
                        v.Add(new Voltage("+12V", 2, 30.9f, 10));
                        v.Add(new Voltage("+5V", 3, 7.15f, 10));
                        v.Add(new Voltage("Vcore", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #2", 1));
                        f.Add(new Fan("Power Fan", 2));
                        f.Add(new Fan("System Fan #1", 3));

                        break;

                    case Model.Z68X_UD7_B3: 
                        v.Add(new Voltage("CPU Termination", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.49f, 10));
                        v.Add(new Voltage("+12V", 2, 30.9f, 10));
                        v.Add(new Voltage("+5V", 3, 7.15f, 10));
                        v.Add(new Voltage("Vcore", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("System #3", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Power Fan", 1));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        f.Add(new Fan("System Fan #3", 4));

                        break;

                    case Model.X79_UD3: 
                        v.Add(new Voltage("CPU Termination", 0));
                        v.Add(new Voltage("DIMM A/B", 1));
                        v.Add(new Voltage("+12V", 2, 10, 2));
                        v.Add(new Voltage("+5V", 3, 15, 10));
                        v.Add(new Voltage("VIN4", 4));
                        v.Add(new Voltage("VCore", 5));
                        v.Add(new Voltage("DIMM C/D", 6));
                        v.Add(new Voltage("+3V Standby", 7, 1, 1));
                        v.Add(new Voltage("CMOS Battery", 8, 1, 1));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Northbridge", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("System Fan #4", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));

                        break;

                    case Model.B550_AORUS_MASTER:
                    case Model.B550_AORUS_PRO:
                    case Model.B550_AORUS_PRO_AC:
                    case Model.B550_AORUS_PRO_AX:
                    case Model.B550_VISION_D:
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("Vcore SoC", 4, 0, 1));
                        v.Add(new Voltage("VDDP", 5, 0, 1));
                        v.Add(new Voltage("DIMM", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("External #1", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("Chipset", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.B550_AORUS_ELITE:
                    case Model.B550_AORUS_ELITE_AX:
                    case Model.B550_GAMING_X:
                    case Model.B550_UD_AC:
                    case Model.B550M_AORUS_PRO:
                    case Model.B550M_AORUS_PRO_AX:
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("Vcore SoC", 4, 0, 1));
                        v.Add(new Voltage("VDDP", 5, 0, 1));
                        v.Add(new Voltage("DIMM", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("System #2", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("Chipset", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.B550I_AORUS_PRO_AX:
                    case Model.B550M_AORUS_ELITE:
                    case Model.B550M_GAMING:
                    case Model.B550M_DS3H:
                    case Model.B550M_DS3H_AC:
                    case Model.B550M_S2H:
                    case Model.B550M_H:
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("Vcore SoC", 4, 0, 1));
                        v.Add(new Voltage("VDDP", 5, 0, 1));
                        v.Add(new Voltage("DIMM", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("VSoC MOS", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("Chipset", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));

                        break;

                    case Model.B660_DS3H_DDR4:
                    case Model.B660_DS3H_AC_DDR4:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 1, 6.5F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("iGPU", 4));
                        v.Add(new Voltage("CPU Input Auxiliary", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("Chipset", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("System #2", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3 / Pump", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3 / Pump", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.B660M_DS3H_AX_DDR4:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("VAXG", 1));
                        v.Add(new Voltage("CPU Input Auxiliary", 2));
                        v.Add(new Voltage("DIMM A/B", 3));
                        v.Add(new Voltage("+12V", 4));
                        v.Add(new Voltage("+3.3V", 5));
                        v.Add(new Voltage("+5V", 6));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("PCIe x16", 2));
                        t.Add(new Temperature("System #1", 3));
                        t.Add(new Temperature("System #2", 4));
                        t.Add(new Temperature("VRAM MOS", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        f.Add(new Fan("System Fan #3", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 2));
                        c.Add(new ControlChannel("System Fan #2", 3));
                        c.Add(new ControlChannel("System Fan #3", 4));

                        break;

                    case Model.X670E_AORUS_XTREME: 
                    case Model.X870E_AORUS_PRO: 
                    case Model.X870E_AORUS_PRO_ICE: 
                    case Model.X870E_AORUS_XTREME_AI_TOP: 
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.49F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("Vcore SoC", 4, 0, 1));
                        v.Add(new Voltage("Vcore Misc", 5, 0, 1));
                        v.Add(new Voltage("CPU VDDIO Memory", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));
                        t.Add(new Temperature("External #1", 5));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    case Model.X670_AORUS_ELITE_AX:
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+3.3V", 1, 6.49F, 10));
                        v.Add(new Voltage("+12V", 2, 5, 1));
                        v.Add(new Voltage("+5V", 3, 1.5F, 1));
                        v.Add(new Voltage("Vcore SoC", 4, 0, 1));
                        v.Add(new Voltage("Vcore Misc", 5, 0, 1));
                        v.Add(new Voltage("CPU VDDIO Memory", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10, 0));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));

                        t.Add(new Temperature("System #1", 0));
                        t.Add(new Temperature("PCH", 1));
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("PCIe x16", 3));
                        t.Add(new Temperature("VRM MOS", 4));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("System Fan #1", 1));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 3));
                        f.Add(new Fan("CPU Optional Fan", 4));

                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("System Fan #1", 1));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 3));
                        c.Add(new ControlChannel("CPU Optional Fan", 4));

                        break;

                    default:
                        v.Add(new Voltage("Voltage #1", 0, true));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("Voltage #3", 2, true));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10, 0, true));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.Biostar:
                switch (model)
                {
                    case Model.B660GTN: 
                        
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("DIMM", 1, 0, 1));
                        v.Add(new Voltage("+12V", 2, 5, 1)); 
                        v.Add(new Voltage("+5V", 3, 147, 100)); 
                        
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10)); 
                        t.Add(new Temperature("System 1", 0));
                        t.Add(new Temperature("System 2", 1)); 
                        t.Add(new Temperature("CPU", 2));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("CPU Optional fan", 2));
                        f.Add(new Fan("System Fan", 4));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("CPU Optional Fan", 2));
                        c.Add(new ControlChannel("System Fan", 4));

                        break;

                    case Model.X670E_Valkyrie: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("+12V", 2, 10, 2));
                        
                        v.Add(new Voltage("Voltage #4", 3, true));
                        
                        v.Add(new Voltage("Voltage #5", 4));
                        v.Add(new Voltage("VDDP", 5));
                        v.Add(new Voltage("VSOC", 6));

                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("VRM", 1));
                        t.Add(new Temperature("System", 2));

                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("CPU Optional Fan", 1));
                        for (int i = 2; i < superIO.Fans.Length; i++)
                            f.Add(new Fan($"System Fan #{i - 1}", i));

                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("CPU Optional Fan", 1));
                        for (int i = 2; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel($"System Fan #{i - 1}", i));

                        break;

                    default:
                        v.Add(new Voltage("Voltage #1", 0, true));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("Voltage #3", 2, true));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10, 0, true));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.Shuttle:
                switch (model)
                {
                    case Model.FH67: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM", 1));
                        v.Add(new Voltage("PCH VCCIO", 2));
                        v.Add(new Voltage("CPU VCCIO", 3));
                        v.Add(new Voltage("Graphics", 4));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("System", 0));
                        t.Add(new Temperature("CPU", 1));
                        f.Add(new Fan("Fan #1", 0));
                        f.Add(new Fan("CPU Fan", 1));

                        break;

                    default:
                        v.Add(new Voltage("Voltage #1", 0, true));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("Voltage #3", 2, true));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10, 0, true));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;

            default:
                v.Add(new Voltage("Voltage #1", 0, true));
                v.Add(new Voltage("Voltage #2", 1, true));
                v.Add(new Voltage("Voltage #3", 2, true));
                v.Add(new Voltage("Voltage #4", 3, true));
                v.Add(new Voltage("Voltage #5", 4, true));
                v.Add(new Voltage("Voltage #6", 5, true));
                v.Add(new Voltage("Voltage #7", 6, true));
                v.Add(new Voltage("+3V Standby", 7, 10, 10, 0, true));
                v.Add(new Voltage("CMOS Battery", 8, 10, 10));

                for (int i = 0; i < superIO.Temperatures.Length; i++)
                    t.Add(new Temperature("Temperature #" + (i + 1), i));

                for (int i = 0; i < superIO.Fans.Length; i++)
                    f.Add(new Fan("Fan #" + (i + 1), i));

                for (int i = 0; i < superIO.Controls.Length; i++)
                    c.Add(new ControlChannel("Fan #" + (i + 1), i));

                break;
        }
    }

    private static void GetIteConfigurationsC(ISuperIO superIO, Manufacturer manufacturer, Model model, IList<Voltage> v, IList<Temperature> t, IList<Fan> f, IList<ControlChannel> c)
    {
        switch (manufacturer)
        {
            case Manufacturer.Gigabyte:
                switch (model)
                {
                    case Model.X570_AORUS_MASTER: 
                    case Model.X570_AORUS_PRO:
                    case Model.X570_AORUS_ULTRA:
                    case Model.B550_AORUS_MASTER:
                    case Model.B550_AORUS_PRO:
                    case Model.B550_AORUS_PRO_AC:
                    case Model.B550_AORUS_PRO_AX:
                    case Model.B550_VISION_D:
                        v.Add(new Voltage("VIN0", 0));
                        v.Add(new Voltage("DDRVTT AB", 1));
                        v.Add(new Voltage("Chipset Core", 2));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("CPU VDD18", 4));
                        v.Add(new Voltage("PM_CLDO12", 5));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 1f, 1f));
                        v.Add(new Voltage("CMOS Battery", 8, 1f, 1f));
                        t.Add(new Temperature("PCIe x8", 0));
                        t.Add(new Temperature("External #2", 1));
                        t.Add(new Temperature("System #2", 2));
                        f.Add(new Fan("System Fan #5 / Pump", 0));
                        f.Add(new Fan("System Fan #6 / Pump", 1));
                        f.Add(new Fan("System Fan #4", 2));
                        c.Add(new ControlChannel("System Fan #5 / Pump", 0));
                        c.Add(new ControlChannel("System Fan #6 / Pump", 1));
                        c.Add(new ControlChannel("System Fan #4", 2));

                        break;

                    case Model.X470_AORUS_GAMING_7_WIFI: 
                        v.Add(new Voltage("VIN0", 0, 0, 1));
                        v.Add(new Voltage("DDR VTT", 1, 0, 1));
                        v.Add(new Voltage("Chipset Core", 2, 0, 1));
                        v.Add(new Voltage("VIN3", 3, 0, 1));
                        v.Add(new Voltage("CPU VDD18", 4, 0, 1));
                        v.Add(new Voltage("Chipset Core +2.5V", 5, 0.5F, 1));
                        v.Add(new Voltage("+3V Standby", 6, 1, 10));
                        v.Add(new Voltage("CMOS Battery", 7, 0.7F, 1));
                        t.Add(new Temperature("PCIe x8", 0));
                        t.Add(new Temperature("System #2", 2));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.Z390_AORUS_PRO: 
                        v.Add(new Voltage("VCore", 0));
                        v.Add(new Voltage("DDRVTT AB", 1));
                        v.Add(new Voltage("Chipset Core", 2));
                        v.Add(new Voltage("VIN3", 3, true));
                        v.Add(new Voltage("VCCIO", 4));
                        v.Add(new Voltage("Voltage #7", 5, true));
                        v.Add(new Voltage("DDR VPP", 6));
                        v.Add(new Voltage("+3V Standby", 7, 1f, 1f));
                        v.Add(new Voltage("CMOS Battery", 8, 1f, 1f));
                        t.Add(new Temperature("PCIe x8", 0));
                        t.Add(new Temperature("External #2", 1));
                        t.Add(new Temperature("System #2", 2));
                        f.Add(new Fan("System Fan #5 / Pump", 0));
                        f.Add(new Fan("System Fan #6 / Pump", 1));
                        f.Add(new Fan("System Fan #4", 2));
                        c.Add(new ControlChannel("System Fan #5 / Pump", 0));
                        c.Add(new ControlChannel("System Fan #6 / Pump", 1));
                        c.Add(new ControlChannel("System Fan #4", 2));

                        break;

                    case Model.Z790_AORUS_PRO_X: 
                    case Model.Z690_AORUS_PRO:
                    case Model.Z690_AORUS_MASTER: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM I/O", 1));
                        v.Add(new Voltage("Chipset +0.82V", 2));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("CPU System Agent", 4));
                        v.Add(new Voltage("Chipset +1.8V", 5));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("PCIe x4", 0));
                        t.Add(new Temperature("External #2", 1));
                        t.Add(new Temperature("System #2", 2));
                        f.Add(new Fan("System Fan #5 / Pump", 0));
                        f.Add(new Fan("System Fan #6 / Pump", 1));
                        f.Add(new Fan("System Fan #4", 2));
                        c.Add(new ControlChannel("System Fan #5 / Pump", 0));
                        c.Add(new ControlChannel("System Fan #6 / Pump", 1));
                        c.Add(new ControlChannel("System Fan #4", 2));
                        break;

                    case Model.X870E_AORUS_PRO:
                    case Model.X870E_AORUS_PRO_ICE: 
                    case Model.X870E_AORUS_XTREME_AI_TOP: 
                        v.Add(new Voltage("VIN0", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("PM_VCC18", 2));
                        v.Add(new Voltage("VIN3", 3));
                        v.Add(new Voltage("CPU VDD18", 4));
                        v.Add(new Voltage("PM_VDD1V", 5));
                        v.Add(new Voltage("VIN6", 6));
                        v.Add(new Voltage("+3V Standby", 7, 1, 1));
                        v.Add(new Voltage("CMOS Battery", 8, 1, 1));
                        t.Add(new Temperature("PCIe x4", 0));
                        t.Add(new Temperature("External #2", 1));
                        t.Add(new Temperature("System #2", 2));
                        f.Add(new Fan("System Fan #5 / Pump", 0));
                        f.Add(new Fan("System Fan #6 / Pump", 1));
                        f.Add(new Fan("System Fan #4 ", 2));
                        c.Add(new ControlChannel("System Fan #5 / Pump", 0));
                        c.Add(new ControlChannel("System Fan #6 / Pump", 1));
                        c.Add(new ControlChannel("System Fan #4", 2));
                        break;

                    case Model.X870_AORUS_ELITE_WIFI7: 
                    case Model.X870_AORUS_ELITE_WIFI7_ICE: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("DIMM I/O", 1));
                        v.Add(new Voltage("Chipset +0.82V", 2));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("CPU System Agent", 4));
                        v.Add(new Voltage("Chipset +1.8V", 5));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("PCIe x4", 0));
                        t.Add(new Temperature("External #2", 1));
                        t.Add(new Temperature("System #2", 2));
                        f.Add(new Fan("System Fan #5 / Pump", 0));
                        f.Add(new Fan("System Fan #6 / Pump", 1));
                        f.Add(new Fan("System Fan #4", 2));
                        c.Add(new ControlChannel("System Fan #5 / Pump", 0));
                        c.Add(new ControlChannel("System Fan #6 / Pump", 1));
                        c.Add(new ControlChannel("System Fan #4", 2));
                        break;

                    default:
                        v.Add(new Voltage("Voltage #1", 0, true));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("Voltage #3", 2, true));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10, 0, true));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;

            default:
                v.Add(new Voltage("Voltage #1", 0, true));
                v.Add(new Voltage("Voltage #2", 1, true));
                v.Add(new Voltage("Voltage #3", 2, true));
                v.Add(new Voltage("Voltage #4", 3, true));
                v.Add(new Voltage("Voltage #5", 4, true));
                v.Add(new Voltage("Voltage #6", 5, true));
                v.Add(new Voltage("Voltage #7", 6, true));
                v.Add(new Voltage("+3V Standby", 7, 10, 10, 0, true));
                v.Add(new Voltage("CMOS Battery", 8, 10, 10));

                for (int i = 0; i < superIO.Temperatures.Length; i++)
                    t.Add(new Temperature("Temperature #" + (i + 1), i));

                for (int i = 0; i < superIO.Fans.Length; i++)
                    f.Add(new Fan("Fan #" + (i + 1), i));

                for (int i = 0; i < superIO.Controls.Length; i++)
                    c.Add(new ControlChannel("Fan #" + (i + 1), i));

                break;
        }
    }

    private static void GetFintekConfiguration(ISuperIO superIO, Manufacturer manufacturer, Model model, IList<Voltage> v, IList<Temperature> t, IList<Fan> f, IList<ControlChannel> c)
    {
        switch (manufacturer)
        {
            case Manufacturer.EVGA:
                switch (model)
                {
                    case Model.X58_SLI_Classified: 
                        v.Add(new Voltage("VCC3V", 0, 150, 150));
                        v.Add(new Voltage("Vcore", 1, 47, 100));
                        v.Add(new Voltage("DIMM", 2, 47, 100));
                        v.Add(new Voltage("CPU Termination", 3, 24, 100));
                        v.Add(new Voltage("IOH Vcore", 4, 24, 100));
                        v.Add(new Voltage("+5V", 5, 51, 12));
                        v.Add(new Voltage("+12V", 6, 56, 6.8f));
                        v.Add(new Voltage("+3V Standby", 7, 150, 150));
                        v.Add(new Voltage("CMOS Battery", 8, 150, 150));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("VREG", 1));
                        t.Add(new Temperature("System", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Power Fan", 1));
                        f.Add(new Fan("Chassis Fan", 2));

                        break;

                    case Model.X58_3X_SLI: 
                        v.Add(new Voltage("VCC3V", 0, 150, 150));
                        v.Add(new Voltage("Vcore", 1, 47, 100));
                        v.Add(new Voltage("DIMM", 2, 47, 100));
                        v.Add(new Voltage("CPU Termination", 3, 24, 100));
                        v.Add(new Voltage("IOH Vcore", 4, 24, 100));
                        v.Add(new Voltage("+5V", 5, 51, 12));
                        v.Add(new Voltage("+12V", 6, 56, 6.8f));
                        v.Add(new Voltage("+3V Standby", 7, 150, 150));
                        v.Add(new Voltage("CMOS Battery", 8, 150, 150));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("VREG", 1));
                        t.Add(new Temperature("System", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Power Fan", 1));
                        f.Add(new Fan("Chassis Fan", 2));
                        f.Add(new Fan("Chipset Fan", 3));
                        c.Add(new ControlChannel("CPU Fan", 0));
                        c.Add(new ControlChannel("Power Fan", 1));
                        c.Add(new ControlChannel("Chassis Fan", 2));
                        c.Add(new ControlChannel("Chipset Fan", 3));

                        break;

                    default:
                        v.Add(new Voltage("VCC3V", 0, 150, 150));
                        v.Add(new Voltage("Vcore", 1));
                        v.Add(new Voltage("Voltage #3", 2, true));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("VSB3V", 7, 150, 150));
                        v.Add(new Voltage("CMOS Battery", 8, 150, 150));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.MSI:
                switch (model)
                {
                    case Model.Z77_MS7751: 
                    case Model.Z68_MS7672: 
                        v.Add(new Voltage("VCC3V", 0, 150, 150));
                        v.Add(new Voltage("Vcore", 1));
                        v.Add(new Voltage("iGPU", 2));
                        v.Add(new Voltage("+5V", 3, 20, 4.7f));
                        v.Add(new Voltage("+12V", 4, 68, 6.8f));
                        v.Add(new Voltage("DIMM", 5, 150, 150));
                        v.Add(new Voltage("CPU I/O", 6));
                        v.Add(new Voltage("+3.3V", 7, 150, 150));
                        v.Add(new Voltage("CMOS Battery", 8, 150, 150));

                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Probe", 1));
                        t.Add(new Temperature("System", 2));

                        f.Add(new Fan("CPU Fan", 0));
                        for (int i = 1; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("System Fan #" + i, i));

                        c.Add(new ControlChannel("CPU Fan", 0));
                        for (int i = 1; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("System Fan #" + i, i));

                        break;
                    default:
                        v.Add(new Voltage("VCC3V", 0, 150, 150));
                        v.Add(new Voltage("Vcore", 1));
                        v.Add(new Voltage("Voltage #3", 2, true));
                        v.Add(new Voltage("Voltage #4", 3, true));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));

                        if (superIO.Chip != Chip.F71808E)
                            v.Add(new Voltage("Voltage #7", 6, true));

                        v.Add(new Voltage("VSB3V", 7, 150, 150));
                        v.Add(new Voltage("CMOS Battery", 8, 150, 150));

                        for (int i = 0; i < superIO.Temperatures.Length; i++)
                            t.Add(new Temperature("Temperature #" + (i + 1), i));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan Control #" + (i + 1), i));

                        break;
                }

                break;

            default:
                v.Add(new Voltage("VCC3V", 0, 150, 150));
                v.Add(new Voltage("Vcore", 1));
                v.Add(new Voltage("Voltage #3", 2, true));
                v.Add(new Voltage("Voltage #4", 3, true));
                v.Add(new Voltage("Voltage #5", 4, true));
                v.Add(new Voltage("Voltage #6", 5, true));
                if (superIO.Chip != Chip.F71808E)
                    v.Add(new Voltage("Voltage #7", 6, true));

                v.Add(new Voltage("VSB3V", 7, 150, 150));
                v.Add(new Voltage("CMOS Battery", 8, 150, 150));

                for (int i = 0; i < superIO.Temperatures.Length; i++)
                    t.Add(new Temperature("Temperature #" + (i + 1), i));

                for (int i = 0; i < superIO.Fans.Length; i++)
                    f.Add(new Fan("Fan #" + (i + 1), i));

                for (int i = 0; i < superIO.Controls.Length; i++)
                    c.Add(new ControlChannel("Fan #" + (i + 1), i));

                break;
        }
    }

    private static void GetNuvotonConfigurationF(ISuperIO superIO, Manufacturer manufacturer, Model model, IList<Voltage> v, IList<Temperature> t, IList<Fan> f, IList<ControlChannel> c)
    {
        switch (manufacturer)
        {
            case Manufacturer.ASUS:
                switch (model)
                {
                    case Model.P8P67: 
                    case Model.P8P67_EVO: 
                    case Model.P8P67_PRO: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+12V", 1, 11, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+5V", 4, 12, 3));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Auxiliary", 2));
                        t.Add(new Temperature("Motherboard", 3));
                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Power Fan", 2));
                        f.Add(new Fan("Chassis Fan #2", 3));
                        c.Add(new ControlChannel("Chassis Fan #2", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Chassis Fan #1", 2));

                        break;

                    case Model.P8P67_M_PRO: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+12V", 1, 11, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+5V", 4, 12, 3));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 3));
                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Chassis Fan #2", 2));
                        f.Add(new Fan("Power Fan", 3));
                        f.Add(new Fan("Auxiliary Fan", 4));

                        break;

                    case Model.P8Z68_V_PRO: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+12V", 1, 11, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+5V", 4, 12, 3));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Auxiliary", 2));
                        t.Add(new Temperature("Motherboard", 3));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.P9X79: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+12V", 1, 11, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+5V", 4, 12, 3));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 3));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Temperature #1", 1));
                        t.Add(new Temperature("Temperature #2", 2));
                        t.Add(new Temperature("Temperature #3", 3));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.ASRock:
                switch (model)
                {
                    case Model.H61M_DGS: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+12V", 1, 28, 5));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("#Voltage #5", 4, 0, 1, 0, true));
                        v.Add(new Voltage("+5V", 5, 2, 1));
                        v.Add(new Voltage("#Voltage #7", 6, 0, 1, 0, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 1));
                        
                        t.Add(new Temperature("Motherboard", 3));
                        f.Add(new Fan("Chassis Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Power Fan", 2));
                        c.Add(new ControlChannel("Chassis Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        
                        break;
                    case Model.B85M_DGS:
                        {
                            v.Add(new Voltage("Vcore", 0, 1, 1));
                            v.Add(new Voltage("+12V", 1, 56, 10));
                            v.Add(new Voltage("AVCC", 2, 34, 34));
                            v.Add(new Voltage("+3.3V", 3, 34, 34));
                            v.Add(new Voltage("VIN1", 4, true));
                            v.Add(new Voltage("+5V", 5, 12, 3));
                            v.Add(new Voltage("VIN3", 6, true));
                            v.Add(new Voltage("+3V Standby", 7, 34, 34));
                            t.Add(new Temperature("CPU", 0));
                            t.Add(new Temperature("Auxiliary", 2));
                            t.Add(new Temperature("Motherboard", 3));
                            f.Add(new Fan("Chassis Fan #1", 0));
                            f.Add(new Fan("CPU Fan", 1));
                            f.Add(new Fan("Power Fan", 2));
                            f.Add(new Fan("Chassis Fan #2", 3));
                            c.Add(new ControlChannel("Chassis Fan #2", 0));
                            c.Add(new ControlChannel("CPU Fan", 1));
                            c.Add(new ControlChannel("Chassis Fan #1", 2));
                        }

                        break;
                    case Model.Z77Pro4M: 
                        v.Add(new Voltage("Vcore", 0, 0, 1));
                        v.Add(new Voltage("+12V", 1, 56, 10));
                        v.Add(new Voltage("AVCC", 2, 10, 10));
                        v.Add(new Voltage("+3.3V", 3, 10, 10));
                        
                        v.Add(new Voltage("+5V", 5, 20, 10));
                        
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Auxiliary", 2));
                        t.Add(new Temperature("Motherboard", 3));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Temperature #1", 1));
                        t.Add(new Temperature("Temperature #2", 2));
                        t.Add(new Temperature("Temperature #3", 3));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;

            default:
                v.Add(new Voltage("Vcore", 0));
                v.Add(new Voltage("Voltage #2", 1, true));
                v.Add(new Voltage("AVCC", 2, 34, 34));
                v.Add(new Voltage("+3.3V", 3, 34, 34));
                v.Add(new Voltage("Voltage #5", 4, true));
                v.Add(new Voltage("Voltage #6", 5, true));
                v.Add(new Voltage("Voltage #7", 6, true));
                v.Add(new Voltage("+3V Standby", 7, 34, 34));
                v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                t.Add(new Temperature("CPU Core", 0));
                t.Add(new Temperature("Temperature #1", 1));
                t.Add(new Temperature("Temperature #2", 2));
                t.Add(new Temperature("Temperature #3", 3));

                for (int i = 0; i < superIO.Fans.Length; i++)
                    f.Add(new Fan("Fan #" + (i + 1), i));

                for (int i = 0; i < superIO.Controls.Length; i++)
                    c.Add(new ControlChannel("Fan #" + (i + 1), i));

                break;
        }
    }

    private static void GetNuvotonConfigurationD(ISuperIO superIO, Manufacturer manufacturer, Model model, int index, IList<Voltage> v, IList<Temperature> t, IList<Fan> f, IList<ControlChannel> c)
    {
        switch (manufacturer)
        {
            case Manufacturer.ASRock:
                switch (model)
                {
                    case Model.A320M_HDV: 
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        v.Add(new Voltage("Chipset 1.05V", 1, 0, 1));
                        v.Add(new Voltage("AVCC", 2, 10, 10));
                        v.Add(new Voltage("+3.3V", 3, 10, 10));
                        v.Add(new Voltage("+12V", 4, 56, 10));
                        v.Add(new Voltage("VcoreRef", 5, 0, 1));
                        v.Add(new Voltage("DIMM", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        
                        v.Add(new Voltage("+5V", 12, 20, 10));
                        
                        t.Add(new Temperature("Motherboard", 2));
                        
                        t.Add(new Temperature("Auxiliary", 5));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.AB350_Pro4: 
                    case Model.AB350M_Pro4:
                    case Model.AB350M:
                    case Model.Fatal1ty_AB350_Gaming_K4:
                    case Model.AB350M_HDV:
                    case Model.B450_Steel_Legend:
                    case Model.B450M_Steel_Legend:
                    case Model.B450_Pro4:
                    case Model.B450M_Pro4:
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        
                        v.Add(new Voltage("AVCC", 2, 10, 10));
                        v.Add(new Voltage("+3.3V", 3, 10, 10));
                        v.Add(new Voltage("+12V", 4, 28, 5));
                        v.Add(new Voltage("Vcore Refin", 5, 0, 1));
                        
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        
                        v.Add(new Voltage("Chipset 1.05V", 11, 0, 1));
                        v.Add(new Voltage("+5V", 12, 20, 10));
                        
                        v.Add(new Voltage("+1.8V", 14, 0, 1));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Auxiliary", 3));
                        t.Add(new Temperature("VRM", 4));
                        t.Add(new Temperature("Auxiliary Index #2", 5));
                        
                        f.Add(new Fan("CPU Fan", 1));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        f.Add(new Fan("CPU Pump", 2));
                        c.Add(new ControlChannel("CPU Pump", 2));

                        f.Add(new Fan("Chassis #1", 0));
                        c.Add(new ControlChannel("Chassis #1", 0));
                        f.Add(new Fan("Chassis #2", 3));
                        c.Add(new ControlChannel("Chassis #2", 3));
                        f.Add(new Fan("Chassis #3", 4));
                        c.Add(new ControlChannel("Chassis #3", 4));

                        break;

                    case Model.B450M_Pro4_R2_0:
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        
                        v.Add(new Voltage("AVCC", 2, 10, 10));
                        v.Add(new Voltage("+3.3V", 3, 10, 10));
                        v.Add(new Voltage("+12V", 4, 28, 5));
                        v.Add(new Voltage("Vcore Refin", 5, 0, 1));
                        
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        
                        v.Add(new Voltage("Chipset 1.05V", 11, 0, 1));
                        v.Add(new Voltage("+5V", 12, 20, 10));
                        
                        v.Add(new Voltage("+1.8V", 14, 0, 1));
                        
                        t.Add(new Temperature("Motherboard", 2));
                        
                        f.Add(new Fan("Chassis #1", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Chassis #2", 2));
                        f.Add(new Fan("CPU Pump", 3));
                        f.Add(new Fan("Chassis #3", 4));
                        c.Add(new ControlChannel("Chassis #1", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Chassis #2", 2));
                        c.Add(new ControlChannel("CPU Pump", 3));
                        c.Add(new ControlChannel("Chassis #3", 4));

                        break;

                    case Model.B550M_Pro4: 
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        v.Add(new Voltage("+5V", 1, 2, 1));
                        v.Add(new Voltage("AVCC", 2, 10, 10));
                        v.Add(new Voltage("+3.3V", 3, 10, 10));
                        v.Add(new Voltage("+12V", 4, 56, 10));
                        v.Add(new Voltage("SoC", 5, 0, 1));
                        v.Add(new Voltage("DIMM", 6, 0, 1, 0));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("VPPM", 11, 3, 1));
                        v.Add(new Voltage("+1.8V", 14, 1, 1));

                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("CPU", 8));

                        f.Add(new Fan("Chassis #3", 0));
                        c.Add(new ControlChannel("Chassis #3", 0));
                        f.Add(new Fan("CPU1", 1));
                        c.Add(new ControlChannel("CPU1", 1));
                        f.Add(new Fan("CPU Pump", 2));
                        c.Add(new ControlChannel("CPU Pump", 2));
                        f.Add(new Fan("Chassis #1", 3));
                        c.Add(new ControlChannel("Chassis #1", 3));
                        f.Add(new Fan("Chassis #2", 4));
                        c.Add(new ControlChannel("Chassis #2", 4));
                        f.Add(new Fan("Chassis #4", 6));
                        c.Add(new ControlChannel("Chassis #4", 6));

                        break;

                    case Model.X399_Phantom_Gaming_6: 
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        v.Add(new Voltage("Chipset 1.05V", 1, 0, 1));
                        v.Add(new Voltage("AVCC", 2, 10, 10));
                        v.Add(new Voltage("+3.3V", 3, 10, 10));
                        v.Add(new Voltage("+12V", 4, 56, 10));
                        v.Add(new Voltage("VDDCR_SOC", 5, 0, 1));
                        v.Add(new Voltage("DIMM", 6, 0, 1));
                        v.Add(new Voltage("+3V Standby", 7, 10, 10));
                        v.Add(new Voltage("CMOS Battery", 8, 10, 10));
                        
                        v.Add(new Voltage("+5V", 12, 20, 10));
                        v.Add(new Voltage("+1.8V", 13, 10, 10));
                        
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Motherboard", 1));
                        t.Add(new Temperature("Auxiliary", 2));
                        t.Add(new Temperature("Chipset", 3));
                        t.Add(new Temperature("Core VRM", 4));
                        t.Add(new Temperature("Core SoC", 5));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.X570_Taichi:
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));

                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("CPU", 8));
                        t.Add(new Temperature("Southbridge", 9));

                        f.Add(new Fan("Chassis #3", 0));
                        f.Add(new Fan("CPU #1", 1));
                        f.Add(new Fan("CPU #2", 2));
                        f.Add(new Fan("Chassis #1", 3));
                        f.Add(new Fan("Chassis #2", 4));
                        f.Add(new Fan("Southbridge Fan", 5));
                        f.Add(new Fan("Chassis #4", 6));

                        c.Add(new ControlChannel("Chassis #3", 0));
                        c.Add(new ControlChannel("CPU #1", 1));
                        c.Add(new ControlChannel("CPU #2", 2));
                        c.Add(new ControlChannel("Chassis #1", 3));
                        c.Add(new ControlChannel("Chassis #2", 4));
                        c.Add(new ControlChannel("Southbridge Fan", 5));
                        c.Add(new ControlChannel("Chassis #4", 6));

                        break;

                    case Model.X570_Phantom_Gaming_ITX:
                        v.Add(new Voltage("+12V", 0));
                        v.Add(new Voltage("+5V", 1));
                        v.Add(new Voltage("Vcore", 2));
                        v.Add(new Voltage("Voltage #1", 3));
                        v.Add(new Voltage("DIMM", 4));
                        v.Add(new Voltage("CPU I/O", 5));
                        v.Add(new Voltage("CPU System Agent", 6));
                        v.Add(new Voltage("Voltage #2", 7));
                        v.Add(new Voltage("AVCC3", 8));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("VRef", 10));
                        v.Add(new Voltage("VSB", 11));
                        v.Add(new Voltage("AVSB", 12));
                        v.Add(new Voltage("CMOS Battery", 13));

                        t.Add(new Temperature("Motherboard", 0));
                        
                        t.Add(new Temperature("CPU", 2));
                        t.Add(new Temperature("Southbridge", 3));
                        f.Add(new Fan("CPU Fan #1", 0)); 
                        f.Add(new Fan("Chassis Fan #1", 1)); 
                        f.Add(new Fan("CPU Fan #2", 2)); 
                        f.Add(new Fan("Chipset Fan", 3));

                        c.Add(new ControlChannel("CPU Fan #1", 0));
                        c.Add(new ControlChannel("Chassis Fan", 1));
                        c.Add(new ControlChannel("CPU Fan #2", 2));
                        c.Add(new ControlChannel("Chipset Fan", 3));
                        break;

                    case Model.Z690_Extreme:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 20, 10));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 110, 10));
                        v.Add(new Voltage("CPU Input Auxiliary", 5, 1, 1));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3.3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("CPU 1.05V", 10, 1, 1));
                        v.Add(new Voltage("Chipset 0.82V", 11, 1, 1));
                        v.Add(new Voltage("Chipset 1.0V", 12));
                        v.Add(new Voltage("CPU System Agent", 13, 1, 1));
                        v.Add(new Voltage("+5V Standby", 14, 2.35f, 1));

                        f.Add(new Fan("CPU Fan #1", 1));
                        f.Add(new Fan("CPU Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #1", 3));
                        f.Add(new Fan("Chassis Fan #2", 4));
                        f.Add(new Fan("Chassis Fan #3", 0));
                        f.Add(new Fan("Chassis Fan #4", 5));
                        f.Add(new Fan("Chassis Fan #5", 6));

                        c.Add(new ControlChannel("CPU Fan #1", 1)); 
                        c.Add(new ControlChannel("CPU Fan #2", 2)); 
                        c.Add(new ControlChannel("Chassis Fan #1", 3)); 
                        c.Add(new ControlChannel("Chassis Fan #2", 4)); 
                        c.Add(new ControlChannel("Chassis Fan #3", 0)); 
                        c.Add(new ControlChannel("Chassis Fan #4", 5)); 
                        c.Add(new ControlChannel("Chassis Fan #5", 6)); 

                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Motherboard", 2));
                        break;

                    case Model.Z690_Steel_Legend:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 20, 10));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 110, 10));
                        v.Add(new Voltage("CPU Input Auxiliary", 5, 1, 1));
                        v.Add(new Voltage("DRAM", 6));
                        v.Add(new Voltage("+3.3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("CPU 1.05V", 10, 1, 1));
                        v.Add(new Voltage("Chipset 0.82V", 11, 1, 1));
                        v.Add(new Voltage("Chipset 1.0V", 12));
                        v.Add(new Voltage("CPU System Agent", 13, 1, 1));
                        v.Add(new Voltage("+5V Standby", 14, 2.35f, 1));

                        f.Add(new Fan("CPU Fan #1", 1));
                        f.Add(new Fan("CPU Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #1", 3));
                        f.Add(new Fan("Chassis Fan #2", 4));
                        f.Add(new Fan("Chassis Fan #3", 0));
                        f.Add(new Fan("Chassis Fan #4", 5));
                        f.Add(new Fan("Chassis Fan #5", 6));

                        c.Add(new ControlChannel("CPU Fan #1", 1)); 
                        c.Add(new ControlChannel("CPU Fan #2", 2)); 
                        c.Add(new ControlChannel("Chassis Fan #1", 3)); 
                        c.Add(new ControlChannel("Chassis Fan #2", 4)); 
                        c.Add(new ControlChannel("Chassis Fan #3", 0)); 
                        c.Add(new ControlChannel("Chassis Fan #4", 5)); 
                        c.Add(new ControlChannel("Chassis Fan #5", 6)); 

                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("VRM", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        break;

                    case Model.Z790_Pro_RS:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 20, 10));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 110, 10));
                        v.Add(new Voltage("CPU Input Auxiliary", 5, 1, 1));
                        v.Add(new Voltage("Integrated Memory Controller", 6));
                        v.Add(new Voltage("+3.3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("CPU 1.05V", 10, 1, 1));
                        v.Add(new Voltage("Chipset 0.82V", 11, 1, 1));
                        v.Add(new Voltage("Chipset 1.0V", 12));
                        v.Add(new Voltage("CPU System Agent", 13, 1, 1));
                        v.Add(new Voltage("+5V Standby", 14, 2.35f, 1));

                        f.Add(new Fan("CPU Fan #1", 1));
                        f.Add(new Fan("CPU Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #1", 3));
                        f.Add(new Fan("Chassis Fan #2", 4));
                        f.Add(new Fan("Chassis Fan #3", 0));
                        f.Add(new Fan("Chassis Fan #4", 5));
                        f.Add(new Fan("Chassis Fan #5", 6));

                        c.Add(new ControlChannel("CPU Fan #1", 1)); 
                        c.Add(new ControlChannel("CPU Fan #2", 2)); 
                        c.Add(new ControlChannel("Chassis Fan #1", 3)); 
                        c.Add(new ControlChannel("Chassis Fan #2", 4)); 
                        c.Add(new ControlChannel("Chassis Fan #3", 0)); 
                        c.Add(new ControlChannel("Chassis Fan #4", 5)); 
                        c.Add(new ControlChannel("Chassis Fan #5", 6)); 

                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Motherboard", 2));
                        break;

                    case Model.X570_Phantom_Gaming_4: 
                        
                        v.Add(new Voltage("Vcore", 0, 1, 1));
                        v.Add(new Voltage("+5V", 1, 2, 1));
                        v.Add(new Voltage("AVCC", 2, 1, 1));
                        v.Add(new Voltage("+3.3V", 3, 1, 1));
                        v.Add(new Voltage("+12V", 4, 56, 10));
                        v.Add(new Voltage("CPU SoC", 5));
                        v.Add(new Voltage("DIMM", 6));
                        v.Add(new Voltage("+3V Standby", 7, 1, 1));
                        v.Add(new Voltage("CMOS Battery", 8, 1, 1));
                        v.Add(new Voltage("DIMM Termination", 9));
                        
                        v.Add(new Voltage("VPPM", 11, 3, 1));
                        v.Add(new Voltage("PREM CPU SoC", 12));
                        v.Add(new Voltage("DIMM Write", 13));
                        v.Add(new Voltage("+1.8V", 14, 1, 1));
                        
                        t.Add(new Temperature("CPU", 8)); 
                        t.Add(new Temperature("Chipset", 9)); 
                        t.Add(new Temperature("Motherboard", 2)); 

                        f.Add(new Fan("Chassis Fan #3", 0));
                        c.Add(new ControlChannel("Chassis Fan #3", 0));

                        f.Add(new Fan("CPU Fan #1", 1));
                        c.Add(new ControlChannel("CPU Fan #1", 1));

                        f.Add(new Fan("CPU Fan #2", 2));
                        c.Add(new ControlChannel("CPU Fan #2", 2));

                        f.Add(new Fan("Chassis Fan #1", 3));
                        c.Add(new ControlChannel("Chassis Fan #1", 3));

                        f.Add(new Fan("Chassis Fan #2", 4));
                        c.Add(new ControlChannel("Chassis Fan #2", 4));

                        f.Add(new Fan("Chipset Fan", 5));
                        c.Add(new ControlChannel("Chipset Fan", 5));

                        break;

                    case Model.Z790_Taichi:
                        v.Add(new Voltage("+1.8V", 0));
                        v.Add(new Voltage("Chipset 0.82V", 1));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("CPU 1.05V", 4));
                        v.Add(new Voltage("Chipset 1.05V", 12, 5, 100));

                        f.Add(new Fan("Chassis Fan #5", 0));
                        f.Add(new Fan("Chassis Fan #6", 1));
                        f.Add(new Fan("Chassis Fan #3", 6));

                        c.Add(new ControlChannel("Chassis Fan #5", 0));
                        c.Add(new ControlChannel("Chassis Fan #6", 1));
                        c.Add(new ControlChannel("Chassis Fan #3", 6));
                        break;

                    case Model.Z790_Nova_WiFi:
                        if (index != 0)
                        {
                            
                            v.Add(new Voltage("1.05V CPU", 0));
                            v.Add(new Voltage("CPU I/O", 1));
                            v.Add(new Voltage("0.82V Chipset", 4));
                            v.Add(new Voltage("1.05V Chipset", 12, 5, 100));

                            f.Add(new Fan("VRM", 0));
                            f.Add(new Fan("MOS", 1));

                            c.Add(new ControlChannel("VRM", 0));
                            c.Add(new ControlChannel("MOS", 1));
                        }
                        else
                        {
                            v.Add(new Voltage("Vcore", 0));
                            v.Add(new Voltage("+5V", 1, 20, 10));
                            v.Add(new Voltage("+3.3V", 3, 34, 34));
                            v.Add(new Voltage("+12V", 4, 110, 10));
                            v.Add(new Voltage("CPU Input Auxiliary", 5, 1, 1));
                            v.Add(new Voltage("CPU System Agent", 13, 1, 1));
                            v.Add(new Voltage("+5V Standby", 14, 235, 100));

                            f.Add(new Fan("Chassis #3", 0));
                            f.Add(new Fan("CPU #1", 1));
                            f.Add(new Fan("CPU #2", 2));
                            f.Add(new Fan("Chassis #1", 3));
                            f.Add(new Fan("Chassis #2", 4));
                            f.Add(new Fan("Chassis #4", 5));
                            f.Add(new Fan("Chassis #5", 6));

                            c.Add(new ControlChannel("Chassis #3", 0));
                            c.Add(new ControlChannel("CPU #1", 1));
                            c.Add(new ControlChannel("CPU #2", 2));
                            c.Add(new ControlChannel("Chassis #1", 3));
                            c.Add(new ControlChannel("Chassis #2", 4));
                            c.Add(new ControlChannel("Chassis #4", 5));
                            c.Add(new ControlChannel("Chassis #5", 6));

                            t.Add(new Temperature("CPU Core", 0));
                            t.Add(new Temperature("Motherboard", 2));
                            t.Add(new Temperature("External #1", 3));
                            t.Add(new Temperature("External #2", 4));
                            t.Add(new Temperature("External #3", 5));
                        }

                        break;

                    case Model.B650M_C: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+12V", 1, 56, 10));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+5V", 4, 20, 10));
                        v.Add(new Voltage("+1.05V ALW", 5));
                        v.Add(new Voltage("+3.3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("Vcore SoC", 10, 1, 1));
                        v.Add(new Voltage("Vcore Misc", 11, 1, 1));
                        v.Add(new Voltage("+1.8V", 13, 1, 1));
                        v.Add(new Voltage("DIMM", 14));

                        t.Add(new Temperature("CPU Core", 9));
                        t.Add(new Temperature("Motherboard", 2));

                        f.Add(new Fan("CPU Fan #1", 1)); 
                        f.Add(new Fan("CPU Fan #2", 0)); 
                        f.Add(new Fan("Chassis Fan #1", 3)); 
                        f.Add(new Fan("Chassis Fan #2", 4)); 
                        f.Add(new Fan("Chassis Fan #3", 6)); 

                        c.Add(new ControlChannel("CPU Fan #1", 1)); 
                        c.Add(new ControlChannel("CPU Fan #2", 0)); 
                        c.Add(new ControlChannel("Chassis Fan #1", 3)); 
                        c.Add(new ControlChannel("Chassis Fan #2", 4)); 
                        c.Add(new ControlChannel("Chassis Fan #3", 6)); 
                        break;

                    case Model.B850M_STEEL_LEGEND_WIFI: 
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));

                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("PCH TS10", 9));
                        t.Add(new Temperature("Thermistor Sensor", 24));

                        f.Add(new Fan("CPU Fan #1", 1)); 
                        f.Add(new Fan("CPU Fan #2 / Pump", 0)); 
                        f.Add(new Fan("Chassis Fan #1", 3)); 
                        f.Add(new Fan("Chassis Fan #2", 4)); 
                        f.Add(new Fan("Chassis Fan #3", 6)); 

                        c.Add(new ControlChannel("CPU Fan #1", 1)); 
                        c.Add(new ControlChannel("CPU Fan #2 / Pump", 0)); 
                        c.Add(new ControlChannel("Chassis Fan #1", 3)); 
                        c.Add(new ControlChannel("Chassis Fan #2", 4)); 
                        c.Add(new ControlChannel("Chassis Fan #3", 6)); 
                        break;

                    case Model.X870E_TAICHI: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+12V", 1, 56, 10));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+5V", 4, 20, 10));
                        v.Add(new Voltage("+1.05V Always-on", 5));
                        v.Add(new Voltage("+3.3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("Vcore SoC", 10, 1, 1));
                        v.Add(new Voltage("Vcore Misc", 11, 1, 1));
                        v.Add(new Voltage("+1.8V", 13, 1, 1));

                        t.Add(new Temperature("Motherboard", 2));

                        t.Add(new Temperature("Thermistor Sensor #1", 5)); 
                        t.Add(new Temperature("Thermistor Sensor #2", 6)); 
                        t.Add(new Temperature("Thermistor Sensor #3", 8)); 

                        f.Add(new Fan("CPU Fan #1", 1)); 
                        f.Add(new Fan("CPU Fan #2", 2)); 
                        f.Add(new Fan("AIO Pump", 3)); 
                        f.Add(new Fan("Chassis Fan #1", 0)); 
                        f.Add(new Fan("Chassis Fan #2", 4)); 

                        c.Add(new ControlChannel("CPU Fan #1", 1)); 
                        c.Add(new ControlChannel("CPU Fan #2", 2)); 
                        c.Add(new ControlChannel("AIO Pump", 3)); 
                        c.Add(new ControlChannel("Chassis Fan #1", 0)); 
                        c.Add(new ControlChannel("Chassis Fan #2", 4)); 
                        break;

                    case Model.X870E_NOVA_WIFI: 
                        
                        v.Add(new Voltage("Vcore", 0)); 
                        v.Add(new Voltage("+12V", 1, 56, 10));  
                        v.Add(new Voltage("Analog VCC", 2, 34, 34)); 
                        v.Add(new Voltage("+3.3V", 3, 34, 34)); 
                        v.Add(new Voltage("+5V", 4, 20, 10)); 
                        v.Add(new Voltage("+1.05 Standby", 5, 0, 1)); 
                        v.Add(new Voltage("Voltage #4", 6, 0, 1)); 
                        v.Add(new Voltage("+3V Standby", 7, 34, 34)); 
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34)); 
                        v.Add(new Voltage("CPU Termination", 9, 1, 1)); 
                        v.Add(new Voltage("SoC", 10, 1, 1)); 
                        v.Add(new Voltage("Voltage #6", 11, 34, 34, 0)); 
                        v.Add(new Voltage("Voltage #2", 12)); 
                        v.Add(new Voltage("+1.8V", 13, 10, 10, 0)); 
                        v.Add(new Voltage("Voltage #7", 14, 0, 1, 0)); 
                        v.Add(new Voltage("Voltage #9", 15)); 

                        t.Add(new Temperature("CPU Socket", 0)); 
                        t.Add(new Temperature("Motherboard", 1)); 
                        t.Add(new Temperature("Auxiliary #0", 2)); 
                        t.Add(new Temperature("Auxiliary #1", 3)); 
                        t.Add(new Temperature("Thermistor Sensor #1", 4)); 
                        t.Add(new Temperature("Thermistor Sensor #2", 5)); 
                        t.Add(new Temperature("Auxiliary #4", 6)); 
                        t.Add(new Temperature("Thermistor Sensor #3", 7)); 
                        t.Add(new Temperature("CPU Core", 8)); 
                        t.Add(new Temperature("CPU (PECI)", 9)); 
                        t.Add(new Temperature("Virtual", 10)); 

                        f.Add(new Fan("Chassis Fan #1", 0)); 
                        f.Add(new Fan("CPU Fan #1", 1)); 
                        f.Add(new Fan("CPU Fan #2", 2)); 
                        f.Add(new Fan("AIO Pump", 3)); 
                        f.Add(new Fan("Water Pump", 4)); 
                        f.Add(new Fan("Chassis Fan #2", 5)); 
                        f.Add(new Fan("Chassis Fan #3", 6)); 

                        c.Add(new ControlChannel("Chassis Fan #1", 0)); 
                        c.Add(new ControlChannel("CPU Fan #1", 1)); 
                        c.Add(new ControlChannel("CPU Fan #2", 2)); 
                        c.Add(new ControlChannel("AIO Pump", 3)); 
                        c.Add(new ControlChannel("Water Pump", 4)); 
                        c.Add(new ControlChannel("Chassis Fan #2", 5)); 
                        c.Add(new ControlChannel("Chassis Fan #3", 6)); 
                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Temperature #1", 1));
                        t.Add(new Temperature("Temperature #2", 2));
                        t.Add(new Temperature("Temperature #3", 3));
                        t.Add(new Temperature("Temperature #4", 4));
                        t.Add(new Temperature("Temperature #5", 5));
                        t.Add(new Temperature("Temperature #6", 6));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.ASUS:
                string[] fanControlNames;
                switch (model)
                {
                    case Model.P8Z77_V: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Auxiliary", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Chassis Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #3", 3));
                        c.Add(new ControlChannel("Chassis Fan #1", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Chassis Fan #2", 2));
                        c.Add(new ControlChannel("Chassis Fan #3", 3));

                        break;

                    case Model.ROG_MAXIMUS_X_APEX: 
                        v.Add(new Voltage("Vcore", 0, 2, 2));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("CPU Graphics", 6, 2, 2));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("DIMM", 10, 1, 1));
                        v.Add(new Voltage("CPU System Agent", 11));
                        v.Add(new Voltage("PCH Core", 12));
                        v.Add(new Voltage("Phase Locked Loop", 13));
                        v.Add(new Voltage("CPU VCCIO/IMC", 14));
                        t.Add(new Temperature("CPU (PECI)", 0));
                        t.Add(new Temperature("T2", 1));
                        t.Add(new Temperature("T1", 2));
                        t.Add(new Temperature("CPU", 3));
                        t.Add(new Temperature("PCH", 4));
                        t.Add(new Temperature("Temperature #4", 5));
                        t.Add(new Temperature("Temperature #5", 6));
                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Chassis Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #3", 3));
                        f.Add(new Fan("AIO Pump", 4));
                        c.Add(new ControlChannel("Chassis Fan #1", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Chassis Fan #2", 2));
                        c.Add(new ControlChannel("Chassis Fan #3", 3));
                        c.Add(new ControlChannel("AIO Pump", 4));

                        break;

                    case Model.Z170_A: 
                        v.Add(new Voltage("Vcore", 0, 2, 2));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, 0, 1, 0, true));
                        v.Add(new Voltage("CPU Graphics", 6, 2, 2));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("DIMM", 10, 1, 1));
                        v.Add(new Voltage("CPU System Agent", 11));
                        v.Add(new Voltage("PCH Core", 12));
                        v.Add(new Voltage("Phase Locked Loop", 13));
                        v.Add(new Voltage("CPU VCCIO/IMC", 14));
                        t.Add(new Temperature("CPU (PECI)", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("CPU", 3));
                        t.Add(new Temperature("PCH", 4));
                        t.Add(new Temperature("Temperature #4", 5));
                        t.Add(new Temperature("Temperature #5", 6));

                        string[] fanNames = ["Chassis Fan 1", "CPU Fan", "Chassis Fan 2", "Chassis Fan 3", "Chassis Fan 4", "CPU Fan Optional"];
                        fanControlNames = ["Chassis Fan 1", "CPU Fan", "Chassis Fan 2", "Chassis Fan 3", "Chassis Fan 4", "Water Pump"];

                        for (int i = 0; i < fanNames.Length; i++)
                            f.Add(new Fan(fanNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.B150M_C: 
                    case Model.B150M_C_D3: 
                        v.Add(new Voltage("Vcore", 0, 2, 2));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("PCH", 9));
                        
                        t.Add(new Temperature("CPU (PECI)", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));

                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("Chassis Fan #2", 2));

                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 0));
                        c.Add(new ControlChannel("System Fan #2", 2));

                        break;

                    case Model.TUF_GAMING_X570_PLUS_WIFI: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));

                        t.Add(new Temperature("CPU", 22));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Chipset", 10));

                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("CPU Optional Fan", 6));
                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("Chassis Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #3", 3));
                        f.Add(new Fan("Chipset Fan", 4));
                        f.Add(new Fan("AIO Pump", 5));

                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("CPU Optional Fan", 6));
                        c.Add(new ControlChannel("Chassis Fan #1", 0));
                        c.Add(new ControlChannel("Chassis Fan #2", 2));
                        c.Add(new ControlChannel("Chassis Fan #3", 3));
                        c.Add(new ControlChannel("Chipset Fan", 4));
                        c.Add(new ControlChannel("AIO Pump", 5));

                        break;
                    case Model.TUF_GAMING_B550M_PLUS_WIFI: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        t.Add(new Temperature("PECI 0", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("System", 2));
                        t.Add(new Temperature("AUX 0", 3));
                        t.Add(new Temperature("AUX 1", 4));
                        t.Add(new Temperature("AUX 2", 5));
                        t.Add(new Temperature("AUX 3", 6));
                        t.Add(new Temperature("AUX 4", 7));
                        t.Add(new Temperature("SMBus 0", 8));
                        t.Add(new Temperature("SMBus 1", 9));
                        t.Add(new Temperature("PECI 1", 10));
                        t.Add(new Temperature("PCH Chip CPU Max", 11));
                        t.Add(new Temperature("PCH Chip", 12));
                        t.Add(new Temperature("PCH CPU", 13));
                        t.Add(new Temperature("PCH MCH", 14));
                        t.Add(new Temperature("Agent 0 DIMM 0", 15));
                        t.Add(new Temperature("Agent 0 DIMM 1", 16));
                        t.Add(new Temperature("Agent 1 DIMM 0", 17));
                        t.Add(new Temperature("Agent 1 DIMM 1", 18));
                        t.Add(new Temperature("Device 0", 19));
                        t.Add(new Temperature("Device 1", 20));
                        t.Add(new Temperature("PECI 0 Calibrated", 21));
                        t.Add(new Temperature("PECI 1 Calibrated", 22));
                        t.Add(new Temperature("Virtual", 23));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.TUF_GAMING_B760M_PLUS_WIFI_D4: 
                        v.Add(new Voltage("Vcore", 0, 15, 136));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("VTT", 9, 1, 1));
                        v.Add(new Voltage("DRAM", 10, 1, 1));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        v.Add(new Voltage("Voltage #16", 15, true));
                        v.Add(new Voltage("Voltage #17", 16, true));
                        t.Add(new Temperature("CPU Package", 0));
                        t.Add(new Temperature("Temperature #2", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Temperature #4", 3));
                        t.Add(new Temperature("CPU", 23));

                        fanControlNames = ["Chassis Fan 1", "CPU Fan", "CPU Optional Fan", "Chassis Fan 2", "Chassis Fan 3", "AIO Pump"];
                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length,
                                                        $"Expected {fanControlNames.Length} fan register in the SuperIO chip");

                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length,
                                                        "Expected counts of cans controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_CROSSHAIR_VIII_HERO: 
                    case Model.ROG_CROSSHAIR_VIII_HERO_WIFI: 
                    case Model.ROG_CROSSHAIR_VIII_DARK_HERO: 
                    case Model.ROG_CROSSHAIR_VIII_FORMULA: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("CPU SoC", 6));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("DIMM", 13));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        t.Add(new Temperature("PECI 0", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("AUX 0", 3));
                        t.Add(new Temperature("AUX 1", 4));
                        t.Add(new Temperature("AUX 2", 5));
                        t.Add(new Temperature("AUX 3", 6));
                        t.Add(new Temperature("AUX 4", 7));
                        t.Add(new Temperature("SMBus 0", 8));
                        t.Add(new Temperature("SMBus 1", 9));
                        t.Add(new Temperature("PECI 1", 10));
                        t.Add(new Temperature("PCH Chip CPU Max", 11));
                        t.Add(new Temperature("PCH Chip", 12));
                        t.Add(new Temperature("PCH CPU", 13));
                        t.Add(new Temperature("PCH MCH", 14));
                        t.Add(new Temperature("Agent 0 DIMM 0", 15));
                        t.Add(new Temperature("Agent 0 DIMM 1", 16));
                        t.Add(new Temperature("Agent 1 DIMM 0", 17));
                        t.Add(new Temperature("Agent 1 DIMM 1", 18));
                        t.Add(new Temperature("Device 0", 19));
                        t.Add(new Temperature("Device 1", 20));
                        t.Add(new Temperature("PECI 0 Calibrated", 21));
                        t.Add(new Temperature("PECI 1 Calibrated", 22));
                        t.Add(new Temperature("Virtual", 23));

                        fanControlNames = ["Chassis Fan 1", "CPU Fan", "Chassis Fan 2", "Chassis Fan 3", "High Amp Fan", "Waterpump", "AIO Pump"];
                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length,
                                                        $"Expected {fanControlNames.Length} fan register in the SuperIO chip");

                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length,
                                                        "Expected counts of cans controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_MAXIMUS_XI_FORMULA: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("VIN8", 5));
                        v.Add(new Voltage("CPU Graphics", 6));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("DIMM", 10));
                        v.Add(new Voltage("CPU VCCIO", 11, 1, 1));
                        v.Add(new Voltage("PCH Core", 12));
                        v.Add(new Voltage("Phase Locked Loop", 13));
                        v.Add(new Voltage("CPU System Agent", 14));

                        t.Add(new Temperature("Motherboard", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("CPU (Weighted)", 6));
                        t.Add(new Temperature("CPU (PECI)", 7));
                        t.Add(new Temperature("CPU", 8));

                        fanControlNames = ["Chassis Fan 1", "CPU Fan", "Chassis Fan 2", "Chassis Fan 3", "High Amp Fan", "Water Pump+", "AIO Pump"];
                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length,
                                                        $"Expected {fanControlNames.Length} fan register in the SuperIO chip");

                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length,
                                                        "Expected counts of cans controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_MAXIMUS_XII_Z490_FORMULA: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("3VCC", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("IVR Atom L2 Cluster #1", 5));
                        v.Add(new Voltage("Voltage #7", 6));
                        v.Add(new Voltage("3VSB", 7, 34, 34));
                        v.Add(new Voltage("VBat", 8, 34, 34));
                        v.Add(new Voltage("VTT", 9, 1, 1));
                        v.Add(new Voltage("Voltage #11", 10));
                        v.Add(new Voltage("IVR Atom L2 Cluster #0", 11, 1, 1));
                        v.Add(new Voltage("PCH", 12));
                        v.Add(new Voltage("Voltage #14", 13));
                        v.Add(new Voltage("Voltage #15", 14));

                        t.Add(new Temperature("Temperature #1", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Temperature #4", 4));
                        t.Add(new Temperature("Temperature #5", 5));
                        t.Add(new Temperature("Temperature #6", 6));
                        t.Add(new Temperature("Temperature #7", 7));
                        t.Add(new Temperature("PCH", 12));
                        t.Add(new Temperature("Temperature #9", 21));

                        fanControlNames = ["Chassis Fan 1", "CPU Fan", "Chassis Fan 2", "Chassis Fan 3", "Chassis Fan 4", "Waterpump", "AIO Pump"];

                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length,
                                                        $"Expected {fanControlNames.Length} fan register in the SuperIO chip");

                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length,
                                                        "Expected counts of cans controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_MAXIMUS_X_HERO_WIFI_AC: 
                        v.Add(new Voltage("Vcore", 0, 2, 2));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("CPU Graphics", 6, 2, 2));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("DIMM", 10, 1, 1));
                        v.Add(new Voltage("CPU System Agent", 11));
                        v.Add(new Voltage("PCH Core", 12));
                        v.Add(new Voltage("Phase Locked Loop", 13));
                        v.Add(new Voltage("CPU VCCIO/IMC", 14));
                        t.Add(new Temperature("CPU (PECI)", 0));
                        t.Add(new Temperature("T2", 1));
                        t.Add(new Temperature("Motherboard", 2)); 
                        t.Add(new Temperature("Temperature #3", 4));
                        t.Add(new Temperature("Temperature #4", 5));
                        t.Add(new Temperature("Temperature #5", 6));

                        fanControlNames = ["Chassis Fan 1", "CPU Fan", "Chassis Fan 2", "Chassis Fan 3", "AIO Pump", "HAMP"];

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_MAXIMUS_Z690_FORMULA: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("3VCC", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("IVR Atom L2 Cluster #1", 5));
                        v.Add(new Voltage("Voltage #7", 6));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("Voltage #10", 10));
                        v.Add(new Voltage("IVR Atom L2 Cluster #0", 11, 1, 1));
                        v.Add(new Voltage("PCH", 12));
                        v.Add(new Voltage("Voltage #14", 13));
                        v.Add(new Voltage("Voltage #15", 14));

                        t.Add(new Temperature("Temperature #1", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Temperature #4", 4));
                        t.Add(new Temperature("Temperature #5", 5));
                        t.Add(new Temperature("Temperature #6", 6));
                        t.Add(new Temperature("Temperature #7", 7));
                        t.Add(new Temperature("PCH", 12));
                        t.Add(new Temperature("Temperature #9", 21));

                        fanControlNames = ["Chassis Fan 1", "CPU Fan", "Chassis Fan 2", "Chassis Fan 3", "Chassis Fan 4", "Waterpump", "AIO Pump"];

                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length,
                                                        $"Expected {fanControlNames.Length} fan register in the SuperIO chip");

                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length,
                                                        "Expected counts of cans controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_MAXIMUS_Z690_HERO: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("3VCC", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("IVR Atom L2 Cluster #1", 5));
                        v.Add(new Voltage("Voltage #7", 6));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("Voltage #11", 10));
                        v.Add(new Voltage("IVR Atom L2 Cluster #0", 11, 1, 1));
                        v.Add(new Voltage("PCH", 12));
                        v.Add(new Voltage("Voltage #14 ", 13));
                        v.Add(new Voltage("Voltage #15", 14));

                        t.Add(new Temperature("CPU Package", 0)); 
                        t.Add(new Temperature("CPU (Weighted)", 1)); 

                        t.Add(new Temperature("Motherboard", 2)); 
                        
                        t.Add(new Temperature("PCH", 12)); 
                        t.Add(new Temperature("CPU", 21)); 

                        fanControlNames = ["Chassis Fan 1", "CPU Fan", "Chassis Fan 2", "Chassis Fan 3", "Chassis Fan 4", "Waterpump", "AIO Pump"];

                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length,
                                                        $"Expected {fanControlNames.Length} fan register in the SuperIO chip");

                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length,
                                                        "Expected counts of cans controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_MAXIMUS_Z690_EXTREME_GLACIAL: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("3VCC", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("IVR Atom L2 Cluster #1", 5));
                        v.Add(new Voltage("Voltage #7", 6));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("Voltage #11", 10));
                        v.Add(new Voltage("IVR Atom L2 Cluster #0", 11, 1, 1));
                        v.Add(new Voltage("PCH", 12));
                        v.Add(new Voltage("Voltage #14", 13));
                        v.Add(new Voltage("Voltage #15", 14));

                        t.Add(new Temperature("Temperature #1", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        
                        t.Add(new Temperature("Temperature #4", 4));
                        t.Add(new Temperature("Temperature #5", 5));
                        t.Add(new Temperature("Temperature #6", 6));
                        t.Add(new Temperature("Temperature #7", 7));
                        t.Add(new Temperature("PCH", 12));
                        t.Add(new Temperature("Temperature #9", 21));

                        fanControlNames = ["Chassis Fan 1", "CPU Fan", "Radiator Fan 1", "Radiator Fan 2", "Chassis Fan 2", "Water Pump 1", "Water Pump 2"];
                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length,
                                                        $"Expected {fanControlNames.Length} fan register in the SuperIO chip");

                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length,
                                                        "Expected counts of fan controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_MAXIMUS_Z790_HERO: 
                        t.Add(new Temperature("CPU Package", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        
                        t.Add(new Temperature("Temperature #8", 8));
                        t.Add(new Temperature("Temperature #9", 9));
                        t.Add(new Temperature("Temperature #10", 10));
                        t.Add(new Temperature("Temperature #11", 11));
                        t.Add(new Temperature("Temperature #12", 12));
                        t.Add(new Temperature("Chipset", 13));
                        t.Add(new Temperature("Temperature #14", 14));
                        t.Add(new Temperature("Temperature #15", 15));
                        t.Add(new Temperature("Temperature #16", 16));
                        t.Add(new Temperature("Temperature #17", 17));
                        t.Add(new Temperature("Temperature #18", 18));
                        t.Add(new Temperature("Temperature #19", 19));
                        t.Add(new Temperature("Temperature #20", 20));
                        t.Add(new Temperature("Temperature #21", 21));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.ROG_MAXIMUS_Z790_DARK_HERO: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("3VCC", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("IVR Atom L2 Cluster #1", 5));
                        v.Add(new Voltage("Voltage #6", 6));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("Voltage #11", 10, 1, 1));
                        v.Add(new Voltage("IVR Atom L2 Cluster #0", 11, 1, 1));
                        v.Add(new Voltage("PCH", 12));
                        v.Add(new Voltage("CPU System Agent", 13, 1, 1));
                        v.Add(new Voltage("CPU Input Auxiliary", 14, 1, 1));
                        v.Add(new Voltage("Voltage #15", 15));

                        t.Add(new Temperature("CPU Package", 0));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Chipset", 12));
                        t.Add(new Temperature("PCH", 13));
                        t.Add(new Temperature("CPU", 22));

                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Chassis Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #3", 3));
                        f.Add(new Fan("Chassis Fan #4", 4));
                        f.Add(new Fan("Water Pump", 5));
                        f.Add(new Fan("AIO Pump", 6));

                        c.Add(new ControlChannel("Chassis Fan #1", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Chassis Fan #2", 2));
                        c.Add(new ControlChannel("Chassis Fan #3", 3));
                        c.Add(new ControlChannel("Chassis Fan #4", 4));
                        c.Add(new ControlChannel("Water Pump", 5));
                        c.Add(new ControlChannel("AIO Pump", 6));

                        break;

                    case Model.ROG_STRIX_B760_I_GAMING_WIFI: 

                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("3VCC", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("IMC VDD", 10, 1, 1));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        v.Add(new Voltage("Voltage #16", 15, true));

                        t.Add(new Temperature("CPU Package", 0));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("VRM", 3));
                        t.Add(new Temperature("Thermistor Sensor", 8));
                        t.Add(new Temperature("PCH", 13));
                        t.Add(new Temperature("CPU Calibrated", 22));

                        f.Add(new Fan("Chassis Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("AIO Pump", 5));

                        c.Add(new ControlChannel("Chassis Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("AIO Pump", 5));

                        break;

                    case Model.ROG_STRIX_Z790_E_GAMING_WIFI_II: 

                        t.Add(new Temperature("CPU Package", 0));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Chipset", 12));
                        t.Add(new Temperature("PCH", 13));
                        t.Add(new Temperature("CPU", 22));

                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Chassis Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #3", 3));
                        f.Add(new Fan("Chassis Fan #4", 4));
                        f.Add(new Fan("Chassis FAN #5", 5));
                        f.Add(new Fan("AIO Pump", 6));

                        c.Add(new ControlChannel("Chassis Fan #1", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Chassis Fan #2", 2));
                        c.Add(new ControlChannel("Chassis Fan #3", 3));
                        c.Add(new ControlChannel("Chassis Fan #4", 4));
                        c.Add(new ControlChannel("Chassis Fan #5", 5));
                        c.Add(new ControlChannel("AIO Pump", 6));

                        break;

                    case Model.ROG_STRIX_B550_I_GAMING: 
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        v.Add(new Voltage("+5V", 1, 4, 1)); 
                        v.Add(new Voltage("AVCC", 2, 10, 10));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1)); 
                        
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));

                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("PCH Chip CPU Max", 11));
                        t.Add(new Temperature("PCH Chip", 12));
                        t.Add(new Temperature("PCH CPU", 13));
                        t.Add(new Temperature("PCH MCH", 14));
                        t.Add(new Temperature("Agent 0 DIMM 0", 15));
                        
                        t.Add(new Temperature("Agent 1 DIMM 0", 17));
                        
                        t.Add(new Temperature("Device 0", 19));
                        t.Add(new Temperature("Device 1", 20));
                        t.Add(new Temperature("PECI 0 Calibrated", 21));
                        t.Add(new Temperature("PECI 1 Calibrated", 22));
                        t.Add(new Temperature("Virtual", 23));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                        {
                            switch (i)
                            {
                                case 0:
                                    f.Add(new Fan("Chassis Fan", 0));
                                    break;
                                case 1:
                                    f.Add(new Fan("CPU Fan", 1));
                                    break;
                                case 4:
                                    f.Add(new Fan("AIO Pump", 4));
                                    break;
                            }
                        }

                        for (int i = 0; i < superIO.Controls.Length; i++)
                        {
                            switch (i)
                            {
                                case 0:
                                    c.Add(new ControlChannel("Chassis Fan", 0));
                                    break;
                                case 1:
                                    c.Add(new ControlChannel("CPU Fan", 1));
                                    break;
                                case 4:
                                    c.Add(new ControlChannel("AIO Pump", 4));
                                    break;
                            }
                        }

                        break;

                    case Model.ROG_ZENITH_II_EXTREME: 
                        
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 6, 1));
                        v.Add(new Voltage("DIMM C/D", 11, 10, 10));
                        v.Add(new Voltage("DIMM A/B", 13));
                        v.Add(new Voltage("Phase Locked Loop", 14));

                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Temperature #3", 3));
                        t.Add(new Temperature("Temperature #4", 4));
                        t.Add(new Temperature("Temperature #5", 5));
                        t.Add(new Temperature("Temperature #6", 6));
                        t.Add(new Temperature("Temperature #7", 7));
                        t.Add(new Temperature("Temperature #21", 21));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                        {
                            switch (i)
                            {
                                case 0:
                                    f.Add(new Fan("Chassis Fan", 0));
                                    break;
                                case 1:
                                    f.Add(new Fan("CPU Fan", 1));
                                    break;
                                case 2:
                                    f.Add(new Fan("CPU Optional Fan", 2));
                                    break;
                                case 4:
                                    f.Add(new Fan("AIO Pump", 4));
                                    break;
                            }
                        }

                        for (int i = 0; i < superIO.Controls.Length; i++)
                        {
                            switch (i)
                            {
                                case 0:
                                    c.Add(new ControlChannel("Chassis Fan", 0));
                                    break;
                                case 1:
                                    c.Add(new ControlChannel("CPU Fan", 1));
                                    break;
                                case 2:
                                    c.Add(new ControlChannel("CPU Optional Fan", 2));
                                    break;
                                case 4:
                                    c.Add(new ControlChannel("AIO Pump", 4));
                                    break;
                            }
                        }

                        break;

                    case Model.ROG_STRIX_X570_I_GAMING: 
                        v.Add(new Voltage("Vcore", 0, 10, 10));
                        v.Add(new Voltage("+5V", 1, 4, 1)); 
                        v.Add(new Voltage("AVCC", 2, 10, 10));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1)); 
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Temperature #3", 3));
                        t.Add(new Temperature("Temperature #4", 4));
                        t.Add(new Temperature("Temperature #5", 5));
                        t.Add(new Temperature("Temperature #6", 6));
                        t.Add(new Temperature("Temperature #7", 7));
                        t.Add(new Temperature("Temperature #21", 21));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                        {
                            switch (i)
                            {
                                case 0:
                                    f.Add(new Fan("Chassis Fan", 0));
                                    break;
                                case 1:
                                    f.Add(new Fan("CPU Fan", 1));
                                    break;
                                case 4:
                                    f.Add(new Fan("AIO Pump", 4));
                                    break;
                            }
                        }

                        for (int i = 0; i < superIO.Controls.Length; i++)
                        {
                            switch (i)
                            {
                                case 0:
                                    c.Add(new ControlChannel("Chassis Fan", 0));
                                    break;
                                case 1:
                                    c.Add(new ControlChannel("CPU Fan", 1));
                                    break;
                                case 4:
                                    c.Add(new ControlChannel("AIO Pump", 4));
                                    break;
                            }
                        }

                        break;

                    case Model.ROG_STRIX_B550_F_GAMING_WIFI: 
                        v.Add(new Voltage("Vcore", 0, 2, 2));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        t.Add(new Temperature("CPU Core", 0));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.PRIME_B650_PLUS: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("CPU VDDIO Memory", 10, 1, 1));

                        t.Add(new Temperature("CPU", 22));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("CPU Package", 3));

                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("CPU Optional Fan", 4));
                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("Chassis Fan #2", 2));
                        f.Add(new Fan("Chassis Fan #3", 3));
                        f.Add(new Fan("AIO Pump", 5));

                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Chassis Fan #1", 0));
                        c.Add(new ControlChannel("Chassis Fan #2", 2));
                        c.Add(new ControlChannel("Chassis Fan #3", 3));
                        c.Add(new ControlChannel("AIO Pump", 5));

                        break;

                    case Model.ROG_CROSSHAIR_X670E_GENE: 
                    case Model.ROG_CROSSHAIR_X670E_HERO:
                        v.Add(new Voltage("Vcore", 0, 2, 2)); 
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9)); 
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Temperature #1", 1)); 
                        t.Add(new Temperature("Motherboard", 2)); 
                        t.Add(new Temperature("Temperature #3", 3)); 
                        t.Add(new Temperature("Temperature #4", 4)); 
                        t.Add(new Temperature("Temperature #5", 5)); 
                        t.Add(new Temperature("Temperature #6", 6)); 
                        t.Add(new Temperature("Temperature #7", 8)); 
                        t.Add(new Temperature("CPU", 22)); 
                        t.Add(new Temperature("Thermistor Sensor", 24));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.ROG_STRIX_X670E_A_GAMING_WIFI: 
                    case Model.ROG_STRIX_X670E_E_GAMING_WIFI: 
                    case Model.ROG_STRIX_X670E_F_GAMING_WIFI: 
                        v.Add(new Voltage("Vcore", 0, 2, 2)); 
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9)); 
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("VRM", 1)); 
                        t.Add(new Temperature("Motherboard", 2)); 
                        t.Add(new Temperature("Temperature #3", 3)); 
                        t.Add(new Temperature("Temperature #4", 4)); 
                        t.Add(new Temperature("Temperature #5", 5)); 
                        t.Add(new Temperature("Temperature #6", 6)); 
                        t.Add(new Temperature("Thermistor Sensor", 24)); 

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.ROG_STRIX_X870E_E_GAMING_WIFI: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, true)); 
                        v.Add(new Voltage("CPU VDDIO Memory", 10, 1, 1));

                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("CPU", 22));

                        fanControlNames = ["Chassis Fan #1", "CPU Fan", "Chassis Fan #2", "Chassis Fan #3", "Chassis Fan #4", "Chassis Fan #5", "AIO Pump"];

                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length, $"Expected {fanControlNames.Length} fan register in the SuperIO chip");
                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length, "Expected counts of cans controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.PROART_X670E_CREATOR_WIFI: 
                        v.Add(new Voltage("Vcore", 0)); 
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        
                        t.Add(new Temperature("CPU", 22));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Thermistor Sensor", 24)); 
                        t.Add(new Temperature("Temperature #1", 1)); 

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.PRIME_X870_P: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 34, 34));
                        v.Add(new Voltage("CPU VDDIO Memory", 10, 1, 1));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("+1.8V Standby", 12, true)); 
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        v.Add(new Voltage("Voltage #16", 15, true));
                        
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("VRM", 7));
                        t.Add(new Temperature("CPU", 22));

                        fanControlNames = ["Chassis Fan #1", "CPU Fan", "Chassis Fan #2", "Chassis Fan #3", "CPU_OPT", "Chassis Fan #4", "AIO Pump"];

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_STRIX_X870_I_GAMING_WIFI: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4.02f, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 10.98f, 1));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("VTT", 9, 34, 34));
                        v.Add(new Voltage("CPU VDDIO Memory", 10, 34, 34));
                        v.Add(new Voltage("VMISC", 11, 34, 34));
                        v.Add(new Voltage("1.8V Standby", 12, 7.66f, 10));

                        t.Add(new Temperature("CPU", 22));
                        t.Add(new Temperature("Motherboard", 2));

                        f.Add(new Fan("Chassis", 0));
                        f.Add(new Fan("CPU", 1));
                        f.Add(new Fan("Chipset / Disk", 5));
                        f.Add(new Fan("AIO Pump", 6));

                        c.Add(new ControlChannel("Chassis", 0));
                        c.Add(new ControlChannel("CPU", 1));
                        c.Add(new ControlChannel("Chipset / Disk", 5));
                        c.Add(new ControlChannel("AIO Pump", 6));

                        break;

                    case Model.ROG_CROSSHAIR_X870E_APEX: 
                        {
                            v.Add(new Voltage("Vcore", 0, 15, 136));
                            v.Add(new Voltage("+5V", 1, 4, 1));
                            v.Add(new Voltage("AVSB", 2, 34, 34));
                            v.Add(new Voltage("+3.3V", 3, 34, 34));
                            v.Add(new Voltage("+12V", 4, 11, 1));
                            v.Add(new Voltage("VMISC", 5, 9, 82));
                            v.Add(new Voltage("CPU SOC", 6, 9, 82));
                            v.Add(new Voltage("+3V Standby", 7, 34, 34));
                            v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                            v.Add(new Voltage("VTT", 9, 34, 34));
                            v.Add(new Voltage("Chipset 0 VDD", 10, 1, 1));
                            v.Add(new Voltage("Chipset 1 VDD", 11, 1, 1));
                            v.Add(new Voltage("Chipset Standby", 12));
                            v.Add(new Voltage("CPU VDDIO Memory", 13, 9, 82));
                            v.Add(new Voltage("1.8V PLL", 14, 17, 36));

                            t.Add(new Temperature("CPU Package", 0));
                            t.Add(new Temperature("VRM", 1));
                            t.Add(new Temperature("Motherboard", 2));

                            t.Add(new Temperature("CPU", 21));

                            t.Add(new Temperature("CPU", 22));

                            for (int i = 3; i < superIO.Temperatures.Length; i++)
                            {
                                
                                if (i != 21 && i != 22)
                                {
                                    t.Add(new Temperature($"Temperature #{i}", i));
                                }
                            }

                            fanControlNames = ["Chassis Fan #1", "CPU Fan", "Chassis Fan #2", "Extra Flow Fan", "Unused", "Water Pump", "AIO Pump"];

                            System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length, $"Expected {fanControlNames.Length} fan register in the SuperIO chip");
                            System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length, "Expected counts of fan controls and fan speed registers to be equal");

                            for (int i = 0; i < fanControlNames.Length; i++)
                            {
                                f.Add(new Fan(fanControlNames[i], i));
                            }

                            for (int i = 0; i < fanControlNames.Length; i++)
                            {
                                c.Add(new ControlChannel(fanControlNames[i], i));
                            }
                        }

                        break;

                    case Model.ROG_CROSSHAIR_X870E_HERO: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, true)); 
                        v.Add(new Voltage("CPU VDDIO Memory", 10, 1, 1));

                        t.Add(new Temperature("CPU Package", 0));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("CPU", 22));

                        fanControlNames = ["Chassis Fan #1", "CPU Fan", "Chassis Fan #2", "Chassis Fan #3", "Chassis Fan #4", "Water Pump", "AIO Pump"];

                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length, $"Expected {fanControlNames.Length} fan register in the SuperIO chip");
                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length, "Expected counts of fan controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.PROART_X870E_CREATOR_WIFI: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));

                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("CPU", 22));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;

                    case Model.PROART_B760_CREATOR_D4: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("3VCC", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("IVR Atom L2 Cluster #1", 5));
                        v.Add(new Voltage("Voltage #7", 6));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9, 1, 1));
                        v.Add(new Voltage("Voltage #11", 10));
                        v.Add(new Voltage("IVR Atom L2 Cluster #0", 11, 1, 1));
                        v.Add(new Voltage("PCH", 12));
                        v.Add(new Voltage("Voltage #14", 13));
                        v.Add(new Voltage("Voltage #15", 14));

                        t.Add(new Temperature("CPU Package", 0)); 
                        t.Add(new Temperature("CPU", 1)); 
                        t.Add(new Temperature("Motherboard", 2)); 
                        t.Add(new Temperature("Thermistor Sensor", 8)); 
                        t.Add(new Temperature("PCH", 13)); 
                        t.Add(new Temperature("PECI 0 Calibrated", 22)); 

                        f.Add(new Fan("Chassis Fan #1", 0)); 
                        f.Add(new Fan("CPU Fan", 1)); 
                        f.Add(new Fan("Chassis Fan #2", 2)); 
                        f.Add(new Fan("Chassis Fan #3", 3)); 
                        f.Add(new Fan("Chassis Fan #4", 4)); 
                        f.Add(new Fan("CPU Optional Fan", 5)); 
                        f.Add(new Fan("AIO Pump", 6)); 

                        c.Add(new ControlChannel("Chassis Fan #1", 0)); 
                        c.Add(new ControlChannel("CPU Fan", 1)); 
                        c.Add(new ControlChannel("Chassis Fan #2", 2)); 
                        c.Add(new ControlChannel("Chassis Fan #3", 3)); 
                        c.Add(new ControlChannel("Chassis Fan #4", 4)); 
                        c.Add(new ControlChannel("CPU Optional Fan", 5)); 
                        c.Add(new ControlChannel("AIO Pump", 6)); 

                        break;

                    case Model.ROG_STRIX_B850_A_GAMING_WIFI: 
                    case Model.TUF_GAMING_B850_BTF_WIFI_W: 
                    case Model.TUF_GAMING_X870_PRO_WIFI7_W_NEO: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4.02f, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 10.98f, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("Voltage #10", 9, true));
                        v.Add(new Voltage("CPU VDDIO Memory", 10, 34, 34));
                        v.Add(new Voltage("VMisc", 11, 34, 34));
                        v.Add(new Voltage("1.8V Standby", 12, 7.66f, 10));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        v.Add(new Voltage("Voltage #16", 15, true));

                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("CPU Package", 26));
                        t.Add(new Temperature("CPU", 21));

                        if (model == Model.TUF_GAMING_X870_PRO_WIFI7_W_NEO)
                        {
                            t.Add(new Temperature("Temperature #14", 27));
                            t.Add(new Temperature("T_Sensor", 24));

                            fanControlNames = ["Chassis Fan #1", "CPU Fan", "Chassis Fan #2", "Chassis Fan #3", "Chassis Fan #4", "Water Pump+", "AIO Pump"];
                        }
                        else
                        {
                            t.Add(new Temperature("VRM", 1));
                            t.Add(new Temperature("T_Sensor", 24));

                            fanControlNames = ["CPU Fan", "CPU Optional Fan", "Chassis Fan #1", "Chassis Fan #2", "Chassis Fan #3", "Chassis Fan #4", "AIO Pump"];
                        }

                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length, $"Expected {fanControlNames.Length} fan register in the SuperIO chip");
                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length, "Expected counts of fans, controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_STRIX_B850_E_GAMING_WIFI: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4.02f, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 10.98f, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("VTT", 9, 34, 34));
                        v.Add(new Voltage("CPU VDDIO Memory", 10, 34, 34));
                        v.Add(new Voltage("VMisc", 11, 34, 34));
                        v.Add(new Voltage("1.8V Standby", 12, 7.66f, 10));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        v.Add(new Voltage("Voltage #16", 15, true));

                        t.Add(new Temperature("CPU", 21));
                        
                        t.Add(new Temperature("Motherboard", 2));
                        
                        fanControlNames = ["Chassis Fan #1", "CPU Fan", "Chassis Fan #2", "Chassis Fan #3", "Chassis Fan #4", "Chassis Fan #5", "AIO Pump"];

                        System.Diagnostics.Debug.Assert(fanControlNames.Length == superIO.Fans.Length, $"Expected {fanControlNames.Length} fan register in the SuperIO chip");
                        System.Diagnostics.Debug.Assert(superIO.Fans.Length == superIO.Controls.Length, "Expected counts of cans controls and fan speed registers to be equal");

                        for (int i = 0; i < fanControlNames.Length; i++)
                            f.Add(new Fan(fanControlNames[i], i));

                        for (int i = 0; i < fanControlNames.Length; i++)
                            c.Add(new ControlChannel(fanControlNames[i], i));

                        break;

                    case Model.ROG_STRIX_B850_I_GAMING_WIFI: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4.02f, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 10.98f, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("VTT", 9, 34, 34));
                        v.Add(new Voltage("CPU VDDIO Memory", 10, 34, 34));
                        v.Add(new Voltage("VMISC", 11, 34, 34));
                        v.Add(new Voltage("1,8V Standby", 12, 7.66f, 10));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        v.Add(new Voltage("Voltage #16", 15, true));

                        t.Add(new Temperature("CPU", 22));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("T-Sensor", 6));

                        f.Add(new Fan("Chassis Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Extra Flow Fan", 5));
                        f.Add(new Fan("AIO Pump", 6));

                        c.Add(new ControlChannel("Chassis Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Extra Flow Fan", 5));
                        c.Add(new ControlChannel("AIO Pump", 6));

                        break;

                    case Model.TUF_GAMING_B850M_PLUS_II: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4.02f, 1));
                        v.Add(new Voltage("AVSB", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 10.98f, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("VTT", 9, 34, 34));
                        v.Add(new Voltage("CPU VDDIO Memory", 10, 34, 34));
                        v.Add(new Voltage("VMISC", 11, 34, 34));
                        v.Add(new Voltage("1,8V Standby", 12, 7.66f, 10));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        v.Add(new Voltage("Voltage #16", 15, true));

                        t.Add(new Temperature("CPU", 22));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("T-Sensor", 6));
                        
                        f.Add(new Fan("Chassis Fan #1", 0)); 
                        f.Add(new Fan("CPU Fan", 1)); 
                        f.Add(new Fan("Chassis Fan #2", 2)); 
                        f.Add(new Fan("Chassis Fan #3", 3)); 
                        f.Add(new Fan("CPU Optional Fan", 4)); 
                        f.Add(new Fan("AIO Pump", 5)); 

                        c.Add(new ControlChannel("Chassis Fan #1", 0)); 
                        c.Add(new ControlChannel("CPU Fan", 1)); 
                        c.Add(new ControlChannel("Chassis Fan #2", 2)); 
                        c.Add(new ControlChannel("Chassis Fan #3", 3)); 
                        c.Add(new ControlChannel("CPU Optional Fan", 4)); 
                        c.Add(new ControlChannel("AIO Pump", 5)); 
                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Temperature #1", 1));
                        t.Add(new Temperature("Temperature #2", 2));
                        t.Add(new Temperature("Temperature #3", 3));
                        t.Add(new Temperature("Temperature #4", 4));
                        t.Add(new Temperature("Temperature #5", 5));
                        t.Add(new Temperature("Temperature #6", 6));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;
            case Manufacturer.MSI:
                switch (model)
                {
                    case Model.B360M_PRO_VDH: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        
                        v.Add(new Voltage("CPU I/O", 6));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("CPU System Agent", 10));
                        
                        v.Add(new Voltage("Northbridge/SoC", 12));
                        v.Add(new Voltage("DIMM", 13, 1, 1));
                        
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Auxiliary", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        t.Add(new Temperature("Temperature #1", 5));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 2));
                        c.Add(new ControlChannel("System Fan #2", 3));

                        break;

                    case Model.B450A_PRO: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("CPU System Agent", 10));
                        
                        v.Add(new Voltage("Northbridge/SoC", 12));
                        v.Add(new Voltage("DIMM", 13, 1, 1));
                        
                        t.Add(new Temperature("CPU", 1));
                        t.Add(new Temperature("System", 2));
                        t.Add(new Temperature("VRM MOS", 3));
                        t.Add(new Temperature("PCH", 5));
                        t.Add(new Temperature("SMBus 0", 8));
                        f.Add(new Fan("Pump Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        f.Add(new Fan("System Fan #3", 4));
                        f.Add(new Fan("System Fan #4", 5));
                        c.Add(new ControlChannel("Pump Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 2));
                        c.Add(new ControlChannel("System Fan #2", 3));
                        c.Add(new ControlChannel("System Fan #3", 4));
                        c.Add(new ControlChannel("System Fan #4", 5));

                        break;

                    case Model.Z270_PC_MATE: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("CPU I/O", 6));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("CPU System Agent", 10));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("PCH", 12));
                        v.Add(new Voltage("DIMM", 13, 1, 1));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Auxiliary", 1));
                        t.Add(new Temperature("Motherboard", 2));
                        f.Add(new Fan("Pump Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        f.Add(new Fan("System Fan #3", 4));
                        f.Add(new Fan("System Fan #4", 5));
                        c.Add(new ControlChannel("Pump Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 2));
                        c.Add(new ControlChannel("System Fan #2", 3));
                        c.Add(new ControlChannel("System Fan #3", 4));
                        c.Add(new ControlChannel("System Fan #4", 5));

                        break;

                    case Model.X570_Gaming_Plus:
                        
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+5V", 1, 4, 1));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+12V", 4, 11, 1));
                        
                        v.Add(new Voltage("VIN4", 6));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        
                        v.Add(new Voltage("Voltage #11", 11));
                        v.Add(new Voltage("CPU NB/SoC", 12));
                        v.Add(new Voltage("DIMM", 13, 1, 1));
                        v.Add(new Voltage("Voltage #14", 14));

                        t.Add(new Temperature("System", 2));
                        t.Add(new Temperature("MOS", 3));
                        t.Add(new Temperature("Chipset", 5));
                        t.Add(new Temperature("CPU", 9));

                        f.Add(new Fan("Pump Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        f.Add(new Fan("System Fan #3", 4));
                        f.Add(new Fan("System Fan #4", 5));
                        f.Add(new Fan("Chipset Fan", 6));

                        c.Add(new ControlChannel("Pump Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 2));
                        c.Add(new ControlChannel("System Fan #2", 3));
                        c.Add(new ControlChannel("System Fan #3", 4));
                        c.Add(new ControlChannel("System Fan #4", 5));
                        c.Add(new ControlChannel("Chipset Fan", 6));

                        break;

                    case Model.X570_MS7C35:
                        
                        v.Add(new Voltage("Vcore", 0));           
                        v.Add(new Voltage("+5V", 1, 12, 3));      
                        v.Add(new Voltage("AVCC", 2, 34, 34));    
                        v.Add(new Voltage("+3.3V", 3, 34, 34));   
                        v.Add(new Voltage("+12V", 4, 220, 20));  
                        v.Add(new Voltage("VRM MOS", 6, true)); 
                        v.Add(new Voltage("+3.3V Standby", 7, 34, 34));   
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34, 0)); 
                        v.Add(new Voltage("CPU 1.8V", 9));        
                        v.Add(new Voltage("CPU VDDP", 10));       
                        v.Add(new Voltage("Voltage #6", 11, true));    
                        v.Add(new Voltage("CPU NB/SoC", 12));     
                        v.Add(new Voltage("DIMM", 13, 1, 1));     
                        v.Add(new Voltage("+5V Standby", 14, 768, 330));  

                        t.Add(new Temperature("CPU Socket", 1));  
                        t.Add(new Temperature("System", 2));      
                        t.Add(new Temperature("VRM MOS", 3));     
                        t.Add(new Temperature("Chipset", 5));     
                        t.Add(new Temperature("CPU", 23));
                        
                        t.Add(new Temperature("MOS CPU", 24));  
                        t.Add(new Temperature("PCH", 25));      

                        f.Add(new Fan("Pump Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("System Fan #1", 2));
                        f.Add(new Fan("System Fan #2", 3));
                        f.Add(new Fan("System Fan #3", 4));
                        f.Add(new Fan("System Fan #4", 5));
                        f.Add(new Fan("Chipset Fan", 6));

                        c.Add(new ControlChannel("Pump Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 2));
                        c.Add(new ControlChannel("System Fan #2", 3));
                        c.Add(new ControlChannel("System Fan #3", 4));
                        c.Add(new ControlChannel("System Fan #4", 5));
                        c.Add(new ControlChannel("Chipset Fan", 6));

                        break;

                    case Model.Z390_GAMING_EDGE_AC: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("+5V", 1, ri: 4, rf: 1));

                        t.Add(new Temperature("System", 2));
                        t.Add(new Temperature("MOS", 1));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("M.2", 3));
                        t.Add(new Temperature("PCI-E", 4));
                        t.Add(new Temperature("CPU Socket", 5));

                        c.Add(new ControlChannel("Pump Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("System Fan #1", 5));
                        c.Add(new ControlChannel("System Fan #2", 2));
                        c.Add(new ControlChannel("System Fan #3", 4));
                        c.Add(new ControlChannel("System Fan #4", 3));
                        c.Add(new ControlChannel("System Fan #5", 6));

                        f.Add(new Fan("Pump Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("System Fan #1", 5));
                        f.Add(new Fan("System Fan #2", 2));
                        f.Add(new Fan("System Fan #3", 4));
                        f.Add(new Fan("System Fan #4", 3));
                        f.Add(new Fan("System Fan #5", 6));
                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("CPU Termination", 9));
                        v.Add(new Voltage("Voltage #11", 10, true));
                        v.Add(new Voltage("Voltage #12", 11, true));
                        v.Add(new Voltage("Voltage #13", 12, true));
                        v.Add(new Voltage("Voltage #14", 13, true));
                        v.Add(new Voltage("Voltage #15", 14, true));
                        t.Add(new Temperature("CPU Core", 0));
                        t.Add(new Temperature("Temperature #1", 1));
                        t.Add(new Temperature("Temperature #2", 2));
                        t.Add(new Temperature("Temperature #3", 3));
                        t.Add(new Temperature("Temperature #4", 4));
                        t.Add(new Temperature("Temperature #5", 5));
                        t.Add(new Temperature("Temperature #6", 6));

                        for (int i = 0; i < superIO.Fans.Length; i++)
                            f.Add(new Fan("Fan #" + (i + 1), i));

                        for (int i = 0; i < superIO.Controls.Length; i++)
                            c.Add(new ControlChannel("Fan #" + (i + 1), i));

                        break;
                }

                break;

            default:
                v.Add(new Voltage("Vcore", 0));
                v.Add(new Voltage("Voltage #2", 1, true));
                v.Add(new Voltage("AVCC", 2, 34, 34));
                v.Add(new Voltage("+3.3V", 3, 34, 34));
                v.Add(new Voltage("Voltage #5", 4, true));
                v.Add(new Voltage("Voltage #6", 5, true));
                v.Add(new Voltage("Voltage #7", 6, true));
                v.Add(new Voltage("+3V Standby", 7, 34, 34));
                v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                v.Add(new Voltage("CPU Termination", 9));
                v.Add(new Voltage("Voltage #11", 10, true));
                v.Add(new Voltage("Voltage #12", 11, true));
                v.Add(new Voltage("Voltage #13", 12, true));
                v.Add(new Voltage("Voltage #14", 13, true));
                v.Add(new Voltage("Voltage #15", 14, true));
                t.Add(new Temperature("CPU Core", 0));
                t.Add(new Temperature("Temperature #1", 1));
                t.Add(new Temperature("Temperature #2", 2));
                t.Add(new Temperature("Temperature #3", 3));
                t.Add(new Temperature("Temperature #4", 4));
                t.Add(new Temperature("Temperature #5", 5));
                t.Add(new Temperature("Temperature #6", 6));

                for (int i = 0; i < superIO.Fans.Length; i++)
                    f.Add(new Fan("Fan #" + (i + 1), i));

                for (int i = 0; i < superIO.Controls.Length; i++)
                    c.Add(new ControlChannel("Fan #" + (i + 1), i));

                break;
        }
    }

    private static void GetWinbondConfigurationEhf(Manufacturer manufacturer, Model model, IList<Voltage> v, IList<Temperature> t, IList<Fan> f, IList<ControlChannel> c)
    {
        switch (manufacturer)
        {
            case Manufacturer.ASRock:
                switch (model)
                {
                    case Model.AOD790GX_128M: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 4, 10, 10));
                        v.Add(new Voltage("+5V", 5, 20, 10));
                        v.Add(new Voltage("+12V", 6, 28, 5));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 2));
                        f.Add(new Fan("CPU Fan", 0));
                        f.Add(new Fan("Chassis Fan", 1));
                        c.Add(new ControlChannel("System Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Auxiliary Fan", 2));

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        v.Add(new Voltage("Voltage #10", 9, true));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Auxiliary", 1));
                        t.Add(new Temperature("System", 2));
                        f.Add(new Fan("System Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Auxiliary Fan", 2));
                        f.Add(new Fan("CPU Fan #2", 3));
                        f.Add(new Fan("Auxiliary Fan #2", 4));
                        c.Add(new ControlChannel("System Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Auxiliary Fan", 2));

                        break;
                }

                break;

            default:
                v.Add(new Voltage("Vcore", 0));
                v.Add(new Voltage("Voltage #2", 1, true));
                v.Add(new Voltage("AVCC", 2, 34, 34));
                v.Add(new Voltage("+3.3V", 3, 34, 34));
                v.Add(new Voltage("Voltage #5", 4, true));
                v.Add(new Voltage("Voltage #6", 5, true));
                v.Add(new Voltage("Voltage #7", 6, true));
                v.Add(new Voltage("+3V Standby", 7, 34, 34));
                v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                v.Add(new Voltage("Voltage #10", 9, true));
                t.Add(new Temperature("CPU", 0));
                t.Add(new Temperature("Auxiliary", 1));
                t.Add(new Temperature("System", 2));
                f.Add(new Fan("System Fan", 0));
                f.Add(new Fan("CPU Fan", 1));
                f.Add(new Fan("Auxiliary Fan", 2));
                f.Add(new Fan("CPU Fan #2", 3));
                f.Add(new Fan("Auxiliary Fan #2", 4));
                c.Add(new ControlChannel("System Fan", 0));
                c.Add(new ControlChannel("CPU Fan", 1));
                c.Add(new ControlChannel("Auxiliary Fan", 2));

                break;
        }
    }

    private static void GetWinbondConfigurationHg(Manufacturer manufacturer, Model model, IList<Voltage> v, IList<Temperature> t, IList<Fan> f, IList<ControlChannel> c)
    {
        switch (manufacturer)
        {
            case Manufacturer.ASRock:
                switch (model)
                {
                    case Model._880GMH_USB3: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+5V", 5, 15, 7.5f));
                        v.Add(new Voltage("+12V", 6, 56, 10));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 2));
                        f.Add(new Fan("Chassis Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Power Fan", 2));
                        c.Add(new ControlChannel("System Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Auxiliary Fan", 2));

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Auxiliary", 1));
                        t.Add(new Temperature("System", 2));
                        f.Add(new Fan("System Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Auxiliary Fan", 2));
                        f.Add(new Fan("CPU Fan #2", 3));
                        f.Add(new Fan("Auxiliary Fan #2", 4));
                        c.Add(new ControlChannel("System Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Auxiliary Fan", 2));

                        break;
                }

                break;
            case Manufacturer.ASUS:
                switch (model)
                {
                    case Model.P6T: 
                    case Model.P6X58D_E: 
                    case Model.RAMPAGE_II_GENE: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+12V", 1, 11.5f, 1.91f));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+5V", 4, 15, 7.5f));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 2));
                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Power Fan", 2));
                        f.Add(new Fan("Chassis Fan #2", 3));
                        f.Add(new Fan("Chassis Fan #3", 4));
                        c.Add(new ControlChannel("System Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Auxiliary Fan", 2));

                        break;

                    case Model.RAMPAGE_EXTREME: 
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("+12V", 1, 12, 2));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("+5V", 4, 15, 7.5f));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Motherboard", 2));
                        f.Add(new Fan("Chassis Fan #1", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Power Fan", 2));
                        f.Add(new Fan("Chassis Fan #2", 3));
                        f.Add(new Fan("Chassis Fan #3", 4));
                        c.Add(new ControlChannel("System Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Auxiliary Fan", 2));

                        break;

                    default:
                        v.Add(new Voltage("Vcore", 0));
                        v.Add(new Voltage("Voltage #2", 1, true));
                        v.Add(new Voltage("AVCC", 2, 34, 34));
                        v.Add(new Voltage("+3.3V", 3, 34, 34));
                        v.Add(new Voltage("Voltage #5", 4, true));
                        v.Add(new Voltage("Voltage #6", 5, true));
                        v.Add(new Voltage("Voltage #7", 6, true));
                        v.Add(new Voltage("+3V Standby", 7, 34, 34));
                        v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                        t.Add(new Temperature("CPU", 0));
                        t.Add(new Temperature("Auxiliary", 1));
                        t.Add(new Temperature("System", 2));
                        f.Add(new Fan("System Fan", 0));
                        f.Add(new Fan("CPU Fan", 1));
                        f.Add(new Fan("Auxiliary Fan", 2));
                        f.Add(new Fan("CPU Fan #2", 3));
                        f.Add(new Fan("Auxiliary Fan #2", 4));
                        c.Add(new ControlChannel("System Fan", 0));
                        c.Add(new ControlChannel("CPU Fan", 1));
                        c.Add(new ControlChannel("Auxiliary Fan", 2));

                        break;
                }

                break;

            default:
                v.Add(new Voltage("Vcore", 0));
                v.Add(new Voltage("Voltage #2", 1, true));
                v.Add(new Voltage("AVCC", 2, 34, 34));
                v.Add(new Voltage("+3.3V", 3, 34, 34));
                v.Add(new Voltage("Voltage #5", 4, true));
                v.Add(new Voltage("Voltage #6", 5, true));
                v.Add(new Voltage("Voltage #7", 6, true));
                v.Add(new Voltage("+3V Standby", 7, 34, 34));
                v.Add(new Voltage("CMOS Battery", 8, 34, 34));
                t.Add(new Temperature("CPU", 0));
                t.Add(new Temperature("Auxiliary", 1));
                t.Add(new Temperature("System", 2));
                f.Add(new Fan("System Fan", 0));
                f.Add(new Fan("CPU Fan", 1));
                f.Add(new Fan("Auxiliary Fan", 2));
                f.Add(new Fan("CPU Fan #2", 3));
                f.Add(new Fan("Auxiliary Fan #2", 4));
                c.Add(new ControlChannel("System Fan", 0));
                c.Add(new ControlChannel("CPU Fan", 1));
                c.Add(new ControlChannel("Auxiliary Fan", 2));

                break;
        }
    }

    public override string GetReport()
    {
        StringBuilder sb = new();
        sb.AppendLine(_superIO.GetReport());
        sb.AppendLine();
        sb.AppendLine("Temperature debug (SuperIO read):");

        try
        {
            
            if (_temperatures != null && _temperatures.Count > 0)
            {
                foreach (Sensor s in _temperatures)
                {
                    int idx = s.Index;
                    float? v = _readTemperature != null ? _readTemperature(idx) : null;
                    sb.Append("  Index ").Append(idx).Append(" \"").Append(s.Name).Append("\": ");
                    sb.AppendLine(v.HasValue ? v.Value.ToString("F2", CultureInfo.InvariantCulture) : "null");
                }
            }
            else
            {
                
                var temps = (_superIO?.Temperatures);
                if (temps != null)
                {
                    for (int i = 0; i < temps.Length; i++)
                    {
                        sb.Append("  Raw Index ").Append(i).Append(": ");
                        sb.AppendLine(temps[i].HasValue ? temps[i].Value.ToString("F2", CultureInfo.InvariantCulture) : "null");
                    }
                }
            }
        }
        catch (Exception ex)
        {
            sb.AppendLine("  Exception while reading temperatures: " + ex.Message);
        }

        return sb.ToString();
    }

    public override void Update()
    {
        _superIO.Update();

        foreach (Sensor sensor in _voltages)
        {
            float? value = _readVoltage(sensor.Index);
            if (value.HasValue)
            {
                sensor.Value = value + ((value - sensor.Parameters[2].Value) * sensor.Parameters[0].Value / sensor.Parameters[1].Value);
                ActivateSensor(sensor);
            }
        }

        foreach (Sensor sensor in _temperatures)
        {
            float? value = _readTemperature(sensor.Index);
            if (value.HasValue)
            {
                sensor.Value = value + sensor.Parameters[0].Value;
                ActivateSensor(sensor);
            }
            else
            {
                if (_motherboard.Model == Model.X570_MS7C35) 
                {
                    float? voltage = null;

                    if (sensor.Index == 24)
                    {
                        voltage = _readVoltage(6);
                    }
                    else if (sensor.Index == 25)
                    {
                        voltage = _readVoltage(11);
                    }

                    if (!voltage.HasValue || voltage.Value <= 0)
                        continue;

                    double R = (10000.0 * voltage.Value / (2.048 - 1.0));            
                    double T = ((298.15 * 3435.0) / ((298.15 * Math.Log(R / 10000.0)) + 3435.0));  
                    float Tc = (float)(T - 273.15);                            

                    sensor.Value = Tc + sensor.Parameters[0].Value;
                    ActivateSensor(sensor);
                }
            }
        }

        foreach (Sensor sensor in _fans)
        {
            float? value = _readFan(sensor.Index);
            if (value.HasValue)
            {
                sensor.Value = value;
                ActivateSensor(sensor);
            }
        }

        foreach (Sensor sensor in _controls)
        {
            sensor.Value = _readControl(sensor.Index);
        }

        _postUpdate();
    }

    public override void Close()
    {
        foreach (Sensor sensor in _controls)
        {
            
            _superIO.SetControl(sensor.Index, null);
        }

        base.Close();
    }

    private delegate float? ReadValueDelegate(int index);

    private delegate void UpdateDelegate();
}
