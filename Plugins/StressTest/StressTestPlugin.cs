// Copyright (c) 2026 OmniSensor

using System.Windows.Controls;
using YanJi.PluginSdk;

namespace YanJi.Plugin.StressTest;

public class StressTestPlugin : IYanJiPlugin, IPluginShutdown
{
    public PluginInfo Info => new("stress", "散热测试", "1.0.0", 50, false);

    public IReadOnlyList<PluginCredit> Credits =>
    [
        new("SysCorePrime（基于 GIMPS Prime95, George Woltman）", "CPU 烤机（历史负载；精确源码/构建对应待核实，见许可库存）", "GIMPS freeware（随附 license.txt）"),
        new("FurMark（Python 版, StanislavPetrovV）", "GPU 烤机渲染原型（C# / OpenTK 重实现，着色器与贴图随附其许可）", "MIT"),
        new("OpenTK 4.x", "GPU 烤机的 OpenGL 窗口与绑定", "MIT；内含第三方材料见完整通知"),
        new("GLFW（OpenTK.redist.glfw）", "OpenTK 原生窗口依赖", "Zlib"),
        new("StbImageSharp", "GPU 烤机贴图解码（stb_image 的 C# 移植）", "MIT / 公有领域双选，保留原文"),
        new("LibreHardwareMonitor / Contributors（本地改名 SysCore）", "传感器监控（温度/功耗采集）", "MPL-2.0；另有文件级 GNU-v2 通知，见许可库存及原文"),
        new("PawnIO（namazso）", "CPU/主板传感器驱动；历史安装器精确来源与相应源码供应待核实", "GPL-2.0-or-later + 设备 I/O 例外，见许可库存"),
        new("PawnIO 嵌入模块", "采集库所带模块；各模块精确构建与源码对应待核实", "既有 LGPL-2.1 通知；逐模块适用性见许可库存"),
    ];

    public UserControl CreatePage(IHostContext host) => new StressTestPage();
    public void Shutdown()
    {
        try { StressTestService.Stop(); }
        finally { SensorMonitorService.Stop(); }
    }
}
