# SNMP-Agent-Example-Reference
Simple example code for adding new module to snmp agent

Here it attached with folder hello_world_emu/, which contains the reference example modules
Steps to follow making this example code with snmp source code

    1. Download the folder
    2. cp hello_world_emu net-snmp-x.x.x/agent/mibgroup/. -rf
    3. vi net-snmp-x.x.x/agent/mibgroup/mibII.h
    Add the below line at the end of the file, to add this module with snmpd
          config_require(hello_world_emu/hello_world_emu)
    
    4. Configuring and building the source code
        ./configure --with-default-snmp-version=3 --with-sys-contact="pengiun.bsp@gmail.com" --with-sys-location="India" --with-logfile=/var/log/snmpd.log --with-persistent-directory=/var/net-snmp --disable-manuals --enable-debugging --with-openssl --without-perl-modules --disable-embedded-perl
     5. make
