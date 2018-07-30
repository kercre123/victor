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

def server = Artifactory.server 'artifactory-dev'

stage('Parallel Build') {
    parallel docker: {
        node('victor-slaves') {
            stage('Build Docker Image') {
                checkout([
                    $class: 'GitSCM',
                    branches: scm.branches,
                    doGenerateSubmoduleConfigurations: scm.doGenerateSubmoduleConfigurations,
                    extensions: scm.extensions,
                    userRemoteConfigs: scm.userRemoteConfigs
                ])
                docker.build('victor-build-img:latest')
            }
            stage('Build Victor Engine') {
                def victorBuildImage = docker.image('victor-build-img:latest')
                victorBuildImage.inside("-v /etc/passwd:/etc/passwd -v ${env.HOME}/.ssh:${env.HOME}/.ssh") {
                    stage('Update submodules') {
                        sh 'git submodule update --init --recursive'
                        sh 'git submodule update --recursive'
                    }
                    stage('json lint') {
                        sh './lib/util/tools/build/jsonLint/jsonLint.sh resources'
                    }
                    withEnv(["HOME=${env.WORKSPACE}"]) {
                        stage('Build Engine') {
                            sh './project/victor/build-victor.sh -c Release -O2 -j8'
                        }
                    }
                }
            }
            withEnv(['PLATFORM=vicos', 'CONFIGURATION=release']) {
                stage("Build Vicos ${CONFIGURATION}") {
                    sh "./project/victor/scripts/victor_build_${CONFIGURATION}.sh -p ${PLATFORM}"
                }
            }
            stage('Zip Deployables') {
                sh './project/buildServer/steps/zipDeployables.sh'
            }
            stage('Pushing VicOS artifacts to artifactory') {
                def vicosFileSpec = """{
                                          "files": [
                                            {
                                              "pattern": "_build/vicos/Release/CMakeCache.txt",
                                              "target": "victor-engine/${env.BRANCH_NAME}/"
                                            },
                                            {
                                              "pattern": "_build/vicos/Release/CMakeFiles/*",
                                              "target": "victor-engine/${env.BRANCH_NAME}/"
                                            },
                                            {
                                                "pattern": "_build/vicos/Release/bin/*",
                                                "target": "victor-engine/${env.BRANCH_NAME}/"
                                            },
                                            {
                                                "pattern": "_build/vicos/Release/bin/*.full",
                                                "target": "victor-engine/${env.BRANCH_NAME}/vicos/release/"
                                            },
                                            {
                                                "pattern": "_build/vicos/Release/lib/*.so.full",
                                                "target": "victor-engine/${env.BRANCH_NAME}/vicos/release/"
                                            },
                                            {
                                                "pattern": "_build/deployables*.tar.gz",
                                                "target": "victor-engine/${env.BRANCH_NAME}/"
                                            }
                                          ]
                                        }"""
                server.upload(vicosFileSpec)
            }
        }
    },
    macosx: {
        def macBuildAgentsList = getListOfOnlineNodesForLabel('mac-slaves')
        if (!macBuildAgentsList.isEmpty()) {
            node('mac-slaves') {
                withEnv(['PLATFORM=mac', 'CONFIGURATION=release']) {
                    stage("Build MacOS ${CONFIGURATION}") {
                        checkout scm
                        sh 'git submodule update --init --recursive'
                        sh 'git submodule update --recursive'
                        sh "./project/victor/scripts/victor_build_${CONFIGURATION}.sh -p ${PLATFORM}"
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
                }
            }
        }
    }
}

node('victor-slaves') {
    stage('Cleaning workspace') {
        cleanWs()
        node('mac-slaves') {
            cleanWs()
        }
    }
}