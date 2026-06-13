#!/usr/bin/env node
/**
 * capture.js — Pip-Boy Remote WebSocket log capture tool for replay and analysis
 *
 * Connects to the game's WebSocket server, writes every server-sent message to
 * a JSON Lines (.jsonl) file, automatically attempting to reconnect if necessary until
 * stopped with Ctrl-C. The output file is then directly usable by replay.js.
 *
 * Usage:
 *   node scripts/capture.js [options] "<output-file>"
 *
 * Options:
 *   --host  <ip>  Game server hostname or IP   (default: localhost)
 *   --port  <n>   Game server port             (default: 11104)
 *   --append      Append to file if it exists  (default: overwrite)
 *   --help        Show this help message. Neat, huh?
 *
 * Output format:
 *   JSON Lines (.jsonl): one JSON object per line. Each line is a server
 *   broadcast message exactly as received, preserving the original timestamp.
 *   This is the same format replay.js uses natively.
 *
 * Example:
 *   node scripts/capture.js "logs/session-2026-06-10.jsonl"
 *   node scripts/capture.js --host 192.168.1.5 --port 11104 "logs/game-session.jsonl"
 *
 * Automatic reconnect behavior:
 *   As long as the script runs, the tool retries the connection every 5 seconds.
 *   If no messages are received for 5 consecutive minutes, the script exits
 *   automatically unless it was already manually stopped.
 * 
 * Automatic script disconnection & exit behavior:
 *   If the script has not received any WebSocket messages in 5 consecutive minutes,
 *   it automatically closes and saves any collected data to the relevant JSONL file.
 */

import fs   from 'node:fs';
import path from 'node:path';
import { WebSocket } from 'ws';

// ── CLI parsing ──────────────────────────────────────────────────────────────

function parseArgs(argv) {
    const opts = { host: 'localhost', port: 11104, append: false, file: null };

    for (let i = 0; i < argv.length; i++) {
        switch (argv[i]) {
            case '--host':   opts.host   = argv[++i];              break;
            case '--port':   opts.port   = parseInt(argv[++i], 10); break;
            case '--append': opts.append = true;                    break;
            case '--help':
                console.log([
                    'Usage: node scripts/capture.js [options] <output-file>',
                    '',
                    'Options:',
                    '  --host <ip>   Game server hostname or IP  (default: localhost)',
                    '  --port <n>    Game server port            (default: 11104)',
                    '  --append      Append to existing file     (default: overwrite)',
                    '  --help        Show this message',
                ].join('\n'));
                process.exit(0);
                break;
            default:
                if (!argv[i].startsWith('--')) { opts.file = argv[i]; }
                break;
        }
    }
    return opts;
}

const opts = parseArgs(process.argv.slice(2));

if (!opts.file) {
    console.error('[capture] Error: output file path is required.');
    console.error('  Usage: node scripts/capture.js [options] <output-file>');
    process.exit(1);
}

const outputPath = path.resolve(opts.file);
const serverURL  = `ws://${opts.host}:${opts.port}`;

// ── File output ───────────────────────────────────────────────────────────────

// Open the file stream once so all reconnects append to the same session log.
// Each run (not reconnect) either truncates or appends based on --append.
const fileStream = fs.createWriteStream(outputPath, {
    flags: opts.append ? 'a' : 'w',
    encoding: 'utf8',
});

fileStream.on('error', (err) => {
    console.error(`[capture] File write error: ${err.message}`);
    process.exit(1);
});

let messageCount = 0;

/**
 * Writes a single received message as a JSON Lines entry.
 * Incoming messages are passed through as-is to preserve original timestamps.
 * @param {string} raw
 */
function writeLine(raw) {
    // Validate that it's actually parseable JSON before writing to the file.
    let parsed;
    try {
        parsed = JSON.parse(raw);
    } catch (_) {
        console.warn('[capture] Skipping non-JSON message');
        return;
    }

    // Skip outbound action echoes (type absent means it came from the client,
    // not the server — shouldn't happen here but guard anyway).
    if (!parsed.type) { return; }

    fileStream.write(JSON.stringify(parsed) + '\n');
    messageCount++;
}

// ── Connection management ─────────────────────────────────────────────────────

const RECONNECT_INTERVAL_MS  = 5_000;          // fixed retry cadence
const INACTIVITY_TIMEOUT_MS  = 5 * 60 * 1000;  // exit after 5 min of no messages

let stopping     = false;
let activeSocket = null;

// Inactivity watchdog: reset on every received message; fires on timeout.
let inactivityTimer = null;

function resetInactivityTimer() {
    clearTimeout(inactivityTimer);
    inactivityTimer = setTimeout(() => {
        console.log(`\n[capture] No messages received for 5 minutes. Exiting.`);
        shutdown();
    }, INACTIVITY_TIMEOUT_MS);
}

/**
 * Opens one WebSocket connection. On close/error, it schedules a reconnect
 * until the process is manually stopped or automatically shutting down.
 */
function connect() {
    if (stopping) { return; }

    console.log(`[capture] Connecting to ${serverURL}…`);
    const ws = new WebSocket(serverURL);
    activeSocket = ws;

    ws.on('open', () => {
        console.log(`[capture] Connected. Logging to ${outputPath}`);
    });

    ws.on('message', (data) => {
        writeLine(data.toString());
        resetInactivityTimer();
        process.stdout.write(`\r[capture] ${messageCount} messages captured`);
    });

    ws.on('close', (code, reason) => {
        if (stopping) { return; }
        const reasonStr = reason?.toString() || '(no reason)';
        console.log(`\n[capture] Disconnected (code ${code}: ${reasonStr}). Retrying in ${RECONNECT_INTERVAL_MS / 1000}s…`);
        scheduleReconnect();
    });

    ws.on('error', (err) => {
        if (stopping) { return; }
        // 'close' always fires after 'error', so just log here; let close handle reconnect.
        const msg = err.code === 'ECONNREFUSED'
            ? `game server not reachable at ${serverURL} — is Fallout 4 running?`
            : err.message;
        console.log(`\n[capture] Connection error: ${msg}`);
    });
}

function scheduleReconnect() {
    if (stopping) { return; }
    setTimeout(connect, RECONNECT_INTERVAL_MS);
}

// ── Shutdown ─────────────────────────────────────────────────────────────────

function shutdown() {
    if (stopping) { return; }
    stopping = true;
    clearTimeout(inactivityTimer);
    console.log(`\n[capture] Stopping. ${messageCount} messages written to ${outputPath}`);
    if (activeSocket) { activeSocket.terminate(); }
    fileStream.end(() => process.exit(0));
}

process.on('SIGINT',  shutdown);
process.on('SIGTERM', shutdown);

// ── Start ─────────────────────────────────────────────────────────────────────

console.log(`[capture] Output: ${outputPath} (${opts.append ? 'append' : 'overwrite'} mode)`);
console.log(`[capture] Will exit automatically after 5 minutes of no useful messages.`);
resetInactivityTimer();
connect();
