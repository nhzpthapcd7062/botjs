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

    // Use annotated tag so it can be pushed by --follow-tags and is more release-friendly.
    execFileSync('git', ['tag', '-a', tag, '-m', tag], { stdio: 'inherit' });
    console.log(`Created tag: ${tag}`);

    const remotes = sh('git', ['remote']);
    if (!remotes) {
        console.log('No git remote configured; skip pushing tag.');
        process.exit(0);
    }

    const hasOrigin = remotes.split(/\s+/).includes('origin');
    const remoteName = hasOrigin ? 'origin' : remotes.split(/\s+/)[0];
    execFileSync('git', ['push', remoteName, tag], { stdio: 'inherit' });
    console.log(`Pushed tag to ${remoteName}: ${tag}`);
} catch (err) {
    console.error(err && err.message ? err.message : err);
    process.exit(1);
}
