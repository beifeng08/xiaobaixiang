// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (C) LibreHardwareMonitor and Contributors.
// Partial Copyright (C) Michael Möller <mmoeller@openhardwaremonitor.org> and Contributors.
// All Rights Reserved.

using System.Drawing;
using System.IO;
using System.Reflection;

namespace SysCore.Windows.Forms.Utilities;

public class EmbeddedResources
{
    const string ResourceNamespace = $"SysCore.Windows.Forms.Resources";

    public static Image GetImage(string name)
    {
        name = $"{ResourceNamespace}." + name;
        string[] names = Assembly.GetExecutingAssembly().GetManifestResourceNames();

        for (int i = 0; i < names.Length; i++)
        {
            if (names[i].Replace('\\', '.') == name)
            {
                using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(names[i]))
                {
                    
                    Image image = Image.FromStream(stream);

                    Bitmap bitmap = new Bitmap(image);

                    image.Dispose();

                    return bitmap;
                }
            }
        }
        return new Bitmap(1, 1);
    }

    public static Icon GetIcon(string name)
    {
        name = $"{ResourceNamespace}." + name;
        string[] names = Assembly.GetExecutingAssembly().GetManifestResourceNames();
        for (int i = 0; i < names.Length; i++)
        {
            if (names[i].Replace('\\', '.') == name)
            {
                using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(names[i]))
                {
                    return new Icon(stream);
                }
            }
        }
        return null;
    }

}
