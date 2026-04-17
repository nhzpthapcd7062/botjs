const path = require("path");

// node-gyp 默认输出到 build/Release/<target_name>.node
const addon = require(path.join(__dirname, "build", "Release", "botjs.node"));

module.exports = addon;
