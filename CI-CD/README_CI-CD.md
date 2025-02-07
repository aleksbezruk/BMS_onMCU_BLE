## __*Continuous Integration and Continuous Delivery (CI/CD)*__

### I. Platform
Jenkins is preferred CI/CD platform because it's flexible, free and has good community support. <br>
https://www.jenkins.io/ <br>
PLanned architecture of CI/CD server: <br>
__*GitHub server -> Jenkins Controller -> Agent*__ <br>
*Jenkins Controller* - my host PC/notebook used for development. <br>
*Agent* - additional Jenkins server that will do build Job, like Raspberry PI4B. Agent allows to reduce load on host PC <br>
Jenkins Controller and Agent(s) introduce __distribited Jenkins server__ platform.

### II. Jenkins intallation for Controller
See instructions here for Linux: <br> 
https://www.jenkins.io/doc/book/installing/linux/#prerequisites <br>

### III. Installation and config Jenkins Agent
1. Install Raspberry PI OS 64-bit to RPI4B.
2. Check connection with Agent: ping RPI4B.local. <br>
> output example: 64 bytes from 192.168.0.121: icmp_seq=1 ttl=64 time=8.28 ms <br>
3. Check if Java is installed on Jenkins Controller: <br>
> java --version <br>
4. Check Jenkins Controller status: <br>
> sudo systemctl status jenkins  -> (should be active/running)
5. Install Jenkins on Agent (RPI4B): <br>
- 5.1 On conttroller side -> ssh-keygen -f '/home/oleksandr/.ssh/known_hosts' -R 'RPI4B.local' <br>
- 5.2 Login Agent: ssh oleksandr@RPI4B.local <br>
- 5.3 Install Java : <br>
> sudo apt update <br>
> sudo apt install fontconfig openjdk-17-jre <br>
> java -version <br>
- 5.4 Install Jenkins: <br>
>curl -fsSL https://pkg.jenkins.io/debian-stable/jenkins.io-2023.key | sudo tee \ <br>
>  /usr/share/keyrings/jenkins-keyring.asc > /dev/null   <br>
>  <br>
>echo deb [signed-by=/usr/share/keyrings/jenkins-keyring.asc] \ <br>
>  https://pkg.jenkins.io/debian-stable binary/ | sudo tee \ <br>
>  /etc/apt/sources.list.d/jenkins.list > /dev/null <br>
>  <br>
> sudo apt update -y <br>
>  <br>
>sudo apt install jenkins -y <br>
>  <br>
>sudo systemctl start jenkins && sudo systemctl enable jenkins <br>
>  <br>
>sudo systemctl status jenkins <br>
>  <br>
>sudo usermod -aG sudo jenkins <br>
>  <br>
>sudo cat /var/lib/jenkins/secrets/initialAdminPassword <br>
>  <br>
>sudo adduser --disabled-password jenkins <br>
>  <br>
>sudo passwd jenkins <br>
>  <br>
>mkdir -p /home/Jenkins/jenkins-agent <br>
>  <br>
6. Generate SSH keys on Agent side <br>
- 6.1 Add a .ssh folder in the Jenkins Slave Server <br>
>mkdir ~/.ssh && cd ~/.ssh <br>
- 6.2 ssh-keygen -t rsa -C "Access key for Jenkins slaves" <br>
- 6.3 cat id_rsa.pub >> ~/.ssh/authorized_keys <br>
- 6.4 cat id_rsa <br>
7. Add the SSH Private Key to Jenkins Credentials on Controller/Master side <br>
8. Connect to the Agent <br>
> Note: if error like "Not sufficient permission occurs on Agent side", then: <br>
> config 'jenkins' user to own the Jenkins folders : <br>
> login RPI4B.loacl as jenkins user: ssh jenkins@RPI4B.local <br>
> sudo chown jenkins: /home/Jenkins <br>
> sudo chmod u+w /home/Jenkins  <br>
> sudo chown jenkins: /home/Jenkins/jenkins-agent <br>
> sudo chmod u+w /home/Jenkins/jenkins-agent <br>
9. Add pipeline on Controller side in order to run Simple job like print "echo" on Agent side: <br>
- 9.1 Click to crete freestyle project. <br>
- 9.2 Setup the project settings: <br>
> In Configuration => Build Steps => Execute Shell => Add a command : <br>
> echo "The pipeline is from the Master Jenkins Node" <br>
> echo "This is a sample pipeline here that we need to follow in Agent" <br>
> <br>
> [MUST] Check for the option: Restrict where this project can be run. <br>
> In the Label Expression add the label that was added while creating the NODE. <br>
> Hit Apply and Save => Click on Build Now. <br>
- 9.3 Run pipeline and check status. <br>

