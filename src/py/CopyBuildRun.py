'''
 * @file   CopyBuildRun.py
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   01 August 2020
 * @version 0.1
 * @brief   This Python script is used for the test and verification automation. 
 *          It is targeted to copy, build, and run a c application module on an  
 *          embedded board. 
 *          After finishing the run cycle of the application, it kills all the running 
 *          components and thereafter copies the created log files back to the host 
 *          machine for further analysis.
'''

# ssh notes: To setup your ssh and scp connection from host machine to remote
#            embedded boards, please consult the following links:
# https://endjin.com/blog/2019/09/passwordless-ssh-from-windows-10-to-raspberry-pi
# https://stackoverflow.com/questions/21659637/how-to-fix-sudo-no-tty-present-and-no-askpass-program-specified-error


from __future__ import print_function,unicode_literals
import subprocess
import sys
from subprocess import Popen, PIPE
from datetime import datetime
import time
import Config as cfg


########################################################################
# P r i n t   l i n e   s e p a r a t o r
########################################################################
def print_line_separator(board_ip_addr):
  print('########################################################################')
  print('Embedded Board Address:{}'.format(board_ip_addr))


########################################################################
# S e t   c u r r e n t   t i m e   t o   r e m o t e   b o a r d
########################################################################
def remote_set_time(board_ip_addr, board_root_usr, board_type):
  print_line_separator(board_ip_addr)
  # get current time
  now = datetime.now()
  time_string = ('%02d %03s %04d %02d:%02d:%02d'%(now.day, now.strftime("%B"), now.year, now.hour,now.minute,now.second))

  print(time_string) 
  # Remotely set time on an embedded board to the current time on the host desktop machine
  ret = subprocess.call([cfg.SSH, '-i', cfg.PRIVATE_KEY_LOCATION,
                         '{}@{}'.format(board_root_usr, board_ip_addr),
                         'sudo date --set \"{}\"'.format(time_string)]);
  if ret == 0:
    print ('Successfully set time to {} on embedded board {}.'.format(time_string, board_type))
  else:  
    raise Exception('Result of setting time to {} on embedded board {}:{}'.format(time_string, board_type, ret))


##############################################################################
# C r e a t e   s o u r c e   c o d e   d i r   o n   r e m o t e   b o a r d
##############################################################################
def remote_create_src_dir(board_ip_addr, board_root_usr, dir_name, board_type):
  print_line_separator(board_ip_addr)
  # Remotely create the source code directory on embedded board
  ret = subprocess.call([cfg.SSH, '-i', cfg.PRIVATE_KEY_LOCATION,
                         '{}@{}'.format(board_root_usr, board_ip_addr),
                         'mkdir -p {}'.format(dir_name)]);
  if ret == 0:
    print ('Successfully created the source code directory on embedded board {}.'.format(board_type))
  else:  
    raise Exception('Result of creating the source code directory on embedded board {}:{}'.format(board_type, ret))


###########################################################################
# C o p y   s o u r c e   c o d e   d i r   o n   r e m o t e   b o a r d
###########################################################################
def remote_copy_src_dir(board_ip_addr, board_root_usr, src_dir_name, dst_dir_name, board_type):
  print_line_separator(board_ip_addr)
  # Remotely copy source code into embedded board
  with Popen([cfg.SCP, '-i', cfg.PRIVATE_KEY_LOCATION,
            '-r',
            src_dir_name,
            '{}@{}:{}'.format(board_root_usr, board_ip_addr, dst_dir_name)],
             stdin  = PIPE,
             stdout = PIPE,
             stderr = PIPE,
             universal_newlines = True) as p:
    p.wait()

    if p.returncode == 0:
      print ('Successfully copied source code directory on embedded board {}.'.format(board_type))
    else:
      raise Exception('Result of copying source code directory on embedded board {}: stderr={} ret={}'.format(board_type, repr(p.stderr.readline()), p.returncode))


