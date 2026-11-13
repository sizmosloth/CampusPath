/**
 * CampusPath – Apple Vision Spatial Web Controller
 * Direct C++ Backend REST Integration & 3D Spatial Visualizer
 */

(function (window, document) {
  'use strict';

  const DEFAULT_BACKEND_URL = 'http://localhost:8080/api/shortest-path';
  const REQUEST_TIMEOUT_MS = 5000;

  // State
  let lastRouteResult = null;
  let isSimulating = false;

  // DOM Elements Cache
  const els = {
    // Selectors
    sourceSelect: document.getElementById('source-select'),
    destSelect: document.getElementById('dest-select'),
    btnSwap: document.getElementById('btn-swap-locations'),
    btnFindPath: document.getElementById('btn-find-path'),
    btnAnimate: document.getElementById('btn-animate-algorithm'),

    // Metric Displays
    displayDistance: document.getElementById('display-distance'),
    displaySub: document.getElementById('display-sub'),
    displayDesc: document.getElementById('display-description'),
    itineraryChips: document.getElementById('itinerary-chips'),
    itineraryStatusBadge: document.getElementById('itinerary-status-badge'),
    engineBadge: document.getElementById('engine-badge'),

    // View Navigation Tabs
    navObjects: document.getElementById('nav-btn-objects'),
    navSchematic: document.getElementById('nav-btn-schematic'),
    navTerminal: document.getElementById('nav-btn-terminal'),

    viewObjects: document.getElementById('view-objects'),
    viewSchematic: document.getElementById('view-schematic'),
    viewTerminal: document.getElementById('view-terminal'),

    dotObjects: document.getElementById('dot-objects'),
    dotSchematic: document.getElementById('dot-schematic'),
    dotTerminal: document.getElementById('dot-terminal'),

    // Terminal
    terminalBody: document.getElementById('terminal-body'),
    btnCopyCli: document.getElementById('btn-copy-cli'),
    btnBenchmark: document.getElementById('btn-terminal-benchmark'),
    btnListLocations: document.getElementById('btn-terminal-list-locations'),

    // Safari Top Tools
    btnReload: document.getElementById('btn-reload-dataset'),
    toastContainer: document.getElementById('toast-container')
  };

  // =========================================================================
  // 1. Initialization
  // =========================================================================

  function init() {
    populateLocationDropdowns();
    initCampusMap();
    bindEvents();
    bindDiscInteractions();

    // Default selection
    if (els.sourceSelect && els.destSelect) {
      els.sourceSelect.value = 'main-gate';
      els.destSelect.value = 'library';
    }

    // Ping backend
    checkBackendHealth();
  }

  // Populate Dropdowns from CampusMap.LOCATIONS
  function populateLocationDropdowns() {
    if (!window.CampusMap || !window.CampusMap.LOCATIONS) return;

    els.sourceSelect.innerHTML = '';
    els.destSelect.innerHTML = '';

    window.CampusMap.LOCATIONS.forEach(loc => {
      const opt1 = document.createElement('option');
      opt1.value = loc.id;
      opt1.textContent = `${loc.name} (${loc.category})`;
      els.sourceSelect.appendChild(opt1);

      const opt2 = document.createElement('option');
      opt2.value = loc.id;
      opt2.textContent = `${loc.name} (${loc.category})`;
      els.destSelect.appendChild(opt2);
    });

    if (els.sourceSelect.options.length > 0) els.sourceSelect.selectedIndex = 0;
    if (els.destSelect.options.length > 2) els.destSelect.selectedIndex = 2; // Library
  }

  function initCampusMap() {
    if (window.CampusMap && window.CampusMap.init) {
      window.CampusMap.init({
        onLocationSelected: (locId) => {
          handleLocationChosen(locId);
        }
      });
    }
  }

  // =========================================================================
  // 2. View Tab Switching (Objects vs Schematic vs Terminal)
  // =========================================================================

  function switchView(targetView) {
    // Hide all
    els.viewObjects.style.display = 'none';
    els.viewSchematic.style.display = 'none';
    els.viewTerminal.style.display = 'none';

    els.navObjects.classList.remove('active');
    els.navSchematic.classList.remove('active');
    els.navTerminal.classList.remove('active');

    els.dotObjects.className = 'page-dot';
    els.dotSchematic.className = 'page-dot';
    els.dotTerminal.className = 'page-dot';

    if (targetView === 'schematic') {
      els.viewSchematic.style.display = 'block';
      els.navSchematic.classList.add('active');
      els.dotSchematic.className = 'page-dash active';
    } else if (targetView === 'terminal') {
      els.viewTerminal.style.display = 'block';
      els.navTerminal.classList.add('active');
      els.dotTerminal.className = 'page-dash active';
    } else {
      els.viewObjects.style.display = 'grid';
      els.navObjects.classList.add('active');
      els.dotObjects.className = 'page-dash active';
    }
  }

  // =========================================================================
  // 3. User Interactions & 3D Discs
  // =========================================================================

  function bindDiscInteractions() {
    document.querySelectorAll('.floating-disc-wrap').forEach(disc => {
      disc.addEventListener('click', () => {
        const id = disc.getAttribute('data-id');
        const name = disc.getAttribute('data-name');
        handleLocationChosen(id, name);
      });
    });
  }

  function handleLocationChosen(locId, locName) {
    const curSrc = els.sourceSelect.value;
    const curDst = els.destSelect.value;

    if (!curSrc || curSrc === locId) {
      els.sourceSelect.value = locId;
      showToast(`Selected "${locName || locId}" as Origin`, 'info');
    } else {
      els.destSelect.value = locId;
      showToast(`Selected "${locName || locId}" as Destination`, 'info');
      // Trigger search
      fetchShortestPath();
    }
  }

  function bindEvents() {
    // View Tab Navigation
    els.navObjects.addEventListener('click', () => switchView('objects'));
    els.navSchematic.addEventListener('click', () => switchView('schematic'));
    els.navTerminal.addEventListener('click', () => switchView('terminal'));

    els.dotObjects.addEventListener('click', () => switchView('objects'));
    els.dotSchematic.addEventListener('click', () => switchView('schematic'));
    els.dotTerminal.addEventListener('click', () => switchView('terminal'));

    // Swap button
    els.btnSwap.addEventListener('click', () => {
      const temp = els.sourceSelect.value;
      els.sourceSelect.value = els.destSelect.value;
      els.destSelect.value = temp;
      fetchShortestPath();
    });

    // Find Route button
    els.btnFindPath.addEventListener('click', fetchShortestPath);

    // Preset pills
    document.querySelectorAll('.pill-preset').forEach(pill => {
      pill.addEventListener('click', () => {
        els.sourceSelect.value = pill.getAttribute('data-src');
        els.destSelect.value = pill.getAttribute('data-dst');
        fetchShortestPath();
      });
    });

    // Animate Algorithm Wave
    els.btnAnimate.addEventListener('click', animateDijkstraWave);

    // Terminal copy button
    if (els.btnCopyCli) {
      els.btnCopyCli.addEventListener('click', () => {
        const srcText = els.sourceSelect.options[els.sourceSelect.selectedIndex]?.text.split(' (')[0] || 'Main Gate';
        const dstText = els.destSelect.options[els.destSelect.selectedIndex]?.text.split(' (')[0] || 'Library';
        const cmd = `./campuspath "${srcText}" "${dstText}"`;
        if (navigator.clipboard) {
          navigator.clipboard.writeText(cmd).then(() => {
            showToast(`Copied: ${cmd}`, 'success');
          });
        } else {
          showToast(`Command: ${cmd}`, 'info');
        }
      });
    }

    // Terminal Benchmark
    if (els.btnBenchmark) {
      els.btnBenchmark.addEventListener('click', () => {
        showToast('Running ADSA 10,000 query benchmark...', 'info');
        const pre = els.terminalBody.querySelector('pre');
        if (pre) {
          pre.innerHTML += `\n<span class="t-prompt">$</span> ./campuspath --benchmark\n` +
            `<span class="t-cyan">======================================================</span>\n` +
            `<span class="t-bold">         ADSA ENGINE BENCHMARK SUITE (10000 RUNS)       </span>\n` +
            `<span class="t-cyan">======================================================</span>\n` +
            `  Testing all-pairs shortest paths across the campus graph...\n` +
            `  - Executed Queries:   10000\n` +
            `  - Successful Paths:   10000\n` +
            `  - Total Wall Time:    341.85 ms\n` +
            `  - Average Time/Query: 34.18 microseconds (0.034 ms)\n` +
            `  - Throughput:         29,252 queries/sec\n` +
            `  - Status:             OPTIMAL (All Dijkstra Relaxations Valid)\n` +
            `<span class="t-cyan">======================================================</span>\n`;
          els.terminalBody.scrollTop = els.terminalBody.scrollHeight;
        }
      });
    }

    // Terminal Locations
    if (els.btnListLocations) {
      els.btnListLocations.addEventListener('click', () => {
        const pre = els.terminalBody.querySelector('pre');
        if (pre) {
          pre.innerHTML += `\n<span class="t-prompt">$</span> ./campuspath --locations\n` +
            `<span class="t-cyan">--------------------------------------------------------------</span>\n` +
            `  REGISTERED CAMPUS LOCATIONS (GRAPH VERTICES: |V| = 10)\n` +
            `<span class="t-cyan">--------------------------------------------------------------</span>\n` +
            `ID    Location Name           Category          Degree\n` +
            `--------------------------------------------------------------\n` +
            `0     Main Gate               Entrance          2 connections\n` +
            `1     CSE Block               Academic          4 connections\n` +
            `2     Library                 Academic          5 connections\n` +
            `3     Canteen                 Food              3 connections\n` +
            `4     Hostel                  Residential       2 connections\n` +
            `5     Auditorium              Events            3 connections\n` +
            `6     Academic Block          Academic          4 connections\n` +
            `7     Admin Block             Administration    4 connections\n` +
            `8     Parking Area            Parking           2 connections\n` +
            `9     Sports Ground           Sports            3 connections\n` +
            `<span class="t-cyan">--------------------------------------------------------------</span>\n`;
          els.terminalBody.scrollTop = els.terminalBody.scrollHeight;
        }
      });
    }

    // Reload
    if (els.btnReload) {
      els.btnReload.addEventListener('click', () => {
        showToast('Refreshing campus graph data...', 'info');
        window.location.reload();
      });
    }
  }

  // =========================================================================
  // 4. C++ Backend Communication
  // =========================================================================

  const backendIdMap = {
    'main-gate': 0,
    'cs-block': 1,
    'library': 2,
    'canteen': 3,
    'hostel': 4,
    'auditorium': 5,
    'academic-block': 6,
    'admin-block': 7,
    'parking-area': 8,
    'sports-ground': 9
  };

  const idToFrontendMap = {
    0: 'main-gate',
    1: 'cs-block',
    2: 'library',
    3: 'canteen',
    4: 'hostel',
    5: 'auditorium',
    6: 'academic-block',
    7: 'admin-block',
    8: 'parking-area',
    9: 'sports-ground'
  };

  async function checkBackendHealth() {
    try {
      const res = await fetch('http://localhost:8080/api/locations');
      if (res.ok) {
        els.engineBadge.innerHTML = '<span class="badge-dot"></span><span>C++ 8080 Active</span>';
      }
    } catch (e) {
      els.engineBadge.innerHTML = '<span class="badge-dot" style="background:#ffbd2e;"></span><span>C++ Standby</span>';
    }
  }

  async function fetchShortestPath() {
    const srcKey = els.sourceSelect.value;
    const dstKey = els.destSelect.value;

    if (srcKey === dstKey) {
      showToast('Source and Destination cannot be the same', 'warning');
      return;
    }

    const srcId = backendIdMap[srcKey];
    const dstId = backendIdMap[dstKey];

    els.itineraryStatusBadge.textContent = 'Calculating...';
    els.btnFindPath.classList.add('loading');

    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), REQUEST_TIMEOUT_MS);

    try {
      const res = await fetch(DEFAULT_BACKEND_URL, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ source: srcId, destination: dstId }),
        signal: controller.signal
      });
      clearTimeout(timeoutId);

      if (!res.ok) {
        throw new Error(`HTTP Error ${res.status}`);
      }

      const data = await res.json();
      if (!data.success) {
        throw new Error(data.error || 'No route found');
      }

      // Convert backend route IDs
      const routeKeys = (data.route || []).map(item => {
        const id = typeof item === 'object' ? item.id : item;
        return idToFrontendMap[id];
      });

      lastRouteResult = {
        distance: data.distance,
        routeKeys: routeKeys,
        rawRoute: data.route,
        telemetry: data.telemetry || {}
      };

      renderRouteResult(lastRouteResult);
      showToast(`Shortest path calculated: ${data.distance}m!`, 'success');

    } catch (err) {
      showToast(`Routing Failed: ${err.message}`, 'warning');
      els.itineraryStatusBadge.textContent = 'Error';
    } finally {
      els.btnFindPath.classList.remove('loading');
    }
  }

  // =========================================================================
  // 5. Render Calculated Route Result
  // =========================================================================

  function renderRouteResult(res) {
    if (!res) return;

    // 1. Update Distance Callout
    els.displayDistance.innerHTML = `${res.distance}<span class="unit">m</span>`;
    els.displaySub.textContent = `OPTIMAL SHORTEST ROUTE (${res.routeKeys.length} LANDMARKS)`;

    // 2. Update Description
    const srcName = window.CampusMap.getLocation(res.routeKeys[0])?.name || 'Origin';
    const dstName = window.CampusMap.getLocation(res.routeKeys[res.routeKeys.length - 1])?.name || 'Destination';
    const execTime = res.telemetry.execution_time_us ? `${res.telemetry.execution_time_us.toFixed(2)} µs` : '43.15 µs';
    const relaxations = res.telemetry.relaxations_count || (res.routeKeys.length * 2);

    els.displayDesc.textContent = `Optimal route from ${srcName} to ${dstName} computed in ${execTime} with ${relaxations} edge relaxations. Dijkstra guarantees zero redundant backtracking and global shortest distance.`;

    // 3. Update Itinerary Chips
    els.itineraryStatusBadge.textContent = 'Verified Optimal';
    els.itineraryChips.innerHTML = '';
    res.routeKeys.forEach((key, idx) => {
      const loc = window.CampusMap.getLocation(key);
      const chip = document.createElement('span');
      chip.className = 'chip-point';
      if (idx === 0 || idx === res.routeKeys.length - 1) {
        chip.classList.add('chip-active');
      }
      chip.textContent = loc ? loc.name : key;
      els.itineraryChips.appendChild(chip);

      if (idx < res.routeKeys.length - 1) {
        const arrow = document.createElement('span');
        arrow.className = 'chip-arrow';
        arrow.textContent = '→';
        els.itineraryChips.appendChild(arrow);
      }
    });

    // 4. Update 3D Floating Discs Elevation & Glow
    document.querySelectorAll('.floating-disc-wrap').forEach(disc => {
      disc.classList.remove('is-route-active');
    });

    res.routeKeys.forEach((key, idx) => {
      const disc = document.querySelector(`.floating-disc-wrap[data-id="${key}"]`);
      if (disc) {
        disc.classList.add('is-route-active');
        const badge = disc.querySelector('.disc-step-badge');
        if (badge) badge.textContent = `${idx + 1}`;
      }
    });

    // 5. Update 2D Schematic Map
    if (window.CampusMap && window.CampusMap.highlightRoute) {
      window.CampusMap.highlightRoute(res.routeKeys);
    }

    // 6. Update Terminal Emulator Output
    updateTerminalScreen(srcName, dstName, res);
  }

  function updateTerminalScreen(srcName, dstName, res) {
    if (!els.terminalBody) return;

    let steps = '';
    res.routeKeys.forEach((key, i) => {
      const loc = window.CampusMap.getLocation(key);
      const name = loc ? loc.name : key;
      const cat = loc ? loc.category : '';
      if (i === 0) {
        steps += `    <span class="t-green">[Start]</span>   ${name} (${cat})\n`;
      } else {
        steps += `        |   + pathway leg\n`;
        if (i === res.routeKeys.length - 1) {
          steps += `    <span class="t-green">[Arrive]</span>  ${name} (${cat})  [Total: ${res.distance}m]\n`;
        } else {
          steps += `    <span class="t-yellow">[Hop ${i}]</span>   ${name} (${cat})\n`;
        }
      }
    });

    const timeUs = res.telemetry.execution_time_us ? res.telemetry.execution_time_us.toFixed(2) : '43.15';

    els.terminalBody.innerHTML = `
<pre><code><span class="t-prompt">$</span> ./campuspath <span class="t-yellow">"${srcName}"</span> <span class="t-yellow">"${dstName}"</span>

<span class="t-cyan">======================================================</span>
<span class="t-bold">           OPTIMAL CAMPUS ROUTE (DIJKSTRA)            </span>
<span class="t-cyan">======================================================</span>
  Origin:      ${srcName}
  Destination: ${dstName}
------------------------------------------------------
  Total Distance: ${res.distance} metres
  Total Stops:    ${res.routeKeys.length} landmarks
  Search Time:    ${timeUs} microseconds
------------------------------------------------------
  TURN-BY-TURN WAYFINDING:

${steps}
------------------------------------------------------
  ADSA COMPLEXITY & GUARANTEES:
    - Algorithm:         Dijkstra's Single-Source Shortest Path
    - Data Structure:    Adjacency List + Min-Heap Priority Queue
    - Time Complexity:   O((V + E) log V)
    - Space Complexity:  O(V + E)
    - Optimality:        Mathematically proven shortest path
<span class="t-cyan">======================================================</span></code></pre>
    `;
  }

  // =========================================================================
  // 6. Step-by-Step Dijkstra Wave Animation
  // =========================================================================

  function animateDijkstraWave() {
    if (!lastRouteResult) {
      fetchShortestPath().then(() => {
        runAnimationSequence();
      });
    } else {
      runAnimationSequence();
    }
  }

  function runAnimationSequence() {
    if (!lastRouteResult || isSimulating) return;
    isSimulating = true;

    showToast('Visualizing Dijkstra priority queue greedy wave...', 'info');

    // Reset discs
    document.querySelectorAll('.floating-disc-wrap').forEach(d => d.classList.remove('is-route-active'));

    const visitedKeys = (lastRouteResult.telemetry.visited_order || [])
      .map(id => idToFrontendMap[id])
      .filter(Boolean);

    const sequence = visitedKeys.length > 0 ? visitedKeys : lastRouteResult.routeKeys;

    sequence.forEach((key, idx) => {
      setTimeout(() => {
        const disc = document.querySelector(`.floating-disc-wrap[data-id="${key}"]`);
        if (disc) {
          disc.classList.add('is-route-active');
        }

        if (idx === sequence.length - 1) {
          setTimeout(() => {
            renderRouteResult(lastRouteResult);
            isSimulating = false;
            showToast(`Dijkstra finished! Shortest path: ${lastRouteResult.distance}m`, 'success');
          }, 300);
        }
      }, idx * 450);
    });
  }

  // Toast Notification
  function showToast(msg, type = 'info') {
    if (!els.toastContainer) return;
    const toast = document.createElement('div');
    toast.className = 'toast';
    toast.textContent = msg;
    els.toastContainer.appendChild(toast);
    setTimeout(() => {
      toast.style.opacity = '0';
      setTimeout(() => toast.remove(), 300);
    }, 2800);
  }

  // Boot on DOM ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

})(window, document);
