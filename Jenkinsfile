#!/usr/bin/env groovy
import java.util.UUID
import hudson.model.*
import jenkins.model.*
import hudson.slaves.*
import hudson.plugins.sshslaves.verifiers.*
import groovy.json.JsonOutput
import groovy.json.JsonSlurperClassic
import org.jenkinsci.plugins.workflow.steps.FlowInterruptedException

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

enabledBuildConfigs = [buildConfig.SHIPPING.getBuildType(), buildConfig.DEBUG.getBuildType()]
buildConfigMap = [:]

def server = Artifactory.server 'artifactory-dev'
library 'victor-helpers@master'
@Library('static-libs')
import com.anki.*

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

def notifyBuildStatus(status, config) {
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
            title: "${jobName} ${primaryStageName} ${env.CHANGE_ID} ${config}, build #${env.BUILD_NUMBER}",
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
    def commitStatusMsg = "PR #${env.CHANGE_ID} :: vicos ${config} :: Finished"
    def statusMap = [msg: commitStatusMsg, result: status.toUpperCase(), sha: commitObj.sha, context: "ci/jenkins/pr/${config}"]
    setGHBuildStatus(statusMap)
}

void setGHBuildStatus(Map statusFields) {
  step([
      $class: "GitHubCommitStatusSetter",
      reposSource: [$class: "ManuallyEnteredRepositorySource", url: "https://github.com/anki/victor"],
      commitShaSource: [$class: "ManuallyEnteredShaSource", sha: statusFields.sha],
      contextSource: [$class: "ManuallyEnteredCommitContextSource", context: statusFields.context],
      errorHandlers: [[$class: "ChangingBuildStatusErrorHandler", result: "UNSTABLE"]],
      statusResultSource: [ $class: "ConditionalStatusResultSource", results: [[$class: "AnyBuildResult", message: statusFields.msg, state: statusFields.result]] ]
  ]);
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
        agent.numExecutors = 2
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
    while ( true ) {
        agent = new EphemeralAgent()
        gatekeeper = new Gatekeeper(this)
        node('master') {
            echo "Checking if resources are available on vSphere..."
            gatekeeper.checkLimits()
            if (gatekeeper.canProvision) {
                stage('Spin up ephemeral VM') {
                    try {
                        uuid = agent.getMachineName()
                        vSphere buildStep: [$class: 'Clone', clone: uuid, cluster: 'sjc-vm-cluster',
                            customizationSpec: '', datastore: 'sjc-vm-04-localssd', folder: 'sjc/build',
                            linkedClone: true, powerOn: false, resourcePool: 'jenkins-build-slaves',
                            sourceName: 'js-photon-os-template', timeoutInSeconds: 60], serverName: vSphereServer

                        vSphere buildStep: [$class: 'Reconfigure', reconfigureSteps: [[$class: 'ReconfigureCpu',
                            coresPerSocket: '1', cpuCores: '2', cpuLimitMHz: '6600']], vm: uuid], serverName: vSphereServer // Max overcommit is 4:1 vCPU to pCPU

                        vSphere buildStep: [$class: 'PowerOn', timeoutInSeconds: 60, vm: uuid], serverName: vSphereServer

                        def buildAgentIP = vSphere buildStep: [$class: 'ExposeGuestInfo',
                            envVariablePrefix: 'VSPHERE', vm: uuid, waitForIp4: true], serverName: vSphereServer

                        agent.setIPAddress(buildAgentIP)
                        agent.Attach()
                        gatekeeper.provisioningDone()
                    } catch (Exception exc) {
                        def jobName = "${env.JOB_NAME}"
                        jobName = jobName.getAt(0..(jobName.indexOf('/') - 1))
                        def reason = "Could not provision VM, retrying..."
                        notifySlack("", slackNotificationChannel,
                            [
                                title: "${jobName} ${primaryStageName} ${env.CHANGE_ID}, build #${env.BUILD_NUMBER}",
                                title_link: "${env.BUILD_URL}",
                                color: "warning",
                                text: "${reason}",
                            ]
                        )
                        node('master') {
                            stage('Destroy ephemeral VM') {
                                vSphere buildStep: [$class: 'Delete', failOnNoExist: true, vm: uuid], serverName: vSphereServer
                            }
                        }
                    }
                }
            } else {
                echo "Not enough resources, retrying VM provisioning in 30 seconds..."
                sleep(time:30, unit:"SECONDS")
            }
        }
        if (gatekeeper.nodeProvisioned) break
    }
    try {
        for (int i = 0; i < enabledBuildConfigs.size() ; i++) {
            def buildFlavor = enabledBuildConfigs[i]
            buildConfigMap[buildFlavor] = {
                node(uuid) {
                    def ghsb = new GithubStatusMsgBuilder(this, buildFlavor)
                    gitCheckout(true)
                    withDockerEnv {
                        ghsb.postBuildStart()
                        buildPRStepsVicOS type: buildFlavor
                        ghsb.postBuildFinished('SUCCESS')
                        notifyBuildStatus('Success', buildFlavor)
                    }
                }
            }
        }
        parallel buildConfigMap
    } catch (FlowInterruptedException ae) {
        node(uuid) {
            notifyBuildStatus('Aborted', 'shipping')
        }
        throw ae
    } catch (exc) {
        node(uuid) {
            notifyBuildStatus('Failure', 'shipping')
        }
        throw exc
    } finally {
        node('master') {
            stage('Cleaning master workspace') {
                cleanWs()
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