// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using SysCore.Hardware.Motherboard.Lpc;
using SysCore.Hardware.Motherboard.Lpc.EC;
using OperatingSystem = SysCore.Software.OperatingSystem;

namespace SysCore.Hardware.Motherboard;

public class Motherboard : IHardware
{
    private readonly LMSensors _lmSensors;
    private readonly LpcIO _lpcIO;
    private readonly string _name;
    private readonly ISettings _settings;
    private string _customName;

    public Motherboard(SMBios smBios, ISettings settings)
    {
        IReadOnlyList<ISuperIO> superIO;
        _settings = settings;
        SMBios = smBios;

        Manufacturer = smBios.Board == null ? Manufacturer.Unknown : Identification.GetManufacturer(smBios.Board.ManufacturerName);
        Model = smBios.Board == null ? Model.Unknown : Identification.GetModel(smBios.Board.ProductName);

        if (smBios.Board != null)
        {
            if (!string.IsNullOrEmpty(smBios.Board.ProductName))
            {
                if (Manufacturer == Manufacturer.Unknown)
                    _name = smBios.Board.ProductName;
                else
                    _name = Manufacturer + " " + smBios.Board.ProductName;
            }
            else
            {
                _name = Manufacturer.ToString();
            }
        }
        else
        {
            _name = nameof(Manufacturer.Unknown);
        }

        _customName = settings.GetValue(new Identifier(Identifier, "name").ToString(), _name);

        if (OperatingSystem.IsUnix)
        {
            _lmSensors = new LMSensors();
            superIO = _lmSensors.SuperIO;
        }
        else
        {
            _lpcIO = new LpcIO(this);
            superIO = _lpcIO.SuperIO;
        }

        EmbeddedController embeddedController = EmbeddedController.Create(Model, settings);

        List<IHardware> subHardwareList = new List<IHardware>();

        foreach (IGrouping<Chip, ISuperIO> group in superIO.GroupBy(x => x.Chip))
        {
            
            foreach ((ISuperIO superIo, int i) in group.Select((x, i) => (x, i)))
            {
                subHardwareList.Add(new SuperIOHardware(this, superIo, Manufacturer, Model, settings, i));
            }
        }

        if (embeddedController != null)
            subHardwareList.Add(embeddedController);

        SubHardware = subHardwareList.ToArray();
    }

#pragma warning disable 67
    
    public event SensorEventHandler SensorAdded;

    public event SensorEventHandler SensorRemoved;
#pragma warning restore 67

    public HardwareType HardwareType => HardwareType.Motherboard;

    public Identifier Identifier => new("motherboard");

    public Manufacturer Manufacturer { get; }

    public Model Model { get; }

    public string Name
    {
        get { return _customName; }
        set
        {
            _customName = !string.IsNullOrEmpty(value) ? value : _name;

            _settings.SetValue(new Identifier(Identifier, "name").ToString(), _customName);
        }
    }

    public virtual IHardware Parent
    {
        get { return null; }
    }

    public virtual IDictionary<string, string> Properties => new SortedDictionary<string, string>();

    public ISensor[] Sensors
    {
        get { return Array.Empty<ISensor>(); }
    }

    public SMBios SMBios { get; }

    public IHardware[] SubHardware { get; }

    public string GetReport()
    {
        StringBuilder r = new();

        r.AppendLine("Motherboard");
        r.AppendLine();
        r.Append(SMBios.GetReport());

        if (_lpcIO != null)
            r.Append(_lpcIO.GetReport());

        return r.ToString();
    }

    public void Update()
    { }

    public void Accept(IVisitor visitor)
    {
        if (visitor == null)
            throw new ArgumentNullException(nameof(visitor));

        visitor.VisitHardware(this);
    }

    public void Traverse(IVisitor visitor)
    {
        foreach (IHardware hardware in SubHardware)
            hardware.Accept(visitor);
    }

    public void Close()
    {
        foreach (IHardware iHardware in SubHardware)
        {
            if (iHardware is Hardware hardware)
                hardware.Close();
        }

        _lmSensors?.Close();
        _lpcIO?.Close();
    }
}
