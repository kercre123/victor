#!/usr/bin/env groovy

node('victor-slaves') {
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
    stage('Build MacOS') {
        sh './project/victor/build-victor.sh -p mac -c Release -DANKI_WEBSERVER_ENABLED=1'
    }
    stage('Webots') {
        sh './project/build-scripts/webots/webotsTest.py'
    }
    stage('Unit Tests') {
        sh './project/buildServer/steps/unittestsUtil.sh'
        sh './project/buildServer/steps/unittestsCoretech.sh'
        sh './project/buildServer/steps/unittestsEngine.sh'
    }
    stage('Build Vicos') {
        sh './project/victor/build-victor.sh -p vicos -c Release -DANKI_WEBSERVER_ENABLED=1'
    }
}