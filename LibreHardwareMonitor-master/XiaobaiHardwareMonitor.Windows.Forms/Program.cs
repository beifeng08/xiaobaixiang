// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

﻿

using System;
using System.IO;
using System.Windows.Forms;
using XiaobaiHardwareMonitor.Windows.Forms.UI;

namespace XiaobaiHardwareMonitor.Windows.Forms;

public static class Program
{
    [STAThread]
    public static void Main()
    {
        if (!AllRequiredFilesAvailable())
            Environment.Exit(0);

        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        using (MainForm form = new MainForm())
        {
            form.FormClosed += delegate
            {
                Application.Exit();
            };
            Application.Run();
        }
    }

    private static bool IsFileAvailable(string fileName)
    {
        string path = Path.GetDirectoryName(Application.ExecutablePath) + Path.DirectorySeparatorChar;
        if (!File.Exists(path + fileName))
        {
            MessageBox.Show("The following file could not be found: " + fileName +
                            "\nPlease extract all files from the archive.", "Error",
                            MessageBoxButtons.OK, MessageBoxIcon.Error);
            return false;
        }
        return true;
    }

    private static bool AllRequiredFilesAvailable()
    {
        if (!IsFileAvailable("Aga.Controls.dll"))
            return false;

        if (!IsFileAvailable("XiaobaiHardwareMonitorLib.dll"))
            return false;

        if (!IsFileAvailable("OxyPlot.dll"))
            return false;

        if (!IsFileAvailable("OxyPlot.WindowsForms.dll"))
            return false;

        return true;
    }
}
