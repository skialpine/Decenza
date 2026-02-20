#pragma once

#include <QString>
#include "base_css.h"
#include "menu_css.h"
#include "menu_html.h"
#include "menu_js.h"

inline QString generateThemePageHtml()
{
    QString html = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Theme Editor - Decenza DE1</title>
<style>
)HTML";

    html += WEB_CSS_VARIABLES;
    html += WEB_CSS_MENU;

    html += R"HTML(
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    background: var(--bg);
    color: var(--text);
    min-height: 100vh;
}

.header {
    display: flex;
    align-items: center;
    padding: 16px 20px;
    background: var(--surface);
    border-bottom: 1px solid var(--border);
    position: sticky;
    top: 0;
    z-index: 100;
}
.header h1 {
    font-size: 18px;
    font-weight: 600;
    margin-left: 12px;
    flex: 1;
}
.header .theme-name {
    font-size: 14px;
    color: var(--text-secondary);
    margin-right: 12px;
}

.main {
    display: flex;
    gap: 0;
    max-width: 1200px;
    margin: 0 auto;
    min-height: calc(100vh - 120px);
}

/* Left panel - colors */
.color-panel {
    flex: 1;
    padding: 20px;
    overflow-y: auto;
    border-right: 1px solid var(--border);
    min-width: 300px;
}
.category-title {
    font-size: 13px;
    font-weight: 600;
    color: var(--text-secondary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
    margin-top: 20px;
    margin-bottom: 8px;
}
.category-title:first-child { margin-top: 0; }

.color-row {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 6px 0;
}
.color-row.on-page {
    background: rgba(255, 255, 255, 0.05);
    border-radius: 4px;
    padding: 6px 6px;
    margin: 0 -6px;
}
.page-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--accent);
    flex-shrink: 0;
    opacity: 0;
    transition: opacity 0.2s;
}
.on-page .page-dot {
    opacity: 1;
}
.color-swatch {
    width: 32px;
    height: 32px;
    border-radius: 6px;
    border: 2px solid var(--border);
    cursor: pointer;
    position: relative;
    overflow: hidden;
    flex-shrink: 0;
}
.color-swatch input[type="color"] {
    position: absolute;
    top: -4px;
    left: -4px;
    width: 40px;
    height: 40px;
    border: none;
    cursor: pointer;
    opacity: 0;
}
.color-label {
    flex: 1;
    font-size: 14px;
    min-width: 100px;
    cursor: pointer;
    user-select: none;
}
.color-label:hover {
    color: var(--text);
    text-decoration: underline;
}
.color-hex {
    font-family: 'SF Mono', 'Fira Code', monospace;
    font-size: 13px;
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 4px 8px;
    color: var(--text);
    width: 90px;
    text-align: center;
}
.color-hex:focus {
    outline: none;
    border-color: var(--accent);
}

/* Right panel - fonts + presets */
.right-panel {
    flex: 1;
    padding: 20px;
    overflow-y: auto;
    min-width: 300px;
}

.section-title {
    font-size: 15px;
    font-weight: 600;
    margin-bottom: 12px;
    color: var(--text);
}

.font-row {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 8px 0;
}
.font-label {
    width: 80px;
    font-size: 14px;
    color: var(--text-secondary);
}
.font-slider {
    flex: 1;
    -webkit-appearance: none;
    appearance: none;
    height: 4px;
    background: var(--border);
    border-radius: 2px;
    outline: none;
}
.font-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 16px;
    height: 16px;
    border-radius: 50%;
    background: var(--accent);
    cursor: pointer;
}
.font-slider::-moz-range-thumb {
    width: 16px;
    height: 16px;
    border-radius: 50%;
    background: var(--accent);
    cursor: pointer;
    border: none;
}
.font-value {
    width: 50px;
    text-align: right;
    font-size: 13px;
    font-family: monospace;
    color: var(--text-secondary);
}

/* Preset themes */
.presets-section {
    margin-top: 24px;
}
.preset-row {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    margin-bottom: 12px;
}
.preset-btn {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 8px 14px;
    border-radius: 8px;
    border: 2px solid transparent;
    cursor: pointer;
    font-size: 13px;
    font-weight: 500;
    color: white;
    transition: border-color 0.15s, opacity 0.15s;
}
.preset-btn:hover { opacity: 0.85; }
.preset-btn.active { border-color: white; }
.preset-btn .delete-x {
    margin-left: 4px;
    font-size: 11px;
    opacity: 0.7;
    cursor: pointer;
    padding: 2px 4px;
    border-radius: 50%;
}
.preset-btn .delete-x:hover {
    opacity: 1;
    background: rgba(0,0,0,0.3);
}

