#!/usr/bin/env node
'use strict';

const child_process = require('child_process');
const fs = require('fs');
const process = require('process');

const execSync = child_process.execSync;
const execSyncTrim = (...args) => execSync(...args).toString().trim();
const execSyncTrimDir = (dir, cmd) => execSync('(cd ' + dir + '; ' + cmd + ')').toString().trim();

const filename = 'godeps.json'
const deps = JSON.parse(fs.readFileSync(filename, 'utf8'));
if (!Array.isArray(deps)) {
  console.error("Dependencies in godeps.json should be an array; parse error");
  process.exit(1);
}
const gopath = execSyncTrim('go env GOPATH');
console.log('Using GOPATH=' + gopath);

// build command list
const commands = {
  list: listDeps,
  add: addDep,
  remove: removeDep,
  update: updateDeps,
  execute: syncDeps
}

const args = process.argv.slice(2);
runCommand(args);

// function to traverse command tree and send user input to command-specific function
function runCommand(args) {
  if (args.length === 0) {
    // list out the commands we know about
    console.log('valid commands: ' + Object.keys(commands).join(' '));
    return;
  }
  else {
    // do we know about the next tree branch?
    if (commands.hasOwnProperty(args[0])) {
      commands[args[0]](args.slice(1));
    }
    else {
      console.log('unknown command: ' + args[0]);
      console.log('valid commands: ' + Object.keys(commands).join(' '));
    }
  }
}

// command functions
function listDeps() {
  console.log('Current dependencies:');
  deps.forEach(dep => {
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
  if (deps.find(dep => dep.name === args[0])) {
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

  let commit = args[1];
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
  let branch = args[2];
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

  const newDep = { name: args[0], commit };
  let logStr = 'added dependency ' + newDep.name + ' @ ' + newDep.commit;
  if (branch) {
    newDep.branch = branch;
    logStr += ' (tracking branch ' + newDep.branch + ')';
  }
  deps.push(newDep);
  console.log(logStr);
  save();
}


function removeDep(args) {
  if (args.length === 0) {
    console.log('usage: remove <package-name>');
    return;
  }
  const idx = deps.findIndex(dep => dep.name === args[0]);
  if (idx < 0) {
    console.error('could not find ' + args[0] + ' in existing deps');
    return;
  }
  deps.splice(idx, 1);
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
  const updateDep = dep => {
    const status = str => console.log(dep.name + ': ' + str);
    if (!dep.branch) {
      status('skipping, no tracking branch');
      return false;
    }
    const depdir = getDir(dep.name);
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
    if (deps.length > 0) {
      // run updateDep on all deps and see if any resulted in changes
      changed = deps.map(updateDep).includes(true);
    } else {
      console.log('no deps to update');
    }
  }
  else {
    const dep = deps.find(dep => dep.name === args[0]);
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


function syncDeps() {
  const changeResults = deps.map(dep => {
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
      execSyncTrimDir(depdir, 'git checkout ' + dep.commit + ' 2> /dev/null');
      console.log(dep.name + ': checked out commit ' + dep.commit);
    }
    catch (e) {
      console.log(dep.name + ': error checking out commit ' + dep.commit);
      process.exit(1);
    }
    return true;
  });
  const numUnchanged = changeResults.filter(val => !val).length;
  if (numUnchanged > 0) {
    console.log('' + numUnchanged + ' dep(s) already up to date');
  }
}


function getDir(name) {
  return gopath + '/src/' + name;
}


function save() {
  fs.writeFileSync(filename, JSON.stringify(deps)) + '\n';
}
