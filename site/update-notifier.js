(function () {
  'use strict';
  // Repository commits are handled by the host; players only see installed builds.
  globalThis.createCod2UpdateNotifier = function ({ loadedBuild, canNotify, setMenuNotice }) {
    let latest, drawn, stopped = false, busy = false, firstCheck = false;
    function available() {
      const loaded = loadedBuild();
      return /^[a-f0-9]{64}$/.test(loaded) &&
        /^[a-f0-9]{64}$/.test(latest?.current?.buildId) && loaded !== latest.current.buildId;
    }
    function render() {
      const show = available() && canNotify();
      if (show !== drawn) { setMenuNotice(show); drawn = show; }
    }
    function reload() {
      if (available() && canNotify()) location.reload();
    }
    async function poll() {
      if (stopped || busy || document.hidden || !loadedBuild()) return;
      firstCheck = true;
      busy = true;
      try {
        const response = await fetch('/version', { cache: 'no-store', signal: AbortSignal.timeout(10000) });
        if (response.ok) { latest = await response.json(); render(); }
      } catch { /* Network failures leave the game and last known notice alone. */ }
      finally { busy = false; }
    }
    let frame;
    function draw() {
      if (stopped) return;
      if (!firstCheck && loadedBuild()) poll();
      render();
      frame = requestAnimationFrame(draw);
    }
    frame = requestAnimationFrame(draw);
    const timer = setInterval(poll, 60000);
    const visible = () => { if (!document.hidden) poll(); };
    document.addEventListener('visibilitychange', visible);
    globalThis.addEventListener('pagehide', () => {
      stopped = true; clearInterval(timer); cancelAnimationFrame(frame);
      document.removeEventListener('visibilitychange', visible);
    }, { once: true });
    return { poll, render, reload };
  };
})();