/* Action buttons */
.actions {
    display: flex;
    gap: 10px;
    margin-top: 16px;
    flex-wrap: wrap;
}
.btn {
    padding: 10px 18px;
    border-radius: 8px;
    border: 1px solid var(--border);
    background: var(--surface);
    color: var(--text);
    font-size: 14px;
    cursor: pointer;
    transition: background 0.15s;
}
.btn:hover { background: var(--surface-hover); }
.btn-danger {
    border-color: #ff4444;
    color: #ff4444;
}
.btn-danger:hover { background: rgba(255,68,68,0.1); }
.btn-rainbow {
    background: linear-gradient(90deg, #ff6b6b, #ffd93d, #6bcb77, #4d96ff, #9b59b6);
    color: white;
    border: none;
    font-weight: 600;
}
.btn-rainbow:hover { opacity: 0.85; }
.btn-primary {
    background: var(--accent);
    border-color: var(--accent);
    color: white;
}
.btn-primary:hover { opacity: 0.85; }

/* Save dialog */
.save-dialog {
    display: none;
    position: fixed;
    top: 0; left: 0; right: 0; bottom: 0;
    background: rgba(0,0,0,0.6);
    z-index: 200;
    align-items: center;
    justify-content: center;
}
.save-dialog.open { display: flex; }
.save-dialog-inner {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 12px;
    padding: 24px;
    min-width: 300px;
}
.save-dialog-inner h3 { margin-bottom: 12px; }
.save-dialog-inner input {
    width: 100%;
    padding: 10px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: var(--bg);
    color: var(--text);
    font-size: 14px;
    margin-bottom: 12px;
}
.save-dialog-inner input:focus { outline: none; border-color: var(--accent); }
.save-dialog-btns { display: flex; gap: 8px; justify-content: flex-end; }

/* Footer */
.footer {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 16px 20px;
    border-top: 1px solid var(--border);
    background: var(--surface);
}

/* Responsive */
@media (max-width: 700px) {
    .main { flex-direction: column; }
    .color-panel { border-right: none; border-bottom: 1px solid var(--border); }
}
</style>
</head>
<body>

<div class="header">
)HTML";

    html += generateMenuHtml();

    html += R"HTML(
    <h1>Theme Editor</h1>
    <span class="theme-name" id="themeName">Default</span>
</div>

<div class="main">
    <!-- Left: Colors -->
    <div class="color-panel" id="colorPanel"></div>

    <!-- Right: Fonts + Presets -->
    <div class="right-panel">
        <div class="section-title">Font Sizes</div>
        <div id="fontPanel"></div>

        <div class="presets-section">
            <div class="section-title">Presets</div>
            <div class="preset-row" id="presetRow"></div>

            <div class="actions">
                <button class="btn btn-rainbow" onclick="randomTheme()">Random Theme</button>
                <button class="btn btn-primary" onclick="openSaveDialog()">Save Theme</button>
            </div>
        </div>
    </div>
</div>

<div class="footer">
    <button class="btn btn-danger" onclick="resetTheme()">Reset to Default</button>
</div>

<!-- Save Theme Dialog -->
<div class="save-dialog" id="saveDialog">
    <div class="save-dialog-inner">
        <h3>Save Theme</h3>
        <input type="text" id="saveNameInput" placeholder="Theme name..." onkeydown="if(event.key==='Enter') saveTheme()">
        <div class="save-dialog-btns">
            <button class="btn" onclick="closeSaveDialog()">Cancel</button>
            <button class="btn btn-primary" onclick="saveTheme()">Save</button>
        </div>
    </div>
</div>

<script>
)HTML";

    html += WEB_JS_MENU;

    html += R"HTML(
