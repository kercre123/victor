#!/usr/bin/env groovy
if (env.CHANGE_ID) {
    properties([pipelineTriggers([pollSCM('H/10 * * * *')])])
} else {
    properties([pipelineTriggers([pollSCM('H/50 * * * *')])]) 
}


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

enum buildConfig {
    SHIPPING, DEBUG, RELEASE, MASTER

    public String getBuildType() {
        return name().toLowerCase()
    }

    public String getArtifactType() {
        def firstChar = name()[0]
        def restOfStringLowerCased = this.getBuildType().substring(1)
        return firstChar + restOfStringLowerCased
    }
}

def server = Artifactory.server 'artifactory-dev'
library 'victor-helpers@master'

def primaryStageName = ''

if (env.CHANGE_ID) {
    primaryStageName = 'Pull Request'
} else {
    primaryStageName = 'Master'
}

stage("${primaryStageName} Build") {
    parallel debug: {
        node('victor-slaves') {
            withDockerEnv {
                buildPRStepsVicOS type: buildConfig.DEBUG.getArtifactType()
                sh 'mv _build/vicos/Debug _build/vicos/Release'
                deployArtifacts type: buildConfig.DEBUG.getArtifactType(), artifactoryServer: server
            }
        }
    },
    shipping: {
        node('victor-slaves') {
            withDockerEnv {
                buildPRStepsVicOS type: buildConfig.SHIPPING.getBuildType()
                deployArtifacts type: buildConfig.SHIPPING.getArtifactType(), artifactoryServer: server
            }
        }
    },
    release: {
        node('victor-slaves') {
            withDockerEnv {
                buildPRStepsVicOS type: buildConfig.RELEASE.getBuildType()
                deployArtifacts type: buildConfig.RELEASE.getArtifactType(), artifactoryServer: server
            }
        }
    },
    macosx: {
        def macBuildAgentsList = getListOfOnlineNodesForLabel('mac-slaves')
        if (!macBuildAgentsList.isEmpty()) {
            node('mac-slaves') {
                withEnv(['PLATFORM=mac', 'CONFIGURATION=release']) {
                    stage('Git Checkout') {
                        checkout scm
                        sh 'git submodule update --init --recursive'
                        sh 'git submodule update --recursive'
                    }
                    if (env.CHANGE_ID) {
                        buildPRStepsMacOS type: 'Debug'
                    } else {
                        stage("Build MacOS ${CONFIGURATION}") {
                            sh "./project/victor/scripts/victor_build_${CONFIGURATION}.sh -p ${PLATFORM}"
                        }
                    }
                }
                stage('Webots') {
                    sh './project/build-scripts/webots/webotsTest.py'
                }
                stage('Unit Tests') {
                    withEnv(['TESTCONFIG=Debug']){
                        sh "./project/buildServer/steps/unittestsUtil.sh -c ${TESTCONFIG}"
                        sh "./project/buildServer/steps/unittestsCoretech.sh -c ${TESTCONFIG}"
                        sh "./project/buildServer/steps/unittestsEngine.sh -c ${TESTCONFIG}"
                    }
                }
                stage('Pushing MacOS artifacts to Artifactory') {
                    def macosFileSpec = """{
                    "files": [
                        {
                        "pattern": "_build/mac/Debug/webots_out*.tar.gz",
                        "target": "victor-engine/${env.BRANCH_NAME}/"
                        }
                    ]
                    }"""
                    server.upload(macosFileSpec)
                }
                stage('Cleaning MacOS workspace...') {
                    cleanWs()
                }
            }
        }
    }
}

node('victor-slaves') {
    stage('Cleaning workspace') {
        cleanWs()
    }
}