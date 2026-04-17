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
    assert.equal(typeof robot.rightClick, 'function');
    assert.equal(typeof robot.getMousePos, 'function');
    assert.equal(typeof robot.moveMouse, 'function');
});

test('getMousePos returns numbers', () => {
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
    assert.throws(() => robot.rightClick('a', 'b'), /Usage:/);
});
