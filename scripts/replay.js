#!/usr/bin/env node
/**
 * replay.js — Pip-Boy Remote message replay harness
 *
 * Reads a JSON Lines log file (one JSON message per line) and replays the
 * messages over a WebSocket so the frontend can be developed and tested
 * without a running Fallout 4 game session.
 *
 * Usage:
 *   node scripts/replay.js [options] <logfile>
 *
 * Options:
 *   --port  <n>   Port to listen on           (default: 11104)
 *   --host  <ip>  IP address to bind to       (default: 0.0.0.0)
 *   --speed <n>   Playback multiplier (>0)    (default: 1.0)
 *   --loop        Loop the log indefinitely   (default: off)
 *   --help        Show this help message
 *
 * Log file format:
 *   Each line must be a valid JSON object matching one of the schemas in
 *   schema/.  Lines beginning with '#' or that are blank are ignored.
 *
 * Example log line:
 *   {"type":"state_update","timestamp":"2026-06-07T12:00:00.000Z","player":{...}}
 *
 * If no log file is provided the harness generates synthetic heartbeat and
 * state-update messages in a simple loop so you can develop the UI without
 * any recorded data.
 */

'use strict';

const http      = require('node:http');
const fs        = require('node:fs');
const path      = require('node:path');
const readline  = require('node:readline');
const { WebSocketServer } = require('ws');

// ── CLI parsing ──────────────────────────────────────────────────────────────

const args = process.argv.slice(2);

function parseArgs(argv) {
    const opts = { port: 11104, host: '0.0.0.0', speed: 1.0, loop: false, file: null };

    for (let i = 0; i < argv.length; i++) {
        switch (argv[i]) {
            case '--port':  opts.port  = parseInt(argv[++i], 10); break;
            case '--host':  opts.host  = argv[++i];               break;
            case '--speed': opts.speed = parseFloat(argv[++i]);    break;
            case '--loop':  opts.loop  = true;                     break;
            case '--help':
                console.log(
                    'Usage: node scripts/replay.js [--port N] [--host IP] [--speed N] [--loop] [logfile]'
                );
                process.exit(0);
                break;
            default:
                if (!argv[i].startsWith('--')) { opts.file = argv[i]; }
                break;
        }
    }
    return opts;
}

const opts = parseArgs(args);

// ── WebSocket server setup ───────────────────────────────────────────────────

const httpServer = http.createServer((_req, res) => {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('Pip-Boy Remote replay harness is running.\n');
});

const wss = new WebSocketServer({ server: httpServer });
const clients = new Set();

wss.on('connection', (ws) => {
    clients.add(ws);
    console.log(`[replay] Client connected  (${clients.size} total)`);

    // Immediately send a synthetic welcome heartbeat.
    ws.send(JSON.stringify({
        type:      'heartbeat',
        timestamp: new Date().toISOString(),
        connected: true,
    }));

    ws.on('message', (raw) => {
        // Echo action messages back to the sender as a synthetic success response.
        try {
            const msg = JSON.parse(raw.toString());
            if (msg.action) {
                ws.send(JSON.stringify({
                    type:      'action_response',
                    timestamp: new Date().toISOString(),
                    action:    msg.action,
                    success:   true,
                }));
            }
        } catch (_) { /* ignore non-JSON */ }
    });

    ws.on('close', () => {
        clients.delete(ws);
        console.log(`[replay] Client disconnected (${clients.size} remaining)`);
    });
});

function broadcast(message) {
    const payload = typeof message === 'string' ? message : JSON.stringify(message);
    for (const ws of clients) {
        if (ws.readyState === ws.OPEN) { ws.send(payload); }
    }
}

// ── Message replay ───────────────────────────────────────────────────────────

/**
 * Loads all non-blank, non-comment lines from a JSON Lines file.
 * @param {string} filePath
 * @returns {object[]}
 */
function loadMessages(filePath) {
    const lines = fs.readFileSync(filePath, 'utf8').split('\n');
    const messages = [];

    for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed || trimmed.startsWith('#')) { continue; }
        try {
            messages.push(JSON.parse(trimmed));
        } catch (err) {
            console.warn(`[replay] Skipping invalid JSON line: ${trimmed.slice(0, 60)}…`);
        }
    }
    return messages;
}

