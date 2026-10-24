// Copyright (c) 2026 OmniSensor

using System.Windows.Controls;
using YanJi.PluginSdk;

namespace YanJi.Plugin.ScreenDeadPixel;

public class ScreenDeadPixelPlugin : IYanJiPlugin
{
    public PluginInfo Info => new("screen", "屏幕坏点", "1.0.0", 30, false);
    public UserControl CreatePage(IHostContext host) => new DeadPixelPage();
}
