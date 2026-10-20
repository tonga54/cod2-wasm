/* Controller vibration through the Gamepad API: the game's strong and weak
 * motors, each 0 to 1, given every frame while either turns. */
function createCod2Haptics(getPads = () => globalThis.navigator?.getGamepads?.() ?? [], now = () => performance.now()) {
  // Every command outlasts the refresh, so a steady rumble never gaps and a
  // page that stops calling, hidden or unloaded, goes still by itself.
  const EFFECT_MS = 120, REFRESH_MS = 50, STEP = 0.03;
  let target = null, strong = 0, weak = 0, sent = 0;
  const quiet = result => { if (result && typeof result.catch === 'function') result.catch(() => {}); };
  const vibrates = pad => !!pad && pad.connected !== false && !!(pad.vibrationActuator || pad.hapticActuators?.length);
  const busy = pad => !!(pad.buttons?.some(b => b.pressed || b.value > 0.1) || pad.axes?.some(a => Math.abs(a) > 0.25));
  // The pad being played with: one held or moved, else the latest to report.
  function pick(pads) {
    let best = null, bestBusy = false;
    for (const pad of pads) {
      if (!vibrates(pad)) continue;
      const active = busy(pad);
      if (!best || (active && !bestBusy) || (active === bestBusy && pad.timestamp > best.timestamp)) {
        best = pad;
        bestBusy = active;
      }
    }
    return best;
  }
  function send(pad, s, w, ms) {
    try {
      const actuator = pad.vibrationActuator;
      if (actuator && !ms && actuator.reset) quiet(actuator.reset());
      else if (actuator) quiet(actuator.playEffect('dual-rumble', { startDelay: 0, duration: ms, strongMagnitude: s, weakMagnitude: w }));
      else quiet(pad.hapticActuators[0].pulse(Math.max(s, w), ms));
    } catch (error) {}
  }
  const pads = () => Array.from(getPads() || []);
  return {
    rumble(s, w) {
      s = Math.min(1, Math.max(0, +s || 0));
      w = Math.min(1, Math.max(0, +w || 0));
      if (!s && !w) {
        const pad = target === null ? null : pads().find(p => vibrates(p) && p.index === target);
        if (pad) send(pad, 0, 0, 0);
        target = null;
        strong = weak = 0;
        return;
      }
      const t = now();
      if (target !== null && Math.abs(s - strong) < STEP && Math.abs(w - weak) < STEP && t - sent < REFRESH_MS) return;
      // A burst stays on the pad it began on.
      const list = pads();
      const pad = (target !== null && list.find(p => vibrates(p) && p.index === target)) || pick(list);
      if (!pad) return;
      send(pad, s, w, EFFECT_MS);
      target = pad.index;
      strong = s;
      weak = w;
      sent = t;
    }
  };
}
Module['cod2CreateHaptics'] = createCod2Haptics;
