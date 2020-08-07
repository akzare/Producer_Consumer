'''
 * @file   Config.py
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   01 August 2020
 * @version 0.1
 * @brief   This file defines constants for remotely connecting to embedded board.
'''

import os
import platform


# Note: The contents of this script must be updated based on your system setting! 

########################################################################
# C O N S T A N T   D E F I N I T I O N S
########################################################################

# Embedded board names (used only as an information)
BOARD1_NAME = "ODROID#1"
BOARD2_NAME = "XAVIER#2"


# Embedded board IP address (used for Linux ssh and scp commands)
BOARD1_IP_ADDR = "100.65.49.235"
BOARD2_IP_ADDR = "100.65.49.152"


# Embedded board user name (used for Linux ssh and scp commands)
BOARD1_USER_NAME = "odroid"
BOARD2_USER_NAME = "nvidia"
PRIVATE_KEY_LOCATION = "C:\\Users\\akzare\\.ssh\\id_rsa"  # private key location here


# Linux/Windows ssh command
if os.name == 'nt':
  # OS : Windows
  # https://gist.github.com/josephcoombe/3a234721fc5a6885ca4f91e3a27860f4
  # Handle execution of 32-bit Python on 64-bit Windows
  system32 = os.path.join(os.environ['SystemRoot'], 'SysNative' if platform.architecture()[0] == '32bit' else 'System32')
  SSH = os.path.join(system32, 'OpenSSH/ssh.exe')
else:
  # OS : Linux
  SSH = 'ssh'


# Linux/Windows scp command
SCP = 'scp'


# isc application command on embedded boards
ISC_APP_SERVER = 'isc'
ISC_APP_CLIENT = 'isc --client -a 100.65.49.152'


# producer application command on embedded boards
PROD_APP = 'producer'


# consumer application command on embedded boards
CONS_APP = 'consumer'


# Logging directory on host desktop machine
HOST_LOG_DIR = 'C:\\Users\\akzare\\Producer_Consumer\\log'
HOST_SERVER_LOG_DIR = 'C:\\Users\\akzare\\Producer_Consumer\\log\\server'
HOST_CLIENT_LOG_DIR = 'C:\\Users\\akzare\\Producer_Consumer\\log\\client'
# Source code root directory on embedded boards
BOARD1_SRC_ROOT_DIR = '/home/odroid/akzare/isc/src'
BOARD2_SRC_ROOT_DIR = '/home/nvidia/akzare/isc/src'


# C Source code directory on embedded board
BOARD_SRC_C_DIR = 'c'


# Source code root directory on host machine
HOST_SRC_ROOT_DIR = 'C:\\Users\\akzare\\Producer_Consumer\\src\\c'


# ISC process log file name
ISC_PRCS_LOG_FILENAME = 'isc.log'


# Socket client thread log file name
SCKT_CLIENT_THRD_LOG_FILENAME = 'sckt_client.log'


# Socket server thread log file name
SCKT_SERVER_THRD_LOG_FILENAME = 'sckt_server.log'


# Shared Memory Xmit thread log file name
SHMEM_XMIT_THRD_LOG_FILENAME = 'shmem_xmit.log'


# Shared Memory Receive thread log file name
SHMEM_REC_THRD_LOG_FILENAME = 'shmem_rec.log'


# Producer process log file name
PROD_PRCS_LOG_FILENAME = 'producer.log'


# Consumer process log file name
CONS_PRCS_LOG_FILENAME = 'consumer.log'


# Parsed data flow log file name
PARSED_DATAFLOW_LOG_FILENAME = 'report_dataflow.log'


# Log files names dictionary
ISC  = 'isc'
SHMEM_REC  = 'shmem_rec'
SHMEM_XMIT = 'shmem_xmit'
SCKT_SERVER = 'sckt_server'
SCKT_CLIENT = 'sckt_client'
PROD = 'prod'
CONS = 'cons'
MEM = 'mem'


YES  = 1
NO  = 0


# Output file, where the parsed data flow loglines will be copied to
OUT_DATAFLOW_FILENAME = os.path.normpath('{}/{}'.format(HOST_LOG_DIR, 
	                                                    PARSED_DATAFLOW_LOG_FILENAME))
