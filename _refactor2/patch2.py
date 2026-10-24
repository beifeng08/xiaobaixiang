# -*- coding: utf-8 -*-
"""工程文件路径补丁"""
import os

R = r'F:/toolkit/_refactor2/white-toolkit'

def rd(p):
    b = open(p, 'rb').read()
    return b.decode('utf-8-sig'), b.startswith(b'\xef\xbb\xbf')

def patch(rel, pairs):
    p = os.path.join(R, rel)
    t, bom = rd(p)
    o = t
    for a, b in pairs:
        t = t.replace(a, b)
    if t != o:
        open(p, 'wb').write((b'\xef\xbb\xbf' if bom else b'') + t.encode('utf-8'))
        print('updated', rel)

patch('src/core/XiaobaiHardwareMonitorLib.csproj', [('..\\bin\\', '..\\..\\bin\\')])
patch('src/core/main.nuget.target', [('..\\bin\\', '..\\..\\bin\\')])
patch('src/third-party/treeview-adv/Aga.Controls.csproj',
      [('..\\bin\\$(Configuration)\\', '..\\..\\..\\bin\\$(Configuration)\\')])

app = [
    ('..\\bin\\$(Configuration)\\', '..\\..\\bin\\$(Configuration)\\'),
    ('..\\Aga.Controls\\Aga.Controls.csproj', '..\\third-party\\treeview-adv\\Aga.Controls.csproj'),
    ('..\\XiaobaiHardwareMonitorLib\\XiaobaiHardwareMonitorLib.csproj', '..\\core\\XiaobaiHardwareMonitorLib.csproj'),
    ('<StartupObject>XiaobaiHardwareMonitor.Windows.Forms.Program</StartupObject>',
     '<StartupObject>SysCore.Windows.Forms.Program</StartupObject>'),
]
for n in ['AboutBox', 'AuthForm', 'InterfacePortForm', 'MainForm', 'ParameterForm']:
    app.append(('UI\\%s.' % n, 'UI\\Forms\\%s.' % n))
patch('src/app/XiaobaiHardwareMonitor.Windows.Forms.csproj', app)

patch('XiaobaiHardwareMonitor.sln', [
    ('"XiaobaiHardwareMonitorLib\\XiaobaiHardwareMonitorLib.csproj"', '"src\\core\\XiaobaiHardwareMonitorLib.csproj"'),
    ('"XiaobaiHardwareMonitor.Windows.Forms\\XiaobaiHardwareMonitor.Windows.Forms.csproj"',
     '"src\\app\\XiaobaiHardwareMonitor.Windows.Forms.csproj"'),
    ('"Aga.Controls\\Aga.Controls.csproj"', '"src\\third-party\\treeview-adv\\Aga.Controls.csproj"'),
])
