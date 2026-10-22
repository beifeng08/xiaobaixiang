(function () {
  "use strict";

  const SCHEMA = "biba-inspection-summary/1";
  const SVGNS = "http://www.w3.org/2000/svg";
  const COLOR = { cpu: "var(--red)", gpu: "var(--blue)" };
  const $ = (id) => document.getElementById(id);

  let data = null;
  let dataLabel = "";
  let isSample = false;

  // ── DOM 工具：数据一律走 textContent，载入的 JSON 不能注入标记 ──
  function h(tag, attributes, ...children) {
    const element = document.createElement(tag);
    setAttrs(element, attributes);
    append(element, children);
    return element;
  }
  function svg(tag, attributes, ...children) {
    const element = document.createElementNS(SVGNS, tag);
    setAttrs(element, attributes);
    append(element, children);
    return element;
  }
  function setAttrs(element, attributes) {
    for (const [name, value] of Object.entries(attributes || {})) {
      if (value == null || value === false) continue;
      if (name === "class") element.setAttribute("class", value);
      else if (name === "dataset") Object.assign(element.dataset, value);
      else if (name.startsWith("on")) element.addEventListener(name.slice(2), value);
      else element.setAttribute(name, value === true ? "" : value);
    }
  }
  function append(element, children) {
    for (const child of children.flat(Infinity)) {
      if (child == null || child === false) continue;
      element.append(child instanceof Node ? child : String(child));
    }
  }
  function fill(id, ...children) {
    const element = $(id);
    element.replaceChildren();
    append(element, children);
    return element;
  }

  // ── 格式化 ──
  // 按字符串里写的墙钟时间显示，避免浏览器时区把北京时间换算走
  function fmtTime(value) {
    if (!value) return "未记录";
    const match = /^(\d{4}-\d{2}-\d{2})[T ](\d{2}:\d{2}(?::\d{2})?)/.exec(value);
    return match ? `${match[1]} ${match[2]}` : String(value);
  }
  function fmtDur(seconds) {
    if (!Number.isFinite(seconds)) return "超出显示范围";
    seconds = Math.max(0, Math.round(seconds));
    const hours = Math.floor(seconds / 3600), minutes = Math.floor((seconds % 3600) / 60), remainingSeconds = seconds % 60;
    const pad = (value) => String(value).padStart(2, "0");
    return hours > 0 ? `${hours}:${pad(minutes)}:${pad(remainingSeconds)}` : `${pad(minutes)}:${pad(remainingSeconds)}`;
  }
  const fmtNum = (value, decimals = 1) => (typeof value !== "number" || !Number.isFinite(value) ? "空" : value.toFixed(decimals));
  const isRecord = (value) => value !== null && typeof value === "object" && !Array.isArray(value);
  function niceStep(raw) {
    const magnitude = Math.pow(10, Math.floor(Math.log10(raw)));
    const fraction = raw / magnitude;
    return (fraction <= 1 ? 1 : fraction <= 2 ? 2 : fraction <= 5 ? 5 : 10) * magnitude;
  }
  const niceCeil = (value) => { const step = niceStep(value / 5); return Math.ceil(value / step) * step; };
  function fmtAxis(value, step) {
    const places = Math.max(0, Math.min(4, Math.ceil(-Math.log10(step))));
    return Number(value.toFixed(places)).toString();
  }

  const textField = { type: "string" }, nullableText = { type: ["string", "null"] };
  const dateTimeField = { type: "string", format: "date-time" }, nullableDateTime = { type: ["string", "null"], format: "date-time" };
  const numberField = { type: "number" }, nullableNumber = { type: ["number", "null"] };
  const boolField = { type: "boolean" };
  const objectField = (properties, required = [], nullable = false) => ({ type: nullable ? ["object", "null"] : "object", properties, required });
  const arrayField = (items) => ({ type: "array", items });
  const entriesField = arrayField(objectField({ name: textField, value: textField }, ["name", "value"]));
  const CONTRACT = objectField({
    schema: { const: SCHEMA }, exportedAt: dateTimeField,
    source: objectField({ appVersion: nullableText, branch: textField, commit: textField, sourceDirty: boolField }, [], true),
    device: objectField({ label: nullableText }, [], true),
    modules: arrayField(objectField({
      id: textField, name: textField, version: textField, order: { type: "integer" },
      terminal: boolField, done: boolField, passable: boolField,
    }, ["id", "name", "version", "order", "terminal", "done"])),
    config: objectField({ collectedAt: dateTimeField, groups: arrayField(objectField({
      title: textField, entries: entriesField, details: entriesField,
    }, ["title", "entries"])) }, ["collectedAt", "groups"], true),
    keyboard: objectField({ layout: { type: "string", enum: ["104", "87"] },
      tested: { type: "integer", minimum: 0 }, total: { type: "integer", minimum: 0 }, untested: arrayField(textField) }, [], true),
    av: objectField({ camera: nullableText, microphone: nullableText, speaker: nullableText, note: nullableText }, [], true),
    stress: objectField({ startedAt: nullableDateTime, sampleIntervalSeconds: { type: "number", exclusiveMinimum: 0 },
      sensors: objectField({ cpuName: nullableText, cpuTemp: nullableText, cpuPower: nullableText,
        gpuName: nullableText, gpuTemp: nullableText, gpuPower: nullableText }, [], true),
      samples: { type: ["array", "null"], items: objectField({ t: numberField, cpuTemp: nullableNumber,
        cpuPower: nullableNumber, gpuTemp: nullableNumber, gpuPower: nullableNumber }, ["t"]) } }, [], true),
    disk: objectField({ drive: nullableText, latest: objectField({ seq_read: { type: "number", minimum: 0 },
      seq_write: { type: "number", minimum: 0 }, rnd4k_read: { type: "number", minimum: 0 },
      rnd4k_write: { type: "number", minimum: 0 } }, [], true),
      history: arrayField(objectField({ time: textField, drive: textField, seqRead: { type: "number", minimum: 0 },
        seqWrite: { type: "number", minimum: 0 }, rnd4kRead: { type: "number", minimum: 0 },
        rnd4kWrite: { type: "number", minimum: 0 } })) }, [], true),
    sectionErrors: { type: "object", additionalProperties: textField },
    expected: objectField({ enteredAt: nullableDateTime, fields: arrayField(objectField({
      key: textField, label: nullableText, value: nullableText,
    }, ["key", "label", "value"])) }, [], true),
    remarks: nullableText,
  }, ["schema", "exportedAt", "modules"]);

  function validate(value, rule = CONTRACT, path = "report") {
    if (Object.hasOwn(rule, "const") && value !== rule.const) return `${path} 应为 ${JSON.stringify(rule.const)}。`;
    const types = [].concat(rule.type || []);
    if (types.length && !types.some((type) => type === "null" ? value === null : type === "object" ? isRecord(value)
      : type === "array" ? Array.isArray(value) : type === "number" ? isNum(value)
      : type === "integer" ? Number.isInteger(value) : typeof value === type))
      return `${path} 应为 ${types.join(" / ")}。`;
    if (value === null) return null;
    if (rule.format === "date-time") {
      const match = typeof value === "string" && /^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})(?:\.\d+)?(?:Z|[+-](\d{2}):(\d{2}))$/i.exec(value);
      if (!match) return `${path} 应为带时区的 ISO date-time。`;
      const year = Number(match[1]), month = Number(match[2]), day = Number(match[3]);
      const leap = year % 4 === 0 && (year % 100 !== 0 || year % 400 === 0);
      const days = [31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31][month - 1];
      if (!days || day < 1 || day > days || Number(match[4]) > 23 || Number(match[5]) > 59 ||
          Number(match[6]) > 59 || Number(match[7] || 0) > 23 || Number(match[8] || 0) > 59 || !Number.isFinite(Date.parse(value)))
        return `${path} 应为带时区的 ISO date-time。`;
    }
    if (rule.enum && !rule.enum.includes(value)) return `${path} 应为 ${rule.enum.join(" / ")}。`;
    if (rule.minimum != null && value < rule.minimum) return `${path} 不得小于 ${rule.minimum}。`;
    if (rule.exclusiveMinimum != null && value <= rule.exclusiveMinimum) return `${path} 应大于 ${rule.exclusiveMinimum}。`;
    if (Array.isArray(value) && rule.items) {
      for (let index = 0; index < value.length; index++) {
        const error = validate(value[index], rule.items, `${path}[${index}]`);
        if (error) return error;
      }
    } else if (isRecord(value)) {
      for (const key of rule.required || []) if (!Object.hasOwn(value, key)) return `${path}.${key} 缺失。`;
      for (const [key, child] of Object.entries(value)) {
        const childRule = Object.hasOwn(rule.properties || {}, key) ? rule.properties[key] : rule.additionalProperties;
        if (childRule) {
          const error = validate(child, childRule, `${path}.${key}`);
          if (error) return error;
        }
      }
    }
    return null;
  }

  function stressSamples() {
    return (data.stress?.samples || []).filter((s) => s && isNum(s.t)).sort((a, b) => a.t - b.t);
  }
  function stressDuration(samples) {
    return samples.length ? samples[samples.length - 1].t - samples[0].t + (data.stress.sampleIntervalSeconds ?? 1) : 0;
  }

  // ── 01 模块 ──
  function sortedModules() {
    return [...data.modules].sort((a, b) => (a.order ?? 0) - (b.order ?? 0));
  }
  const passable = () => sortedModules().filter((m) => !m.terminal && m.passable !== false);
  const moduleById = (id) => data.modules.find((m) => m.id === id);

  function renderPoster() {
    drawPosterBg();
    const mods = passable();
    const done = mods.filter((m) => m.done).length;
    $("sealCount").textContent = `${done}/${mods.length}`;
    $("sealLabel").setAttribute("aria-label", `人工确认 ${done} / ${mods.length} 个模块`);
    const day = /^(\d{4})-(\d{2})-(\d{2})/.exec(data.exportedAt || "");
    $("vdate").textContent = day ? `${day[1]}·${day[2]}·${day[3]}` : "";
    const src = data.source || {};
    $("posterMeta").textContent = [
      data.device?.label ? `设备 ${data.device.label}` : "设备编号未填",
      `导出 ${fmtTime(data.exportedAt)}`,
      src.appVersion ? `v${src.appVersion}` : null,
    ].filter(Boolean).join("  ·  ");
    $("sampleStamp").hidden = !isSample;
    const line = fill("sourceLine");
    if (isSample) append(line, ["当前：", h("b", null, "示例数据"), "（不代表任何真实机器）"]);
    else append(line, ["当前：", h("b", null, dataLabel)]);
  }

  function renderModules() {
    const mods = sortedModules();
    fill("moduleProgress", passable().map((m) => h("span", { class: m.done ? "on" : null, title: m.name })));
    fill("moduleList", mods.map((m) => {
      const unavailable = !m.terminal && m.passable === false;
      const state = m.terminal ? "终点页 · 不参与通过" : unavailable ? "不参与通过确认" : m.done ? "■ 已人工确认" : "□ 待确认";
      return h("li", { class: `module ${m.terminal || unavailable ? "unavailable" : m.done ? "done" : "pending"}` },
        h("div", { class: "m-top" }, h("span", null, String(m.order ?? "–").padStart(2, "0")), h("span", null, m.id)),
        h("p", { class: "m-name" }, m.name),
        h("span", { class: "m-ver" }, `清单 v${m.version ?? "?"}`),
        h("div", { class: "m-state" }, state));
    }));
  }

  // ── 02 配置：字段标签只提示口径，不下结论 ──
  const MISSING_RE = /无法读取|未知|获取失败|未取得|容量未知|失败/;
  const REVIEW_RULES = [
    [/^通电时间/, "通电时间只提示人工复核，没有既定的判新旧阈值。"],
    [/^介质与数据完整性错误/, "NVMe 介质错误只提示人工复核，没有既定的不合格阈值。"],
    [/^健康度/, "SMART 健康度为硬盘自报值，需结合原值人工复核。"],
    [/^循环次数/, "电池循环与 powercfg /batteryreport 同源，不能互证；空值不能当 0。"],
    [/^损耗/, "电池容量与 batteryreport 同源，不是独立互证。"],
  ];
  const SCOPE_RULES = [
    [/^显存/, /估算/, "WMI AdapterRAM 为 32 位，大于 4 GB 的显存会截断，只能作估算。"],
    [/^色域/, null, "EDID 色度坐标的三角形面积比，不是标准覆盖率，也不是仪器实测或三维色体积。"],
    [/^配置速度/, null, "ConfiguredClockSpeed 是配置值，不是实时测频。"],
  ];
  function classify(name, value) {
    const flags = [];
    if (!value.trim() || MISSING_RE.test(value)) flags.push({ kind: "missing", label: "∅ 未取得", why: "来源不可用或读取失败；不表示硬件不存在，也不能当作 0 或合格。" });
    for (const [re, why] of REVIEW_RULES) if (re.test(name)) flags.push({ kind: "review", label: "! 需人工复核", why });
    for (const [re, vre, why] of SCOPE_RULES) if (re.test(name) && (!vre || vre.test(value))) flags.push({ kind: "scope", label: "i 口径提示", why });
    return flags;
  }

  function flagButton(f) {
    return h("button", {
      type: "button", class: `flag ${f.kind}`, "aria-label": `${f.label.slice(2)}：${f.why}`,
      onmouseenter: (e) => showTip([f.why], e.clientX, e.clientY),
      onmousemove: (e) => moveTip(e.clientX, e.clientY),
      onmouseleave: hideTip,
      onfocus: (e) => { const r = e.target.getBoundingClientRect(); showTip([f.why], r.left, r.bottom); },
      onblur: hideTip,
    }, f.label);
  }

  function renderConfig() {
    const cfg = data.config;
    if (!cfg) {
      fill("configStats");
      fill("configGroups", h("div", { class: "empty" }, "未导出配置采集结果", h("small", null, "config 为空：视为未采集，不代表通过。")));
      return;
    }
    const count = { entries: 0, review: 0, missing: 0, scope: 0 };
    const cards = cfg.groups.map((g) => {
      const rows = g.entries.map((e) => {
        if (!e.name && !e.value) return h("div", { class: "row sep", "aria-hidden": "true" });
        count.entries++;
        const flags = classify(e.name || "", String(e.value ?? ""));
        const kinds = [...new Set(flags.map((f) => f.kind))];
        kinds.forEach((k) => count[k]++);
        return h("div", { class: `row${kinds.includes("missing") ? " is-missing" : ""}`, dataset: { kinds: kinds.join(" ") } },
          h("dt", null, e.name || "（未命名）", flags.length ? h("span", { class: "flags" }, flags.map(flagButton)) : null),
          h("dd", null, e.value.trim() ? e.value : "未取得"));
      });
      const details = Array.isArray(g.details) && g.details.length
        ? h("details", null, h("summary", null, `详细信息 · 来源（${g.details.length}）`),
            h("dl", { class: "rows" }, g.details.map((d) => h("div", { class: "row" }, h("dt", null, d.name), h("dd", null, d.value)))))
        : null;
      return h("article", { class: `group${g.title === "采集诊断" ? " diag" : ""}` },
        h("h3", null, g.title), h("dl", { class: "rows" }, rows), details);
    });
    fill("configStats",
      `采集于 ${fmtTime(cfg.collectedAt)} · `, h("b", null, cfg.groups.length), "组 ", h("b", null, count.entries), "项 · 需复核",
      h("b", null, count.review), "· 未取得", h("b", null, count.missing), "· 口径提示", h("b", null, count.scope));
    fill("configGroups", cards);
    applyFilter(currentFilter);
  }

  let currentFilter = "all";
  function applyFilter(kind) {
    currentFilter = kind;
    document.querySelectorAll(".chip[data-filter]").forEach((c) => {
      const on = c.dataset.filter === kind;
      c.classList.toggle("is-on", on);
      c.setAttribute("aria-pressed", String(on));
    });
    document.querySelectorAll("#configGroups .group").forEach((g) => {
      let visible = 0;
      g.querySelectorAll(":scope > .rows > .row").forEach((r) => {
        const show = kind === "all" || (r.dataset.kinds || "").split(" ").includes(kind);
        r.hidden = !show;
        if (show && !r.classList.contains("sep")) visible++;
      });
      const det = g.querySelector(":scope > details");
      if (det) det.hidden = kind !== "all";
      g.hidden = kind !== "all" && visible === 0;
    });
  }

  // ── 03 散热 ──
  const METRICS = [
    { key: "cpuTemp", name: "CPU", unit: "°C", group: "cpu", label: "CPU 温度" },
    { key: "gpuTemp", name: "GPU", unit: "°C", group: "gpu", label: "GPU 温度" },
    { key: "cpuPower", name: "CPU", unit: "W", group: "cpu", label: "CPU 功耗" },
    { key: "gpuPower", name: "GPU", unit: "W", group: "gpu", label: "GPU 功耗" },
  ];
  const isNum = (v) => typeof v === "number" && isFinite(v);

  function metricStats(samples, key) {
    let peak = null, peakT = null, nulls = 0, mean = 0, n = 0;
    const tEnd = samples[samples.length - 1].t;
    for (const s of samples) {
      const v = s[key];
      if (!isNum(v)) { nulls++; continue; }
      if (peak == null || v > peak) { peak = v; peakT = s.t; }
      if (s.t >= tEnd - 300) { n++; mean = mean * ((n - 1) / n) + v / n; }
    }
    return { peak, peakT, nulls, tail: n ? mean : null };
  }

  function renderStress() {
    const st = data.stress;
    const samples = stressSamples();
    $("stressTableWrap").hidden = samples.length === 0;
    if (!samples.length) {
      fill("stressNote");
      fill("stressTiles", h("div", { class: "empty" }, "未导出散热采样", h("small", null, "stress 为空或没有样本：视为未记录，不代表散热合格。")));
      fill("stressCharts");
      fill("stressTable");
      return;
    }
    const sen = st.sensors || {};
    const interval = isNum(st.sampleIntervalSeconds) && st.sampleIntervalSeconds > 0 ? st.sampleIntervalSeconds : 1;
    const t0 = samples[0].t, dur = stressDuration(samples);
    const anyNull = samples.filter((s) => METRICS.some((m) => !isNum(s[m.key]))).length;
    fill("stressNote",
      `开始于 ${fmtTime(st.startedAt)}，采样间隔 ${interval} 秒。传感器：CPU「${sen.cpuName || "未知"}」（温度 ${sen.cpuTemp || "未映射"} / 功耗 ${sen.cpuPower || "未映射"}），GPU「${sen.gpuName || "未知"}」（温度 ${sen.gpuTemp || "未映射"} / 功耗 ${sen.gpuPower || "未映射"}）。`,
      "读数为空的拍以灰带标出、不补 0；有数值也不代表新鲜。散热是否合格由操作者判断，应用不设强制阈值。");

    const tiles = [h("div", { class: "tile" },
      h("div", { class: "t-label" }, "记录时长"),
      h("div", { class: "t-value" }, fmtDur(dur)),
      h("div", { class: "t-sub" }, `${samples.length} 拍 · 含空读数 ${anyNull} 拍`))];
    for (const m of METRICS) {
      const s = metricStats(samples, m.key);
      tiles.push(h("div", { class: `tile ${m.group}` },
        h("div", { class: "t-label" }, `${m.label} · 峰值`),
        h("div", { class: "t-value" }, fmtNum(s.peak), h("small", null, m.unit)),
        h("div", { class: "t-sub" }, s.peak == null ? "全程无读数" : `出现在 ${fmtDur(s.peakT - t0)} · 末 5 分钟均值 ${fmtNum(s.tail)} ${m.unit}${s.nulls ? ` · 空 ${s.nulls} 拍` : ""}`)));
    }
    fill("stressTiles", tiles);

    const charts = fill("stressCharts");
    const temp = h("figure", { class: "chart", style: "margin:0" });
    const power = h("figure", { class: "chart", style: "margin:0" });
    charts.append(temp, power);
    lineChart(temp, { title: "温度", unit: "°C", zero: false, samples, t0, series: METRICS.filter((m) => m.unit === "°C") });
    lineChart(power, { title: "功耗", unit: "W", zero: true, samples, t0, series: METRICS.filter((m) => m.unit === "W") });

    const rows = samples.filter((s, i) => i === 0 || Math.floor((s.t - t0) / 30) !== Math.floor((samples[i - 1].t - t0) / 30));
    fill("stressTable", h("div", { class: "tbl-wrap" }, h("table", { class: "tbl" },
      h("thead", null, h("tr", null, h("th", null, "时刻"), METRICS.map((m) => h("th", { class: "num" }, `${m.label}（${m.unit}）`)))),
      h("tbody", null, rows.map((s) => h("tr", null, h("td", null, fmtDur(s.t - t0)), METRICS.map((m) => h("td", { class: "num" }, fmtNum(s[m.key])))))))));
  }

  function lineChart(fig, o) {
    const W = Math.max(320, Math.round(fig.clientWidth - 34));
    const H = 260, M = { l: 42, r: 92, t: 12, b: 28 };
    const pw = W - M.l - M.r, ph = H - M.t - M.b;
    const { samples, t0, series } = o;
    const tN = samples[samples.length - 1].t;
    let lo = Infinity, hi = -Infinity;
    for (const sample of samples) for (const metric of series) {
      const value = sample[metric.key];
      if (isNum(value)) { lo = Math.min(lo, value); hi = Math.max(hi, value); }
    }
    if (lo === Infinity) { lo = 0; hi = 1; }
    const step = niceStep(Math.max(1, hi - (o.zero ? 0 : lo)) / 5);
    lo = o.zero ? Math.min(0, Math.floor(lo / step) * step) : Math.floor(lo / step) * step;
    hi = Math.ceil(hi / step) * step;
    if (hi === lo) hi = lo + step;
    if (!Number.isFinite(hi - lo) || hi <= lo || !Number.isFinite(tN - t0))
      throw new Error("散热样本的数值范围超出图表可显示范围");
    const x = (t) => M.l + ((t - t0) / Math.max(1, tN - t0)) * pw;
    const y = (v) => M.t + (1 - (v - lo) / (hi - lo)) * ph;

    const root = svg("svg", { viewBox: `0 0 ${W} ${H}`, role: "img", "aria-label": `${o.title}随时间变化：${series.map((m) => m.label).join("、")}` });
    const grid = svg("g", { class: "grid" }), axis = svg("g", { class: "axis" });
    for (let index = 0; index <= Math.min(20, Math.round((hi - lo) / step)); index++) {
      const v = lo + index * step;
      grid.append(svg("line", { x1: M.l, x2: M.l + pw, y1: y(v), y2: y(v) }));
      axis.append(svg("text", { x: M.l - 8, y: y(v) + 4, "text-anchor": "end" }, fmtAxis(v, step)));
    }
    const tStep = [30, 60, 120, 300, 600, 900, 1800, 3600].find((s) => (tN - t0) / s <= 7) || niceStep((tN - t0) / 7);
    for (let index = 0; index <= Math.min(8, Math.floor((tN - t0) / tStep)); index++) {
      const t = index * tStep;
      axis.append(svg("text", { x: x(t0 + t), y: H - 8, "text-anchor": "middle" }, fmtDur(t)));
    }
    axis.append(svg("text", { x: M.l - 8, y: M.t - 2, "text-anchor": "end" }, o.unit));

    // 空读数灰带：任一序列为空的连续区间
    const gaps = svg("g");
    let gapStart = null;
    samples.forEach((s, i) => {
      const empty = series.some((m) => !isNum(s[m.key]));
      if (empty && gapStart == null) gapStart = i;
      if ((!empty || i === samples.length - 1) && gapStart != null) {
        const a = samples[Math.max(0, gapStart - 1)].t, b = samples[i].t;
        gaps.append(svg("rect", { class: "gap-mark", x: x(a), y: M.t, width: Math.max(3, x(b) - x(a)), height: ph }));
        gapStart = null;
      }
    });

    const lines = svg("g", { fill: "none", "stroke-width": 2, "stroke-linejoin": "round", "stroke-linecap": "round" });
    const ends = [];
    for (const m of series) {
      let d = "", pen = false, last = null;
      for (const s of samples) {
        const v = s[m.key];
        if (!isNum(v)) { pen = false; continue; }
        d += `${pen ? "L" : "M"}${x(s.t).toFixed(1)} ${y(v).toFixed(1)}`;
        pen = true;
        last = v;
      }
      lines.append(svg("path", { d, stroke: COLOR[m.group] }));
      if (last != null) ends.push({ m, v: last, y: y(last) });
    }
    ends.sort((a, b) => a.y - b.y);
    for (let i = 1; i < ends.length; i++) if (ends[i].y - ends[i - 1].y < 15) ends[i].y = ends[i - 1].y + 15;
    const labels = svg("g");
    for (const e of ends) {
      labels.append(svg("rect", { x: M.l + pw + 6, y: e.y - 1.5, width: 10, height: 3, rx: 1.5, fill: COLOR[e.m.group] }));
      labels.append(svg("text", { class: "end-label", x: M.l + pw + 20, y: e.y + 4 }, `${e.m.name} ${fmtNum(e.v)}`));
    }

    const cross = svg("line", { class: "cross", y1: M.t, y2: M.t + ph, visibility: "hidden" });
    const dots = series.map((m) => svg("circle", { r: 4.5, fill: COLOR[m.group], stroke: "var(--paper)", "stroke-width": 2, visibility: "hidden" }));
    const hit = svg("rect", { x: M.l, y: M.t, width: pw, height: ph, fill: "transparent", style: "cursor:crosshair" });
    const nearest = (t) => {
      let a = 0, b = samples.length - 1;
      while (b - a > 1) { const c = (a + b) >> 1; if (samples[c].t < t) a = c; else b = c; }
      return Math.abs(samples[a].t - t) <= Math.abs(samples[b].t - t) ? samples[a] : samples[b];
    };
    hit.addEventListener("pointermove", (ev) => {
      const r = root.getBoundingClientRect();
      const px = ((ev.clientX - r.left) / r.width) * W;
      const s = nearest(t0 + ((px - M.l) / pw) * (tN - t0));
      cross.setAttribute("x1", x(s.t)); cross.setAttribute("x2", x(s.t)); cross.setAttribute("visibility", "visible");
      series.forEach((m, i) => {
        const v = s[m.key];
        dots[i].setAttribute("visibility", isNum(v) ? "visible" : "hidden");
        if (isNum(v)) { dots[i].setAttribute("cx", x(s.t)); dots[i].setAttribute("cy", y(v)); }
      });
      showTip([h("b", null, `T+${fmtDur(s.t - t0)}`), ...series.map((m) =>
        h("div", { class: "tt-row" }, h("i", { style: `background:${COLOR[m.group]}` }), `${m.label} ${isNum(s[m.key]) ? `${fmtNum(s[m.key])} ${m.unit}` : "空读数"}`))], ev.clientX, ev.clientY);
    });
    hit.addEventListener("pointerleave", () => {
      cross.setAttribute("visibility", "hidden");
      dots.forEach((d) => d.setAttribute("visibility", "hidden"));
      hideTip();
    });

    root.append(grid, gaps, axis, lines, labels, cross, ...dots, hit);
    append(fig, [
      h("h3", null, `${o.title}（${o.unit}）`),
      h("div", { class: "legend" }, series.map((m) => h("span", null, h("i", { style: `background:${COLOR[m.group]}` }), m.label))),
      root]);
  }

  // ── 04 硬盘 ──
  function barPanel(title, sub, items) {
    const max = niceCeil(Math.max(...items.map((i) => isNum(i.v) ? i.v : 0), 1));
    return h("article", { class: "bars" },
      h("h3", null, title), h("p", { class: "b-sub" }, sub),
      items.map((i) => h("div", { class: "bar-row" },
        h("span", { class: "b-name" }, i.name),
        h("div", { class: "bar-track", role: "img", "aria-label": `${i.name} ${fmtNum(i.v)} MB/s`,
          onmousemove: (e) => showTip([h("b", null, `${title} · ${i.name}`), `${fmtNum(i.v)} MB/s（刻度满格 ${max} MB/s）`], e.clientX, e.clientY),
          onmouseleave: hideTip },
          h("div", { class: "bar-fill", style: `width:${isNum(i.v) ? Math.max(0, Math.min(100, (i.v / max) * 100)) : 0}%` })),
        h("span", { class: "b-val" }, fmtNum(i.v), h("small", null, " MB/s")))));
  }

  function renderDisk() {
    const dk = data.disk;
    const r = isRecord(dk?.latest) ? dk.latest : null;
    if (!r || !["seq_read", "seq_write", "rnd4k_read", "rnd4k_write"].some((key) => isNum(r[key]))) {
      fill("diskPanels", h("div", { class: "empty" }, "未导出硬盘成绩", h("small", null, "disk.latest 为空：视为未跑分，不代表通过。")));
    } else {
      fill("diskPanels",
        barPanel(`顺序读写 · ${dk.drive ? dk.drive + " 盘" : "盘符未记录"}`, "SEQ1M Q8T1 · 5 轮 + 1 预热", [{ name: "读取", v: r.seq_read }, { name: "写入", v: r.seq_write }]),
        barPanel("4K 随机读写", "RND4K Q1T1 · 5 轮 + 1 预热", [{ name: "读取", v: r.rnd4k_read }, { name: "写入", v: r.rnd4k_write }]));
    }
    const hist = Array.isArray(dk?.history) ? dk.history : [];
    fill("diskHistory", hist.length ? h("div", { class: "tbl-wrap" }, h("table", { class: "tbl" },
      h("caption", null, "成绩记录（硬盘成绩记录.csv）"),
      h("thead", null, h("tr", null, ["时间", "盘符"].map((t) => h("th", null, t)),
        ["顺序读取", "顺序写入", "随机读取 4K", "随机写入 4K"].map((t) => h("th", { class: "num" }, `${t}（MB/s）`)))),
      h("tbody", null, hist.map((row) => h("tr", null, h("td", null, row.time), h("td", null, row.drive),
        [row.seqRead, row.seqWrite, row.rnd4kRead, row.rnd4kWrite].map((v) => h("td", { class: "num" }, fmtNum(v)))))))) : null);
  }

  // ── 05 外设 ──
  function stateChip(id) {
    const m = moduleById(id);
    if (!m) return h("span", { class: "pending" }, "模块未加载");
    return m.terminal || m.passable === false
      ? h("span", { class: "unavailable" }, "不参与通过确认")
      : m.done ? h("span", { class: "done" }, "■ 已人工确认") : h("span", { class: "pending" }, "□ 待确认");
  }
  const SCREEN_CYCLE = [["白", "#ffffff"], ["红", "#ff0000"], ["绿", "#00ff00"], ["蓝", "#0000ff"], ["黑", "#000000"]];

  function renderIo() {
    const kb = data.keyboard, av = data.av;
    const kbBody = kb && isNum(kb.tested) && isNum(kb.total) && kb.total > 0
      ? [h("div", { class: "io-big" }, kb.tested, h("small", null, ` / ${kb.total} 键`)),
         h("div", { class: "meter", role: "img", "aria-label": `已测试 ${kb.tested} / ${kb.total}` }, h("span", { style: `width:${Math.min(100, (kb.tested / kb.total) * 100)}%` })),
         kb.layout ? h("p", { class: "io-note", style: "margin-top:10px" }, `布局：${kb.layout} 键${kb.layout === "87" ? "（无数字小键盘）" : ""}`) : null,
         Array.isArray(kb.untested) && kb.untested.length
           ? h("p", { class: "io-note", style: "margin-top:4px" }, `未按过：${kb.untested.slice(0, 24).join("、")}${kb.untested.length > 24 ? ` 等 ${kb.untested.length} 个` : ""}`)
           : null,
         h("p", { class: "io-note" }, "按键事件被识别过的数量；交互测试不是全键电气质量保证。")]
      : [h("div", { class: "empty" }, "未导出按键计数")];

    const devs = [["摄像头", av?.camera], ["麦克风", av?.microphone], ["扬声器", av?.speaker]];
    const avBody = av
      ? [h("dl", { class: "devices" }, devs.map(([k, v]) => h("div", null, h("dt", null, k), h("dd", null, v || "未检测到")))),
         av.note ? h("p", { class: "io-note" }, `页面状态：${av.note}`) : null,
         h("p", { class: "io-note" }, "设备名存在不代表可用；画面与声音须人工确认。")]
      : [h("div", { class: "empty" }, "未导出影音设备信息")];

    fill("ioPanels",
      h("article", { class: "io-card" }, h("h3", null, "键盘测试", stateChip("keyboard")), h("div", { class: "io-body" }, kbBody)),
      h("article", { class: "io-card" }, h("h3", null, "屏幕坏点", stateChip("screen")), h("div", { class: "io-body" },
        h("div", { class: "swatches", "aria-label": "全屏纯色轮播顺序：白、红、绿、蓝、黑" }, SCREEN_CYCLE.map(([n, c]) => h("span", { style: `background:${c}`, dataset: { name: n } }))),
        h("p", { class: "io-note" }, "全屏纯色轮播供人工观察；不导出坏点坐标，人工观察不等于专业仪器测量。"))),
      h("article", { class: "io-card" }, h("h3", null, "影音会议", stateChip("av")), h("div", { class: "io-body" }, avBody)));
  }

  // ── 已知信息对照：录入值与采集值并排，字面提示只帮人找差异，不做判定 ──
  const EXPECT_MAP = {
    model: [/^整机/, /^(厂商|型号)/],
    cpu: [/处理器/, /^型号(?:\s+#?\d+|\s*[（(].*)?$/],
    gpu: [/显卡/, /^型号(?:\s+#?\d+|\s*[（(].*)?$/],
    memory: [/^内存/, /^总容量/],
    storage: [/^硬盘/, /^硬盘/],
    display: [/^屏幕/, /^(分辨率|刷新率|尺寸)/],
    battery: [/^电池/, /^设计容量/],
  };
  function collectedFor(key) {
    const m = Object.hasOwn(EXPECT_MAP, key) ? EXPECT_MAP[key] : null;
    if (!m || !data.config) return [];
    return data.config.groups.filter((g) => m[0].test(g.title))
      .flatMap((g) => g.entries.filter((e) => e.name && m[1].test(e.name) && e.value.trim()).map((e) => e.value));
  }
  const norm = (s) => String(s).toLowerCase().replace(/\((r|tm)\)|®|™/g, "").replace(/×/g, "x").replace(/[\s,，、/（）()@\-_:：]+/g, " ").trim();
  function literalHint(expected, collected) {
    if (!collected.length) return { kind: "none", text: "未采集" };
    const hay = norm(collected.join(" ")), compact = hay.replace(/ /g, "");
    const tokens = norm(expected).split(" ").filter(Boolean);
    if (!tokens.length) return { kind: "none", text: "无可比字样" };
    return tokens.every((t) => hay.includes(t) || compact.includes(t)) ? { kind: "same", text: "字面包含" } : { kind: "diff", text: "字面不同" };
  }
  function renderExpected() {
    const fields = (data.expected?.fields || []).filter((f) => f && f.value && String(f.value).trim());
    if (!fields.length) { fill("expectedCompare"); return; }
    fill("expectedCompare", h("article", { class: "compare" },
      h("h3", null, `已知信息对照（验机前录入 · ${fmtTime(data.expected.enteredAt)}）`),
      h("div", { class: "tbl-wrap" }, h("table", { class: "tbl" },
        h("thead", null, h("tr", null, ["项目", "录入（已知 / 订单）", "采集值", "字面提示"].map((t) => h("th", null, t)))),
        h("tbody", null, fields.map((f) => {
          const got = collectedFor(f.key);
          const hint = !got.length && Object.hasOwn(EXPECT_MAP, f.key) ? { kind: "none", text: "未采集" }
            : f.key === "battery" ? { kind: "none", text: "单位需人工对照" }
            : Object.hasOwn(EXPECT_MAP, f.key) ? literalHint(f.value, got) : { kind: "none", text: "无对应采集项" };
          return h("tr", null,
            h("td", null, f.label || f.key),
            h("td", null, String(f.value)),
            h("td", null, got.length ? got.join(" / ") : "—"),
            h("td", null, h("span", { class: `cmp-hint ${hint.kind}` }, hint.text)));
        })))),
      h("p", { class: "sec-note", style: "margin:10px 14px" },
        "「字面包含」只表示录入的每个词都出现在采集值里，「字面不同」也可能只是写法不同（如 16G 与 16.0 GB）。是否一致由操作者判断。")));
  }

  // ── 指标总览抽屉：按类别收起关键指标，点开逐块展开 ──
  function entryOf(groupRe, nameRe) {
    const g = data.config?.groups.find((x) => groupRe.test(x.title));
    return g?.entries.find((e) => e.name && nameRe.test(e.name))?.value ?? null;
  }
  function metricTiles(cat) {
    const t = (k, v, opts = {}) => ({ k, v: v == null || v === "" ? "未取得" : String(v), ...opts });
    if (cat === "hardware") {
      return [
        t("处理器", entryOf(/处理器/, /^型号/), { small: true, href: "#sec-config" }),
        t("显卡", entryOf(/显卡/, /^型号/), { small: true, cls: "gpu", href: "#sec-config" }),
        t("内存总容量", entryOf(/^内存/, /^总容量/), { href: "#sec-config" }),
        t("硬盘", entryOf(/^硬盘/, /^硬盘/), { small: true, href: "#sec-config" }),
        t("屏幕", [entryOf(/^屏幕/, /^分辨率/), entryOf(/^屏幕/, /^刷新率/)].filter(Boolean).join(" · ") || null, { small: true, href: "#sec-config" }),
        t("电池损耗", entryOf(/^电池/, /^损耗/), { small: true, href: "#sec-config" }),
      ];
    }
    if (cat === "load") {
      const ss = stressSamples();
      const peak = (key) => ss.reduce((value, sample) => isNum(sample[key]) ? value == null ? sample[key] : Math.max(value, sample[key]) : value, null);
      const dur = ss.length ? stressDuration(ss) : null;
      const r = data.disk?.latest;
      return [
        t("烤机时长", dur == null ? null : fmtDur(dur), { href: "#sec-stress" }),
        t("CPU 温度峰值", peak("cpuTemp") == null ? null : `${fmtNum(peak("cpuTemp"))} °C`, { href: "#sec-stress" }),
        t("GPU 温度峰值", peak("gpuTemp") == null ? null : `${fmtNum(peak("gpuTemp"))} °C`, { cls: "gpu", href: "#sec-stress" }),
        t("CPU / GPU 功耗峰值", peak("cpuPower") == null && peak("gpuPower") == null ? null : `${fmtNum(peak("cpuPower"))} / ${fmtNum(peak("gpuPower"))} W`, { href: "#sec-stress" }),
        t("顺序读 / 写", r ? `${fmtNum(r.seq_read, 0)} / ${fmtNum(r.seq_write, 0)}` : null, { href: "#sec-disk" }),
        t("4K 随机读 / 写", r ? `${fmtNum(r.rnd4k_read)} / ${fmtNum(r.rnd4k_write)}` : null, { href: "#sec-disk" }),
      ];
    }
    if (cat === "io") {
      const kb = data.keyboard, av = data.av, st = (id) => {
        const m = moduleById(id);
        return !m ? "模块未加载" : m.terminal || m.passable === false ? "不参与通过确认" : m.done ? "已人工确认" : "待确认";
      };
      return [
        t("键盘", kb && isNum(kb.total) && isNum(kb.tested) ? `${kb.tested} / ${kb.total}` : null, { href: "#sec-io" }),
        t("键盘布局", kb?.layout ? `${kb.layout} 键` : null, { href: "#sec-io" }),
        t("屏幕坏点", st("screen"), { small: true, href: "#sec-io" }),
        t("影音会议", st("av"), { small: true, href: "#sec-io" }),
        t("摄像头", av?.camera, { small: true, cls: "muted", href: "#sec-io" }),
      ];
    }
    const mods = passable(), done = mods.filter((m) => m.done);
    const pending = mods.filter((m) => !m.done).map((m) => m.name);
    const unavailable = sortedModules().filter((m) => !m.terminal && m.passable === false).map((m) => m.name);
    return [
      t("人工确认", `${done.length} / ${mods.length}`, { href: "#sec-modules" }),
      t("待确认", pending.join("、") || "无", { small: true, href: "#sec-modules" }),
      t("不参与确认", unavailable.join("、") || "无", { small: true, href: "#sec-modules" }),
      t("已知信息录入", data.expected?.fields?.length ? `${data.expected.fields.filter((f) => f.value?.trim()).length} 项` : "未录入", { small: true, href: "#sec-config" }),
      t("源码状态", data.source?.sourceDirty === true ? "含未提交补丁" : data.source?.commit ? `@ ${data.source.commit}` : null, { small: true, cls: "muted", href: "#sec-report" }),
    ];
  }
  let openCat = null;
  function renderDrawer() {
    const drawer = $("metricDrawer");
    document.querySelectorAll(".btn-metric").forEach((b) => b.setAttribute("aria-expanded", String(b.dataset.cat === openCat)));
    if (!openCat || !data) { drawer.classList.remove("open"); return; }
    fill("metricDrawerInner", h("div", { class: "md-panel" }, metricTiles(openCat).map((x, i) =>
      h("div", { class: `md-tile${x.cls ? " " + x.cls : ""}`, style: `--i:${i}` },
        h("div", { class: "md-k" }, x.k),
        h("div", { class: `md-v${x.small ? " small" : ""}` }, x.v),
        x.href ? h("a", { href: x.href, onclick: () => { openCat = null; renderDrawer(); } }, "查看章节 →") : null))));
    drawer.classList.add("open");
  }
  document.querySelectorAll(".btn-metric").forEach((b) => b.addEventListener("click", () => {
    openCat = openCat === b.dataset.cat ? null : b.dataset.cat;
    renderDrawer();
  }));
  addEventListener("keydown", (e) => { if (e.key === "Escape" && openCat) { openCat = null; renderDrawer(); } });

  // ── 06 报告 ──
  function renderReport() {
    const src = data.source || {};
    const mods = passable();
    const done = mods.filter((m) => m.done), pending = mods.filter((m) => !m.done);
    const samples = stressSamples();
    const dur = stressDuration(samples);
    const kv = (pairs) => h("dl", { class: "kv" }, pairs.map(([k, v]) => h("div", null, h("dt", null, k), h("dd", null, v))));

    const left = h("div", null,
      h("article", { class: "r-card" }, h("h3", null, "版本与身份"),
        kv([
          ["设备编号", data.device?.label || "未填写"],
          ["导出时间", fmtTime(data.exportedAt)],
          ["配置采集", data.config ? fmtTime(data.config.collectedAt) : "未采集"],
          ["程序版本", src.appVersion || "未记录"],
          ["分支 / 提交", [src.branch, src.commit].filter(Boolean).join(" @ ") || "未记录"],
          ["源码状态", src.sourceDirty === true ? "含未提交补丁（sourceDirty）" : src.sourceDirty === false ? "与提交一致" : "未记录"],
          ["数据契约", data.schema],
        ]),
        src.sourceDirty ? h("p", { class: "warn" }, "! 只检出该提交不能精确还原此制品；请同时保存未提交补丁。") : null,
        Object.keys(data.sectionErrors || {}).length
          ? h("p", { class: "warn" }, `部分模块数据读取失败：${Object.entries(data.sectionErrors).map(([id, message]) => `${id}：${message}`).join("；")}`)
          : null),
      h("article", { class: "r-card", style: "margin-top:18px" }, h("h3", null, `人工确认 ${done.length} / ${mods.length}`),
        kv([["已确认", done.map((m) => m.name).join("、") || "无"], ["待确认", pending.map((m) => m.name).join("、") || "无"]]),
        h("h3", { style: "margin-top:16px" }, "人工备注"),
        h("p", { class: "remarks" }, data.remarks || "（无备注）"),
        h("div", { class: "signs" }, h("p", null, "人工记录人"), h("p", null, "记录时间"))));

    const right = h("article", { class: "r-card" }, h("h3", null, "本报告不能证明什么"),
      h("ol", { class: "cannot" }, [
        "「已人工确认」是操作者点击通过的记录；软件不做整机合格判定，本页也不生成“整机合格”结论。",
        "配置值不等于与订单一致：需要人工逐项对照订单与铭牌。",
        "多个字段是“主源 + 条件回退”，不是两个独立来源互相证明。",
        "传感器有数值不代表新鲜：上游更新失败可能保留旧值。",
        dur > 0 ? `本次散热记录 ${fmtDur(dur)}，不能外推为更长时间（例如连续三小时）的稳定性。` : "没有散热记录，不能推断散热或长时稳定性。",
        "CPU 负载调用 Prime95，GPU 负载为 C# / OpenTK 重实现，不能声称与任何官方工具负载等同。",
        "硬盘成绩是四项基准成绩，不是整机验收报告。",
        "本页不包含 AI、OCR、订单自动解析或自动比对功能。",
      ].map((t) => h("li", null, t))));

    fill("reportBody", left, right);
  }

  // ── 提示框 ──
  const tip = $("tooltip");
  function showTip(nodes, cx, cy) { tip.replaceChildren(); append(tip, nodes); tip.hidden = false; moveTip(cx, cy); }
  function moveTip(cx, cy) {
    const r = tip.getBoundingClientRect();
    let left = cx + 14, top = cy + 14;
    if (left + r.width > innerWidth - 8) left = cx - r.width - 14;
    if (top + r.height > innerHeight - 8) top = cy - r.height - 14;
    tip.style.left = `${Math.max(8, left)}px`;
    tip.style.top = `${Math.max(8, top)}px`;
  }
  function hideTip() { tip.hidden = true; }

  // ── 载入 ──
  function renderAll() {
    renderPoster();
    renderModules();
    renderConfig();
    renderExpected();
    renderStress();
    renderDisk();
    renderIo();
    renderReport();
    openCat = null;
    renderDrawer();
  }
  function showError(msg) {
    const el = $("loadError");
    el.textContent = msg ? `载入失败：${msg} ${data ? "页面保留上一份数据。" : "尚未载入报告，请选择有效 JSON 或载入示例。"}` : "";
    el.hidden = !msg;
  }
  function load(obj, label, sample) {
    const err = validate(obj);
    if (err) { showError(err); return; }
    const previous = { data, dataLabel, isSample };
    data = obj;
    dataLabel = label;
    isSample = sample;
    try {
      hideTip();
      renderAll();
      showError(null);
    } catch (error) {
      data = previous.data;
      dataLabel = previous.dataLabel;
      isSample = previous.isSample;
      if (data) renderAll();
      else {
        for (const id of ["moduleProgress", "moduleList", "configStats", "configGroups", "expectedCompare",
          "stressNote", "stressTiles", "stressCharts", "stressTable", "diskPanels", "diskHistory", "ioPanels", "reportBody", "posterMeta", "sourceLine", "vdate"])
          fill(id);
        $("sealCount").textContent = "–/–";
        $("sampleStamp").hidden = true;
      }
      showError(`报告无法显示（${error.message}）。`);
    }
  }
  function loadText(text, label) {
    let obj;
    try { obj = JSON.parse(text.replace(/^\uFEFF/, "")); } catch (e) { showError(`不是合法 JSON（${e.message}）。`); return; }
    load(obj, label, false);
  }
  function loadFile(file) {
    if (!file) return;
    return file.text().then((t) => loadText(t, file.name)).catch((e) => showError(e.message));
  }

  // ── 构成主义海报绘制 ──
  function drawPosterBg() {
    const canvas = $("posterBg");
    if (!canvas || !canvas.getContext) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    const styles = getComputedStyle(document.documentElement);
    const pal = Object.fromEntries(["paper", "ink", "red", "gold"].map((key) => [key, styles.getPropertyValue(`--${key}`).trim()]));

    // Background
    ctx.fillStyle = pal.paper;
    ctx.fillRect(0, 0, 260, 780);

    // Big circle
    ctx.fillStyle = pal.red;
    ctx.beginPath();
    ctx.arc(130, 180, 95, 0, 2 * Math.PI);
    ctx.fill();

    // Gold diagonal band
    ctx.fillStyle = pal.gold;
    ctx.globalAlpha = 0.92;
    ctx.beginPath();
    ctx.moveTo(180, 0);
    ctx.lineTo(220, 0);
    ctx.lineTo(90, 780);
    ctx.lineTo(50, 780);
    ctx.closePath();
    ctx.fill();
    ctx.globalAlpha = 1;

    // Bottom-left red triangle
    ctx.fillStyle = pal.red;
    ctx.beginPath();
    ctx.moveTo(0, 650);
    ctx.lineTo(80, 520);
    ctx.lineTo(90, 650);
    ctx.closePath();
    ctx.fill();

    // Right bar
    ctx.fillStyle = pal.ink;
    ctx.fillRect(210, 420, 50, 200);

    // Circle hole
    ctx.fillStyle = pal.paper;
    ctx.beginPath();
    ctx.arc(235, 520, 22, 0, 2 * Math.PI);
    ctx.fill();

    // Red triangle inside hole
    ctx.fillStyle = pal.red;
    ctx.beginPath();
    ctx.moveTo(235, 505);
    ctx.lineTo(247, 520);
    ctx.lineTo(235, 535);
    ctx.closePath();
    ctx.fill();

    // Diagonal line
    ctx.strokeStyle = pal.ink;
    ctx.lineWidth = 3.5;
    ctx.beginPath();
    ctx.moveTo(30, 0);
    ctx.lineTo(230, 650);
    ctx.stroke();

    // Gauge marks
    ctx.fillStyle = pal.ink;
    ctx.fillRect(12, 380, 8, 48);
    ctx.fillRect(12, 445, 8, 15);
    ctx.fillRect(12, 472, 8, 15);
    ctx.fillRect(12, 499, 8, 15);
  }

  // ── 初始化 ──
  const themeMedia = window.matchMedia("(prefers-color-scheme: dark)");
  let theme = "auto";
  function applyTheme() {
    const dark = theme === "dark" || (theme === "auto" && themeMedia.matches);
    document.documentElement.dataset.theme = dark ? "dark" : "light";
    $("btnTheme").textContent = `主题：${theme === "auto" ? "跟随系统" : dark ? "深色" : "浅色"}`;
    $("btnTheme").setAttribute("aria-label", `${$("btnTheme").textContent}；点击切换主题`);
    drawPosterBg();
  }
  $("btnTheme").addEventListener("click", () => {
    theme = theme === "auto" ? themeMedia.matches ? "light" : "dark" : theme === "dark" ? "light" : "auto";
    applyTheme();
  });
  themeMedia.addEventListener("change", applyTheme);
  applyTheme();

  $("btnLoad").addEventListener("click", () => $("fileInput").click());
  $("fileInput").addEventListener("change", (e) => { loadFile(e.target.files[0]); e.target.value = ""; });
  $("btnSample").addEventListener("click", () => load(window.BIBA_SAMPLE, "示例数据", true));
  $("btnPrint").addEventListener("click", () => window.print());
  $("btnExport").addEventListener("click", () => {
    if (!data) return;
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: "application/json" });
    const a = h("a", { href: URL.createObjectURL(blob), download: `inspection-summary-${(data.device?.label || "export").replace(/[^\w.-]+/g, "_")}.json` });
    document.body.append(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(a.href), 1000);
  });
  document.querySelectorAll(".chip[data-filter]").forEach((c) => c.addEventListener("click", () => applyFilter(c.dataset.filter)));

  const dropHint = $("dropHint");
  let dragDepth = 0;
  const hasFiles = (e) => [...(e.dataTransfer?.types || [])].includes("Files");
  addEventListener("dragenter", (e) => { if (!hasFiles(e)) return; e.preventDefault(); dragDepth++; dropHint.hidden = false; });
  addEventListener("dragover", (e) => { if (hasFiles(e)) e.preventDefault(); });
  addEventListener("dragleave", () => { if (--dragDepth <= 0) { dragDepth = 0; dropHint.hidden = true; } });
  addEventListener("drop", (e) => {
    if (!hasFiles(e)) return;
    e.preventDefault();
    dragDepth = 0;
    dropHint.hidden = true;
    loadFile(e.dataTransfer.files[0]);
  });

  let resizeTimer;
  addEventListener("resize", () => { clearTimeout(resizeTimer); resizeTimer = setTimeout(() => data && renderStress(), 150); });

  const openedForPrint = [];
  addEventListener("beforeprint", () => {
    document.querySelectorAll("details:not([open])").forEach((d) => { d.open = true; openedForPrint.push(d); });
  });
  addEventListener("afterprint", () => { openedForPrint.splice(0).forEach((d) => { d.open = false; }); });

  // ?data=<url> 仅在 http(s) 下可用；file:// 双击打开时用「载入 JSON」或拖放
  const url = new URLSearchParams(location.search).get("data");
  if (url && /^https?:$/.test(location.protocol)) {
    fetch(url).then((r) => { if (!r.ok) throw new Error(`HTTP ${r.status}`); return r.text(); })
      .then((t) => loadText(t, url.split("/").pop()))
      .catch((e) => { load(window.BIBA_SAMPLE, "示例数据", true); showError(`无法读取 ${url}（${e.message}）。`); });
  } else if (Object.hasOwn(window, "BIBA_REPORT")) {
    load(window.BIBA_REPORT, window.BIBA_REPORT_LABEL || "本机验机报告", false);
  } else {
    load(window.BIBA_SAMPLE, "示例数据", true);
  }
})();
