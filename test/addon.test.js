const assert = require('assert').strict;

const robot = require('../index');

function test(name, fn) {
    try {
        fn();
        process.stdout.write(`ok - ${name}\n`);
    } catch (err) {
        process.stderr.write(`not ok - ${name}\n`);
        process.stderr.write(`${err && err.stack ? err.stack : err}\n`);
        process.exitCode = 1;
    }
}

test('exports shape', () => {
    assert.equal(typeof robot.leftClick, 'function');
    assert.equal(typeof robot.leftDoubleClick, 'function');
    assert.equal(typeof robot.rightClick, 'function');
    assert.equal(typeof robot.getMousePos, 'function');
    assert.equal(typeof robot.moveMouse, 'function');
    assert.equal(typeof robot.keyDown, 'function');
    assert.equal(typeof robot.keyUp, 'function');
    assert.equal(typeof robot.keyTap, 'function');
});

test('getMousePos returns numbers', () => {
    if (process.platform === 'linux' && !process.env.DISPLAY) {
        assert.throws(() => robot.getMousePos());
        return;
    }

    const pos = robot.getMousePos();
    assert.ok(pos && typeof pos === 'object');
    assert.equal(typeof pos.x, 'number');
    assert.equal(typeof pos.y, 'number');
});

test('moveMouse validates args (no side effects)', () => {
    assert.throws(() => robot.moveMouse(), /Usage:/);
    assert.throws(() => robot.moveMouse({ x: 1 }), /x and y/);
    assert.throws(() => robot.moveMouse('a', 'b'), /Usage:/);
});

test('click validates args (no side effects)', () => {
    assert.throws(() => robot.leftClick({ x: 1 }), /x and y/);
    assert.throws(() => robot.leftDoubleClick({ x: 1 }), /x and y/);
    assert.throws(() => robot.rightClick('a', 'b'), /Usage:/);
});

test('keyboard validates args (no side effects)', () => {
    assert.throws(() => robot.keyDown(), /Key is required/);
    assert.throws(() => robot.keyUp(), /Key is required/);
    assert.throws(() => robot.keyTap(), /Key is required/);
    assert.throws(() => robot.keyDown('a', {}), /Usage: keyDown\(key\)/);
    assert.throws(() => robot.keyUp('a', {}), /Usage: keyUp\(key\)/);
    assert.throws(() => robot.keyTap('a', 'bad'), /Modifiers must be an object/);
    assert.throws(() => robot.keyTap('unknown_key_name'), /Unsupported key name/);
});
