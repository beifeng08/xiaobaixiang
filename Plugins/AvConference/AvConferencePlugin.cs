// Copyright (c) 2026 OmniSensor

using System.Windows.Controls;
using YanJi.PluginSdk;

namespace YanJi.Plugin.AvConference;

public class AvConferencePlugin : IYanJiPlugin
{
    public PluginInfo Info => new("av", "影音会议", "1.0.0", 40, false);

    public IReadOnlyList<PluginCredit> Credits =>
    [
        new("NAudio 2.2.1", "麦克风/扬声器（WASAPI 设备枚举、电平、录制、播放）", "MIT"),
    ];

    public UserControl CreatePage(IHostContext host) => new AvConferencePage();
}
