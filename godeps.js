#!/usr/bin/env node
'use strict';

const child_process = require('child_process');
const fs = require('fs');
const process = require('process');
const path = require('path');

const execSync = child_process.execSync;
const execSyncTrim = (...args) => execSync(...args).toString().trim();
const execSyncTrimDir = (dir, cmd) => execSync('(cd ' + dir + '; ' + cmd + ')').toString().trim();

const filename = 'godeps.json'
const deps = JSON.parse(fs.readFileSync(filename, 'utf8'));
if (!(deps != null && typeof deps === 'object')) {
  console.error("Dependencies in godeps.json should be an object; parse error");
  process.exit(1);
}
const gopath = execSyncTrim('go env GOPATH');
console.log('Using GOPATH=' + gopath);

// build command list
const commands = {
  list: [listDeps, 'list all currently versioned dependencies'],
  add: [addDep, 'add a dependency with the given name'],
  remove: [removeDep, 'remove the given dependency from versioning'],
  update: [updateDeps, 'update the given dep (or all deps, with --all flag) to the latest of its tracked branch'],
  execute: [recordDeps, 'record every remote dependency in $GOPATH as a dependency, check out required versions'],
}

const args = process.argv.slice(2);
runCommand(args);

function listCommands() {
  console.log('valid commands:\n' + Object.keys(commands).map(val => val + ' - ' + commands[val][1]).join('\n'));
}

// function to traverse command tree and send user input to command-specific function
function runCommand(args) {
  if (args.length === 0) {
    // list out the commands we know about
    listCommands();
    return;
  }
  else {
    // do we know about the next tree branch?
    if (commands.hasOwnProperty(args[0])) {
      commands[args[0]][0](args.slice(1));
    }
    else {
      console.log('unknown command: ' + args[0]);
      listCommands();
    }
  }
}

// Returns keyed list of deps (i.e. "github.com/anki/whatever": {...data}) as an array
// and adds the name as a field in the returned objects
function getAllDeps() {
  return Object.keys(deps).map(key => Object.assign({ name: key }, deps[key]));
}


// command functions
function listDeps() {
  console.log('Current dependencies:');
  getAllDeps().forEach(dep => {
    var logStr = dep.name + ': ' + dep.commit;
    if (dep.branch) {
      logStr += ' (branch: ' + dep.branch + ')';
    }
    console.log(logStr);
  });
}

function addDep(args) {
  if (args.length === 0) {
    console.log('usage: add <package-name> [optional-commit] [optional-tracking-branch]');
    console.log('- package name should have url form, i.e. github.com/anki/sai-go-util');
    console.log('- if [optional-commit] not specified, defaults to current HEAD');
    console.log('- if [optional-tracking-branch] omitted, defaults to current branch (if not detached)');
    return;
  }
  if (deps[args[0]]) {
    console.log('dep already exists: ' + args[0]);
    return;
  }
  const depdir = getDir(args[0]);
  try {
    const pathstats = fs.lstatSync(depdir);
    if (!pathstats.isDirectory()) {
      console.error('error: ' + depdir + ' is not a directory');
      return;
    }
  } catch (e) {
    console.error('error: could not find directory ' + depdir);
    return;
  }
  const toplevel = execSyncTrimDir(depdir, 'git rev-parse --show-toplevel');
  if (toplevel != depdir) {
    console.error('not a top level git repo: ' + depdir);
    return;
  }

  addDepInternal(args[0], args[1], args[2]);
  save();
}


