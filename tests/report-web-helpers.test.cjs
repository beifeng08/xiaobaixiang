// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

const assert = require("node:assert/strict");
const crypto = require("node:crypto");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const { test } = require("node:test");

const sourcePath = process.env.REPORT_APP_SOURCE || path.join(__dirname, "../验机软件/ReportWeb/app.js");
const source = fs.readFileSync(sourcePath, "utf8");
const boundary = source.indexOf("  const textField =");
assert.ok(boundary > 0, "The real helper section must exist before the contract");
const trace = [];
class Element {
  constructor(tag) { this.tag = tag; this.children = []; this.attributes = {}; this.dataset = {}; this.events = []; }
  append(child) { this.children.push(child); }
  replaceChildren() { this.children = []; }
  setAttribute(key, value) { this.attributes[key] = String(value); trace.push(["attribute", key, String(value)]); }
  addEventListener(name, callback) { this.events.push([name, callback]); trace.push(["event", name]); }
}
const target = new Element("target");
const document = {
  createElement: (tag) => new Element(tag),
  createElementNS: (namespace, tag) => { const element = new Element(tag); element.namespace = namespace; return element; },
  getElementById: (id) => id === "target" ? target : null,
};
const context = vm.createContext({ document, Node: Element });
vm.runInContext(source.slice(0, boundary) + `
  globalThis.helpers = { h, svg, setAttrs, append, fill, fmtTime, fmtDur, fmtNum, isRecord, niceStep, niceCeil, fmtAxis };
})();`, context, { timeout: 3000 });
const helpers = context.helpers;
const observations = [];
const capture = (value) => observations.push(value);
const plain = (value) => JSON.parse(JSON.stringify(value));
const numeric = (value) => Number.isNaN(value) ? "NaN" : Object.is(value, -0) ? "-0" : String(value);

// These tests execute the production helpers; only the browser's DOM is substituted.
test("HTML and SVG helpers preserve attributes, nested text and event callbacks", () => {
  const callback = () => "clicked";
  const child = new Element("strong");
  const element = helpers.h("div", { class: "card", dataset: { kind: "hardware" }, onclick: callback,
    hidden: false, disabled: true, title: "<raw>" }, "<script>alert(1)</script>", [0, [child, null, false]], "中文");
  assert.equal(element.tag, "div");
  assert.deepEqual(element.attributes, { class: "card", disabled: "", title: "<raw>" });
  assert.deepEqual(element.dataset, { kind: "hardware" });
  assert.deepEqual(element.children, ["<script>alert(1)</script>", "0", child, "中文"]);
  assert.equal(element.events[0][0], "click");
  assert.equal(element.events[0][1], callback);
  assert.equal(element.events[0][1](), "clicked");
  const svg = helpers.svg("text", { x: 0, "aria-label": "读数" }, "<unsafe>");
  assert.equal(svg.namespace, "http://www.w3.org/2000/svg");
  assert.deepEqual(svg.children, ["<unsafe>"]);
  capture([element.tag, element.attributes, element.dataset, element.children.map((value) => value === child ? "<node>" : value), svg.namespace, svg.attributes, svg.children]);
});

test("attribute branches keep null skipping, true conversion and enumeration order", () => {
  const element = new Element("span");
  const start = trace.length;
  helpers.setAttrs(element, null);
  helpers.setAttrs(element, { skipped: undefined, ignored: null, false: false, zero: 0, empty: "", yes: true,
    class: "test", dataset: { one: 1 }, oninput: () => {} });
  assert.deepEqual(element.attributes, { zero: "0", empty: "", yes: "", class: "test" });
  assert.deepEqual(trace.slice(start).map((entry) => entry.slice(0, 2)),
    [["attribute", "zero"], ["attribute", "empty"], ["attribute", "yes"], ["attribute", "class"], ["event", "input"]]);
  const reads = [];
  const attrs = { get first() { reads.push("first"); return "a"; }, get second() { reads.push("second"); return "b"; } };
  helpers.setAttrs(element, attrs);
  assert.deepEqual(reads, ["first", "second"]);
  capture([element.attributes, element.dataset, reads, trace.slice(start)]);
});

test("append and fill retain flattening, coercion, clearing and errors", () => {
  const element = new Element("p");
  const node = new Element("em");
  helpers.append(element, [undefined, null, false, true, "", 0, -1, [[node]], { toString: () => "object-text" }]);
  assert.deepEqual(element.children, ["true", "", "0", "-1", node, "object-text"]);
  target.children.push("old");
  assert.equal(helpers.fill("target", "new", [null, "nested"]), target);
  assert.deepEqual(target.children, ["new", "nested"]);
  helpers.fill("target");
  assert.deepEqual(target.children, []);
  assert.throws(() => helpers.fill("missing", "text"), { name: "TypeError" });
  assert.throws(() => helpers.append(element, null), { name: "TypeError" });
  const large = new Element("large");
  helpers.append(large, Array.from({ length: 10000 }, (_, index) => index));
  assert.equal(large.children.length, 10000);
  assert.equal(large.children[9999], "9999");
  capture([element.children.map((value) => value === node ? "<node>" : value), target.children, large.children.length, large.children[9999], "TypeError", "TypeError"]);
});