########################################################################
# B u i l d   & c l e a n   s u b s y s t e m   o n   r e m o t e   b o a r d
########################################################################
def remote_build_subsys(board_ip_addr, board_root_usr, subsys_root_dir_name, subsys_subdir_name, subsys_name, build, board_type):
  print_line_separator(board_ip_addr)
  # Remotely clean & build the source code on embedded board
  if build == 1:
    # Remotely build a subsystem on embedded board
    ret = subprocess.call([cfg.SSH, '-i', cfg.PRIVATE_KEY_LOCATION,
                           '{}@{}'.format(board_root_usr, board_ip_addr),
                           'cd {}/{} ; make {}'.format(subsys_root_dir_name, subsys_subdir_name, subsys_name)]);
    if ret == 0:
      print ('Successfully built {} subsystem on embedded board {}.'.format(subsys_name, board_type))
    else:
      raise Exception('Result of building {} subsystem on embedded board {}: {}'.format(subsys_name, board_type, ret))
  else:
    # Remotely clean a subsystem on embedded board
    if subsys_name != 'all':
      ret = subprocess.call([cfg.SSH, '-i', cfg.PRIVATE_KEY_LOCATION,
                             '{}@{}'.format(board_root_usr, board_ip_addr),
                             'cd {}/{} ; make clean_{}'.format(subsys_root_dir_name, subsys_subdir_name, subsys_name)]);
    else:  
      ret = subprocess.call([cfg.SSH, '-i', cfg.PRIVATE_KEY_LOCATION,
                             '{}@{}'.format(board_root_usr, board_ip_addr),
                             'cd {}/{} ; make clean'.format(subsys_root_dir_name, subsys_subdir_name)]);
    if ret == 0:
      print ('Successfully cleaned {} subsystem on embedded board {}.'.format(subsys_name, board_type))
    else:
      raise Exception('Result of cleaning {} on embedded board {}: {}'.format(subsys_name, board_type, ret))


########################################################################
# R u n   s u b s y s t e m   o n   r e m o t e   b o a r d
########################################################################
def remote_run_subsys(board_ip_addr, board_root_usr, subsys_root_dir_name, subsys_subdir_name, subsys_name, board_type):
  print_line_separator(board_ip_addr)
  # Remotely run subsystem in background on embedded board
  ret = subprocess.call([cfg.SSH, '-i', cfg.PRIVATE_KEY_LOCATION,
                         '{}@{}'.format(board_root_usr, board_ip_addr),
                         'cd {}/{} ; ./{} >&- &'.format(subsys_root_dir_name, subsys_subdir_name, subsys_name)]);
  if ret == 0:
    print ('Successfully run the {} subsystem on embedded board {}.'.format(subsys_name, board_type))
  else:  
    raise Exception('Result of running the {} subsystem on embedded board {}:{}'.format(subsys_name, board_type, ret))


########################################################################
# G e t   P I D   o f   s u b s y s   o n   r e m o t e   b o a r d
########################################################################
def remote_getpid_subsys(board_ip_addr, board_root_usr, subsys_name, board_type):
  print_line_separator(board_ip_addr)
  kill_cmd = ''
  # Remotely get process ID of running consumer on embedded board
  with Popen([cfg.SSH, '-i', cfg.PRIVATE_KEY_LOCATION,
            '-T', 
            '{}@{}'.format(board_root_usr, board_ip_addr)],
             stdin  = PIPE,
             stdout = PIPE,
             stderr = PIPE,
             shell  = False,
             universal_newlines = True) as p:
    if subsys_name == cfg.CONS_APP:
      kill_cmd, error = p.communicate("""
            ps -aux | grep consumer | grep -v grep | awk \'{print \"kill -10 \" $2}\'
            """)

    elif subsys_name == cfg.ISC_APP_SERVER or subsys_name == cfg.ISC_APP_CLIENT:
      kill_cmd, error = p.communicate("""
            ps -aux | grep isc | grep -v grep | awk \'{print \"kill -10 \" $2}\'
            """)
    else :
      raise Exception('Invalid subsystem {} t getPID!'.format(subsys_name))

    matched_lines = [line for line in kill_cmd.split('\n') if "kill -10" in line]
    kill_cmd = matched_lines[0]

    if p.returncode == 0 and error.find('error') == -1:
      print ('Successfully generated the kill command for {} subsystem on embedded board {}:{}'.format(subsys_name, board_type, repr(kill_cmd)))
      if error.find('warning') != -1:
        print ('  Warnings: \n{}'.format(error))
    else:
      raise Exception('Building source code directory on embedded board {} is failed: error={} ret={}'.format(board_type, error, p.returncode))
  
  return kill_cmd


