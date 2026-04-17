const fs = require("fs");
const path = require("path");

// node-gyp 默认输出到 build/Release/<target_name>.node
function loadAddon() {
    const releaseDir = path.join(__dirname, "build", "Release");
    const expected = path.join(releaseDir, "crobot.node");

    if (fs.existsSync(expected)) {
        return require(expected);
    }

    // Fallback: load the first .node in build/Release
    if (fs.existsSync(releaseDir)) {
        const files = fs.readdirSync(releaseDir).filter((f) => f.endsWith(".node"));
        if (files.length > 0) {
            return require(path.join(releaseDir, files[0]));
        }
    }

    throw new Error(
        "Native addon not found. Run `npm run build` first (expected build/Release/crobot.node)."
    );
}

module.exports = loadAddon();
