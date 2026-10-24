// Copyright (c) 2026 OmniSensor

namespace YanJi.Plugin.ConfigCheck;

public record InfoEntry(string Name, string Value);

public class InfoGroup(string title)
{
    public string Title { get; } = title;
    public List<InfoEntry> Entries { get; } = [];
    
    public List<InfoEntry> Details { get; } = [];
    public void Add(string name, string value) => Entries.Add(new InfoEntry(name, value));
    public void AddDetail(string name, string value) => Details.Add(new InfoEntry(name, value));
}

public class HardwareReport
{
    public DateTime CollectedAt { get; } = DateTime.Now;
    public List<InfoGroup> Groups { get; } = [];
}
