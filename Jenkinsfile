pipeline {
    agent {label "jenkins-controller"}
    options {
        buildDiscarder logRotator(artifactDaysToKeepStr: '', artifactNumToKeepStr: '5', daysToKeepStr: '', numToKeepStr: '5')
        disableConcurrentBuilds()
    }
    stages {
        stage('Build Debug build configuration') {
            agent {label "jenkins-controller"}
            options {
                // Timeout counter starts BEFORE agent is allocated
                timeout(time: 100, unit: 'SECONDS')
            }
            steps {
                sh "CI-CD/build/rebuild_debug.sh"
            }
        }
        stage('Copy Debug build firmware image to remote Agent') {
            agent {label "jenkins-controller"}
            steps {
                // TODO: implement, just stub for now
                echo "Copy Debug build firmware image to remote Agent is in progress..."
            }
        }
        stage('Ping Jenjins Agent') {
            agent {label "jenkins-agent"}
            steps {
                echo "Jenkins agent is ready for BMS CI/CD Job"
            }
        }
    }
}