#!/usr/bin/env groovy
import java.util.UUID
import hudson.model.*
import jenkins.model.*
import hudson.slaves.*
import hudson.plugins.sshslaves.verifiers.*
import groovy.json.JsonOutput
import groovy.json.JsonSlurperClassic 

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

primaryStageName = ''
def vSphereServer = 'ankicore'

slackNotificationChannel = "#build-eng-pr-notices"

def notifySlack(text, channel, attachments) {
    def slackURL = "https://anki.slack.com/services/hooks/jenkins-ci?token="
    def jenkinsIcon = 'https://jenkins.io/images/jenkins-x-logo.png'

    def payload = JsonOutput.toJson([text: text,
        channel: channel,
        username: "Jenkins-Buildbot",
        icon_url: jenkinsIcon,
        attachments: [attachments]
    ])
    withCredentials([string(credentialsId: 'jenkins-professional', variable: 'TOKEN')]) {
        sh "curl -X POST --data-urlencode \'payload=${payload}\' ${slackURL}${TOKEN}"
    }
}

def getCommitDetails(project, commit) {
    withCredentials([string(credentialsId: 'github-jenkins-pat', variable: 'TOKEN')]) {
        def payload = sh (
            script: "curl --user \"nakkapeddi:${TOKEN}\" https://api.github.com/repos/anki/${project}/commits/${commit}",
            returnStdout: true
        ).trim()
        return payload
    }
}

@NonCPS
def jsonParsePayload(def json) {
    new groovy.json.JsonSlurperClassic().parseText(json)
}

def notifyBuildStatus(status) {
    def jobName = "${env.JOB_NAME}"
    jobName = jobName.getAt(0..(jobName.indexOf('/') - 1))
    def pullRequestURL = "https://github.com/anki/${jobName}/pull/${env.CHANGE_ID}"
    def commitSHA = sh(returnStdout: true, script: "git rev-list --no-walk HEAD^")
    def commitInfo = getCommitDetails(jobName, commitSHA)
    def commitObj = jsonParsePayload(commitInfo)
    def slackColorMapping = [
        Success: "good",
        Failure: "danger",
        Aborted: "warning"
    ]

    notifySlack("", slackNotificationChannel, 
        [
            title: "${jobName} ${primaryStageName} ${env.CHANGE_ID}, build #${env.BUILD_NUMBER}",
            title_link: "${env.BUILD_URL}",
            color: slackColorMapping[status],
            text: "${status}\nAuthor: ${commitObj.commit.author.name} <${commitObj.commit.author.email}>",
            mrkdwn_in: ["fields"],
            fields: [
                [
                    title: "View on Github",
                    value: "${pullRequestURL}",
                    short: true
                ],
                [
                    title: "Commit Msg",
                    value: "${commitObj.commit.message}",
                    short: false
                ],
                [
                    title: "Stats (LoC)",
                    value: ":heavy_plus_sign: ${commitObj.stats.additions} / :heavy_minus_sign: ${commitObj.stats.deletions}",
                    short: false
                ]
            ]
        ]
    )
    
}


class EphemeralAgent {
    private String IPAddress;
    private String MachineName;

    EphemeralAgent() {
        this.MachineName = "jenkins-" + UUID.randomUUID().toString()
    }

    String getIPAddress() {
        return this.IPAddress;
    }

    String getMachineName() {
        return this.MachineName;
    }

    void setIPAddress(String ip) {
        this.IPAddress = ip
    }

    void Attach() {
        SshHostKeyVerificationStrategy hostKeyVerificationStrategy = new NonVerifyingKeyVerificationStrategy()
        ComputerLauncher launcher = new hudson.plugins.sshslaves.SSHLauncher(
            this.IPAddress, // Host
            22, // Port
            "92b3994b-c270-4088-acc9-d85d6a2c7f50", // Credentials
            (String)null, // JVM Options
            (String)null, // JavaPath
            (String)null, // Prefix Start Slave Command
            (String)null, // Suffix Start Slave Command
            (Integer)null, // Connection Timeout in Seconds
            (Integer)null, // Maximum Number of Retries
            (Integer)null, // The number of seconds to wait between retries
            hostKeyVerificationStrategy // Host Key Verification Strategy
        )


        Slave agent = new DumbSlave(
                this.MachineName,
                "/home/build/jenkins",
                launcher)
        agent.nodeDescription = "dynamic build agent"
        agent.numExecutors = 1
        agent.labelString = "reserved"
        agent.mode = Node.Mode.EXCLUSIVE
        agent.retentionStrategy = new RetentionStrategy.Always()

        Jenkins.instance.addNode(agent)
    }

    void Detach() {
        for (s in hudson.model.Hudson.instance.slaves) {
            if (s.name == this.MachineName) {
                s.getComputer().doDoDelete()
            }
        }
    }

}

if (env.CHANGE_ID) {
    primaryStageName = 'Pull Request'
} else {
    primaryStageName = 'Master'
}

stage("${primaryStageName} Build") {
    /*
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
    }*/
    agent = new EphemeralAgent()
    node('master') {
        stage('Spin up ephemeral VM') {
            uuid = agent.getMachineName()
            vSphere buildStep: [$class: 'Clone', clone: uuid, cluster: 'sjc-vm-cluster',
                customizationSpec: '', datastore: 'sjc-vm-04-localssd', folder: 'sjc/build',
                linkedClone: true, powerOn: true, resourcePool: 'vic-os',
                sourceName: 'photonos-test', timeoutInSeconds: 60], serverName: vSphereServer

            try {
                def buildAgentIP = vSphere buildStep: [$class: 'ExposeGuestInfo', envVariablePrefix: 'VSPHERE', vm: uuid, waitForIp4: true], serverName: vSphereServer
                agent.setIPAddress(buildAgentIP)
            } catch (e) {
                throw e
            }
        }
        stage('Attach ephemeral build agent VM to Jenkins') {
            agent.Attach()
        }
    }
    node(uuid) {
        try {
            withDockerEnv {
                buildPRStepsVicOS type: buildConfig.SHIPPING.getBuildType()
                stage('DAS Unit Tests') {
                    withEnv(["CXX=clang++", "LDFLAGS=-lpthread -luuid -lcurl -stdlib=libc++ -v"]) {
                        sh "make -C ./lib/das-client/unix run-unit-tests"
                        sh "make -f Makefile_sqs -C ./lib/das-client/unix run-unit-tests"
                    }
                }
                notifyBuildStatus('Success')
                //deployArtifacts type: buildConfig.SHIPPING.getArtifactType(), artifactoryServer: server
            }
        } catch (hudson.AbortException ae) {
            notifyBuildStatus('Aborted')
            throw ae
        } catch (exc) {
            notifyBuildStatus('Failure')
            throw exc
        } finally {
            stage('Cleaning slave workspace') {
                cleanWs()
            }
            node('master') {
                stage('Cleaning master workspace') {
                    def workspace = pwd()
                    dir("${workspace}@script") {
                        deleteDir()
                    }
                }
                stage('Destroy ephemeral VM') {
                    vSphere buildStep: [$class: 'Delete', failOnNoExist: true, vm: uuid], serverName: vSphereServer
                }
                stage('Detach ephemeral build agent from Jenkins') {
                    agent.Detach()
                }
            }
        }
    }
}

/*
node('victor-slaves') {
    stage('Cleaning workspace') {
        cleanWs()
    }
}
*/