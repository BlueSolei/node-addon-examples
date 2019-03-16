var addon = require("bindings")("hello");

console.log(addon.hello()); // 'hello'
console.log(addon.world()); // 'world'
console.log(`2 + 3 = ${addon.add(2, 3)}`);
