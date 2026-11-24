// Export body shells separately + mouthpiece local, for collision/clearance checks.
const fs = require('fs')
const { serialize } = require('@jscad/stl-serializer')
const M = require('./puffpal-v4.js')
function w (name, g) {
  const s = serialize({ binary: true }, [g])
  fs.writeFileSync('/tmp/' + name, Buffer.concat(s.map(x => Buffer.isBuffer(x) ? x : Buffer.from(x))))
}
M.buildBody().forEach((g, i) => w('body-shell-' + String(i).padStart(2, '0') + '.stl', g))
M.buildMouthpiece().forEach((g, i) => w('mouth-local-' + i + '.stl', g))
console.log('body shells:', M.buildBody().length, 'mouth parts:', M.buildMouthpiece().length)
