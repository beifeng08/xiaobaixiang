# -*- coding: utf-8 -*-
"""从工作树重建：统一行尾 -> 按功能域重组 -> 收敛命名空间。全程 \n 空间处理，落盘 \r\n。"""
import os, re, shutil

LIVE = os.path.abspath(r'F:/toolkit/white-toolkit')
OUT = os.path.abspath(r'F:/toolkit/_refactor2/white-toolkit')

CORE_OLD, APP_OLD, AGA_OLD = 'XiaobaiHardwareMonitorLib', 'XiaobaiHardwareMonitor.Windows.Forms', 'Aga.Controls'

if os.path.isdir(OUT):
    shutil.rmtree(OUT)
shutil.copytree(LIVE, OUT, ignore=shutil.ignore_patterns('bin', 'obj'))

# ---------- 1. 行尾归一（跳过第三方） ----------
norm = 0
for dp, dn, fn in os.walk(OUT):
    if AGA_OLD in dp.split(os.sep):
        continue
    for f in fn:
        if not f.endswith('.cs'):
            continue
        p = os.path.join(dp, f)
        raw = open(p, 'rb').read()
        if b'\r\r' in raw or b'\r' in raw:
            fixed = re.sub(rb'\r+\n', b'\n', raw).replace(b'\r', b'\n')
            open(p, 'wb').write(fixed)
            norm += 1
print('行尾归一:', norm)

# ---------- 2. 路径映射 ----------
HW = 'Hardware/'
ROOT_FILE_MAP = {
    HW+'IComputer.cs': 'Contracts/IComputer.cs', HW+'IControl.cs': 'Contracts/IControl.cs',
    HW+'IElement.cs': 'Contracts/IElement.cs', HW+'IGroup.cs': 'Contracts/IGroup.cs',
    HW+'IHardware.cs': 'Contracts/IHardware.cs', HW+'IHardwareChanged.cs': 'Contracts/IHardwareChanged.cs',
    HW+'IParameter.cs': 'Contracts/IParameter.cs', HW+'ISensor.cs': 'Contracts/ISensor.cs',
    HW+'ISensorLimits.cs': 'Contracts/ISensorLimits.cs', HW+'ISettings.cs': 'Contracts/ISettings.cs',
    HW+'IVisitor.cs': 'Contracts/IVisitor.cs',
    HW+'CompositeSensor.cs': 'Model/CompositeSensor.cs', HW+'Control.cs': 'Model/Control.cs',
    HW+'Hardware.cs': 'Model/Hardware.cs', HW+'HardwareType.cs': 'Model/HardwareType.cs',
    HW+'CoreType.cs': 'Model/CoreType.cs', HW+'Identifier.cs': 'Model/Identifier.cs',
    HW+'Parameter.cs': 'Model/Parameter.cs', HW+'ParameterDescription.cs': 'Model/ParameterDescription.cs',
    HW+'Sensor.cs': 'Model/Sensor.cs', HW+'SensorVisitor.cs': 'Model/SensorVisitor.cs',
    HW+'D3DDisplayDevice.cs': 'Platform/D3DDisplayDevice.cs', HW+'FirmwareTable.cs': 'Platform/FirmwareTable.cs',
    HW+'GroupAffinity.cs': 'Platform/GroupAffinity.cs', HW+'Mutexes.cs': 'Platform/Mutexes.cs',
    HW+'OpCode.cs': 'Platform/OpCode.cs', HW+'SMBios.cs': 'Platform/SMBios.cs',
    HW+'ThreadAffinity.cs': 'Platform/ThreadAffinity.cs', 'Software/OperatingSystem.cs': 'Platform/OperatingSystem.cs',
    HW+'Computer.cs': 'Computer.cs',
    HW+'Motherboard/Fan.cs': 'Sensors/Fan/Fan.cs',
    HW+'Motherboard/Voltage.cs': 'Sensors/Voltage/Voltage.cs',
    HW+'Motherboard/Temperature.cs': 'Sensors/Temperature/Temperature.cs',
    HW+'Motherboard/Control.cs': 'Sensors/Control/ControlChannel.cs',
    HW+'Memory/Sensors/SpdThermalSensor.cs': 'Sensors/Temperature/SpdThermalSensor.cs',
    'RAMSPDToolkitDriver.cs': 'Interop/PawnIo/RAMSPDToolkitDriver.cs',
    HW+'RyzenSMU.cs': 'Devices/Cpu/RyzenSMU.cs',
}

