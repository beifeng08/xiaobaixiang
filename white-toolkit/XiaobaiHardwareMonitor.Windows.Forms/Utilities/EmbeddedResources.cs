// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

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
