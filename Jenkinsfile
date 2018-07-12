#!/usr/bin/env groovy

@NonCPS
def getListOfOnlineNodesForLabel(label) {
  def nodes = []
  jenkins.model.Jenkins.instance.computers.each { c ->
    if (c.node.labelString.contains(label) && c.node.toComputer().isOnline()) {
      nodes.add(c.node.selfLabel.name)
    }
  }
  return nodes
}

node('victor-slaves') {
    withEnv(['CONFIGURATION=release']) {
        checkout([
            $class: 'GitSCM',
            branches: scm.branches,
            doGenerateSubmoduleConfigurations: scm.doGenerateSubmoduleConfigurations,
            extensions: scm.extensions,
            userRemoteConfigs: scm.userRemoteConfigs
        ])
        stage('Update submodules') {
            sh 'git submodule update --init --recursive'
            sh 'git submodule update --recursive'
        }
        stage('json lint') {
            sh './lib/util/tools/build/jsonLint/jsonLint.sh resources'
        }
        stage('Build') {
            sh './project/victor/build-victor.sh -c Release'
        }
        def macBuildAgentsList = getListOfOnlineNodesForLabel('mac-slaves')
        if (!macBuildAgentsList.isEmpty()) {
            echo "Running MacOS specific build steps..."
            withEnv(['PLATFORM=mac']) {
                stage("Build MacOS ${CONFIGURATION}") {
                    sh "./project/victor/scripts/victor_build_${CONFIGURATION}.sh -p ${PLATFORM}"
                    stash includes: '_build/macos/*', name: 'builtMacOSTargets'
                }
            }
            stage('Webots') {
                sh './project/build-scripts/webots/webotsTest.py'
            }
            stage('Unit Tests') {
                sh './project/buildServer/steps/unittestsUtil.sh'
                sh './project/buildServer/steps/unittestsCoretech.sh'
                sh './project/buildServer/steps/unittestsEngine.sh'
            }
        }
        withEnv(['PLATFORM=vicos']) {
            stage("Build Vicos ${CONFIGURATION}") {
                unstash 'builtMacOSTargets'
                sh "./project/victor/scripts/victor_build_${CONFIGURATION}.sh -p ${PLATFORM}"
            }
        }
    }
}