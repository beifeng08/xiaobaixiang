// 示例数据：仅用于演示网页如何呈现 biba-inspection-summary/1，不代表任何真实机器的验机结果。
window.BIBA_SAMPLE = (function () {
  let seed = 20261019;
  const rnd = () => ((seed = (seed * 1664525 + 1013904223) >>> 0) / 4294967296);
  const approach = (from, to, t, tau) => to - (to - from) * Math.exp(-t / tau);
  const round1 = (v) => Math.round(v * 10) / 10;

  const samples = [];
  for (let t = 0; t <= 1200; t++) {
    const cpuPower = t < 4 ? 18 : t < 56 ? 112 + rnd() * 6 : 78 + rnd() * 5;
    const cpuTemp = t < 56 ? approach(46, 97, t, 9) : approach(96, 88, t - 56, 40);
    const gpuPower = t < 6 ? 22 : 104 + rnd() * 8;
    const gpuTemp = approach(41, 79, t, 150);
    const gap = t >= 412 && t <= 416;
    samples.push({
      t,
      cpuTemp: round1(cpuTemp + (rnd() - 0.5) * 2.2),
      cpuPower: round1(cpuPower),
      gpuTemp: gap ? null : round1(gpuTemp + (rnd() - 0.5) * 1.2),
      gpuPower: gap ? null : round1(gpuPower),
    });
  }

  return {
    schema: "biba-inspection-summary/1",
    exportedAt: "2026-10-19T15:42:08+08:00",
    source: { appVersion: "0.0.6-preview.1", branch: "dev", commit: "de110b9", sourceDirty: true },
    device: { label: "SAMPLE-0001" },
    modules: [
      { id: "config", name: "配置核对", version: "1.0.0", order: 10, terminal: false, passable: true, done: true },
      { id: "keyboard", name: "键盘测试", version: "1.0.0", order: 20, terminal: false, passable: true, done: true },
      { id: "screen", name: "屏幕坏点", version: "1.0.0", order: 30, terminal: false, passable: true, done: true },
      { id: "av", name: "影音会议", version: "1.0.0", order: 40, terminal: false, passable: true, done: false },
      { id: "stress", name: "散热测试", version: "1.0.0", order: 50, terminal: false, passable: true, done: true },
      { id: "disk", name: "硬盘跑分", version: "1.0.1", order: 80, terminal: false, passable: true, done: true },
    ],
    config: {
      collectedAt: "2026-10-19T15:03:51+08:00",
      groups: [
        {
          title: "处理器（CPU）",
          entries: [
            { name: "型号", value: "Intel Core i7-13700H" },
            { name: "核心 / 线程（WMI）", value: "14 核 / 20 线程" },
          ],
          details: [
            { name: "数据来源", value: "型号优先 LHM CPU（CPUID）；核心/线程来自 Win32_Processor。WMI 与 SMBIOS 按各自记录列出，不与 LHM 按索引合并。" },
            { name: "WMI 型号", value: "13th Gen Intel(R) Core(TM) i7-13700H" },
          ],
        },
        {
          title: "显卡（GPU）",
          entries: [
            { name: "型号 #1", value: "NVIDIA GeForce RTX 4060 Laptop GPU" },
            { name: "显存 #1", value: "8.0 GB" },
            { name: "驱动版本 #1", value: "32.0.15.6094" },
            { name: "", value: "" },
            { name: "型号 #2", value: "Intel(R) Iris(R) Xe Graphics" },
            { name: "显存 #2", value: "约 2.0 GB（WMI 估算值）" },
            { name: "驱动版本 #2", value: "31.0.101.4502" },
          ],
          details: [
            { name: "数据来源", value: "型号/驱动为 Win32_VideoController；显存依次用名称匹配的 LHM GPU Memory Total、注册表 qwMemorySize、WMI AdapterRAM（32 位，仅估算）。" },
            { name: "LHM 显存 #1", value: "8.0 GB" },
          ],
        },
        {
          title: "内存",
          entries: [
            { name: "总容量", value: "16.0 GB" },
            { name: "配置速度（WMI）", value: "5200 MT/s" },
          ],
          details: [
            { name: "插槽占用", value: "2 / 2" },
            { name: "SPD 来源", value: "LHM DIMM 直读（需 PawnIO 驱动）；本次未安装驱动，SPD 未读取。" },
            { name: "WMI 来源", value: "Win32_PhysicalMemory：容量、最大速度 Speed、配置速度 ConfiguredClockSpeed；非直接 SPD 读数。" },
          ],
        },
        {
          title: "硬盘",
          entries: [
            { name: "硬盘", value: "1.0 TB（C: D:）　SAMSUNG MZVL21T0HCLR-00B00" },
            { name: "健康度", value: "100 %" },
            { name: "通电时间", value: "37 小时" },
            { name: "介质与数据完整性错误", value: "0 次" },
          ],
          details: [
            { name: "数据来源", value: "LHM StorageDevice / DiskInfoToolkit 提供型号、容量和 SMART；不可用时退回 Win32_DiskDrive，WMI 无 SMART 则保持未知。" },
          ],
        },
        {
          title: "网卡",
          entries: [
            { name: "网卡 #1", value: "Intel(R) Wi-Fi 6E AX211 160MHz" },
            { name: "网卡 #2", value: "Realtek PCIe GbE Family Controller" },
          ],
          details: [{ name: "数据来源", value: "Win32_NetworkAdapter，PhysicalAdapter=True；包含未连接的物理网卡。" }],
        },
        {
          title: "屏幕",
          entries: [
            { name: "分辨率", value: "2560 × 1600" },
            { name: "刷新率", value: "165 Hz" },
            { name: "尺寸", value: "16.0 英寸（34 × 22 cm）" },
            { name: "面板", value: "MNG007DA1-1（CSO 1600）" },
            { name: "色域", value: "≈ 102% sRGB / 76% DCI-P3（EDID 容积比）" },
          ],
          details: [{ name: "数据来源", value: "DisplayInfoService：分辨率/刷新率来自 Windows 显示模式，尺寸/面板标识/色度坐标来自 EDID；色域为 EDID 推算的容积比，非覆盖率或仪器实测。" }],
        },
        {
          title: "电池",
          entries: [
            { name: "设计容量（LHM）", value: "80,080 mWh" },
            { name: "完全充电容量（LHM）", value: "77,410 mWh" },
            { name: "损耗（LHM）", value: "3.3 %（健康度 96.7 %）" },
            { name: "循环次数（WMI）", value: "无法读取，未用容量反推" },
          ],
          details: [{ name: "数据来源", value: "LHM 电池容量为电池类驱动 IOCTL；循环次数仅由 WMI 提供。LHM、Win32_Battery 与 root\\WMI 不按索引合并。" }],
        },
        {
          title: "BIOS",
          entries: [
            { name: "厂商", value: "LENOVO" },
            { name: "版本", value: "LPCN49WW" },
            { name: "发布日期", value: "2026-05-22" },
          ],
          details: [{ name: "数据来源", value: "LHM SMBios.Bios（Type 0），固件提供的厂商、版本和发布日期；非实时检测。" }],
        },
        {
          title: "整机",
          entries: [
            { name: "厂商", value: "LENOVO" },
            { name: "型号", value: "82WM" },
          ],
          details: [{ name: "数据来源", value: "LHM SMBios.System（Type 1），整机厂商填写的静态标识，未填写或占位值不展示。" }],
        },
        {
          title: "主板",
          entries: [{ name: "采集结果", value: "未取得可用信息（可能未安装、来源不可用或固件未填写）" }],
          details: [{ name: "SMBIOS", value: "未取得主板固件记录" }],
        },
      ],
    },
    keyboard: { layout: "87", tested: 84, total: 87, untested: ["ScrLk", "Pause", "Menu"] },
    av: {
      camera: "Integrated Camera",
      microphone: "麦克风阵列 (Realtek(R) Audio)",
      speaker: "扬声器 (Realtek(R) Audio)",
      note: "摄像头已启动，等待画面…（操作者未确认）",
    },
    stress: {
      startedAt: "2026-10-19T15:12:30+08:00",
      sampleIntervalSeconds: 1,
      sensors: {
        cpuName: "Intel Core i7-13700H",
        cpuTemp: "CPU Package",
        cpuPower: "CPU Package",
        gpuName: "NVIDIA GeForce RTX 4060 Laptop GPU",
        gpuTemp: "GPU Core",
        gpuPower: "GPU Package",
      },
      samples,
    },
    disk: {
      drive: "C",
      latest: { seq_read: 4892.3, seq_write: 4310.7, rnd4k_read: 71.4, rnd4k_write: 182.9 },
      history: [
        { time: "2026-10-19 15:35:02", drive: "C", seqRead: 4892.3, seqWrite: 4310.7, rnd4kRead: 71.4, rnd4kWrite: 182.9 },
        { time: "2026-10-19 15:31:47", drive: "D", seqRead: 4870.1, seqWrite: 4126.5, rnd4kRead: 69.8, rnd4kWrite: 176.2 },
      ],
    },
    expected: {
      enteredAt: "2026-10-19T14:58:10+08:00",
      fields: [
        { key: "model", label: "整机型号", value: "82WM" },
        { key: "cpu", label: "处理器", value: "i7-13700H" },
        { key: "gpu", label: "显卡", value: "RTX 4060" },
        { key: "memory", label: "内存", value: "16G" },
        { key: "storage", label: "硬盘", value: "1TB 三星" },
        { key: "display", label: "屏幕", value: "2560x1600 165Hz" },
        { key: "battery", label: "电池", value: "" },
        { key: "notes", label: "其它", value: "卖家称无拆修" },
      ],
    },
    remarks: "影音会议：摄像头画面未出现，未点确认；其余模块已人工确认。",
  };
})();
