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
                buildDiscarder logRotator(artifactDaysToKeepStr: '', artifactNumToKeepStr: '5', daysToKeepStr: '', numToKeepStr: '5')
                disableConcurrentBuilds()
            }
            steps {
                echo "hello, BMS CI/CD Job"
            }
        }
    }
}