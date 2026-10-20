// Adapt the pinned framework document before any page scripts can paint it.
const fs = require('node:fs');
const MANIFEST = '<link rel="manifest" href="/app.webmanifest">';
function prepareWebShell(html) {
  if (html.includes('id="cod2-startup-gate"')) return html;
  if (!html.includes('<main id="launcher"') || !html.includes('</head>') || !html.includes(MANIFEST))
    throw Error('Unexpected framework document');
  const gate = '<style id="cod2-startup-gate">html,body{background:#000!important}#launcher{display:none!important}#launcher:has(#error:not([hidden])){display:grid!important}#launcher-form>:not(#error):not(#status){display:none!important}</style>\n  <link rel="stylesheet" href="/startup.css">\n' +
    ['Medium', 'Regular'].map(weight =>
      `  <link rel="preload" href="/fonts/Barlow-${weight}.ttf" as="font" type="font/ttf" crossorigin>\n`).join('') +
    // Safari's Add to Home Screen and Add to Dock prefer this icon to the manifest's.
    '  <link rel="apple-touch-icon" href="/game-data/files/cod2-icon/apple-touch-icon.png">\n' +
    // Before any adapter: Chromium can offer installation as soon as the page qualifies.
    '  <script src="/install-app.js" defer></script>\n';
  // Behind a host's login, a manifest fetched without cookies (the default) fails, and the game cannot be installed.
  return html.replace(MANIFEST, MANIFEST.replace('>', ' crossorigin="use-credentials">'))
    .replace('</head>', gate + '</head>');
}
module.exports = {prepareWebShell};
if (require.main === module) {
  const path = process.argv[2];
  fs.writeFileSync(path, prepareWebShell(fs.readFileSync(path, 'utf8')));
}
