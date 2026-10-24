// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Collections.Generic;
using System.Linq;
using DiskInfoToolkit;
using StorageDIT = DiskInfoToolkit.Storage;

namespace SysCore.Hardware.Storage;

internal class StorageGroup : IGroup, IHardwareChanged
{
    private readonly List<StorageDevice> _hardware = new();

    private readonly ISettings _settings;

    public event HardwareEventHandler HardwareAdded;
    public event HardwareEventHandler HardwareRemoved;

    public StorageGroup(ISettings settings)
    {
        _settings = settings;

        AddHardware(settings);
    }

    public IReadOnlyList<IHardware> Hardware => _hardware;

    private void AddHardware(ISettings settings)
    {
        StorageDIT.DevicesChanged -= OnStoragesChanged;

        var disks = StorageDIT.GetDisks();

        _hardware.AddRange(disks.Select(s => new StorageDevice(s, settings)));

        StorageDIT.DevicesChanged += OnStoragesChanged;
    }

    private void OnStoragesChanged(object sender, StorageDevicesChangedEventArgs e)
    {
        foreach (var added in e.Added)
        {
            var storageDevice = new StorageDevice(added, _settings);

            _hardware.Add(storageDevice);
            HardwareAdded?.Invoke(storageDevice);
        }

        foreach (var removed in e.Removed)
        {
            var storageDevice = _hardware.Find(sd => sd.Storage == removed);
            if (storageDevice != null)
            {
                _hardware.Remove(storageDevice);
                HardwareRemoved?.Invoke(storageDevice);
            }
        }
    }

    public void Close()
    {
        StorageDIT.DevicesChanged -= OnStoragesChanged;

        foreach (var hardware in _hardware)
        {
            hardware.Close();
        }
    }

    public string GetReport() => null;
}