test("time formatting keeps wall clock, fallback text and coercion errors", () => {
  const cases = [[null, "未记录"], [undefined, "未记录"], ["", "未记录"], [0, "未记录"],
    ["2026-10-21T12:34:56+08:00", "2026-10-21 12:34:56"], ["2026-10-21 12:34", "2026-10-21 12:34"],
    ["invalid", "invalid"], [123, "123"], ["2026-99-99T99:99", "2026-99-99 99:99"]];
  for (const [value, expected] of cases) assert.equal(helpers.fmtTime(value), expected);
  assert.throws(() => helpers.fmtTime(Symbol("time")), { name: "TypeError" });
  capture(cases.map(([value]) => helpers.fmtTime(value)));
});

test("duration covers rounding, negative values, hours and nonfinite inputs", () => {
  const cases = [[0, "00:00"], [-10, "00:00"], [59.49, "00:59"], [59.5, "01:00"], [3599.5, "1:00:00"],
    [3601, "1:00:01"], [360000, "100:00:00"], [NaN, "超出显示范围"], [Infinity, "超出显示范围"],
    [null, "超出显示范围"], ["60", "超出显示范围"]];
  for (const [value, expected] of cases) assert.equal(helpers.fmtDur(value), expected);
  capture(cases.map(([value]) => helpers.fmtDur(value)));
});

test("number and record helpers preserve type checks and decimal errors", () => {
  const values = [null, undefined, "1", NaN, Infinity, 0, -0, -1.25, 1.25, Number.MAX_VALUE];
  const formatted = values.map((value) => helpers.fmtNum(value));
  assert.deepEqual(formatted.slice(0, 5), ["空", "空", "空", "空", "空"]);
  assert.equal(helpers.fmtNum(1.25, 2), "1.25");
  assert.equal(helpers.fmtNum(1.25, 0), "1");
  assert.throws(() => helpers.fmtNum(1, 101), { name: "RangeError" });
  const records = [null, undefined, [], 0, "", () => {}, {}, Object.create(null), new Date(0)];
  const results = records.map((value) => helpers.isRecord(value));
  assert.deepEqual(results, [false, false, false, false, false, false, true, true, true]);
  capture([formatted, results, helpers.fmtNum(1.25, 2), helpers.fmtNum(1.25, 0), "RangeError"]);
});

test("chart helpers cover nice-step thresholds, ceilings, axes and extremes", () => {
  const steps = [[0.1, 0.1], [1, 1], [1.01, 2], [2, 2], [2.01, 5], [5, 5], [5.01, 10], [10, 10]];
  for (const [input, expected] of steps) assert.equal(helpers.niceStep(input), expected);
  assert.equal(helpers.niceCeil(13), 15);
  assert.equal(helpers.niceCeil(100), 100);
  assert.equal(helpers.fmtAxis(1.23456, 0.01), "1.23");
  assert.equal(helpers.fmtAxis(1.23456, 0.000001), "1.2346");
  assert.equal(helpers.fmtAxis(-0, 1), "0");
  assert.equal(helpers.fmtAxis(1, -1), "1");
  assert.throws(() => helpers.fmtAxis("1", 1), { name: "TypeError" });
  const extremes = [0, -1, NaN, Infinity, Number.MIN_VALUE, Number.MAX_VALUE];
  capture([steps.map(([value]) => helpers.niceStep(value)), extremes.map((value) => numeric(helpers.niceStep(value))),
    extremes.map((value) => numeric(helpers.niceCeil(value))), [helpers.niceCeil(13), helpers.niceCeil(100)],
    [helpers.fmtAxis(1.23456, 0.01), helpers.fmtAxis(1.23456, 0.000001), helpers.fmtAxis(-0, 1), helpers.fmtAxis(1, -1)], "TypeError"]);
});

test("helper contract keeps callable names and arity, with a stable behavior digest", () => {
  const arities = { h: 2, svg: 2, setAttrs: 2, append: 2, fill: 1, fmtTime: 1, fmtDur: 1, fmtNum: 1,
    isRecord: 1, niceStep: 1, niceCeil: 1, fmtAxis: 2 };
  for (const [name, arity] of Object.entries(arities)) {
    assert.equal(helpers[name].name, name);
    assert.equal(helpers[name].length, arity);
  }
  capture(plain(arities));
  const digest = crypto.createHash("sha256").update(JSON.stringify(observations)).digest("hex");
  console.log("HELPER_BEHAVIOR_SHA256=" + digest);
});
