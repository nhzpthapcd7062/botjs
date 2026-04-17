const { execFileSync } = require('child_process');

function sh(cmd, args) {
    return execFileSync(cmd, args, { stdio: ['ignore', 'pipe', 'pipe'] })
        .toString('utf8')
        .trim();
}

const version = process.env.npm_package_version;
if (!version) {
    console.error('npm_package_version is missing; run via npm scripts');
    process.exit(1);
}

const tag = `v${version}`;

try {
    const tags = sh('git', ['tag', '--list', tag]);
    if (tags === tag) {
        console.log(`Tag already exists: ${tag}`);
        process.exit(0);
    }

    // Require clean working tree to avoid tagging dirty state.
    const status = sh('git', ['status', '--porcelain']);
    if (status) {
        console.error('Working tree is not clean. Commit/stash changes before tagging.');
        process.exit(1);
    }

    execFileSync('git', ['tag', tag], { stdio: 'inherit' });
    console.log(`Created tag: ${tag}`);
    console.log('Next: git push --follow-tags');
} catch (err) {
    console.error(err && err.message ? err.message : err);
    process.exit(1);
}
