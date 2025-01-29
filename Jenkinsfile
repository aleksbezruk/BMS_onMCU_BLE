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
                sh "./CI-CD/build/rebuild_debug.sh"
            }
        }
        stage('Copy Debug build firmware image to remote Agent') {
            agent {label "jenkins-controller"}
            steps {
                echo "Copy Debug build firmware image to remote Agent is in progress..."
                sh "./CI-CD/build/copy_hex_to_jenkins_agent.sh"
            }
        }
        stage('Ping Jenjins Agent and Flash BMS firmware hex file') {
            agent {label "jenkins-agent"}
            steps {
                echo "Jenkins agent is ready for BMS CI/CD Job"
                sh "./CI-CD/build/flash_hex.sh"
            }
        }
        stage('Run BMS Integration tests') {
            agent {label "jenkins-agent"}
            steps {
                sh "./Integration_Tests/run_ITs.sh & ../../qtools/bin/qspy -u 7701 -c /dev/ttyACM0 -b 115200"
            }
        }
    }
}