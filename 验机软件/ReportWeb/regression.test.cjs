const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const { test } = require("node:test");

const read = (name) => fs.readFileSync(path.join(__dirname, name), "utf8");
const html = read("index.html");
const source = read("app.js").replace(/\}\)\(\);\s*$/, `
  window.testing = { validate, CONTRACT, load, loadText, loadFile, collectedFor, literalHint,
    fmtAxis, metricTiles, stateChip, renderStress, applyFilter };
})();`);

class Element {
  constructor(tag = "div") {
    this.tagName = tag;
    this.children = [];
    this.dataset = {};
    this.attributes = {};
    this.events = {};
    this.style = {};
    this.hidden = false;
    this.clientWidth = 600;
    this.classList = {
      contains: (name) => (this.attributes.class || "").split(" ").includes(name),
      toggle: (name, enabled) => {
        const names = new Set((this.attributes.class || "").split(" ").filter(Boolean));
        if (enabled) names.add(name); else names.delete(name);
        this.attributes.class = [...names].join(" ");
      },
      add: (name) => this.classList.toggle(name, true),
      remove: (name) => this.classList.toggle(name, false),
    };
  }
  append(...children) { this.children.push(...children); }
  replaceChildren(...children) { this.children = children; }
  set textContent(value) { this.children = [String(value)]; }
  get textContent() { return this.children.map((child) => child instanceof Element ? child.textContent : String(child)).join(""); }
  setAttribute(name, value) { this.attributes[name] = String(value); }
  getAttribute(name) { return this.attributes[name]; }
  addEventListener(name, callback) { (this.events[name] ||= []).push(callback); }
  click() { for (const callback of this.events.click || []) callback({ target: this }); }
  remove() {}
  getBoundingClientRect() { return { left: 0, top: 0, bottom: 260, width: 600, height: 260 }; }
  descendants() { return this.children.flatMap((child) => child instanceof Element ? [child, ...child.descendants()] : []); }
  querySelectorAll(selector) {
    if (selector === ":scope > .rows > .row") return this.children
      .filter((child) => child instanceof Element && child.classList.contains("rows"))
      .flatMap((child) => child.children.filter((row) => row instanceof Element && row.classList.contains("row")));
    if (selector === ":scope > details") return this.children.filter((child) => child.tagName === "details");
    return [];
  }
  querySelector(selector) { return this.querySelectorAll(selector)[0] || null; }
}