### IV. Install software, toolchains on RPI4B Agent
1. Install VNC Server on your Raspberry Pi device & VNC viewer/Client on Host PC side <br>
> This will allow us to start GUI for RPI4B (Virtual Desktop without connected monitor to RPI4B) <br>

### V. Binding GitHub repo(s) to Jenkins Server
+1. Find instructions here: https://www.jenkins.io/doc/book/using/best-practices/ <br>
> Section "GitHub multibranch Pipelines"

### VI. "Source pull" Job 
The Job is already built-in in Jenkins server out of the box.

### VII. Build Job 
1. Python dependencies: <br>
> pip3 install -r ../requirements.txt --break-system-packages
2. In order to run 'CI-CD/build/debug_config_postbuild.sh' over SSH do: <br>
> - generate Public keys if no such - $ ssh-keygen -t ed25519 ;   <br>
> - copy generated keys to jenkins diraectory '/var/lib/jenkins/.ssh' ; <br>
> - change keys owner to 'jenkins' user - $ sudo chown -R jenkins: .ssh , $ sudo chmod -R u+w .ssh
> - copy public key to RPI4B - $ ssh-copy-id -i .ssh/id_ed25519.pub jenkins@RPI4B.local .

### VIII. Integration tests Job 
1. Compiling Openocd on RPI4B: <br>
> Start by doing a fresh '$ sudo apt-get update' this will make sure you have the latest packages and repository set up ; <br>
> $ sudo apt-get install git autoconf libtool make pkg-config libusb-1.0-0 libusb-1.0-0-dev ; <br>
> git clone http://openocd.zylin.com/openocd ; <br>
> $ cd openocd ; <br>
> $ ./bootstrap ; <br>
> $ ./configure --enable-sysfsgpio --enable-bcm2835gpio ; <br>
> $ make ; <br>
> $ sudo make install ; <br>
2. Alternative solution is PyOCD : <br>
> $ python3 -m pip install pyocd --break-system-packages ; <br>
> PATH=$PATH:$HOME/.local/bin ; <br>
3. Flashing FW image: <br>
> check -> $ pyocd list --targets ; <br>
> debug probes list (includes UIDs): $ pyocd list ; <br>
> example for Flash erase: $ pyocd erase --chip --target cy8c6xx7_nosmif --uid <debug probe UID> <br>
> example for Hex load: $pyocd load --target cy8c6xx7_nosmif --uid <debug probe UID> <path to hex> <br>
4. Run intagration tests: <br>
> install python >=3.10 if not installed yet ; <br>
> install 'pytest': <br>
> - $ PATH=$PATH:$HOME/.local/bin ; <br> 
> - $ pip3 install -U pytest --break-system-packages ; <br>
> - $ pip install pytest-dependency --break-system-packages ; <br>
> - $ pip install pytest-repeat --break-system-packages ; <br>
> - for more details also see https://pypi.org/project/pytest-repeat/ ; <br>
> bring-up BLE stack on RPI4B side: <br>
> - check BlueZ version: $ bluetoothctl --version <br> ;
> - $ bluetoothctl -> open BLE manager & use cmds loke 'help', 'list'->list of BLE controllers ; <br>
> - $ bluetoothctl power on -> power on controller ; <br>
> - $ bluetoothctl scan on -> scan devices => Find advertising BMS device ; <br>
> - $ rfkill block bluetooth; $ rfkill unblock bluetooth; $ rfkill toggle bluetooth ; <br>
> - $ sudo apt install blueman -> install BLE manager for Linux OS ; <br>
> - upgrade bluetoothctl: ; <br>
> - $ wget http://www.kernel.org/pub/linux/bluetooth/bluez-5.79.tar.xz ; <br>
> - $ tar xf bluez-5.79.tar.xz ; <br>
> - $ cd bluez-5.79 ; <br>
> - $ ./configure ; <br>
> - If there issues on previuous stepm run: <br>
> - add -> PATH=$PATH:$HOME/.local/bin -> to $HOME/.bashrc file ; <br>
> - $ sudo apt-get install libglib2.0-dev ; <br>
> - $ sudo apt-get install libgtk2.0-dev ; <br>
> - $ sudo apt install libdbus-1-dev ; <br>
> - $ sudo apt install libical-dev ; <br>
> - $ sudo apt-get install libreadline-dev ; <br>
> - $ make ; <br>
> - $ sudo make install ; <br>
> install 'SimplePyBLE' : <br>
> - check installed libarary version -> $ pip freeze | grep simplepyble
> - $ pip3 install simplepyble --break-system-packages ; <br>
> - there is also a way to build simplepyble from Source code: https://github.com/simpleble/simpleble ; <br>
> run ITs' shell script Jenkins job: <br>
> - $ sh "Integration_Tests/run_ITs.sh" ; <br>
> - to close used UDP port -> $ fuser -k -n udp 7701 .
