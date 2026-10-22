// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Text;
using HidSharp;

namespace LibreHardwareMonitor.Hardware;

public class Identifier : IComparable<Identifier>
{
    private const char Separator = '/';
    private readonly string _identifier;

    public Identifier(params string[] identifiers)
    {
        CoerceIdentifiers(identifiers);
        StringBuilder s = new();
        for (int i = 0; i < identifiers.Length; i++)
        {
            s.Append(Separator);
            s.Append(identifiers[i]);
        }

        _identifier = s.ToString();
    }

    public Identifier(Identifier identifier, params string[] extensions)
    {
        CoerceIdentifiers(extensions);
        StringBuilder s = new();
        s.Append(identifier);
        for (int i = 0; i < extensions.Length; i++)
        {
            s.Append(Separator);
            s.Append(extensions[i]);
        }

        _identifier = s.ToString();
    }

    public Identifier(HidDevice dev)
    {
        string[] identifiers;
        try
        {
            identifiers = ["usbhid", dev.VendorID.ToString("X4"), dev.ProductID.ToString("X4"), dev.GetSerialNumber()];
        }
        catch
        {
            identifiers = ["usbhid", dev.DevicePath];
        }

        CoerceIdentifiers(identifiers);
        StringBuilder s = new();
        for (int i = 0; i < identifiers.Length; i++)
        {
            s.Append(Separator);
            s.Append(identifiers[i]);
        }

        _identifier = s.ToString();
    }

    public int CompareTo(Identifier other)
    {
        if (other == null)
            return 1;

        return string.Compare(_identifier,
                              other._identifier,
                              StringComparison.Ordinal);
    }

    private static void CoerceIdentifiers(string[] identifiers)
    {
        for (int i = 0; i < identifiers.Length; i++)
        {
            string s = identifiers[i];
            identifiers[i] = Uri.EscapeDataString(identifiers[i]);
        }
    }

    public override string ToString()
    {
        return _identifier;
    }

    public override bool Equals(object obj)
    {
        if (obj == null)
            return false;

        Identifier id = obj as Identifier;
        if (id == null)
            return false;

        return _identifier == id._identifier;
    }

    public override int GetHashCode()
    {
        return _identifier.GetHashCode();
    }

    public static bool operator ==(Identifier id1, Identifier id2)
    {
        if (id1 is null && id2 is null)
            return true;

        return id1 is not null && id1.Equals(id2);
    }

    public static bool operator !=(Identifier id1, Identifier id2)
    {
        return !(id1 == id2);
    }

    public static bool operator <(Identifier id1, Identifier id2)
    {
        if (id1 == null)
            return id2 != null;

        return id1.CompareTo(id2) < 0;
    }

    public static bool operator >(Identifier id1, Identifier id2)
    {
        if (id1 == null)
            return false;

        return id1.CompareTo(id2) > 0;
    }
}
