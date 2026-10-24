// Copyright (c) 2026 OmniSensor

using System.Windows.Controls;
using YanJi.PluginSdk;

namespace YanJi.Plugin.ConfigCheck;

public class ConfigCheckPlugin : IYanJiPlugin, IPluginShutdown
{
    ConfigCheckPage? _page;
    public PluginInfo Info => new("config", "配置核对", "1.0.0", 10, false);

    public IReadOnlyList<PluginCredit> Credits =>
    [
        new("LibreHardwareMonitor / Contributors（本地改名 SysCore）", "硬件信息采集库（源码项目引用，不是独立原创）", "MPL-2.0；另有文件级 GNU-v2 通知，见许可库存及原文"),
        new("CrystalDiskInfo", "硬盘信息采集的实现参考（SMART 直读思路）", "MIT"),
    ];

    public UserControl CreatePage(IHostContext host) => _page = new ConfigCheckPage();
    public void Shutdown() => _page?.StopCollection();
}
