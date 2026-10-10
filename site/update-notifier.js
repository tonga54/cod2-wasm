(function () {
  'use strict';
  const guide = 'https://github.com/tonga54/cod2-wasm/blob/master/docs/UPDATES.md';
  globalThis.createCod2UpdateNotifier = function ({ canNotify }) {
    let loadedBuild, latest, dismissed, drawnKey, stopped = false, busy = false;
    const panel = document.createElement('aside');
    panel.id = 'cod2-update';
    panel.hidden = true;
    panel.setAttribute('role', 'status');
    panel.setAttribute('aria-live', 'polite');
    const message = document.createElement('p');
    const actions = document.createElement('div');
    const action = document.createElement('button');
    action.type = 'button';
    const help = document.createElement('a');
    help.textContent = 'How to update';
    help.href = guide;
    help.target = '_blank';
    help.rel = 'noopener noreferrer';
    const later = document.createElement('button');
    later.type = 'button';
    later.textContent = 'Later';
    later.addEventListener('click', () => { dismissed = noticeKey(); render(); });
    action.addEventListener('click', () => {
      // A match may have started after the notice was drawn.
      if (canNotify() && loadedBuild && latest?.current?.buildId &&
          latest.current.buildId !== loadedBuild) location.reload();
    });
    actions.append(action, help, later);
    panel.append(message, actions);
    document.body.appendChild(panel);
    // Keep notification interactions out of SDL/canvas input handling.
    for (const type of ['keydown', 'keyup', 'keypress', 'mousedown', 'mouseup', 'click'])
      panel.addEventListener(type, event => event.stopPropagation());
    function noticeKey() {
      if (!loadedBuild || !/^[a-f0-9]{64}$/.test(latest?.current?.buildId)) return null;
      if (loadedBuild !== latest.current.buildId) return `build:${latest.current.buildId}`;
      return latest.update?.available && /^[a-f0-9]{40}$/.test(latest.update.revision)
        ? `repo:${latest.update.revision}` : null;
    }
    function render() {
      const key = noticeKey();
      const hidden = !key || key === dismissed || !canNotify();
      if (panel.hidden !== hidden) panel.hidden = hidden;
      // Engine state is sampled every frame; update the live region just once.
      if (hidden || drawnKey === key) return;
      drawnKey = key;
      const reload = loadedBuild !== latest.current.buildId;
      message.textContent = reload ? 'The game has been updated. Reload to use the new version.' :
        'A new game version is available. The host can update the server.';
      action.hidden = !reload;
      action.textContent = 'Reload';
      help.hidden = reload;
    }
    // The framework samples engine state after interactions, not continuously.
    // Observe menu/disconnect transitions even when nobody presses a key.
    let frame;
    function draw() {
      if (stopped) return;
      render();
      frame = requestAnimationFrame(draw);
    }
    frame = requestAnimationFrame(draw);
    async function poll() {
      if (stopped || busy || document.hidden || !loadedBuild) return;
      busy = true;
      try {
        const response = await fetch('/version', { cache: 'no-store', signal: AbortSignal.timeout(10000) });
        if (response.ok) { latest = await response.json(); render(); }
      } catch { /* Network failures do not interrupt the game. */ }
      finally { busy = false; }
    }
    const ready = (async () => {
      try {
        const response = await fetch('/build-info.json', { cache: 'no-store', signal: AbortSignal.timeout(3000) });
        const build = response.ok && await response.json();
        if (/^[a-f0-9]{64}$/.test(build?.buildId)) loadedBuild = build.buildId;
      } catch { /* Older hosts can still play without update metadata. */ }
      await poll();
    })();
    const timer = setInterval(poll, 60000);
    const visible = () => { if (!document.hidden) poll(); };
    document.addEventListener('visibilitychange', visible);
    globalThis.addEventListener('pagehide', () => {
      stopped = true; clearInterval(timer);
      cancelAnimationFrame(frame);
      document.removeEventListener('visibilitychange', visible);
    }, { once: true });
    return { ready, render };
  };
})();
