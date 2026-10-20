(function () {
  'use strict';
  // Chromium offers to install the page (beforeinstallprompt) once it
  // qualifies, perhaps before the adapter has loaded; Safari and Firefox never
  // do. The offer waits in the WWII menu's right key-prompt place while the
  // adapter's cod2InstallMoment() reports the loading screen or the main menu.
  // The game draws its own cursor there, so the label takes no pointer events:
  // a click over it is taken before it reaches the game.
  const installed = ['standalone', 'fullscreen', 'minimal-ui', 'window-controls-overlay']
    .map(mode => matchMedia(`(display-mode: ${mode})`));
  let offer = null, label = null, frame = 0;
  function over(event) {
    if (!label || label.hidden) return false;
    const rect = label.getBoundingClientRect();
    return event.clientX >= rect.left && event.clientX <= rect.right &&
      event.clientY >= rect.top && event.clientY <= rect.bottom;
  }
  function update() {
    cancelAnimationFrame(frame);
    frame = 0;
    const show = Boolean(offer) && !installed.some(query => query.matches) &&
      globalThis.cod2InstallMoment?.() === true;
    if (show && !label) {
      label = document.createElement('div');
      label.id = 'cod2-install-app';
      label.setAttribute('aria-hidden', 'true');
      label.innerHTML = '<svg viewBox="0 0 24 24"><path d="M12 3.5v11M7.25 9.75 12 14.5l4.75-4.75M4.5 15.5v4h15v-4"/></svg>Install app';
      document.body.appendChild(label);
    }
    if (label) {
      label.hidden = !show;
      if (!show) label.removeAttribute('data-hover');
    }
    if (offer) frame = requestAnimationFrame(update);
  }
  addEventListener('beforeinstallprompt', event => {
    event.preventDefault();
    offer = event;
    update();
  });
  addEventListener('appinstalled', () => {
    offer = null;
    update();
  });
  addEventListener('pointermove', event => {
    if (label) label.toggleAttribute('data-hover', over(event));
  }, { capture: true, passive: true });
  addEventListener('pointerdown', event => {
    if (event.button !== 0 || !offer || !over(event)) return;
    event.preventDefault();
    event.stopImmediatePropagation();
    // An offer prompts once, accepted or not.
    const pending = offer;
    offer = null;
    update();
    new Promise(resolve => resolve(pending.prompt())).catch(() => {
      /* A refused prompt leaves the browser's own install entry. */
    });
  }, true);
})();
