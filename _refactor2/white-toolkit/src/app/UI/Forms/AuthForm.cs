// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

﻿

using System;
using System.Windows.Forms;

namespace SysCore.Windows.Forms.UI;

public partial class AuthForm : Form
{
    private readonly MainForm _parent;

    public AuthForm(MainForm m)
    {
        InitializeComponent();
        _parent = m;
    }

    private void AuthForm_Load(object sender, EventArgs e)
    {
        httpAuthUsernameTextBox.Enabled = httpAuthPasswordTextBox.Enabled = enableHTTPAuthCheckBox.Checked = _parent.Server.AuthEnabled;
        httpAuthUsernameTextBox.Text = _parent.Server.UserName;
    }

    private void HttpAuthCancelButton_Click(object sender, EventArgs e)
    {
        Close();
    }

    private void HttpAuthOkButton_Click(object sender, EventArgs e)
    {
        _parent.Server.UserName = httpAuthUsernameTextBox.Text;
        if (!string.IsNullOrWhiteSpace(httpAuthPasswordTextBox.Text))
        {
            _parent.Server.SetPassword(httpAuthPasswordTextBox.Text);
        }
        _parent.Server.AuthEnabled = enableHTTPAuthCheckBox.Checked;
        _parent.AuthWebServerMenuItemChecked = _parent.Server.AuthEnabled;
        Close();
    }

    private void EnableHTTPAuthCheckBox_CheckedChanged(object sender, EventArgs e)
    {
        httpAuthUsernameTextBox.Enabled = httpAuthPasswordTextBox.Enabled = enableHTTPAuthCheckBox.Checked;
    }
}
