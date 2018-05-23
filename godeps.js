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
const allGopaths = execSyncTrim('go env GOPATH').split(':');
const gopath = allGopaths[0];
const gopathSrc = gopath + '/src/';
console.log('Using GOPATH=' + gopath);

// build command list
const commands = {
  list: [listDeps, 'list all currently versioned dependencies'],
  update: [updateDeps, 'update the given dep (or all deps, with --all flag) to the latest of its tracked branch'],
  execute: [runRoutine, 'clone and verify all versioned deps, update godeps.json to reflect new/removed deps']
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


function updateDeps(args) {
  if (args.length === 0) {
    console.log('usage: update (--all | <dep-name>)');
    console.log('- using --all will try to update all deps that have tracking branches');
    console.log('- otherwise, give a specific dep name to update');
    console.log('- this command will update the json but will NOT check out newest (use \'execute\' for that)');
    return;
  }

  // define function to update each dep; returns if anything was changed
  const summaries = []
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
        const summary = execSyncTrimDir(depdir, 'git log --oneline --no-decorate ' + dep.commit + '..' + latestCommit);
        summaries.push(name + ':\n' + summary);
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
    changed = updateDep(args[0], dep);
  }
  if (changed) {
    console.log('wrote updates to ' + filename + ', run \'execute\' command to check out updated commits');
    save();
    console.log('\nchange summary:\n');
    console.log(summaries.join('\n\n'));
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


function runRoutine(args) {
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

  // step 1: clone all repos that are specified in the json but don't exist on disk
  const startDeps = getAllDeps();
  cloneDeps(startDeps);
  // update these repos to desired commit
  const syncResults = startDeps.map(dep => syncDep(dep));

  // get list of .godir.lst files in generated dir
  const godirFiles = fs.readdirSync(args[0]).filter(val => val.endsWith('godir.lst')).map(val => path.join(args[0], val));
  if (godirFiles.length === 0) {
    console.error('could not find any godir.lst files in ' + args[0]);
    return;
  }

  const stdLibPackages = execSyncTrim('go list std').split('\n')
    .concat(execSyncTrim('GOOS="android" go list std').split('\n'))
    .reduce((agg, pkg) => !agg.includes(pkg) ? agg + pkg : agg, []);

  const allGoDirs = godirFiles
    .map(filename => fs.readFileSync(filename, 'utf8'))
    .join(' ');

  // step 2: run 'go get' on all godir-specified folders - will pick up any new dependencies
  execSync('go get -d -v ' + allGoDirs, { stdio: 'inherit' });

  const extraIncludes = ["github.com/golang/protobuf/protoc-gen-go", "github.com/grpc-ecosystem/grpc-gateway/protoc-gen-grpc-gateway", "github.com/golang/glog"];
  // now that 'go get' has pulled everything we need, get full dependency list for packages
  const allDeps = [...new Set(execSyncTrim('go list -f \'{{ join .Deps "\\n" }}\' ' + allGoDirs).split('\n'))].concat(extraIncludes)
    .filter(dep => !stdLibPackages.includes(dep)).sort();

  // step 3: check that all remote dependencies are tracked by us and checked out at correct commit
  //
  // already cloned + verified tracked dependencies, so remove them from list
  let remainingDeps = allDeps.slice();
  Object.keys(deps).forEach(name => {
    remainingDeps = remainingDeps.filter(dep => !dep.startsWith(name));
  });

  const ourToplevel = execSyncTrim('git rev-parse --show-toplevel');
  let changed = false;

  // process 1 dep at a time, since we might be able to remove several deps at once when
  // multiple dependent packages live in the same repository
  while (remainingDeps.length > 0) {
    // is this dep part of a remote repo? see if its git toplevel is the same as our toplevel
    const dir = getDirAllPaths(remainingDeps[0]);
    const toplevel = execSyncTrimDir(dir, 'git rev-parse --show-toplevel');
    if (toplevel != ourToplevel) {
      // this is in a remote repo - add the package name implied by the toplevel name to our dependency list
      if (!toplevel.startsWith(gopathSrc)) {
        console.log('WTF, why doesn\'t ' + toplevel + ' start with ' + gopathSrc + ' ??');
        process.exit(1);
      }
      const remote = toplevel.substr(gopathSrc.length);
      if (deps[remote]) {
        console.log('WTF, how did we get here if already tracked: ' + remote);
        process.exit(1);
      }
      // remove all sub-dependencies contained in this repo
      remainingDeps = remainingDeps.filter(name => !name.startsWith(remote));

      addDepInternal(remote);
      changed = true;
    }
    else {
      // this is local, remove it and continue
      remainingDeps.splice(0, 1);
    }
  }

  // see if any of our tracked deps are no longer used and should be removed
  startDeps
    // find deps in our tracked list that don't contain any dependencies found by go
    .filter(dep => !allDeps.find(name => name.startsWith(dep.name)))
    // remove each of them
    .forEach(dep => {
      console.log('dependency ' + dep.name + ' is no longer used, removing');
      delete deps[dep.name];
      changed = true;
    }
  );

  if (changed) {
    save();
    console.log('Updates written to godeps.json; now exiting with failure');
    console.log('(builds fail after update to make sure developers commit changes to godeps.json');
    console.log(' BEFORE pushing for pull request/master builds)');
    process.exit(1);
  }

  const numUnchanged = syncResults.filter(val => !val).length;
  if (numUnchanged > 0) {
    console.log('' + numUnchanged + ' dep(s) already up to date');
  }
}


function cloneDeps(deplist) {
  deplist.forEach(dep => {
    const depdir = getDir(dep.name);
    if (fs.existsSync(depdir)) {
      // path where this dependency goes exists, don't worry about it
      return;
    }

    // path doesn't exist, need to grab this repo
    let cloneCmd = '';
    try {
      cloneCmd = getCloneCommand(dep.name);
    }
    catch (e) {
      console.log('Clone error: ' + e);
      process.exit(1);
    }
    console.log("Cloning " + dep.name + ": " + cloneCmd);
    execSync('mkdir -p ' + depdir);
    execSync('(cd ' + depdir + '; ' + cloneCmd + ' ' + depdir + ' )', { stdio: 'inherit' });
    execSync('(cd ' + depdir + '; ' + 'git checkout ' + dep.commit + ' )', { stdio: 'pipe' });
    execSync('(cd ' + depdir + '; ' + 'git submodule update --init --recursive )', { stdio: 'inherit' });
  });
}


function getCloneCommand(name) {
  // for github, we know the syntax
  if (name.startsWith('github.com/')) {
    return 'git clone git@github.com:' + name.substr('github.com/'.length);
  }

  // otherwise, get meta tags
  const curlOut = execSyncTrim('curl -s ' + name + '?go-get=1 | grep go-import');
  const startIdx = curlOut.indexOf(name);
  if (startIdx < 0) {
    throw 'Could not find dependency go-import tag in: ' + curlOut
  }
  const strEnd = curlOut.substr(startIdx);
  const endIdx = strEnd.indexOf('"');
  if (endIdx < 0) {
    throw 'Could not find closing quote in: ' + strEnd;
  }

  const [_, vcs, url] = strEnd.substring(0, endIdx).split(' ');
  if (vcs != 'git') {
    throw 'Script doesn\'t yet know how to clone from non-git VCS: ' + vcs;
  }
  return 'git clone ' + url;
}


function getDir(name) {
  return gopathSrc + name;
}


function getDirAllPaths(name) {
  const path = allGopaths.reduce((val, path) => val ? val :
    (fs.existsSync(path + '/src/' + name) ? path + '/src/' + name : undefined), undefined);
  return path || getDir(name)
}


function save() {
  fs.writeFileSync(filename, JSON.stringify(deps, null, 2) + '\n');
}