def core_map(rel):
    if rel in ROOT_FILE_MAP:
        return ROOT_FILE_MAP[rel]
    for pre, new in (
        ('Hardware/Motherboard/Lpc/EC/', 'Devices/Motherboard/EmbeddedControllers/'),
        ('Hardware/Motherboard/Lpc/', 'Devices/Motherboard/SuperIo/'),
        ('Hardware/Motherboard/', 'Devices/Motherboard/'),
        ('Hardware/Controller/', 'Controllers/'),
        ('Hardware/Psu/', 'Power/'), ('Hardware/PowerMonitor/', 'Power/Monitors/'),
        ('Interop/PowerMonitor/', 'Power/Monitors/Protocol/'),
        ('Interop/', 'Interop/'), ('PawnIo/', 'Interop/PawnIo/'), ('Software/', 'Platform/'),
        ('Hardware/Cpu/', 'Devices/Cpu/'), ('Hardware/Gpu/', 'Devices/Gpu/'),
        ('Hardware/Memory/', 'Devices/Memory/'), ('Hardware/Storage/', 'Devices/Storage/'),
        ('Hardware/Network/', 'Devices/Network/'), ('Hardware/Battery/', 'Devices/Battery/'),
    ):
        if rel.startswith(pre):
            return new + rel[len(pre):]
    return rel

APP_SUB = {}
for sub, names in {
    'Forms': ['AboutBox','AuthForm','InterfacePortForm','MainForm','ParameterForm'],
    'Tree': ['HardwareNode','HardwareTypeImage','IExpandPersistNode','Node','NodeToolTipProvider','SensorNode','TreeModel','TypeNode'],
    'Gadgets': ['Gadget','GadgetWindow','SensorGadget','SensorNotifyIcon','NotifyIconAdv','SystemTray','SplitContainerAdv','ShowDesktop'],
    'Plot': ['PlotPanel','ScaledPlotModel'],
    'Themes': ['BlackTheme','DarkTheme','LightTheme','Theme','ThemedHScrollIndicator','ThemedToolStripRenderer','ThemedVScrollIndicator'],
}.items():
    for n in names:
        APP_SUB[n] = sub

def app_map(rel):
    if rel.startswith('Utilities/'):
        return 'Infrastructure/' + rel[len('Utilities/'):]
    if rel.startswith('UI/'):
        rest = rel[3:]
        sub = APP_SUB.get(os.path.basename(rest).split('.')[0])
        return 'UI/%s/%s' % (sub, rest) if sub else 'UI/%s' % rest
    return rel

# ---------- 3. 命名空间 ----------
NS_MAP = [
    ('SysCore.Hardware.Motherboard.Lpc.EC', 'SysCore.Devices.EmbeddedControllers'),
    ('SysCore.Hardware.Motherboard.Lpc', 'SysCore.Devices.SuperIo'),
    ('SysCore.Hardware.Motherboard', 'SysCore.Devices'),
    ('SysCore.Hardware.Controller', 'SysCore.Controllers'),
    ('SysCore.Hardware.Psu', 'SysCore.Power'),
    ('SysCore.Hardware.PowerMonitor', 'SysCore.Power'),
    ('SysCore.Hardware.Memory.Sensors', 'SysCore.Sensors'),
    ('SysCore.Hardware.Battery', 'SysCore.Devices'),
    ('SysCore.Hardware.Cpu', 'SysCore.Devices'), ('SysCore.Hardware.Gpu', 'SysCore.Devices'),
    ('SysCore.Hardware.Memory', 'SysCore.Devices'), ('SysCore.Hardware.Storage', 'SysCore.Devices'),
    ('SysCore.Hardware.Network', 'SysCore.Devices'),
    ('SysCore.Interop.PowerMonitor', 'SysCore.Power'),
    ('SysCore.PawnIo', 'SysCore.Interop.PawnIo'),
    ('SysCore.Software', 'SysCore.Platform'),
    ('SysCore.Windows.Forms.UI.Themes', 'SysCore.Windows.Forms.UI'),
    ('SysCore.Windows.Forms.Utilities', 'SysCore.Windows.Forms.Infrastructure'),
]
SPLIT = ['SysCore.Contracts', 'SysCore.Model', 'SysCore.Platform']