function addDepInternal(name, commit, branch) {
  const depdir = getDir(name);

  if (!commit) {
    commit = execSyncTrimDir(depdir, 'git rev-parse HEAD');
  }
  else {
    // verify given commit exists
    commit = execSyncTrimDir(depdir, 'git rev-parse ' + commit);
    try {
      execSyncTrimDir(depdir, 'git cat-file -e ' + commit);
    } catch (e) {
      console.error('could not verify existence of commit: ' + commit);
      return;
    }
  }

  if (!branch) {
    try {
      branch = execSyncTrimDir(depdir, 'git symbolic-ref -q --short HEAD');
    } catch (e) {
      // detached HEAD = no branch, which is fine
    }
  }
  else {
    // verify given branch exists
    const output = execSyncTrimDir(depdir, 'git branch --list ' + branch);
    if (!output) {
      console.error('could not verify existence of branch: ' + branch);
    }
  }

  const newDep = { commit };
  let logStr = 'added dependency ' + name + ' @ ' + newDep.commit;
  if (branch) {
    newDep.branch = branch;
    logStr += ' (tracking branch ' + newDep.branch + ')';
  }
  deps[name] = newDep;
  console.log(logStr);
}


function removeDep(args) {
  if (args.length === 0) {
    console.log('usage: remove <package-name>');
    return;
  }
  if (!deps[args[0]]) {
    console.error('could not find ' + args[0] + ' in existing deps');
    return;
  }
  delete deps[args[0]]
  console.log('removed dep ' + args[0]);
  save();
}


function updateDeps(args) {
  if (args.length === 0) {
    console.log('usage: update (--all | <dep-name>)');
    console.log('- using --all will try to update all deps that have tracking branches');
    console.log('- otherwise, give a specific dep name to update');
    console.log('- this command will update the json but will NOT check out newest (use \'execute\' for that)');
    return;
  }

  // define function to update each dep; returns if anything was changed
  const updateDep = (name, dep) => {
    const status = str => console.log(name + ': ' + str);
    if (!dep.branch) {
      status('skipping, no tracking branch');
      return false;
    }
    const depdir = getDir(name);
    execSyncTrimDir(depdir, 'git fetch');
    try {
      const latestCommit = execSyncTrimDir(depdir, 'git rev-parse origin/' + dep.branch);
      if (latestCommit != dep.commit) {
        status('updating branch ' + dep.branch + ' to latest: ' + latestCommit);
        dep.commit = latestCommit;
        return true;
      }
      else {
        status('branch ' + dep.branch + ' is up to date');
      }
    } catch (e) {
      status('error getting latest for branch ' + dep.branch + ', recommend investigating');
    }
    return false;
  };

  let changed = false;
  if (args[0] === '--all') {
    if (Object.keys(deps).length > 0) {
      // run updateDep on all deps and see if any resulted in changes
      changed = Object.keys(deps).map(val => updateDep(val, deps[val])).includes(true);
    } else {
      console.log('no deps to update');
    }
  }
  else {
    const dep = deps[args[0]];
    if (!dep) {
      console.error('could not find dep ' + args[0] + ' to update');
      return;
    }
    changed = updateDep(dep);
  }
  if (changed) {
    console.log('wrote updates to ' + filename + ', run \'execute\' command to check out updated commits');
    save();
  }
  else {
    console.log('no changes from update');
  }
}


function syncDep(dep) {
  const depdir = getDir(dep.name);
  const currentHead = execSyncTrimDir(depdir, 'git rev-parse HEAD');
  if (currentHead === dep.commit) {
    // current checked-out commit is what we want
    return false;
  }

  // need to check something out - do we know about the revision?
  const verifyFunc = commit => {
    try {
      execSyncTrimDir(depdir, 'git rev-parse --verify --quiet ' + commit + '^{commit}');
      return true;
    } catch (e) {
      return false;
    }
  };
  if (!verifyFunc(dep.commit)) {
    // we don't know about this commit; fetch and try again
    execSyncTrimDir(depdir, 'git fetch');
    if (!verifyFunc(dep.commit)) {
      console.error('ERROR: could not find commit ' + dep.commit + ' in dep ' + dep.name);
      process.exit(1);
    }
  }
  try {
    execSyncTrimDir(depdir, 'git reset --hard ' + dep.commit);
    console.log(dep.name + ': checked out commit ' + dep.commit);
  }
  catch (e) {
    console.log(dep.name + ': error checking out commit ' + dep.commit);
    process.exit(1);
  }
  return true;
}