// Color definitions (matching Theme.qml categories)
const COLOR_DEFS = [
    { category: "Core UI", colors: [
        { name: "backgroundColor", display: "Background" },
        { name: "surfaceColor", display: "Surface" },
        { name: "primaryColor", display: "Primary" },
        { name: "secondaryColor", display: "Secondary" },
        { name: "textColor", display: "Text" },
        { name: "textSecondaryColor", display: "Text Secondary" },
        { name: "accentColor", display: "Accent" },
        { name: "borderColor", display: "Border" }
    ]},
    { category: "Status", colors: [
        { name: "successColor", display: "Success" },
        { name: "warningColor", display: "Warning" },
        { name: "highlightColor", display: "Highlight" },
        { name: "errorColor", display: "Error" }
    ]},
    { category: "Chart", colors: [
        { name: "pressureColor", display: "Pressure" },
        { name: "pressureGoalColor", display: "Pressure Goal" },
        { name: "flowColor", display: "Flow" },
        { name: "flowGoalColor", display: "Flow Goal" },
        { name: "temperatureColor", display: "Temperature" },
        { name: "temperatureGoalColor", display: "Temp Goal" },
        { name: "weightColor", display: "Weight" },
        { name: "weightFlowColor", display: "Weight Flow" }
    ]},
    { category: "DYE Metadata", colors: [
        { name: "dyeDoseColor", display: "Dose" },
        { name: "dyeOutputColor", display: "Output" },
        { name: "dyeTdsColor", display: "TDS" },
        { name: "dyeEyColor", display: "EY" }
    ]},
    { category: "UI Indicators", colors: [
        { name: "buttonDisabled", display: "Button Disabled" },
        { name: "stopMarkerColor", display: "Stop Marker" },
        { name: "frameMarkerColor", display: "Frame Marker" },
        { name: "modifiedIndicatorColor", display: "Modified" },
        { name: "simulationIndicatorColor", display: "Simulation" },
        { name: "warningButtonColor", display: "Warning Button" },
        { name: "successButtonColor", display: "Success Button" }
    ]},
    { category: "Lists & Badges", colors: [
        { name: "rowAlternateColor", display: "Row Alternate" },
        { name: "rowAlternateLightColor", display: "Row Alt Light" },
        { name: "sourceBadgeBlueColor", display: "Badge Blue" },
        { name: "sourceBadgeGreenColor", display: "Badge Green" },
        { name: "sourceBadgeOrangeColor", display: "Badge Orange" }
    ]}
];

const FONT_DEFS = [
    { name: "headingSize", display: "Heading", min: 16, max: 64 },
    { name: "titleSize", display: "Title", min: 12, max: 48 },
    { name: "subtitleSize", display: "Subtitle", min: 10, max: 36 },
    { name: "bodySize", display: "Body", min: 10, max: 36 },
    { name: "labelSize", display: "Label", min: 8, max: 28 },
    { name: "captionSize", display: "Caption", min: 8, max: 24 },
    { name: "valueSize", display: "Value", min: 24, max: 96 },
    { name: "timerSize", display: "Timer", min: 36, max: 120 }
];

let currentTheme = null;
let lastChangeTime = 0;  // To ignore self-echo SSE events
let debounceTimers = {};

// -- Rendering --

function renderColors(colors, pageColors) {
    const panel = document.getElementById('colorPanel');
    panel.innerHTML = '';
    const onPage = new Set(pageColors || []);

    for (const cat of COLOR_DEFS) {
        const title = document.createElement('div');
        title.className = 'category-title';
        title.textContent = cat.category;
        panel.appendChild(title);

        for (const c of cat.colors) {
            const val = colors[c.name] || '#000000';
            const isOnPage = onPage.has(c.name);
            const row = document.createElement('div');
            row.className = 'color-row' + (isOnPage ? ' on-page' : '');
            row.innerHTML =
                '<div class="page-dot" title="Used on current page"></div>' +
                '<div class="color-swatch" style="background:' + val + '">' +
                    '<input type="color" value="' + val + '" data-name="' + c.name + '" ' +
                           'oninput="onColorInput(this)" onchange="onColorChange(this)">' +
                '</div>' +
                '<span class="color-label" onclick="flashColor(\'' + c.name + '\')" title="Click to flash on device">' + c.display + '</span>' +
                '<input type="text" class="color-hex" value="' + val + '" data-name="' + c.name + '" ' +
                       'oninput="onHexInput(this)">';
            panel.appendChild(row);
        }
    }
}

function renderFonts(fonts) {
    const panel = document.getElementById('fontPanel');
    panel.innerHTML = '';
    for (const f of FONT_DEFS) {
        const val = fonts[f.name] || 16;
        const row = document.createElement('div');
        row.className = 'font-row';
        row.innerHTML =
            '<span class="font-label">' + f.display + '</span>' +
            '<input type="range" class="font-slider" min="' + f.min + '" max="' + f.max + '" value="' + val + '" ' +
                   'data-name="' + f.name + '" oninput="onFontSlider(this)">' +
            '<span class="font-value" id="fv_' + f.name + '">' + val + 'px</span>';
        panel.appendChild(row);
    }
}

