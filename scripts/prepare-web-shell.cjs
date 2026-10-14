// Adapt the pinned framework document before any page scripts can paint it.
const fs = require('node:fs');
function prepareWebShell(html) {
  if (html.includes('id="cod2-startup-gate"')) return html;
  if (!html.includes('<main id="launcher"') || !html.includes('</head>'))
    throw Error('Unexpected framework document');
  const gate = '<style id="cod2-startup-gate">html,body{background:#000!important}#launcher{display:none!important}#launcher:has(#error:not([hidden])){display:grid!important}#launcher-form>:not(#error):not(#status){display:none!important}</style>\n  <link rel="stylesheet" href="/startup.css">\n';
  return html.replace('</head>', gate + '</head>');
}
module.exports = {prepareWebShell};
if (require.main === module) {
  const path = process.argv[2];
  fs.writeFileSync(path, prepareWebShell(fs.readFileSync(path, 'utf8')));
}
