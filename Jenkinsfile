#!/usr/bin/env groovy

node('victor-slaves') {
    withEnv(['CONFIGURATION=release']) {
        checkout([
            $class: 'GitSCM',
            branches: scm.branches,
            doGenerateSubmoduleConfigurations: scm.doGenerateSubmoduleConfigurations,
            extensions: scm.extensions,
            userRemoteConfigs: scm.userRemoteConfigs
        ])
        stage('json lint') {
            sh './lib/util/tools/build/jsonLint/jsonLint.sh resources'
        }
        stage('Build') {
            sh './project/victor/build-victor.sh -c Release'
        }
        withEnv(['PLATFORM=mac']) {
            stage("Build MacOS ${CONFIGURATION}") {
                sh "./project/victor/scripts/victor_build_${CONFIGURATION}.sh -p ${PLATFORM}"
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
        withEnv(['PLATFORM=vicos']) {
            stage("Build Vicos ${CONFIGURATION}") {
                sh "./project/victor/scripts/victor_build_${CONFIGURATION}.sh -p ${PLATFORM}"
            }
        }
    }
}