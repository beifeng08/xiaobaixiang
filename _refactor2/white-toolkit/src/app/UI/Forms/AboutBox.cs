// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

﻿

using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;
using SysCore.Windows.Forms.UI;

namespace SysCore.Windows.Forms.UI;

public sealed partial class AboutBox : Form
{
    public AboutBox()
    {
        InitializeComponent();
        Font = SystemFonts.MessageBoxFont;
        label3.Text = "Version " + Application.ProductVersion;
        projectLinkLabel.Links.Remove(projectLinkLabel.Links[0]);
        projectLinkLabel.Links.Add(0, projectLinkLabel.Text.Length, "");
        licenseLinkLabel.Links.Remove(licenseLinkLabel.Links[0]);
        licenseLinkLabel.Links.Add(0, licenseLinkLabel.Text.Length, "");
        Theme.Current.Apply(this);
    }

    private void LinkLabel_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
        try
        {
            Process.Start(new ProcessStartInfo(e.Link.LinkData.ToString()));
        }
        catch { }
    }
}
