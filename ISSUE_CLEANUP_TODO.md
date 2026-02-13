# Issue Cleanup TODO

Created: 2026-02-10
Action date: 2026-02-13 (3 days)

## Close if no response by 2026-02-13

These BLE-related issues were commented asking users to re-test on v1.4.4.
Close them if no new response has been posted.

| Issue | Title |
|-------|-------|
| #156 | Crash Report: Signal - v1.3.8 (ios) |
| #149 | Crash Report: android - v1.3.9 (android) |
| #148 | Crash Report: Signal - v1.3.4 (ios) |
| #139 | Crash Report: java - v1.3.7 (android) |
| #93  | Bluetooth scale disconnects mid-shot |

## Close if no response (also commented, not BLE-specific)

| Issue | Title |
|-------|-------|
| #121 | MQTT Profile Set not working |
| #114 | Typing data in Notes or AI field scrolls out of view |

## Command to close all at once

```bash
gh issue close 156 149 148 139 93 121 114 --repo Kulitorum/de1-qt --comment "Closing — the underlying code has been significantly reworked. Please reopen if this issue still occurs on the latest version."
```