def ns_for(rel, proj):
    if proj == 'app':
        if rel.startswith('Infrastructure/'):
            return 'SysCore.Windows.Forms.Infrastructure'
        return 'SysCore.Windows.Forms.UI' if rel.startswith('UI/') else 'SysCore.Windows.Forms'
    for pre, ns in (
        ('Contracts/', 'SysCore.Contracts'), ('Model/', 'SysCore.Model'), ('Platform/', 'SysCore.Platform'),
        ('Sensors/', 'SysCore.Sensors'), ('Controllers/', 'SysCore.Controllers'), ('Power/', 'SysCore.Power'),
        ('Devices/Motherboard/SuperIo/', 'SysCore.Devices.SuperIo'),
        ('Devices/Motherboard/EmbeddedControllers/', 'SysCore.Devices.EmbeddedControllers'),
        ('Devices/', 'SysCore.Devices'),
        ('Interop/PawnIo/', 'SysCore.Interop.PawnIo'), ('Interop/', 'SysCore.Interop'),
    ):
        if rel.startswith(pre):
            return ns
    return 'SysCore'

# 只在 Sensors/Control 与 Motherboard 子树执行：把描述符 Control 改名为 ControlChannel
DESC = [
    (r'(?<![\w.])class Control\b', 'class ControlChannel'),
    (r'(?<![\w.])public Control\(', 'public ControlChannel('),
    (r'(?<![\w.])new Control\(', 'new ControlChannel('),
    (r'\bIList<Control>', 'IList<ControlChannel>'),
    (r'\bList<Control>', 'List<ControlChannel>'),
    (r'\bICollection<Control>', 'ICollection<ControlChannel>'),
    (r'\bIEnumerable<Control>', 'IEnumerable<ControlChannel>'),
    (r'\bControl\[\]', 'ControlChannel[]'),
    (r'\bforeach \(Control ', 'foreach (ControlChannel '),
    (r'(?<![\w.])Control (ctrl|control)\b', r'ControlChannel \1'),
    (r'(?<![\w.])Control (c|ctrl|control),', r'ControlChannel \1,'),
]

def is_descope(new_rel):
    return (new_rel == 'Sensors/Control/ControlChannel.cs'
            or new_rel.startswith('Devices/Motherboard/'))

def dedupe_header(t):
    m = re.search(r'^namespace\s+', t, re.M)
    if not m: return t
    head, rest = t[:m.start()], t[m.start():]
    seen = set(); out = []
    for line in head.split('\n'):
        s = line.strip()
        if re.match(r'^using\s+(static\s+)?[\w.]+\s*;$', s) or re.match(r'^using\s+\w+\s*=\s*[\w.]+\s*;$', s):
            if s in seen: continue
            seen.add(s)
        out.append(line)
    return '\n'.join(out) + rest

