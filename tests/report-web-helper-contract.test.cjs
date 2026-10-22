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
assert.ok(boundary > 0);
const calls = [];
class Element {
  constructor(tag) { this.tag = tag; this.children = []; this.attributes = {}; this.dataset = {}; }
  append(...children) { calls.push(["append", children.length]); this.children.push(...children); }
  replaceChildren(...children) { calls.push(["replaceChildren", children.length]); this.children = [...children]; }
  setAttribute(...args) { calls.push(["setAttribute", args.length]); this.attributes[args[0]] = String(args[1]); }
  addEventListener(...args) { calls.push(["addEventListener", args.length, args[0]]); }
}
const target = new Element("target");
const document = {
  createElement: (...args) => { calls.push(["createElement", ...args]); return new Element(args[0]); },
  createElementNS: (...args) => { calls.push(["createElementNS", ...args]); const element = new Element(args[1]); element.namespace = args[0]; return element; },
  getElementById: (...args) => { calls.push(["getElementById", ...args]); return target; },
};
const context = vm.createContext({ document, Node: Element });
vm.runInContext(source.slice(0, boundary) + `
  globalThis.helpers = { h, svg, setAttrs, append, fill, fmtTime, fmtDur, fmtNum, isRecord, niceStep, niceCeil, fmtAxis };
})();`, context, { timeout: 3000 });
const helpers = context.helpers;
const observations = [];

// Independent additive checks; the original frozen helper test remains unchanged.
test("DOM factories and fill keep exact method arguments and operation order", () => {
  calls.length = 0;
  const html = helpers.h("p", { title: "name", onclick: () => {} }, "a", "b");
  assert.deepEqual(html.children, ["a", "b"]);
  assert.deepEqual(calls, [["createElement", "p"], ["setAttribute", 2], ["addEventListener", 2, "click"], ["append", 1], ["append", 1]]);
  observations.push(calls.slice());
  calls.length = 0;
  const svg = helpers.svg("text", { x: 0 }, "label");
  assert.equal(svg.tag, "text");
  assert.equal(svg.namespace, "http://www.w3.org/2000/svg");
  assert.deepEqual(svg.children, ["label"]);
  assert.deepEqual(calls, [["createElementNS", "http://www.w3.org/2000/svg", "text"], ["setAttribute", 2], ["append", 1]]);
  observations.push(calls.slice());
  calls.length = 0;
  assert.equal(helpers.fill("target", "new"), target);
  assert.deepEqual(target.children, ["new"]);
  assert.deepEqual(calls, [["getElementById", "target"], ["replaceChildren", 0], ["append", 1]]);
  observations.push(calls.slice());
});

test("target property reads and attribute enumeration retain count and ordering", () => {
  const reads = [];
  const target = new Proxy({ dataset: {}, setAttribute() {}, addEventListener() {} }, {
    get(object, key) { reads.push("target:" + key); return object[key]; },
  });
  const attrs = { get dataset() { reads.push("input:dataset"); return { kind: "cpu" }; },
    get title() { reads.push("input:title"); return "label"; },
    get onclick() { reads.push("input:onclick"); return () => {}; } };
  helpers.setAttrs(target, attrs);
  assert.deepEqual(reads, ["input:dataset", "input:title", "input:onclick", "target:dataset", "target:setAttribute", "target:addEventListener"]);
  observations.push(reads);
  const appendReads = [];
  const child = { toString() { appendReads.push("coerce"); return "text"; } };
  helpers.append({ get append() { appendReads.push("get:append"); return (...args) => appendReads.push(args); } }, [child]);
  assert.deepEqual(appendReads, ["get:append", "coerce", ["text"]]);
  observations.push(appendReads);
});

test("coercion errors preserve timing and partial DOM state", () => {
  const order = [];
  const value = { toString() { order.push("coerce"); return "fallback"; } };
  assert.equal(helpers.fmtTime(value), "fallback");
  assert.deepEqual(order, ["coerce", "coerce"]);
  const children = [];
  assert.throws(() => helpers.append({ append: (...args) => children.push(...args) },
    ["before", { toString() { throw new Error("coercion-failed"); } }, "after"]), /coercion-failed/);
  assert.deepEqual(children, ["before"]);
  const element = new Element("span");
  assert.throws(() => helpers.setAttrs(element, { first: "one", get second() { throw new Error("getter-failed"); } }), /getter-failed/);
  assert.deepEqual(element.attributes, {});
  observations.push([order, children, element.attributes]);
});

test("numeric defaults and invalid chart boundaries have direct assertions", () => {
  assert.equal(helpers.fmtNum(1.25), "1.3");
  assert.equal(helpers.fmtNum(-1.25), "-1.3");
  assert.equal(helpers.fmtNum(-0), "0.0");
  assert.throws(() => helpers.fmtNum(1, -1), { name: "RangeError" });
  assert.equal(helpers.niceStep(0), 0);
  assert.ok(Number.isNaN(helpers.niceStep(-1)));
  assert.ok(Number.isNaN(helpers.niceCeil(0)));
  assert.ok(Number.isNaN(helpers.niceCeil(Infinity)));
  assert.equal(helpers.fmtAxis(1.23456, 0), "1.2346");
  assert.equal(helpers.fmtAxis(1.23456, Infinity), "1");
  assert.equal(helpers.fmtAxis(1.23456, NaN), "1");
  observations.push([helpers.fmtNum(1.25), helpers.fmtNum(-1.25), helpers.fmtNum(-0),
    helpers.fmtAxis(1.23456, 0), helpers.fmtAxis(1.23456, Infinity), helpers.fmtAxis(1.23456, NaN)]);
});

test("diagnostic messages are captured separately from stable business observations", () => {
  const messages = {};
  const invalid = { axis: () => helpers.fmtAxis("1", 1), append: () => helpers.append({}, {}), attributes: () => helpers.setAttrs({}, { title: "x" }) };
  for (const [name, invoke] of Object.entries(invalid)) {
    try { invoke(); assert.fail("Expected invalid input to throw"); }
    catch (error) { assert.equal(error.name, "TypeError"); messages[name] = error.message; }
  }
  console.log("DIAGNOSTICS_JSON=" + JSON.stringify(messages));
  console.log("CONTRACT_BEHAVIOR_SHA256=" + crypto.createHash("sha256").update(JSON.stringify(observations)).digest("hex"));
});
