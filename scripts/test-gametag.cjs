// Exercise the real prompt, persistence and native-command input boundary.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync('site/native-game-adapter.js', 'utf8');
const functions = source.slice(source.indexOf('  function validGametag('), source.indexOf('  function showOriginalStartup('));
const storage = new Map();
function client({preferred='Player', blocked=false}={}) {
  const elements = [];
  class Element {
    constructor(tag) {this.tag=tag;this.listeners={};elements.push(this);}
    setAttribute() {}
    append(...children) {this.children=children;}
    appendChild(child) {this.append(child);}
    focus() {this.focused=true;}
    remove() {this.removed=true;}
    addEventListener(type, fn) {this.listeners[type]=fn;}
  }
  const context = vm.createContext({TextEncoder, localStorage:{
    getItem(key) {if(blocked)throw Error('storage disabled');return storage.get(key);},
    setItem(key,value) {if(blocked)throw Error('storage disabled');storage.set(key,value);}
  }, document:{createElement:tag=>new Element(tag),body:new Element('body')}});
  vm.runInContext(`let gametag;const gametagStorageKey='cod2-wasm-gametag';${functions}`, context);
  const pending = context.requireGametag({preferences:{values:()=>({playerName:preferred})},setLoading(){}});
  return {pending,elements,valid:context.validGametag,submit(value) {
    const input=elements.find(e=>e.tag==='input');input.value=value;
    elements.find(e=>e.tag==='form').listeners.submit({preventDefault(){}});
  }};
}
(async()=>{
 const first=client();
 assert.ok(first.elements.find(e=>e.tag==='input').focused);
 for(const name of ['', 'Player','Unknown Soldier','x;quit','x" +connect evil','x+quit','x\nquit','x\\y','é'.repeat(16)]) {
  assert.equal(first.valid(name),'');first.submit(name);
  assert.equal(first.elements.find(e=>e.tag==='section').removed,undefined);
 }
 first.submit('  Gaston R  ');
 assert.equal(await first.pending,'Gaston R');
 assert.equal(storage.get('cod2-wasm-gametag'),'Gaston R');
 assert.equal(first.elements.find(e=>e.tag==='section').removed,true);
 const returning=client();assert.equal(await returning.pending,'Gaston R');
 assert.equal(returning.elements.some(e=>e.tag==='form'),false);
 storage.clear();
 const previous=client({preferred:'Jugador viejo'});assert.equal(await previous.pending,'Jugador viejo');
 const disabled=client({blocked:true});disabled.submit('Nuevo jugador');
 assert.equal(await disabled.pending,'Nuevo jugador');
 console.log('PASS: first-entry prompt, saved-name reload, legacy name, disabled storage and command/UTF-8 boundaries');
})().catch(error=>{console.error(error);process.exitCode=1;});