def transform(t, new_rel, proj):
    # 特殊别名：OperatingSystem 指向 SysCore.Software 的那条 using 直接指向新类型
    t = t.replace('using OperatingSystem = SysCore.Software.OperatingSystem;',
                  'using OperatingSystem = SysCore.Platform.OperatingSystem;')
    # 裸 using 裂成三个
    t = re.sub(r'^using SysCore\.Hardware;$',
               '\n'.join('using %s;' % u for u in SPLIT), t, flags=re.M)
    for old, new in NS_MAP:
        t = re.sub(r'\b' + re.escape(old) + r'\b', new, t)
    t = re.sub(r'\bSysCore\.Hardware\b', 'SysCore.Model', t)
    # 合并后残留的厂商子命名空间
    t = re.sub(r'\bSysCore\.Controllers\.[A-Za-z]+', 'SysCore.Controllers', t)
    t = re.sub(r'\bSysCore\.Power\.(Corsair|Msi)\b', 'SysCore.Power', t)
    t = re.sub(r'\bSysCore\.Devices\.Sensors\b', 'SysCore.Sensors', t)
    # 命名空间限定名的历史写法
    t = re.sub(r'(?<![\w.])Software\.OperatingSystem\b', 'SysCore.Platform.OperatingSystem', t)
    t = re.sub(r'(?<![\w.])Utilities\.', 'Infrastructure.', t)
    t = re.sub(r'(?<![\w.])Cpu\.IntelCpu\b', 'IntelCpu', t)
    t = re.sub(r'(?<![\w.])OperatingSystem\.Is', 'SysCore.Platform.OperatingSystem.Is', t)

    if is_descope(new_rel):
        for pat, rep in DESC:
            t = re.sub(pat, rep, t)

    # namespace 声明
    m = re.search(r'^namespace\s+[\w.]+\s*[;{]', t, re.M)
    target = ns_for(new_rel, proj)
    if m:
        t = t[:m.start()] + re.sub(r'(namespace\s+)[\w.]+', r'\g<1>' + target, m.group(0)) + t[m.end():]

    # 补 using
    if m:
        need = []
        if proj == 'core':
            need += [u for u in SPLIT if not re.search(r'^using\s+' + re.escape(u) + r'\s*;', t, re.M)]
        if re.search(r'\b(Voltage|Temperature|Fan|ControlChannel)\b', t) and 'SysCore.Sensors' != target:
            need.append('SysCore.Sensors')
        if re.search(r'(?<![\w.])PawnIo\.[A-Z]', t) and target != 'SysCore.Interop.PawnIo':
            need.append('PawnIo = SysCore.Interop.PawnIo')
        if re.search(r'\bRAMSPDToolkitDriver\b', t) and target != 'SysCore.Interop.PawnIo':
            need.append('SysCore.Interop.PawnIo')
        need = [u for u in need if not re.search(r'^using\s+' + re.escape(u) + r'\s*;', t, re.M)]
        if need:
            t = t[:m.start()] + '\n'.join('using %s;' % u for u in need) + '\n' + t[m.start():]

    # Interop.PawnIo 内部：PawnIo.PawnIo -> PawnIo
    if target == 'SysCore.Interop.PawnIo':
        t = t.replace('PawnIo.PawnIo', 'PawnIo').replace('using PawnIo = SysCore.Interop.PawnIo;\n', '')
    return dedupe_header(t)

def migrate(proj_old, new_root, mapper, proj):
    src = os.path.join(OUT, proj_old)
    n = 0
    for dp, dn, fn in os.walk(src):
        for f in fn:
            rel = os.path.relpath(os.path.join(dp, f), src).replace('\\', '/')
            new_rel = mapper(rel)
            dst = os.path.join(new_root, new_rel)
            os.makedirs(os.path.dirname(dst) or new_root, exist_ok=True)
            if f.endswith('.cs') and proj != 'aga':
                raw = open(os.path.join(dp, f), 'rb').read()
                bom = raw.startswith(b'\xef\xbb\xbf')
                t = transform(raw.decode('utf-8-sig').replace('\r\n', '\n'), new_rel, proj)
                open(dst, 'wb').write((b'\xef\xbb\xbf' if bom else b'') + t.replace('\n', '\r\n').encode('utf-8'))
            else:
                shutil.copy2(os.path.join(dp, f), dst)
            n += 1
    shutil.rmtree(src)
    return n

print('迁移:', migrate(CORE_OLD, os.path.join(OUT, 'src', 'core'), core_map, 'core'),
      migrate(APP_OLD, os.path.join(OUT, 'src', 'app'), app_map, 'app'),
      migrate(AGA_OLD, os.path.join(OUT, 'src', 'third-party', 'treeview-adv'), lambda r: r, 'aga'))