########################################################################
# S t o p   s u b s y s t e m   o n   r e m o t e   b o a r d
########################################################################
def remote_stop_subsys(board_ip_addr, board_root_usr, subsys_kill_cmd, board_type):
  print_line_separator(board_ip_addr)
  # Remotely stop the subsystem
  ret = subprocess.call([cfg.SSH, '-i', cfg.PRIVATE_KEY_LOCATION,
                         '{}@{}'.format(board_root_usr, board_ip_addr),
                         subsys_kill_cmd]);
  if ret == 0:
    print ('Successfully executed the stop command {} on embedded board {}.'.format(subsys_kill_cmd, board_type))
  else:  
    raise Exception('Result of executing the stop command {} on embedded board {}:{}'.format(subsys_kill_cmd, board_type, ret))



########################################################################
# C o p y   l o g   f i l e   f r o m   r e m o t e   b o a r d
########################################################################
def remote_copy_log_file(board_ip_addr, board_root_usr, subsys_root_dir_name, subsys_subdir_name, log_file_name, host_log_dir, board_type):
  print_line_separator(board_ip_addr)
  # Remotely copy generated log file from embedded board into host machine
  with Popen([cfg.SCP, '-i', cfg.PRIVATE_KEY_LOCATION,
            '-r',
            '{}@{}:{}/{}/{}'.format(board_root_usr, board_ip_addr, subsys_root_dir_name, subsys_subdir_name, log_file_name),
            host_log_dir],
             stdin  = PIPE,
             stdout = PIPE,
             stderr = PIPE,
             universal_newlines = True) as p:
    p.wait()
    if p.returncode == 0:
      print ('Successfully copied log file {} from embedded board {} into host.'.format(log_file_name, board_type))
    else:  
      raise Exception('Result of copying log file {} from embedded board {}:{}'.format(log_file_name, board_type, p.returncode))


########################################################################
# M A I N   P R O C E D U R E
########################################################################
if __name__ == '__main__':
  # ##############################################################
  # Set current time into embedded boards #1 & #2
  # The following commands are only necessary if the time on the target embedded
  # board is not globally sync.
