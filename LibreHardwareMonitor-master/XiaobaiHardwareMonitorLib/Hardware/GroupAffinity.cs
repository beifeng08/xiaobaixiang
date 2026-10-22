// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace XiaobaiHardwareMonitor.Hardware;

public readonly struct GroupAffinity
{
    public static GroupAffinity Undefined = new(ushort.MaxValue, 0);

    public GroupAffinity(ushort group, ulong mask)
    {
        Group = group;
        Mask = mask;
    }

    public static GroupAffinity Single(ushort group, int index)
    {
        return new GroupAffinity(group, 1UL << index);
    }

    public ushort Group { get; }

    public ulong Mask { get; }

    public override bool Equals(object o)
    {
        if (o == null || GetType() != o.GetType())
            return false;

        GroupAffinity a = (GroupAffinity)o;
        return (Group == a.Group) && (Mask == a.Mask);
    }

    public override int GetHashCode()
    {
        return Group.GetHashCode() ^ Mask.GetHashCode();
    }

    public static bool operator ==(GroupAffinity a1, GroupAffinity a2)
    {
        return (a1.Group == a2.Group) && (a1.Mask == a2.Mask);
    }

    public static bool operator !=(GroupAffinity a1, GroupAffinity a2)
    {
        return (a1.Group != a2.Group) || (a1.Mask != a2.Mask);
    }
}