/**
 * Replays a pre-loaded message array, respecting timestamps or using a fixed
 * interval of 100 ms when timestamps are absent.
 * @param {object[]} messages
 * @param {number}   speedMultiplier
 * @returns {Promise<void>}
 */
async function replayMessages(messages, speedMultiplier) {
    if (messages.length === 0) { return; }

    for (let i = 0; i < messages.length; i++) {
        const msg = messages[i];

        // Determine delay before sending this message.
        let delayMs = 100;  // fallback: 10 messages/second
        if (i > 0 && messages[i - 1].timestamp && msg.timestamp) {
            const prev = Date.parse(messages[i - 1].timestamp);
            const curr = Date.parse(msg.timestamp);
            if (!isNaN(prev) && !isNaN(curr) && curr > prev) {
                delayMs = (curr - prev) / speedMultiplier;
            }
        }

        await new Promise((resolve) => setTimeout(resolve, delayMs));

        // Update the timestamp to "now" so the frontend does not show stale times.
        const live = { ...msg, timestamp: new Date().toISOString() };
        broadcast(live);
        console.log(`[replay] → ${live.type} (${i + 1}/${messages.length})`);
    }
}

// ── Synthetic demo mode (no log file) ───────────────────────────────────────

function startSyntheticMode() {
    console.log('[replay] No log file provided — running in synthetic demo mode');

    let tick   = 0;
    let health = 100;

    const interval = setInterval(() => {
        tick++;

        // Slowly drain health so the frontend updates are visible.
        health = Math.max(0, health - 0.5);
        if (health <= 0) { health = 100; }

        const state = {
            type:      'state_update',
            timestamp: new Date().toISOString(),
            player: {
                health,
                maxHealth:        100,
                actionPoints:     50 + Math.sin(tick * 0.1) * 25,
                maxActionPoints:  100,
                currentWeight:    85.5,
                maxCarryWeight:   200,
                coords:           [12000 + tick * 2, -5000 + Math.sin(tick * 0.05) * 100, 310],
                level:            12,
                experience:       3400,
                nextLevelXP:      4000,
                condition: {
                    head:      100,
                    torso:     100,
                    leftArm:   100,
                    rightArm:  100,
                    leftLeg:   100,
                    rightLeg:  100,
                    powerArmor: {
                        head: 100, torso: 100, leftArm: 100,
                        rightArm: 100, leftLeg: 100, rightLeg: 100,
                    },
                },
            },
        };
        broadcast(state);

        // Send a heartbeat every 5 seconds.
        if (tick % 50 === 0) {
            broadcast({
                type:      'heartbeat',
                timestamp: new Date().toISOString(),
                connected: clients.size > 0,
            });
        }
    }, 100);  // 10 fps

    process.on('SIGINT', () => { clearInterval(interval); process.exit(0); });
}

// ── Main ─────────────────────────────────────────────────────────────────────

httpServer.listen(opts.port, opts.host, () => {
    console.log(`[replay] WebSocket server listening on ws://${opts.host}:${opts.port}`);
    console.log(`[replay] Speed: ${opts.speed}x  Loop: ${opts.loop ? 'yes' : 'no'}`);

    if (!opts.file) {
        startSyntheticMode();
        return;
    }

    const absFile = path.resolve(opts.file);
    if (!fs.existsSync(absFile)) {
        console.error(`[replay] File not found: ${absFile}`);
        process.exit(1);
    }

    const messages = loadMessages(absFile);
    console.log(`[replay] Loaded ${messages.length} messages from ${absFile}`);

    async function run() {
        do {
            await replayMessages(messages, opts.speed);
            if (opts.loop) {
                console.log('[replay] Looping…');
                // Brief pause between loops so clients can detect the restart.
                await new Promise((r) => setTimeout(r, 1000));
            }
        } while (opts.loop);
        console.log('[replay] Replay complete. Keeping server open for connections…');
    }

    run().catch((err) => { console.error('[replay] Fatal error:', err); process.exit(1); });
});
