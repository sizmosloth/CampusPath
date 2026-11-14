/**
 * CampusPath – Minimalist Architectural Campus Map Module
 * Pure SVG, Apple Light Aesthetic, zero lag, hardware-accelerated
 */

(function (window) {
  'use strict';

  const CAMPUS_CONFIG = {
    viewBox: { width: 1280, height: 820 },
    zoom: { min: 0.65, max: 2.5, step: 0.2 }
  };

  /**
   * Official Campus Landmark Vertices (|V| = 10)
   * Spaced cleanly on 1280x820 canvas
   */
  const CAMPUS_LOCATIONS = [
    {
      id: 'main-gate',
      backendId: 0,
      name: 'Main Gate',
      x: 160,
      y: 680,
      width: 176,
      height: 72,
      category: 'Entrance',
      code: 'MG',
      accentColor: '#0071e3', // Apple Blue
      sub: 'Main Entrance & Security',
      description: 'Primary campus entrance, visitor verification checkpoint and transit hub.'
    },
    {
      id: 'parking-area',
      backendId: 8,
      name: 'Parking Area',
      x: 160,
      y: 260,
      width: 176,
      height: 72,
      category: 'Transit',
      code: 'PK',
      accentColor: '#34c759', // Apple Green
      sub: 'EV Charging & Multi-Bay',
      description: 'Multi-level vehicular parking with 24 fast EV charging stations.'
    },
    {
      id: 'admin-block',
      backendId: 7,
      name: 'Admin Block',
      x: 420,
      y: 550,
      width: 176,
      height: 72,
      category: 'Administration',
      code: 'AD',
      accentColor: '#ff9500', // Apple Orange
      sub: 'Registrar & Student Affairs',
      description: 'Directorate, Dean of Academic Affairs, Admissions, and Finance.'
    },
    {
      id: 'auditorium',
      backendId: 5,
      name: 'Auditorium',
      x: 420,
      y: 200,
      width: 176,
      height: 72,
      category: 'Conventions',
      code: 'AU',
      accentColor: '#af52de', // Apple Purple
      sub: '1,500-Seat Amphitheater',
      description: 'Acoustic hall for university conventions, symposia, and cultural events.'
    },
    {
      id: 'library',
      backendId: 2,
      name: 'Central Library',
      x: 670,
      y: 380,
      width: 176,
      height: 72,
      category: 'Academic Core',
      code: 'LB',
      accentColor: '#0071e3', // Apple Blue
      sub: 'Knowledge & Study Suites',
      description: 'Central library, quiet study suites, computer center, and digital archives.'
    },
    {
      id: 'academic-block',
      backendId: 6,
      name: 'Academic Block',
      x: 670,
      y: 680,
      width: 176,
      height: 72,
      category: 'Lectures',
      code: 'AC',
      accentColor: '#5ac8fa', // Apple Teal
      sub: 'Tiered Lecture Theaters',
      description: 'Multidisciplinary lecture complexes and faculty departments.'
    },
    {
      id: 'canteen',
      backendId: 3,
      name: 'Canteen',
      x: 920,
      y: 200,
      width: 176,
      height: 72,
      category: 'Dining & Social',
      code: 'CN',
      accentColor: '#ff9500', // Apple Orange
      sub: 'Food Court & Cafe',
      description: 'Dining hall, outdoor garden cafe, coffee bar, and student commons.'
    },
    {
      id: 'cs-block',
      backendId: 1,
      name: 'CSE Block',
      x: 920,
      y: 470,
      width: 176,
      height: 72,
      category: 'Engineering & CS',
      code: 'CS',
      accentColor: '#5856d6', // Apple Indigo
      sub: 'Computing & AI Labs',
      description: 'Supercomputing center, AI robotics labs, and departmental complexes.'
    },
    {
      id: 'hostel',
      backendId: 4,
      name: 'Hostel Complex',
      x: 1130,
      y: 220,
      width: 176,
      height: 72,
      category: 'Residential',
      code: 'HS',
      accentColor: '#af52de', // Apple Purple
      sub: 'Student Residential Towers',
      description: 'Residential towers, recreational lounges, and student amenities.'
    },
    {
      id: 'sports-ground',
      backendId: 9,
      name: 'Sports Ground',
      x: 1130,
      y: 620,
      width: 176,
      height: 72,
      category: 'Athletics',
      code: 'SG',
      accentColor: '#34c759', // Apple Green
      sub: 'Running Track & Turf Pitch',
      description: 'Synthetic running track, full grass soccer pitch, and tennis courts.'
    },
    {
      id: 'stationery',
      backendId: 10,
      name: 'Stationery Shop',
      x: 420,
      y: 375,
      width: 176,
      height: 72,
      category: 'Store & Books',
      code: 'ST',
      accentColor: '#ff9500', // Apple Orange
      sub: 'Books, Supplies & Print',
      description: 'Campus bookstore, stationery supplies, photocopying and academic materials.'
    }
  ];

  /**
   * 19 Official Walkways with Exact Distances (|E| = 19)
   */
  const CAMPUS_PATHWAYS = [
    { from: 'main-gate', to: 'admin-block', distance: 250 },
    { from: 'main-gate', to: 'parking-area', distance: 180 },
    { from: 'parking-area', to: 'auditorium', distance: 220 },
    { from: 'auditorium', to: 'library', distance: 190 },
    { from: 'auditorium', to: 'admin-block', distance: 280 },
    { from: 'admin-block', to: 'library', distance: 200 },
    { from: 'admin-block', to: 'academic-block', distance: 240 },
    { from: 'library', to: 'canteen', distance: 180 },
    { from: 'library', to: 'academic-block', distance: 210 },
    { from: 'library', to: 'cs-block', distance: 230 },
    { from: 'canteen', to: 'hostel', distance: 200 },
    { from: 'canteen', to: 'cs-block', distance: 250 },
    { from: 'hostel', to: 'sports-ground', distance: 180 },
    { from: 'academic-block', to: 'cs-block', distance: 170 },
    { from: 'academic-block', to: 'sports-ground', distance: 220 },
    { from: 'cs-block', to: 'sports-ground', distance: 160 },
    { from: 'stationery', to: 'admin-block', distance: 120 },
    { from: 'stationery', to: 'library', distance: 140 },
    { from: 'stationery', to: 'auditorium', distance: 150 }
  ];

  // Lookup Indexes
  const LOCATIONS_BY_ID = new Map();
  CAMPUS_LOCATIONS.forEach(loc => {
    LOCATIONS_BY_ID.set(loc.id, loc);
    LOCATIONS_BY_ID.set(loc.name.toLowerCase(), loc);
    LOCATIONS_BY_ID.set(loc.code.toLowerCase(), loc);
    LOCATIONS_BY_ID.set(String(loc.backendId), loc);
    LOCATIONS_BY_ID.set(loc.backendId, loc);
  });

  // Pan & Zoom State
  let zoomLevel = 1.0;
  let panX = 0;
  let panY = 0;
  let isPanning = false;
  let panStartX = 0;
  let panStartY = 0;
  let panMovedDistance = 0;

  let activeRouteIds = [];
  let onSelectCallback = null;

  // =========================================================================
  // 1. Initialization
  // =========================================================================

  function init(options) {
    if (options && options.onLocationSelected) {
      onSelectCallback = options.onLocationSelected;
    }

    renderLandscapeZones();
    renderWalkwayConduits();
    renderArchitecturalNodes();
    setupPanZoomEvents();
    setupTooltips();
  }

  function getLocation(idOrName) {
    if (idOrName === undefined || idOrName === null) return null;
    if (typeof idOrName === 'number') {
      return LOCATIONS_BY_ID.get(idOrName) || null;
    }
    const str = String(idOrName).trim();
    return LOCATIONS_BY_ID.get(str.toLowerCase()) || LOCATIONS_BY_ID.get(str) || null;
  }

  // =========================================================================
  // 2. Landscape & Zones (Clean Minimal Light)
  // =========================================================================

  function renderLandscapeZones() {
    const landscapeG = document.getElementById('campus-landscape');
    if (!landscapeG) return;

    landscapeG.innerHTML = `
      <!-- Sports Pitch Turf -->
      <rect x="1040" y="560" width="180" height="120" rx="14" fill="#f0fdf4" stroke="#bbf7d0" stroke-width="1.2" stroke-dasharray="5 3" />
      <circle cx="1130" cy="620" r="28" fill="none" stroke="#86efac" stroke-width="1" />
      <line x1="1040" y1="620" x2="1220" y2="620" stroke="#86efac" stroke-width="1" />

      <!-- Library Central Plaza -->
      <circle cx="670" cy="380" r="110" fill="#f8fafc" stroke="#e2e8f0" stroke-width="1" stroke-dasharray="4 4" />
      <circle cx="670" cy="380" r="140" fill="none" stroke="#edf2f7" stroke-width="1" stroke-dasharray="2 6" />

      <!-- Boulevard Avenue Path -->
      <path d="M 160 680 L 420 550 L 670 380 L 920 470 L 1130 620" fill="none" stroke="#f1f5f9" stroke-width="32" stroke-linecap="round" stroke-linejoin="round" />
    `;
  }

  // =========================================================================
  // 3. Walkways & Distance Badges
  // =========================================================================

  function renderWalkwayConduits() {
    const pathwaysG = document.getElementById('campus-pathways-group');
    if (!pathwaysG) return;

    pathwaysG.innerHTML = '';

    CAMPUS_PATHWAYS.forEach(edge => {
      const u = getLocation(edge.from);
      const v = getLocation(edge.to);
      if (!u || !v) return;

      const segG = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      segG.setAttribute('class', 'walkway-segment');
      segG.setAttribute('id', `edge-${u.id}_${v.id}`);
      segG.setAttribute('data-u', u.id);
      segG.setAttribute('data-v', v.id);

      // 1. Base Paved Conduit
      const baseLine = document.createElementNS('http://www.w3.org/2000/svg', 'line');
      baseLine.setAttribute('x1', u.x);
      baseLine.setAttribute('y1', u.y);
      baseLine.setAttribute('x2', v.x);
      baseLine.setAttribute('y2', v.y);
      baseLine.setAttribute('class', 'walkway-base');

      // 2. Dashed Centerline Guide
      const centerLine = document.createElementNS('http://www.w3.org/2000/svg', 'line');
      centerLine.setAttribute('x1', u.x);
      centerLine.setAttribute('y1', u.y);
      centerLine.setAttribute('x2', v.x);
      centerLine.setAttribute('y2', v.y);
      centerLine.setAttribute('class', 'walkway-center');

      // 3. Crisp Distance Badge at Midpoint
      const midX = (u.x + v.x) / 2;
      const midY = (u.y + v.y) / 2;

      const badgeG = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      badgeG.setAttribute('class', 'walkway-badge-group');
      badgeG.setAttribute('transform', `translate(${midX}, ${midY})`);

      badgeG.innerHTML = `
        <rect x="-22" y="-10" width="44" height="20" rx="10" class="edge-badge-rect" />
        <text x="0" y="4" text-anchor="middle" class="edge-badge-text">${edge.distance}m</text>
      `;

      segG.appendChild(baseLine);
      segG.appendChild(centerLine);
      segG.appendChild(badgeG);

      pathwaysG.appendChild(segG);
    });
  }

  // =========================================================================
  // 4. Architectural Building Nodes (Apple Light Style)
  // =========================================================================

  function renderArchitecturalNodes() {
    const nodesG = document.getElementById('campus-nodes-group');
    if (!nodesG) return;

    nodesG.innerHTML = '';

    CAMPUS_LOCATIONS.forEach(loc => {
      const bx = loc.x - loc.width / 2;
      const by = loc.y - loc.height / 2;

      const nodeG = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      nodeG.setAttribute('class', 'campus-node-card');
      nodeG.setAttribute('id', `node-${loc.id}`);
      nodeG.setAttribute('data-id', loc.id);
      nodeG.setAttribute('data-name', loc.name);

      nodeG.innerHTML = `
        <!-- Soft Card Drop Shadow -->
        <rect x="${bx + 2}" y="${by + 3}" width="${loc.width - 4}" height="${loc.height}" rx="12" class="node-shadow" />

        <!-- Card Chassis Body -->
        <rect x="${bx}" y="${by}" width="${loc.width}" height="${loc.height}" rx="12" class="node-chassis" />

        <!-- Left Accent Color Indicator -->
        <rect x="${bx}" y="${by}" width="4" height="${loc.height}" rx="2" class="node-accent-bar" style="fill: ${loc.accentColor};" />

        <!-- Category Tag -->
        <text x="${bx + 14}" y="${by + 20}" class="node-category">${loc.category.toUpperCase()}</text>

        <!-- ID Badge Pill -->
        <rect x="${bx + loc.width - 32}" y="${by + 10}" width="22" height="15" rx="4" class="node-id-badge" />
        <text x="${bx + loc.width - 21}" y="${by + 21}" text-anchor="middle" class="node-id-text">#${loc.backendId}</text>

        <!-- Main Building Title -->
        <text x="${bx + 14}" y="${by + 42}" class="node-title">${loc.name}</text>

        <!-- Subtitle Information -->
        <text x="${bx + 14}" y="${by + 58}" class="node-sub">${loc.sub}</text>
      `;

      // Click event for interactive route selection
      nodeG.addEventListener('click', (e) => {
        e.stopPropagation();
        if (panMovedDistance > 8) return;
        if (onSelectCallback) {
          onSelectCallback(loc.id, loc.name);
        }
      });

      nodesG.appendChild(nodeG);
    });
  }

  // =========================================================================
  // 5. Route Highlighting & Dynamic Path Drawing
  // =========================================================================

  function highlightRoute(orderedIds, stopNodeId) {
    clearRoute();

    if (!orderedIds || !Array.isArray(orderedIds) || orderedIds.length < 2) {
      if (orderedIds && orderedIds.length === 1) {
        const single = getLocation(orderedIds[0]);
        if (single) {
          const el = document.getElementById(`node-${single.id}`);
          if (el) el.classList.add('is-source-node');
        }
      }
      return;
    }

    const resolved = orderedIds.map(id => getLocation(id)).filter(Boolean);
    if (resolved.length < 2) return;

    activeRouteIds = resolved.map(n => n.id);

    // 1. Activate Walkway Conduits
    for (let i = 0; i < resolved.length - 1; i++) {
      const u = resolved[i].id;
      const v = resolved[i + 1].id;

      const seg1 = document.getElementById(`edge-${u}_${v}`);
      const seg2 = document.getElementById(`edge-${v}_${u}`);

      if (seg1) seg1.classList.add('is-active');
      if (seg2) seg2.classList.add('is-active');
    }

    // 2. Draw Clean Solid Route Path
    const line = document.getElementById('route-path-line');
    let d = '';
    resolved.forEach((n, idx) => {
      d += (idx === 0 ? `M ${n.x} ${n.y} ` : `L ${n.x} ${n.y} `);
    });

    if (line) {
      line.setAttribute('d', d);
      line.style.opacity = '1';
    }

    // 3. Highlight Node Cards
    resolved.forEach((n, idx) => {
      const nodeEl = document.getElementById(`node-${n.id}`);
      if (!nodeEl) return;

      nodeEl.classList.add('is-route-node');

      if (stopNodeId && n.id === stopNodeId) {
        nodeEl.classList.add('is-stop-node');
      } else if (idx === 0) {
        nodeEl.classList.add('is-source-node');
      } else if (idx === resolved.length - 1) {
        nodeEl.classList.add('is-dest-node');
      } else {
        nodeEl.classList.add('is-waypoint-node');
      }
    });
  }

  function clearRoute() {
    activeRouteIds = [];

    const line = document.getElementById('route-path-line');
    if (line) { line.setAttribute('d', ''); line.style.opacity = '0'; }

    document.querySelectorAll('.walkway-segment.is-active').forEach(s => s.classList.remove('is-active'));
    document.querySelectorAll('.campus-node-card').forEach(n => {
      n.classList.remove('is-route-node', 'is-source-node', 'is-dest-node', 'is-waypoint-node', 'is-stop-node', 'is-exploring');
    });
  }

  // =========================================================================
  // 6. Dijkstra Simulation Wave Animation
  // =========================================================================

  let simTimeouts = [];

  function animateDijkstraSimulation(visitedOrder, finalRoute, onStep, onFinish) {
    stopDijkstraSimulation();
    clearRoute();

    if (!visitedOrder || visitedOrder.length === 0) {
      highlightRoute(finalRoute);
      if (onFinish) onFinish();
      return;
    }

    const stepMs = 340;
    visitedOrder.forEach((idOrNum, idx) => {
      const tid = setTimeout(() => {
        const loc = getLocation(idOrNum);
        if (loc) {
          const el = document.getElementById(`node-${loc.id}`);
          if (el) {
            el.classList.add('is-exploring');
          }
          if (onStep) onStep(loc.id, idx);
        }

        // Final step: draw confirmed path
        if (idx === visitedOrder.length - 1) {
          const finTid = setTimeout(() => {
            highlightRoute(finalRoute);
            if (onFinish) onFinish();
          }, 250);
          simTimeouts.push(finTid);
        }
      }, idx * stepMs);

      simTimeouts.push(tid);
    });
  }

  function stopDijkstraSimulation() {
    simTimeouts.forEach(t => clearTimeout(t));
    simTimeouts = [];
    document.querySelectorAll('.campus-node-card.is-exploring').forEach(n => n.classList.remove('is-exploring'));
  }

  // =========================================================================
  // 7. Pan & Zoom Viewport
  // =========================================================================

  function setupPanZoomEvents() {
    const container = document.getElementById('map-scroll-container');
    const svg = document.getElementById('campus-svg');
    const btnIn = document.getElementById('btn-zoom-in');
    const btnOut = document.getElementById('btn-zoom-out');
    const btnReset = document.getElementById('btn-reset-view');

    if (!container || !svg) return;

    function update() {
      svg.style.transform = `translate(${panX}px, ${panY}px) scale(${zoomLevel})`;
    }

    if (btnIn) {
      btnIn.addEventListener('click', () => {
        zoomLevel = Math.min(CAMPUS_CONFIG.zoom.max, +(zoomLevel + CAMPUS_CONFIG.zoom.step).toFixed(2));
        update();
      });
    }

    if (btnOut) {
      btnOut.addEventListener('click', () => {
        zoomLevel = Math.max(CAMPUS_CONFIG.zoom.min, +(zoomLevel - CAMPUS_CONFIG.zoom.step).toFixed(2));
        update();
      });
    }

    if (btnReset) {
      btnReset.addEventListener('click', () => {
        zoomLevel = 1.0;
        panX = 0;
        panY = 0;
        update();
      });
    }

    // Wheel zoom
    container.addEventListener('wheel', (e) => {
      e.preventDefault();
      const delta = e.deltaY < 0 ? 0.08 : -0.08;
      zoomLevel = Math.min(CAMPUS_CONFIG.zoom.max, Math.max(CAMPUS_CONFIG.zoom.min, +(zoomLevel + delta).toFixed(2)));
      update();
    }, { passive: false });

    // Drag to pan
    container.addEventListener('mousedown', (e) => {
      if (e.target.closest('.campus-node-card')) return;
      isPanning = true;
      panMovedDistance = 0;
      panStartX = e.clientX - panX;
      panStartY = e.clientY - panY;
      container.style.cursor = 'grabbing';
    });

    window.addEventListener('mousemove', (e) => {
      if (!isPanning) return;
      const nx = e.clientX - panStartX;
      const ny = e.clientY - panStartY;
      panMovedDistance += Math.abs(nx - panX) + Math.abs(ny - panY);
      panX = nx;
      panY = ny;
      update();
    });

    window.addEventListener('mouseup', () => {
      if (isPanning) {
        isPanning = false;
        container.style.cursor = '';
      }
    });
  }

  // =========================================================================
  // 8. Tooltip Integration
  // =========================================================================

  function setupTooltips() {
    const tooltip = document.getElementById('map-tooltip');
    const container = document.getElementById('map-scroll-container');
    const svg = document.getElementById('campus-svg');

    if (!tooltip || !container || !svg) return;

    svg.addEventListener('mousemove', (e) => {
      const card = e.target.closest('.campus-node-card');
      if (card) {
        const id = card.getAttribute('data-id');
        const loc = getLocation(id);
        if (loc) {
          const rect = container.getBoundingClientRect();
          const x = e.clientX - rect.left;
          const y = e.clientY - rect.top;

          document.getElementById('tooltip-title').textContent = `${loc.name} (#${loc.backendId})`;
          document.getElementById('tooltip-category').textContent = loc.category.toUpperCase();
          document.getElementById('tooltip-desc').textContent = loc.description;

          tooltip.style.left = `${x + 14}px`;
          tooltip.style.top = `${y + 14}px`;
          tooltip.classList.add('visible');
        }
      } else {
        tooltip.classList.remove('visible');
      }
    });

    svg.addEventListener('mouseleave', () => {
      tooltip.classList.remove('visible');
    });
  }

  // =========================================================================
  // 9. Public Export
  // =========================================================================

  window.CampusMap = {
    CONFIG: CAMPUS_CONFIG,
    LOCATIONS: CAMPUS_LOCATIONS,
    PATHWAYS: CAMPUS_PATHWAYS,
    init,
    getLocation,
    highlightRoute,
    clearRoute,
    animateDijkstraSimulation,
    stopDijkstraSimulation
  };

})(window);