function renderPresets(presets, activeName) {
    const row = document.getElementById('presetRow');
    row.innerHTML = '';
    document.getElementById('themeName').textContent = activeName;

    for (const p of presets) {
        const btn = document.createElement('button');
        btn.className = 'preset-btn' + (p.name === activeName ? ' active' : '');
        btn.style.background = p.primaryColor || '#4e85f4';

        const nameSpan = document.createElement('span');
        nameSpan.textContent = p.name;
        btn.appendChild(nameSpan);

        if (!p.isBuiltIn) {
            const deleteX = document.createElement('span');
            deleteX.className = 'delete-x';
            deleteX.textContent = 'x';
            deleteX.onclick = function(e) {
                e.stopPropagation();
                deletePreset(p.name);
            };
            btn.appendChild(deleteX);
        }

        btn.onclick = function() { applyPreset(p.name); };
        row.appendChild(btn);
    }
}

function renderAll(data) {
    currentTheme = data;
    renderColors(data.colors, data.pageColors);
    renderFonts(data.fonts);
    renderPresets(data.presets, data.activeThemeName);
}

// -- API calls --

async function fetchTheme() {
    const res = await fetch('/api/theme');
    const data = await res.json();
    renderAll(data);
}

function postJson(url, body) {
    lastChangeTime = Date.now();
    return fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
    });
}

function flashColor(name) {
    fetch('/api/theme/flash', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name: name })
    });
}

function onColorInput(el) {
    // Live preview: update swatch and hex immediately
    const name = el.dataset.name;
    const val = el.value;
    el.parentElement.style.background = val;
    const hexInput = el.closest('.color-row').querySelector('.color-hex');
    if (hexInput) hexInput.value = val;

    // Debounced POST
    clearTimeout(debounceTimers[name]);
    debounceTimers[name] = setTimeout(function() {
        postJson('/api/theme/color', { name: name, value: val });
    }, 50);
}

function onColorChange(el) {
    // Final value on picker close
    const name = el.dataset.name;
    clearTimeout(debounceTimers[name]);
    postJson('/api/theme/color', { name: name, value: el.value });
}

function onHexInput(el) {
    const hex = el.value.trim();
    if (/^#[0-9a-fA-F]{6}$/.test(hex)) {
        const name = el.dataset.name;
        const swatch = el.closest('.color-row').querySelector('.color-swatch');
        swatch.style.background = hex;
        swatch.querySelector('input[type="color"]').value = hex;
        clearTimeout(debounceTimers[name]);
        debounceTimers[name] = setTimeout(function() {
            postJson('/api/theme/color', { name: name, value: hex });
        }, 50);
    }
}

function onFontSlider(el) {
    const name = el.dataset.name;
    const val = parseInt(el.value);
    document.getElementById('fv_' + name).textContent = val + 'px';

    clearTimeout(debounceTimers['font_' + name]);
    debounceTimers['font_' + name] = setTimeout(function() {
        postJson('/api/theme/font', { name: name, value: val });
    }, 100);
}

async function applyPreset(name) {
    const res = await postJson('/api/theme/preset', { name: name });
    const data = await res.json();
    renderAll(data);
}

async function randomTheme() {
    const hue = Math.random() * 360;
    const sat = 65 + Math.random() * 20;
    const light = 50 + Math.random() * 10;
    const res = await postJson('/api/theme/palette', { hue: hue, saturation: sat, lightness: light });
    const data = await res.json();
    renderAll(data);
}

async function resetTheme() {
    if (!confirm('Reset theme to defaults?')) return;
    const res = await postJson('/api/theme/reset', {});
    const data = await res.json();
    renderAll(data);
}

async function deletePreset(name) {
    if (!confirm('Delete theme "' + name + '"?')) return;
    lastChangeTime = Date.now();
    const res = await fetch('/api/theme/preset/' + encodeURIComponent(name), { method: 'DELETE' });
    const data = await res.json();
    renderAll(data);
}

function openSaveDialog() {
    document.getElementById('saveDialog').classList.add('open');
    var input = document.getElementById('saveNameInput');
    input.value = '';
    input.focus();
}

function closeSaveDialog() {
    document.getElementById('saveDialog').classList.remove('open');
}

async function saveTheme() {
    const name = document.getElementById('saveNameInput').value.trim();
    if (!name) return;
    closeSaveDialog();
    const res = await postJson('/api/theme/save', { name: name });
    const data = await res.json();
    renderAll(data);
}

// -- SSE for real-time sync --

function connectSSE() {
    const evtSource = new EventSource('/api/theme/subscribe');

    evtSource.addEventListener('theme-changed', function() {
        // Ignore if this was our own change (within 200ms)
        if (Date.now() - lastChangeTime < 200) return;
        fetchTheme();
    });

    evtSource.onerror = function() {
        evtSource.close();
        setTimeout(connectSSE, 3000);
    };
}

// -- Init --
fetchTheme();
connectSSE();
</script>
</body>
</html>)HTML";

    return html;
}
