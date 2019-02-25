#!/usr/bin/env groovy
import java.util.UUID
import hudson.model.*
import jenkins.model.*
import hudson.slaves.*
import hudson.plugins.sshslaves.verifiers.*
import groovy.json.JsonOutput
import groovy.json.JsonSlurperClassic
import org.jenkinsci.plugins.workflow.steps.FlowInterruptedException

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

@NonCPS
def checkIfAgentsAreBusy() {
    for (node in hudson.model.Hudson.instance.slaves) {
        if (node.getLabelString() == 'victor-shipping') {
            if (node.getComputer().countIdle() > 0) {
                return false
            }
            return true
        }
    }
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
    writeFile file: 'slackPayload.json', text: payload
    withCredentials([string(credentialsId: 'jenkins-professional', variable: 'TOKEN')]) {
        sh "curl -X POST -d @slackPayload.json ${slackURL}${TOKEN}"
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

def parseCommitDetails(jobName, commitSHA) {
    def commitInfo = getCommitDetails(jobName, commitSHA)
    def commitObj = jsonParsePayload(commitInfo)

    if (commitObj.message) {
        println commitObj.message
        def firstAncestorSHA = sh(returnStdout: true, script: "git rev-list --no-walk HEAD^")
        return parseCommitDetails(jobName, firstAncestorSHA)
    }

    return commitObj
}

@NonCPS
def jsonParsePayload(def json) {
    new groovy.json.JsonSlurperClassic().parseText(json)
}

def notifyBuildStatus(status) {
    def jobName = "${env.JOB_NAME}"
    jobName = jobName.getAt(0..(jobName.indexOf('/') - 1))
    def pullRequestURL = "https://github.com/anki/${jobName}/pull/${env.CHANGE_ID}"
    def latestCommitSHA = sh(returnStdout: true, script: "git rev-list --no-walk HEAD")
    if (!env.CHANGE_ID) {
        pullRequestURL = "https://github.com/anki/${jobName}/commit/${latestCommitSHA}"
    }
    def commitObj = parseCommitDetails(jobName, latestCommitSHA)

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
            "ankibuildssh", // Credentials
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
    agent = new EphemeralAgent()
    node('master') {
        stage('Spin up ephemeral VM') {
            try {
                uuid = agent.getMachineName()
                vSphere buildStep: [$class: 'Clone', clone: uuid, cluster: 'sjc-vm-cluster',
                    customizationSpec: '', datastore: 'sjc-vm-04-localssd', folder: 'sjc/build',
                    linkedClone: true, powerOn: false, resourcePool: 'vic-os',
                    sourceName: 'js-photon-os-template', timeoutInSeconds: 60], serverName: vSphereServer

                vSphere buildStep: [$class: 'Reconfigure', reconfigureSteps: [[$class: 'ReconfigureCpu',
                    coresPerSocket: '1', cpuCores: '2']], vm: uuid], serverName: vSphereServer // Max overcommit is 4:1 vCPU to pCPU

                vSphere buildStep: [$class: 'PowerOn', timeoutInSeconds: 60, vm: uuid], serverName: vSphereServer

                def buildAgentIP = vSphere buildStep: [$class: 'ExposeGuestInfo', envVariablePrefix: 'VSPHERE', vm: uuid, waitForIp4: true], serverName: vSphereServer
                agent.setIPAddress(buildAgentIP)
            } catch (Exception exc) {
                def jobName = "${env.JOB_NAME}"
                jobName = jobName.getAt(0..(jobName.indexOf('/') - 1))
                def reason = exc.getMessage()
                notifySlack("", slackNotificationChannel,
                    [
                        title: "${jobName} ${primaryStageName} ${env.CHANGE_ID}, build #${env.BUILD_NUMBER}",
                        title_link: "${env.BUILD_URL}",
                        color: "warning",
                        text: "${reason}",
                    ]
                )
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
                }
                currentBuild.rawBuild.result = Result.ABORTED
                throw new hudson.AbortException('vSphere Exception!')
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
                //deployArtifacts type: buildConfig.SHIPPING.getArtifactType(), artifactoryServer: server
            }
            notifyBuildStatus('Success')
        } catch (FlowInterruptedException ae) {
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