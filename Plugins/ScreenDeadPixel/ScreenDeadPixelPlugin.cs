// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Windows.Controls;
using YanJi.PluginSdk;

namespace YanJi.Plugin.ScreenDeadPixel;

public class ScreenDeadPixelPlugin : IYanJiPlugin
{
    public PluginInfo Info => new("screen", "屏幕坏点", "1.0.0", 30, false);
    public UserControl CreatePage(IHostContext host) => new DeadPixelPage();
}