function setup(options = {}) {
  const elements = Object.fromEntries([...html.matchAll(/id="([^"]+)"/g)].map((match) => [match[1], new Element()]));
  const buttons = ["hardware", "load", "io", "review"].map((category) => {
    const button = new Element("button"); button.dataset.cat = category; return button;
  });
  const filters = ["all", "review", "missing", "scope"].map((kind) => {
    const button = new Element("button"); button.dataset.filter = kind; return button;
  });
  const canvasColors = [];
  const canvas = new Proxy({}, { get: (target, key) => target[key] || (() => {}),
    set: (target, key, value) => { target[key] = value; if (key === "fillStyle") canvasColors.push(value); return true; } });
  elements.posterBg.getContext = () => options.noCanvas ? null : canvas;
  const events = {};
  const media = { matches: options.dark || false, addEventListener: (name, callback) => { media.changed = callback; } };
  const document = {
    documentElement: new Element("html"), body: new Element("body"),
    getElementById: (id) => elements[id],
    createElement: (tag) => new Element(tag), createElementNS: (namespace, tag) => new Element(tag),
    querySelectorAll: (selector) => selector === ".btn-metric" ? buttons
      : selector === ".chip[data-filter]" ? filters
      : selector === "#configGroups .group" ? elements.configGroups.descendants().filter((element) => element.classList.contains("group"))
      : [],
  };
  const context = vm.createContext({ document, Node: Element, URLSearchParams, URL, Blob,
    location: { search: "", protocol: "file:" }, innerWidth: 1280, innerHeight: 900,
    setTimeout, clearTimeout, addEventListener: (name, callback) => { events[name] = callback; },
    matchMedia: () => media, print: () => {},
    getComputedStyle: () => ({ getPropertyValue: (name) => `${document.documentElement.dataset.theme}:${name}` }),
  });
  context.window = context;
  if (Object.hasOwn(options, "report")) context.BIBA_REPORT = options.report;
  vm.runInContext(read("sample-data.js"), context);
  vm.runInContext(source, context, { timeout: 3000 });
  return { api: context.testing, elements, document, buttons, filters, context, media, canvasColors, events };
}

const copy = (value) => JSON.parse(JSON.stringify(value));
const minimum = () => ({ schema: "biba-inspection-summary/1", exportedAt: "2026-10-21T12:00:00+08:00", modules: [] });

function structuralRule(rule, root) {
  if (rule.$ref) return structuralRule(root.$defs[rule.$ref.split("/").pop()], root);
  const result = {};
  for (const [key, value] of Object.entries(rule)) {
    if (["type", "const", "enum", "minimum", "exclusiveMinimum", "format"].includes(key)) result[key] = value;
    if (key === "required" && value.length) result[key] = [...value].sort();
    if (key === "properties") result[key] = Object.fromEntries(Object.entries(value).map(([name, child]) => [name, structuralRule(child, root)]));
    if (["items", "additionalProperties"].includes(key)) result[key] = structuralRule(value, root);
  }
  return result;
}

test("offline validation contract matches the published schema", () => {
  const { api } = setup();
  const schema = JSON.parse(read("inspection-summary.schema.json"));
  assert.deepEqual(copy(structuralRule(api.CONTRACT, api.CONTRACT)), structuralRule(schema, schema));
});

test("sample and host-shaped null values render without claiming missing sections passed", () => {
  const { api, elements, context } = setup();
  assert.equal(elements.loadError.hidden, true);
  assert.equal(elements.sealCount.textContent, "5/6");
  assert.equal(api.validate(context.BIBA_SAMPLE), null);
  const report = { ...minimum(), source: { appVersion: null }, device: { label: null },
    config: null, keyboard: null, av: null, disk: null,
    stress: { sensors: null, samples: null }, expected: { fields: [{ key: "cpu", label: null, value: null }] } };
  api.load(report, "null.json", false);
  assert.equal(elements.loadError.hidden, true);
  assert.match(elements.posterMeta.textContent, /设备编号未填/);
  assert.match(elements.configGroups.textContent, /未导出配置采集结果/);
  assert.equal(elements.sealCount.textContent, "0/0");
});

test("nested malformed imports report field paths and preserve the previous report", () => {
  const { api, elements, context } = setup();
  const before = elements.reportBody.textContent;
  const invalid = [null, [], {}, { ...minimum(), exportedAt: {} }, { ...minimum(), source: [] },
    { ...minimum(), device: { label: {} } }, { ...minimum(), av: { camera: [] } },
    { ...minimum(), exportedAt: "2026-02-30T12:00:00+08:00" },
    { ...minimum(), exportedAt: "2026-10-21T24:00:00+08:00" },
    { ...minimum(), config: { collectedAt: minimum().exportedAt, groups: [{ title: "CPU", entries: [null] }] } },
    { ...minimum(), config: { collectedAt: minimum().exportedAt, groups: [{ title: "CPU", entries: [], details: [null] }] } },
    { ...minimum(), expected: { fields: {} } }, { ...minimum(), expected: { fields: [null] } },
    { ...minimum(), stress: { sensors: [] } }, { ...minimum(), stress: { sensors: { cpuTemp: {} } } },
    { ...minimum(), stress: { samples: [null] } }, { ...minimum(), stress: { samples: [{ t: 0, cpuTemp: "80" }] } },
    { ...minimum(), stress: { sampleIntervalSeconds: 0 } }, { ...minimum(), disk: { history: [null] } },
    { ...minimum(), disk: { latest: { seq_read: "100" } } }, { ...minimum(), disk: { history: [{ time: {} }] } },
    { ...minimum(), disk: { latest: { seq_read: -1 } } },
    { ...minimum(), keyboard: { tested: -1 } }, { ...minimum(), sectionErrors: { stress: {} } }];
  const wrongFlag = copy(context.BIBA_SAMPLE); wrongFlag.modules[0].passable = "false"; invalid.push(wrongFlag);
  for (const report of invalid) {
    assert.ok(api.validate(report));
    assert.doesNotThrow(() => api.loadText(JSON.stringify(report), "bad.json"));
    assert.equal(elements.loadError.hidden, false);
    assert.equal(elements.reportBody.textContent, before);
    assert.equal(elements.sealCount.textContent, "5/6");
  }
  api.loadText("{broken", "broken.json");
  assert.match(elements.loadError.textContent, /不是合法 JSON/);
  assert.match(api.validate(invalid[9]), /entries\[0\]/);
  assert.match(api.validate(invalid[10]), /details\[0\]/);
  api.loadText("\uFEFF" + JSON.stringify(minimum()), "bom.json");
  assert.equal(elements.loadError.hidden, true);
});

test("passable and terminal modules agree across cards, progress, IO and drawer", () => {
  const { api, elements, context } = setup();
  const report = copy(context.BIBA_SAMPLE);
  report.modules[1].passable = false;
  report.modules[2].terminal = true;
  api.load(report, "flags.json", false);
  assert.equal(elements.sealCount.textContent, "3/4");
  assert.equal(elements.moduleProgress.children.length, 4);
  assert.match(elements.moduleList.children[1].textContent, /不参与通过确认/);
  assert.match(elements.moduleList.children[2].textContent, /终点页/);
  assert.doesNotMatch(elements.moduleList.children[1].getAttribute("class"), /done|pending/);
  assert.equal(api.stateChip("keyboard").textContent, "不参与通过确认");
  assert.equal(api.metricTiles("io").find((tile) => tile.k === "屏幕坏点").v, "不参与通过确认");
  delete report.modules[0].passable;
  api.load(report, "legacy.json", false);
  assert.equal(elements.sealCount.textContent, "3/4");
});

test("known fields map real plugin labels and unknown prototype names remain plain text", () => {
  const { api, elements, context } = setup();
  assert.ok(api.collectedFor("cpu").includes("Intel Core i7-13700H"));
  assert.equal(api.collectedFor("gpu").length, 2);
  for (const key of ["model", "memory", "storage", "display", "battery"]) assert.ok(api.collectedFor(key).length);
  const report = copy(context.BIBA_SAMPLE);
  report.config.groups[0].entries = [{ name: "型号（WMI） #1", value: "WMI CPU" }];
  report.expected.fields.push({ key: "__proto__", label: null, value: "<img src=x onerror=alert(1)>" },
    { key: "constructor", label: "未知", value: "x" }, { key: "toString", label: "未知", value: "x" });
  api.load(report, "unknown.json", false);
  assert.equal(elements.loadError.hidden, true);
  assert.equal(api.collectedFor("cpu")[0], "WMI CPU");
  assert.match(elements.expectedCompare.textContent, /无对应采集项/);
  assert.match(elements.expectedCompare.textContent, /<img src=x/);
  assert.ok(!elements.expectedCompare.descendants().some((element) => element.tagName === "img"));
  assert.equal(api.literalHint("RTX 4060 8 G", ["RTX 4060 6 GB"]).text, "字面不同");
});

test("charts keep fractional ticks, missing values, zero bars and sorted duration consistent", () => {
  const { api, elements } = setup();
  api.load({ ...minimum(), stress: { sensors: null, samples: [{ t: 1, cpuTemp: 40.2 }, { t: 0, cpuTemp: 40.1 }] },
    disk: { latest: { seq_read: 0, seq_write: 10, rnd4k_read: 0, rnd4k_write: 0 } } }, "short.json", false);
  assert.equal(elements.loadError.hidden, true);
  assert.equal(api.fmtAxis(40.2, 0.2), "40.2");
  const axes = elements.stressCharts.descendants().filter((element) => element.getAttribute("class") === "axis");
  assert.match(axes[0].textContent, /40\.2/);
  assert.match(elements.stressTiles.textContent, /00:02/);
  assert.match(elements.reportBody.textContent, /本次散热记录 00:02/);
  assert.equal(api.metricTiles("load")[0].v, "00:02");
  assert.ok(!elements.stressCharts.descendants().some((element) => Object.values(element.attributes).some((value) => /NaN|Infinity/.test(value))));
  const fills = elements.diskPanels.descendants().filter((element) => element.getAttribute("class") === "bar-fill");
  assert.equal(fills[0].getAttribute("style"), "width:0%");
  assert.match(elements.stressNote.textContent, /未映射/);
  api.load({ ...minimum(), stress: { samples: [{ t: 0, cpuTemp: 0 }] } }, "one.json", false);
  assert.equal(api.metricTiles("load")[0].v, "00:01");
  assert.match(elements.reportBody.textContent, /本次散热记录 00:01/);
});

test("long recordings avoid argument-count limits and extreme charts fail visibly", () => {
  const { api, elements } = setup();
  const report = { ...minimum(), stress: { samples: Array.from({ length: 70000 }, (_, index) => ({ t: index, cpuTemp: 60, gpuTemp: 40, cpuPower: 15, gpuPower: null })) } };
  api.load(report, "long.json", false);
  assert.equal(elements.loadError.hidden, true);
  assert.equal(api.metricTiles("load")[1].v, "60.0 °C");
  api.load({ ...minimum(), stress: { samples: [{ t: 0, cpuTemp: 1e308 }, { t: 1, cpuTemp: -1e308 }] } }, "extreme.json", false);
  assert.equal(elements.loadError.hidden, false);
  assert.match(elements.loadError.textContent, /超出图表/);
  assert.match(elements.sourceLine.textContent, /long.json/);
});

test("theme changes repaint canvas and manual theme survives system changes", () => {
  const { elements, document, media, canvasColors } = setup();
  assert.equal(document.documentElement.dataset.theme, "light");
  media.matches = true; media.changed();
  assert.equal(document.documentElement.dataset.theme, "dark");
  assert.equal(canvasColors.at(-1), "dark:--ink");
  elements.btnTheme.click();
  assert.equal(document.documentElement.dataset.theme, "light");
  media.changed();
  assert.equal(document.documentElement.dataset.theme, "light");
  assert.equal(setup({ noCanvas: true }).elements.loadError.hidden, true);
});

test("file reading failures and invalid embedded reports are visible", async () => {
  const { api, elements } = setup();
  await api.loadFile({ name: "denied.json", text: () => Promise.reject(new Error("读取被拒绝")) });
  assert.match(elements.loadError.textContent, /读取被拒绝/);
  const invalid = setup({ report: null });
  assert.equal(invalid.elements.loadError.hidden, false);
  assert.match(invalid.elements.loadError.textContent, /尚未载入报告/);
});