function recordDeps(args) {
  if (args.length < 1) {
    console.log('usage: execute <generated lst dir>');
    return;
  }
  const tryVerifyFunc = func => {
    try {
      return func();
    } catch (e) {
      return false;
    }
  };
  const isdir = tryVerifyFunc(() => fs.lstatSync(args[0]).isDirectory());
  if (!isdir) {
    console.error('could not open directory: ' + args[0]);
    return;
  }

  // get list of .godir.lst files in generated dir
  const godirFiles = fs.readdirSync(args[0]).filter(val => val.endsWith('godir.lst')).map(val => path.join(args[0], val));
  if (godirFiles.length === 0) {
    console.error('could not find any godir.lst files in ' + args[0]);
    return;
  }

  const stdLibPackages = execSyncTrim('go list std').split('\n');

  const allDeps = godirFiles
    // get contents of godir files
    .map(filename => fs.readFileSync(filename, 'utf8'))
    // get dependencies of each godir
    .map(godir => execSyncTrim('go list -f \'{{ join .Deps "\\n" }}\' ' + godir))
    // split newline-separated deps into array, remove std lib packages
    .map(deps => deps.split('\n').filter(dep => !stdLibPackages.includes(dep)))
    // merge all deps into one array, removing duplicates
    .reduce((agg, deps) => agg.concat(deps.filter(dep => !agg.includes(dep))), [])
  ;

  // starting from $GOPATH/src, recurse over child dirs to find dependencies
  let names = [''];
  let changed = false;
  const syncResults = [];
  while (names.length > 0) {
    const subdirs = []
    names.forEach(name => {
      // if this dir is already in godeps.json, make sure it's up to date and leave
      if (deps[name]) {
        syncResults.push(syncDep(Object.assign({ name }, deps[name])));
        return;
      }
      // if this is not the top level of a git repo, maybe its children are?
      const dir = getDir(name);
      const toplevel = execSyncTrimDir(dir, 'git rev-parse --show-toplevel');
      if (toplevel != dir) {
        // add all child directories to processing list
        // first get all subdirs in this directory...
        const contents = fs.readdirSync(dir).filter(filename => fs.lstatSync(path.join(dir, filename)).isDirectory());
        // now add them to the processing list, making sure to build on the existing dir name
        subdirs.push(...contents.map(str => path.join(name, str)));
        return;
      }

      // if this remote repo is not in our dependencies, ignore it
      if (!allDeps.find(dep => dep.startsWith(name))) {
        return;
      }

      // it passed the tests, add it as a dependency
      addDepInternal(name);
      changed = true;
    });
    names = subdirs;
  }
  if (changed) {
    save();
    console.log('Updates written to godeps.json; now exiting with failure');
    console.log('(builds fail after update to make sure developers commit changes to godeps.json');
    console.log(' BEFORE pushing for pull request/master builds)');
    process.exit(1);
  }

  // it should now be the case that all external dependencies are tracked deps or exist on hard drive
  const trackedDeps = Object.keys(deps);
  const untrackedDeps = allDeps.filter(dep => !trackedDeps.find(val => dep.startsWith(val)));
  const isDepMissing = dep => {
    try {
      // missing if it contains no go files...
      return fs.readdirSync(getDir(dep))
        .filter(file => file.endsWith('.go'))
        .length === 0;
    } catch (e) {
      // ...or if trying to read its contents throws an exception
      return true;
    }
  }
  const missingDeps = untrackedDeps.filter(isDepMissing);
  if (missingDeps.length > 0) {
    missingDeps.forEach(dep => console.log('error: ' + dep + ' is a dependency, but not versioned and not present on disk'));
    process.exit(1);
  }

  const numUnchanged = syncResults.filter(val => !val).length;
  if (numUnchanged > 0) {
    console.log('' + numUnchanged + ' dep(s) already up to date');
  }
}


function getDir(name) {
  return gopath + '/src/' + name;
}


function save() {
  fs.writeFileSync(filename, JSON.stringify(deps, null, 2) + '\n');
}
