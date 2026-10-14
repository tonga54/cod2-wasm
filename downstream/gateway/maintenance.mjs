import { readFileSync } from 'node:fs';

// Set by the host updater only while replacing an idle stack. Expiry permits
// recovery after a killed updater; an existing match is never disconnected.
export function createMaintenanceCheck({ path = '/game-ui/host-maintenance.json',
  read = readFileSync, now = Date.now } = {}) {
  return () => {
    try {
      const raw = read(path, 'utf8');
      if (raw.length > 1024) return false;
      const value = JSON.parse(raw);
      return Number.isFinite(value.until) && value.until > now() && value.until <= now() + 600000;
    } catch { return false; }
  };
}
