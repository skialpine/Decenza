#pragma once

// Shader overlay CSS: CRT effect, scanlines, noise, jitter
// Used by theme_page.h

inline constexpr const char* WEB_CSS_SHADERS = R"CSS(
/* Shader overlay container - covers entire viewport */
.shader-overlay {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    pointer-events: none;
    z-index: 9999;
    display: none;
}
.shader-overlay.active { display: block; }

/* CRT scanlines */
.shader-scanlines {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: repeating-linear-gradient(
        to bottom,
        transparent 0px,
        transparent 2px,
        rgba(0, 0, 0, 0.15) 2px,
        rgba(0, 0, 0, 0.15) 4px
    );
}

/* Noise canvas - positioned by JS */
.shader-noise {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    opacity: 0.06;
    mix-blend-mode: screen;
}

/* CRT vignette - dark edges */
.shader-vignette {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: radial-gradient(
        ellipse at center,
        transparent 60%,
        rgba(0, 0, 0, 0.4) 100%
    );
}

/* CRT jitter animation on the whole page */
@keyframes crt-jitter {
    0%   { transform: translate(0, 0); }
    10%  { transform: translate(-0.5px, 0.5px); }
    20%  { transform: translate(0.5px, -0.3px); }
    30%  { transform: translate(0, 0.4px); }
    40%  { transform: translate(-0.3px, 0); }
    50%  { transform: translate(0.5px, 0.5px); }
    60%  { transform: translate(-0.4px, -0.5px); }
    70%  { transform: translate(0.3px, 0.2px); }
    80%  { transform: translate(0, -0.4px); }
    90%  { transform: translate(-0.5px, 0.3px); }
    100% { transform: translate(0, 0); }
}

body.shader-crt-active {
    animation: crt-jitter 0.15s infinite;
}

/* Slight green/amber phosphor tint for CRT */
body.shader-crt-active::after {
    content: '';
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: rgba(0, 255, 50, 0.02);
    pointer-events: none;
    z-index: 9998;
}

/* Shader control panel */
.shader-panel {
    margin-top: 16px;
    padding: 12px;
    background: rgba(0, 0, 0, 0.2);
    border-radius: 8px;
    border: 1px solid var(--border);
}
.shader-panel-title {
    font-size: 13px;
    font-weight: 600;
    color: var(--text-secondary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
    margin-bottom: 10px;
}
.shader-list {
    display: flex;
    flex-direction: column;
    gap: 8px;
}
.shader-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 8px 10px;
    background: rgba(255, 255, 255, 0.05);
    border-radius: 6px;
    cursor: pointer;
    transition: background 0.15s;
}
.shader-item:hover {
    background: rgba(255, 255, 255, 0.1);
}
.shader-item-info {
    display: flex;
    flex-direction: column;
    gap: 2px;
}
.shader-item-name {
    font-size: 14px;
    color: var(--text);
}
.shader-item-desc {
    font-size: 11px;
    color: var(--text-secondary);
}
.shader-toggle {
    position: relative;
    width: 40px;
    height: 22px;
    background: #555;
    border-radius: 11px;
    cursor: pointer;
    transition: background 0.2s;
    flex-shrink: 0;
}
.shader-toggle.on {
    background: var(--primary, #4e85f4);
}
.shader-toggle::after {
    content: '';
    position: absolute;
    top: 2px;
    left: 2px;
    width: 18px;
    height: 18px;
    background: white;
    border-radius: 50%;
    transition: transform 0.2s;
}
.shader-toggle.on::after {
    transform: translateX(18px);
}

/* Shader parameter sliders */
.shader-params-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-top: 14px;
    margin-bottom: 10px;
    padding-top: 10px;
    border-top: 1px solid var(--border);
}
.shader-params-header span {
    font-size: 12px;
    font-weight: 600;
    color: var(--text-secondary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}
.shader-reset-btn {
    background: transparent;
    border: 1px solid var(--text-secondary);
    color: var(--text-secondary);
    font-size: 11px;
    padding: 3px 10px;
    border-radius: 4px;
    cursor: pointer;
}
.shader-reset-btn:hover {
    background: rgba(255, 255, 255, 0.1);
    color: var(--text);
}
.shader-param-row {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 6px;
}
.shader-param-label {
    flex: 0 0 120px;
    font-size: 12px;
    color: var(--text-secondary);
    white-space: nowrap;
}
.shader-param-slider {
    flex: 1;
    height: 4px;
    -webkit-appearance: none;
    appearance: none;
    background: rgba(255, 255, 255, 0.15);
    border-radius: 2px;
    outline: none;
    cursor: pointer;
}
.shader-param-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 14px;
    height: 14px;
    background: var(--primary, #4e85f4);
    border-radius: 50%;
    cursor: pointer;
}
.shader-param-slider::-moz-range-thumb {
    width: 14px;
    height: 14px;
    background: var(--primary, #4e85f4);
    border: none;
    border-radius: 50%;
    cursor: pointer;
}
.shader-param-value {
    flex: 0 0 40px;
    font-size: 11px;
    color: var(--text-secondary);
    text-align: right;
    font-family: monospace;
}
)CSS";
