// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System.Runtime.InteropServices;

namespace SysCore.Interop.PowerMonitor;

[StructLayout(LayoutKind.Sequential, Pack = 4)]
public struct VendorDataStruct
{
    public byte VendorId;
    public byte ProductId;
    public byte FwVersion;
}

[StructLayout(LayoutKind.Sequential, Pack = 4)]
public struct PowerSensor
{
    public short Voltage;
    public uint Current;
    public uint Power;
}

[StructLayout(LayoutKind.Sequential, Pack = 4)]
public struct SensorStruct
{
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
    public short[] Ts; 

    public ushort Vdd; 
    public byte FanDuty; 

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 6)]
    public PowerSensor[] PowerReadings;

    public uint TotalPower; 
    public uint TotalCurrent; 
    public ushort AvgVoltage; 
    public HpwrCapability HpwrCapability; 
    public ushort FaultStatus;
    public ushort FaultLog;
}

[StructLayout(LayoutKind.Sequential, Pack = 4)]
public struct FanConfigStruct
{
    public FanMode Mode;
    public TempSource TempSource;
    public byte DutyMin;
    public byte DutyMax;
    public short TempMin;
    public short TempMax;
}

[StructLayout(LayoutKind.Sequential, Pack = 4)]
public struct UiConfigStructV1
{
    public CurrentScale CurrentScale;
    public PowerScale PowerScale;
    public Theme Theme;
    public DisplayRotation DisplayRotation;
    public TimeoutMode TimeoutMode;
    public byte CycleScreens;
    public byte CycleTime;
    public byte Timeout;
}

[StructLayout(LayoutKind.Sequential, Pack = 4)]
public struct UiConfigStructV2
{
    public Screen DefaultScreen;
    public CurrentScale CurrentScale;
    public PowerScale PowerScale;
    public DisplayRotation DisplayRotation;
    public TimeoutMode TimeoutMode;
    public byte CycleScreens; 
    public byte CycleTime; 
    public byte Timeout; 
    public uint PrimaryColor;
    public uint SecondaryColor;
    public uint HighlightColor;
    public uint BackgroundColor;
    public byte BackgroundBitmapId;
    public byte FanBitmapId;
    public DISPLAY_INVERSION DisplayInversion;
}

[StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Ansi)]
public struct DeviceConfigStructV1
{
    public ushort Crc;
    public byte Version;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
    public byte[] FriendlyName;

    public FanConfigStruct FanConfig;
    public byte BacklightDuty;

    public ushort FaultDisplayEnable;
    public ushort FaultBuzzerEnable;
    public ushort FaultSoftPowerEnable;
    public ushort FaultHardPowerEnable;
    public short TsFaultThreshold; 
    public byte OcpFaultThreshold; 
    public byte WireOcpFaultThreshold; 
    public ushort OppFaultThreshold; 
    public byte CurrentImbalanceFaultThreshold; 
    public byte CurrentImbalanceFaultMinLoad; 
    public byte ShutdownWaitTime; 
    public byte LoggingInterval; 
    public UiConfigStructV1 Ui;
}

[StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Ansi)]
public struct DeviceConfigStructV2
{
    public ushort Crc;
    public byte Version;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
    public byte[] FriendlyName;

    public FanConfigStruct FanConfig;
    public byte BacklightDuty;

    public ushort FaultDisplayEnable;
    public ushort FaultBuzzerEnable;
    public ushort FaultSoftPowerEnable;
    public ushort FaultHardPowerEnable;
    public short TsFaultThreshold; 
    public byte OcpFaultThreshold; 
    public byte WireOcpFaultThreshold; 
    public ushort OppFaultThreshold; 
    public byte CurrentImbalanceFaultThreshold; 
    public byte CurrentImbalanceFaultMinLoad; 
    public byte ShutdownWaitTime; 
    public byte LoggingInterval; 
    public AVG Average;
    public UiConfigStructV1 Ui;
}

[StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Ansi)]
public struct DeviceConfigStructV3
{
    public ushort Crc;
    public byte Version;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
    public byte[] FriendlyName;

    public FanConfigStruct FanConfig;
    public byte BacklightDuty;

    public ushort FaultDisplayEnable;
    public ushort FaultBuzzerEnable;
    public ushort FaultSoftPowerEnable;
    public ushort FaultHardPowerEnable;
    public short TsFaultThreshold; 
    public byte OcpFaultThreshold; 
    public byte WireOcpFaultThreshold; 
    public ushort OppFaultThreshold; 
    public byte CurrentImbalanceFaultThreshold; 
    public byte CurrentImbalanceFaultMinLoad; 
    public byte ShutdownWaitTime; 
    public byte LoggingInterval; 
    public AVG Average;
    public UiConfigStructV2 Ui;
}