#  remote_set_time(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_NAME)
#  remote_set_time(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_NAME)

  # ##############################################################
  # Creating the source code directory on embedded boards #1 & #2
  remote_create_src_dir(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD1_NAME)
  remote_create_src_dir(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD2_NAME)
    
  # ##############################################################
  # Copying C source code into embedded boards #1 & #2
  remote_copy_src_dir(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.HOST_SRC_ROOT_DIR, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD1_NAME)
  remote_copy_src_dir(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.HOST_SRC_ROOT_DIR, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD2_NAME)

  # ##############################################################
  # Cleaning C source code on embedded boards #1 & #2
  remote_build_subsys(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, 'all', cfg.NO, cfg.BOARD1_NAME)
  remote_build_subsys(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, 'all', cfg.NO, cfg.BOARD2_NAME)

  # ##############################################################
  # Building C source code on embedded boards #1 & #2
  remote_build_subsys(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, 'all', cfg.YES, cfg.BOARD1_NAME)
  remote_build_subsys(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, 'all', cfg.YES, cfg.BOARD2_NAME)

  # ##############################################################
  # Cleaning producer subsystem on embedded board #1
  remote_build_subsys(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.PROD, cfg.NO, cfg.BOARD1_NAME)
  # Building producer subsystem on embedded board #1
  remote_build_subsys(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.PROD, cfg.YES, cfg.BOARD1_NAME)

  # ##############################################################
  # Cleaning consumer subsystem on embedded board #2
  remote_build_subsys(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.CONS, cfg.NO, cfg.BOARD2_NAME)
  # Building consumer subsystem on embedded board #2
  remote_build_subsys(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.CONS, cfg.YES, cfg.BOARD2_NAME)

  # ##############################################################
  # Running consumer subsystem on embedded board #2
  remote_run_subsys(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.CONS_APP, cfg.BOARD2_NAME)
  # GetPID for the consumer subsystem
  consumer_kill_cmd = remote_getpid_subsys(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.CONS_APP, cfg.BOARD2_NAME)

  # ##############################################################
  # Running isc server subsystem on embedded board #2
  remote_run_subsys(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.ISC_APP_SERVER, cfg.BOARD2_NAME)
  # GetPID for the isc server subsystem
  isc_server_kill_cmd = remote_getpid_subsys(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.ISC_APP_SERVER, cfg.BOARD2_NAME)

  # ##############################################################
  # Running isc client subsystem on embedded board #1
  remote_run_subsys(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.ISC_APP_CLIENT, cfg.BOARD1_NAME)
  # GetPID for the isc client subsystem
  isc_client_kill_cmd = remote_getpid_subsys(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.ISC_APP_CLIENT, cfg.BOARD1_NAME)

  # ##############################################################
  # Running producer subsystem on embedded board #1
  remote_run_subsys(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.PROD_APP, cfg.BOARD1_NAME)

  # ##############################################################
  # Sleep for 3 seconds until current run finishes
  time.sleep(3) 

  # ##############################################################
  # Stopping isc client subsystem on embedded board #1
  remote_stop_subsys(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, isc_client_kill_cmd, cfg.BOARD1_NAME)

  # ##############################################################
  # Sleep for 3 seconds until client side shuts down 
  time.sleep(3) 
   
  # ##############################################################
  # Stopping consumer subsystem on embedded board #2
  remote_stop_subsys(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, consumer_kill_cmd, cfg.BOARD2_NAME)

  # ##############################################################
  # Stopping isc server subsystem on embedded board #2
  remote_stop_subsys(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, isc_server_kill_cmd, cfg.BOARD2_NAME)
    
  # ##############################################################
  # Sleep for 3 seconds until the whole system shuts down
  time.sleep(3) 

  # ##############################################################
  # Copying the ISC server log file from embedded board #2 into host
  remote_copy_log_file(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.ISC_PRCS_LOG_FILENAME, cfg.HOST_SERVER_LOG_DIR, cfg.BOARD2_NAME)

  # ##############################################################
  # Copying the consumer log file from embedded board #2 into host
  remote_copy_log_file(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.CONS_PRCS_LOG_FILENAME, cfg.HOST_SERVER_LOG_DIR, cfg.BOARD2_NAME)

  # ##############################################################
  # Copying the Socket server thread log file from embedded board #2 into host
  remote_copy_log_file(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.SCKT_SERVER_THRD_LOG_FILENAME, cfg.HOST_SERVER_LOG_DIR, cfg.BOARD2_NAME)

  # ##############################################################
  # Copying the Shared Memory Receive thread log file from embedded board #2 into host
  remote_copy_log_file(cfg.BOARD2_IP_ADDR, cfg.BOARD2_USER_NAME, cfg.BOARD2_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.SHMEM_REC_THRD_LOG_FILENAME, cfg.HOST_SERVER_LOG_DIR, cfg.BOARD2_NAME)

  # ##############################################################
  # Copying the ISC client log file from embedded board #1 into host
  remote_copy_log_file(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.ISC_PRCS_LOG_FILENAME, cfg.HOST_CLIENT_LOG_DIR, cfg.BOARD1_NAME)

  # ##############################################################
  # Copying the producer log file from embedded board #1 into host
  remote_copy_log_file(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.PROD_PRCS_LOG_FILENAME, cfg.HOST_CLIENT_LOG_DIR, cfg.BOARD1_NAME)

  # ##############################################################
  # Copying the Socket client thread log file from embedded board #1 into host
  remote_copy_log_file(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.SCKT_CLIENT_THRD_LOG_FILENAME, cfg.HOST_CLIENT_LOG_DIR, cfg.BOARD1_NAME)

  # ##############################################################
  # Copying the Shared Memory Xmit thread log file from embedded board #1 into host
  remote_copy_log_file(cfg.BOARD1_IP_ADDR, cfg.BOARD1_USER_NAME, cfg.BOARD1_SRC_ROOT_DIR, cfg.BOARD_SRC_C_DIR, cfg.SHMEM_XMIT_THRD_LOG_FILENAME, cfg.HOST_CLIENT_LOG_DIR, cfg.BOARD1_NAME)
