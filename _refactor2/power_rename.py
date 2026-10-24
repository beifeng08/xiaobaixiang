# -*- coding: utf-8 -*-
"""合并到同一命名空间后，消除 Power 下的同名类型"""
import os, re

R = r'F:/toolkit/_refactor2/white-toolkit/src/core/Power'

def rn(folder, pairs):
    n = 0
    for dp, dn, fn in os.walk(os.path.join(R, folder)):
        for f in fn:
            if not f.endswith('.cs'):
                continue
            p = os.path.join(dp, f)
            raw = open(p, 'rb').read()
            bom = raw.startswith(b'\xef\xbb\xbf')
            t = raw.decode('utf-8-sig')
            o = t
            for a, b in pairs:
                t = re.sub(r'\b' + a + r'\b', b, t)
            if t != o:
                open(p, 'wb').write((b'\xef\xbb\xbf' if bom else b'') + t.encode('utf-8'))
                n += 1
    return n

print('Corsair 改名:', rn('Corsair', [('UsbApi', 'CorsairUsbApi'), ('FirmwareInfo', 'CorsairFirmwareInfo')]))
print('Msi 改名:', rn('Msi', [('UsbApi', 'MsiUsbApi'), ('FirmwareInfo', 'MsiFirmwareInfo')]))
