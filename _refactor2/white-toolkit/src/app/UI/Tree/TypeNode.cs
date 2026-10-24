// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
using SysCore.Windows.Forms.Infrastructure;

using SysCore.Sensors;
namespace SysCore.Windows.Forms.UI;

public sealed class TypeNode : Node, IExpandPersistNode
{
    private readonly PersistentSettings _settings;
    private readonly string _expandedIdentifier;
    private bool _expanded;

    public TypeNode(SensorType sensorType, Identifier parentId, PersistentSettings settings)
    {
        SensorType = sensorType;
        _expandedIdentifier = new Identifier(parentId, SensorType.ToString(), ".expanded").ToString();
        _settings = settings;

        switch (sensorType)
        {
            case SensorType.Voltage:
                Image = Infrastructure.EmbeddedResources.GetImage("voltage.png");
                Text = "Voltages";
                break;
            case SensorType.Current:
                Image = Infrastructure.EmbeddedResources.GetImage("voltage.png");
                Text = "Currents";
                break;
            case SensorType.Energy:
                Image = Infrastructure.EmbeddedResources.GetImage("battery.png");
                Text = "Capacities";
                break;
            case SensorType.Clock:
                Image = Infrastructure.EmbeddedResources.GetImage("clock.png");
                Text = "Clocks";
                break;
            case SensorType.Load:
                Image = Infrastructure.EmbeddedResources.GetImage("load.png");
                Text = "Load";
                break;
            case SensorType.Temperature:
                Image = Infrastructure.EmbeddedResources.GetImage("temperature.png");
                Text = "Temperatures";
                break;
            case SensorType.Fan:
                Image = Infrastructure.EmbeddedResources.GetImage("fan.png");
                Text = "Fans";
                break;
            case SensorType.Flow:
                Image = Infrastructure.EmbeddedResources.GetImage("flow.png");
                Text = "Flows";
                break;
            case SensorType.Control:
                Image = Infrastructure.EmbeddedResources.GetImage("control.png");
                Text = "Controls";
                break;
            case SensorType.Level:
                Image = Infrastructure.EmbeddedResources.GetImage("level.png");
                Text = "Levels";
                break;
            case SensorType.Power:
                Image = Infrastructure.EmbeddedResources.GetImage("power.png");
                Text = "Powers";
                break;
            case SensorType.Data:
                Image = Infrastructure.EmbeddedResources.GetImage("data.png");
                Text = "Data";
                break;
            case SensorType.SmallData:
                Image = Infrastructure.EmbeddedResources.GetImage("data.png");
                Text = "Data";
                break;
            case SensorType.Factor:
                Image = Infrastructure.EmbeddedResources.GetImage("factor.png");
                Text = "Factors";
                break;
            case SensorType.Frequency:
                Image = Infrastructure.EmbeddedResources.GetImage("clock.png");
                Text = "Frequencies";
                break;
            case SensorType.Throughput:
                Image = Infrastructure.EmbeddedResources.GetImage("throughput.png");
                Text = "Throughput";
                break;
            case SensorType.TimeSpan:
                Image = Infrastructure.EmbeddedResources.GetImage("time.png");
                Text = "Times";
                break;
            case SensorType.Timing:
                Image = Infrastructure.EmbeddedResources.GetImage("time.png");
                Text = "Timings";
                break;
            case SensorType.Noise:
                Image = Infrastructure.EmbeddedResources.GetImage("loudspeaker.png");
                Text = "Noise Levels";
                break;
            case SensorType.Conductivity:
                Image = Infrastructure.EmbeddedResources.GetImage("voltage.png");
                Text = "Conductivities";
                break;
            case SensorType.Humidity:
                Image = Infrastructure.EmbeddedResources.GetImage("humidity.png");
                Text = "Humidity Levels";
                break;
        }

        NodeAdded += TypeNode_NodeAdded;
        NodeRemoved += TypeNode_NodeRemoved;
        _expanded = settings.GetValue(_expandedIdentifier, true);
    }

    private void TypeNode_NodeRemoved(Node node)
    {
        node.IsVisibleChanged -= Node_IsVisibleChanged;
        Node_IsVisibleChanged(null);
    }

    private void TypeNode_NodeAdded(Node node)
    {
        node.IsVisibleChanged += Node_IsVisibleChanged;
        Node_IsVisibleChanged(null);
    }

    private void Node_IsVisibleChanged(Node node)
    {
        foreach (Node n in Nodes)
        {
            if (n.IsVisible)
            {
                IsVisible = true;
                return;
            }
        }
        IsVisible = false;
    }

    public SensorType SensorType { get; }

    public bool Expanded
    {
        get => _expanded;
        set
        {
            _expanded = value;
            _settings.SetValue(_expandedIdentifier, _expanded);
        }
    }
}
