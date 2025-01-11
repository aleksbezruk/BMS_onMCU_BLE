pipeline {
    //agent {label "jenkins-agent"}
    agent {label "Built-In Node"}
    options {
        buildDiscarder logRotator(artifactDaysToKeepStr: '', artifactNumToKeepStr: '5', daysToKeepStr: '', numToKeepStr: '5')
        disableConcurrentBuilds()
    }
    stages {
        stage('Hello') {
            agent {label "jenkins-agent"}
            options {
                // Timeout counter starts BEFORE agent is allocated
                timeout(time: 10, unit: 'SECONDS')
            }
            steps {
                echo "hello, BMS CI/CD Job"
            }
        }
    }
}