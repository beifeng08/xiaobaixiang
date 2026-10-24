// Copyright (c) 2026 OmniSensor

using System.Windows.Controls;
using YanJi.PluginSdk;

namespace YanJi.Plugin.KeyboardTest;

public class KeyboardTestPlugin : IYanJiPlugin
{
    public PluginInfo Info => new("keyboard", "键盘测试", "1.0.0", 20, false);
    public UserControl CreatePage(IHostContext host) => new KeyboardTestPage();
}
