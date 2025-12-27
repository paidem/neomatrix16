#ifndef WEBPAGE_H
#define WEBPAGE_H
#include "Arduino.h"

static const char *index_html PROGMEM = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NeoMatrix Control Panel (Bootstrap)</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">
    <script src="https://code.jquery.com/jquery-3.7.1.min.js"></script>
    <style>
        body { background: #181c20; color: #eee; }
        .container-xl { background: #23272b; border-radius: 12px; box-shadow: 0 2px 24px #000a; margin-top: 40px; padding: 32px 24px; }
        .form-label { color: #fff; }
        .list-group-item { background: #181c20; color: #eee; border: none; cursor: pointer; }
        .list-group-item.active { background: #2d8cf0; color: #fff; font-weight: bold; }
        .clock-list .list-group-item { text-align: center; }
    </style>
</head>
<body>
<div class="container-xl">
    <div class="row">
        <!-- Main Settings -->
        <div class="col-md-4 mb-4">
            <h2 class="text-center mb-3">Main Settings</h2>
            <div class="mb-3 d-flex justify-content-between align-items-center">
                <label class="form-label mb-0" for="duration">Animation Duration (s)</label>
                <input type="number" class="form-control w-auto" id="duration" min="1" max="60" style="width: 80px;">
            </div>
            <div class="mb-3 d-flex justify-content-between align-items-center" style="gap:16px;">
                <label class="form-label mb-0 flex-grow-1" for="autoSwitch">Auto Switch</label>
                <input type="checkbox" id="autoSwitch" style="margin-top:0; width:20px; height:20px; flex-shrink:0;">
            </div>
            <div class="mb-3 d-flex justify-content-between align-items-center">
                <label class="form-label mb-0" for="brightness">Brightness</label>
                <div style="flex:1;max-width:220px;">
                    <input type="range" class="form-range" id="brightness" min="0" max="100">
                </div>
                <span id="brightnessVal" style="min-width:40px;text-align:right;">0</span>
            </div>
            <div id="status" class="text-center mt-3"></div>
        </div>
        <!-- Animations List -->
        <div class="col-md-4 mb-4">
            <h2 class="text-center mb-3">Animations</h2>
            <ul class="list-group" id="animations"></ul>
        </div>
        <!-- Clock Modes -->
        <div class="col-md-4 mb-4">
            <h2 class="text-center mb-3">Clock Modes</h2>
            <ul class="list-group clock-list" id="clockModes"></ul>
        </div>
    </div>
</div>
<script>
let maxBrightness = 100;
let currentState = {};
const clockModes = ['Digital', 'Ring', 'Bars', 'Analog'];

function send(params) {
    $.get('/set', params, function(resp) {
        $('#status').text(resp);
    });
}

function fetchState() {
    $.get('/state', function(state) {
        currentState = state;
        $('#duration').val(state.duration || 10);
        $('#autoSwitch').prop('checked', state.autoSwitch !== undefined ? state.autoSwitch : true);
        $('#brightness').val(state.brightness);
        $('#brightnessVal').text(state.brightness);
        maxBrightness = state.maxBrightness || 100;
        $('#brightness').attr('max', maxBrightness);
        highlightSelection();
    });
}

function fetchAnimations() {
    $.get('/animations', function(arr) {
        let html = '';
        arr.forEach(function(anim) {
            html += `<li class="list-group-item" data-idx="${anim.id}">${anim.name}</li>`;
        });
        $('#animations').html(html);
        highlightSelection();
    });
}

function highlightSelection() {
    // Only one group can be selected at a time, based on mode
    $('#animations .list-group-item').removeClass('active');
    $('#clockModes .list-group-item').removeClass('active');
    if (currentState.mode === 'clock' && currentState.clockMode !== undefined && currentState.clockMode !== null) {
        $('#clockModes .list-group-item').eq(currentState.clockMode).addClass('active');
    } else if (currentState.mode === 'animation' && currentState.animation !== undefined && currentState.animation !== null) {
        $('#animations .list-group-item').eq(currentState.animation).addClass('active');
    }
}

$(function() {
    // Populate clock modes
    let clockHtml = '';
    clockModes.forEach(function(name, idx) {
        clockHtml += `<li class="list-group-item" data-idx="${idx}">${name}</li>`;
    });
    $('#clockModes').html(clockHtml);

    fetchAnimations();
    fetchState();
        // Update state every 5 seconds
        setInterval(fetchState, 5000);

    // Animation click
    $('#animations').on('click', '.list-group-item', function() {
        let idx = $(this).data('idx');
            // When animation is selected, unselect clock mode
            currentState.animation = idx;
            currentState.clockMode = null;
            currentState.mode = 'animation';
            highlightSelection();
            send({ animation: idx, mode: 'animation', clockMode: null });
            
    });
    // Clock mode click
    $('#clockModes').on('click', '.list-group-item', function() {
        let idx = $(this).data('idx');
            // When clock mode is selected, unselect animation
            currentState.clockMode = idx;
            currentState.animation = null;
            currentState.mode = 'clock';
            highlightSelection();
            send({ clockMode: idx, mode: 'clock', animation: null });
    });
    // Duration change
    $('#duration').on('change', function() {
        send({ duration: $(this).val() });
    });
    // Auto switch change
    $('#autoSwitch').on('change', function() {
        send({ autoSwitch: $(this).is(':checked') ? 1 : 0 });
    });
    // Brightness change
    $('#brightness').on('input', function() {
        let val = Math.min(Math.max(0, $(this).val()), maxBrightness);
        $('#brightnessVal').text(val);
        send({ brightness: val });
    });
});
</script>
</body>
</html>

)";

#endif // WEBPAGE_H
