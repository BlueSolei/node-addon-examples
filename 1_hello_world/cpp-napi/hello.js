var addon = require('bindings')('hello');

console.log(addon.hello()); // 'hello'
console.log(addon.world()); // 'world'
