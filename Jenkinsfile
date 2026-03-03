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
                timeout(time: 200, unit: 'SECONDS')
            }
            steps {
                sh "./CI-CD/build/rebuild_debug.sh QN9080"
            }
        }
        stage('Copy Debug build firmware image to remote Agent') {
            agent {label "jenkins-controller"}
            steps {
                echo "Copy Debug build firmware image to remote Agent is in progress..."
                sh "./CI-CD/build/copy_hex_to_jenkins_agent.sh QN9080"
            }
        }
        stage('Ping Jenjins Agent and Flash BMS firmware hex file') {
            agent {label "jenkins-agent"}
            steps {
                echo "Jenkins agent is ready for BMS CI/CD Job"
                sh "./CI-CD/build/flash_hex.sh QN9080"
            }
        }
        stage('Run BMS Integration tests') {
            agent {label "jenkins-agent"}
            steps {
                parallel(
                    qspy_logging: {
                        sh "./Integration_Tests/start_qspy.sh QN9080"
                    },
                    tests: {
                        sh "./Integration_Tests/run_ITs_QN9080.sh QN9080"; sh "./Integration_Tests/close_qspy.sh QN9080"
                    }
                )
            }
        }
    }
}