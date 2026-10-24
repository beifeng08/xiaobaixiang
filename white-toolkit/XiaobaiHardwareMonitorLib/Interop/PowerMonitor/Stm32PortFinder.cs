// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (C) LibreHardwareMonitor and Contributors.
// Partial Copyright (C) Michael Möller <mmoeller@openhardwaremonitor.org> and Contributors.
// All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using Windows.Win32;
using Windows.Win32.Devices.DeviceAndDriverInstallation;
using Windows.Win32.Foundation;

namespace SysCore.Interop.PowerMonitor;

public static class Stm32PortFinder
{
    private const int BufferSize = 1024;

    public static List<string> FindMatchingComPorts(uint vid, uint pid)
    {
        return FindMatchingComPorts($"{vid:X4}", $"{pid:X4}");
    }

    public static List<string> FindMatchingComPorts(string vid, string pid)
    {
        var result = new List<string>();

#if WINDOWS
        
        SetupDiDestroyDeviceInfoListSafeHandle devInfo = PInvoke.SetupDiGetClassDevs(PInvoke.GUID_DEVCLASS_PORTS, null, HWND.Null, SETUP_DI_GET_CLASS_DEVS_FLAGS.DIGCF_PRESENT);

        if (devInfo.IsInvalid)
        {
            return result;
        }

        try
        {
            SP_DEVINFO_DATA devInfoData = new() { cbSize = (uint)Marshal.SizeOf<SP_DEVINFO_DATA>() };

            uint index = 0;
            while (PInvoke.SetupDiEnumDeviceInfo(devInfo, index++, ref devInfoData))
            {
                
                string hwId = GetProperty(devInfo, SETUP_DI_REGISTRY_PROPERTY.SPDRP_HARDWAREID, devInfoData);
                if (string.IsNullOrWhiteSpace(hwId))
                {
                    continue;
                }

                if (!hwId.Contains($"VID_{vid}", StringComparison.OrdinalIgnoreCase) ||
                    !hwId.Contains($"PID_{pid}", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                string friendlyName = GetProperty(devInfo, SETUP_DI_REGISTRY_PROPERTY.SPDRP_FRIENDLYNAME, devInfoData);

                int start = friendlyName.LastIndexOf("(COM", StringComparison.OrdinalIgnoreCase);
                int end = friendlyName.LastIndexOf(')');

                if (start >= 0 && end > start)
                {
                    string com = friendlyName.Substring(start + 1, end - start - 1);
                    result.Add(com);
                }
            }
        }
        finally
        {
            
            devInfo.Dispose();
        }

        static string GetProperty(SafeHandle hDevInfo, SETUP_DI_REGISTRY_PROPERTY property, SP_DEVINFO_DATA devInfoData)
        {
            byte[] buffer = new byte[BufferSize];

            if (PInvoke.SetupDiGetDeviceRegistryProperty(hDevInfo, devInfoData, property, buffer))
            {
                
                return NormalizeString(Encoding.Unicode.GetString(buffer));
            }

            return null;
        }
#endif

        return result;
    }

    private static string NormalizeString(string str)
    {
        int end = str.IndexOf('\0');
        if (end == -1)
        {
            end = 0;
        }

        return str.Substring(0, end);
    }
}
