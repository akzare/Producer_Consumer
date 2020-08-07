'''
 * @file   LogFileParser.py
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   03 August 2020
 * @version 0.1
 * @brief   This file defines Log_File_Parser class which is used 
 *          for parsing of the ISC log files.
'''

from __future__ import print_function,unicode_literals
import os
import re
import Config as cfg


class Log_File_Parser:
  """
  Define log file parser class.
  """
  def __init__(self, subSys, log_dir):
    # Log files names dictionary
    self.log_files_dic = {cfg.ISC : cfg.ISC_PRCS_LOG_FILENAME, 
                          cfg.SHMEM_REC  : cfg.SHMEM_REC_THRD_LOG_FILENAME, 
                          cfg.SHMEM_XMIT : cfg.SHMEM_XMIT_THRD_LOG_FILENAME, 
                          cfg.SCKT_SERVER : cfg.SCKT_SERVER_THRD_LOG_FILENAME, 
                          cfg.SCKT_CLIENT : cfg.SCKT_CLIENT_THRD_LOG_FILENAME, 
                          cfg.PROD : cfg.PROD_PRCS_LOG_FILENAME, 
                          cfg.CONS : cfg.CONS_PRCS_LOG_FILENAME}
    # Subsystem should be one of the defined subsystems in Config file
    self.subSys = subSys
    self.log_dir = log_dir
    # Check the validity of the Subsystem
    try:
      if self.subSys == cfg.ISC:
        self.payloadFunName = ''
      elif self.subSys == cfg.SHMEM_REC:
        self.payloadFunName = 'shmem_rec'
      elif self.subSys == cfg.SHMEM_XMIT:
        self.payloadFunName = 'shmem_xmit'
      elif self.subSys == cfg.SCKT_SERVER:
        self.payloadFunName = 'sckt_server'
      elif self.subSys == cfg.SCKT_CLIENT:
        self.payloadFunName = 'sckt_client'
      elif self.subSys == cfg.PROD:
        self.payloadFunName = 'producer'
      elif self.subSys == cfg.CONS:
        self.payloadFunName = 'consumer'
      else:
        raise Subsystemerror("Error: Log_File_Parser : incorrect subsystem!")
    except Subsystemerror as e:
      print (e.args)
    # List of events (during program run time) dictionary
    self.eventDicList = []
    # List of occurred errors (during program run time) dictionary
    self.errorDicList = []
    # List of occurred warnings (during program run time) dictionary
    self.warningDicList = []
    # List of data payload dictionary
    self.payloadDicList = []
    # Packet number
    self.packet_num = 0


  ######################################################################
  # F I N D   V A L I D   L I N E S   B Y   D A T E   F O R M A T
  ######################################################################
  def matchDate(self, line):
    matchThis = ""
    # define regular expression to match the date
    # date format example: '2020-06-18 03:25:53,180'
    matched = re.match(r'\d\d\d\d-\d\d-\d\d\ \d\d:\d\d:\d\d,\d\d\d', line)
    if matched:
      # matches a date and adds it to matchThis            
      matchThis = matched.group() 
    else:
      matchThis = "NONE"
    return matchThis


  ######################################################################
  # S P L I T   U P   T H E   L O G   F I L E
  ######################################################################
  def generateDicts(self, log_fh):
    currentDict = {}
    for line in log_fh:
      if line.startswith(self.matchDate(line)):
        if currentDict:
          yield currentDict
        # 'date': timestamp
        # 'type': type of message
        # 'fun' : type of message
        # 'msg' : message body
        # Example:
        #   {'fun': 'ipc_init', 'date': '2020-07-19 01:21:49,120', 'msg': 'shmem:ipc_init...', 'type': 'INFO'}
        currentDict = {"date":line.split("__")[0][:23], 
                       "type":line.split("-",5)[3].strip(), 
                       "fun":line.split("-",5)[4].strip(), 
                       "msg":line.split("-",5)[-1].strip()}
    yield currentDict

  ######################################################################
  # G E N E R A T E   E V E N T   D I C T I O N A R I E S
  ######################################################################
  def generateEventDicts(self, mainList):
    currentDict = {}
    for n in mainList:
      if currentDict:
        yield currentDict
      timestampDict = {}
      # Extract the timestamp
      # Y : Year; M : Month; D : Day; H : Hour; M : Minute; S : Second; MS : Mili-Second;
      timestampDict = {"year":int(n.get('date').split("-",3)[0]),
                       "month":int(n.get('date').split("-",3)[1]),
                       "day":int(n.get('date').split("-",3)[2][0:2]),
                       "hour":int(n.get('date').split(" ",2)[1].split(":",3)[0]),
                       "minute":int(n.get('date').split(" ",2)[1].split(":",3)[1]),
                       "second":int(n.get('date').split(" ",2)[1].split(":",3)[2][0:2]),
                       "microsecond":int(n.get('date').split(",",2)[1])}
      # Extract the packet index number and convert it into integer
      msg_type = n.get('type')
      fun_type = n.get('fun')
      if self.subSys == cfg.SHMEM_XMIT or self.subSys == cfg.SHMEM_REC:
        msg_body = ''
      else:
        msg_body = n.get('msg').split("-",2)[0].strip()
      currentDict = {"timestamp":timestampDict,
                     "type":msg_type,
                     "fun":fun_type,
                     "msg_body":msg_body}
    yield currentDict

  ######################################################################
  # G E N E R A T E   P A Y L O A D   D I C T I O N A R I E S
  ######################################################################
  def generatePayloadDicts(self, mainList):
    currentDict = {}
    dicList = []
    for n in mainList:
#      if currentDict:
#        yield currentDict

#      print(n)
      timestampDict = {}
      # Extract the timestamp
      # Y : Year; M : Month; D : Day; H : Hour; M : Minute; S : Second; MS : Micro-Second;
      timestampDict = {"year":int(n.get('date').split("-",3)[0]),
                       "month":int(n.get('date').split("-",3)[1]),
                       "day":int(n.get('date').split("-",3)[2][0:2]),
                       "hour":int(n.get('date').split(" ",2)[1].split(":",3)[0]),
                       "minute":int(n.get('date').split(" ",2)[1].split(":",3)[1]),
                       "second":int(n.get('date').split(" ",2)[1].split(":",3)[2][0:2]),
                       "microsecond":int(n.get('date').split(",",2)[1])}
      # Time reference
      # ToDo: Adding microsecond to timeref might need another scaling!
      timeref = int(timestampDict.get("month")*2.628e+9 + \
                timestampDict.get("day")*8.64e+7 + \
                timestampDict.get("hour")*3.6e+6 + \
                timestampDict.get("minute")*60000 + \
                timestampDict.get("second")*1000 + \
                timestampDict.get("microsecond"))
#      print(timeref)

      if self.subSys == cfg.PROD:
#        print(n.get('msg').split("-",2)[0])          
        # Extract the packet index number and convert it into integer
        if ('Xmited' in n.get('msg').split("-",2)[0]):
          self.packet_num = int(n.get('msg').split("-",2)[0].strip().split("(",2)[1][:-1])
        else:
          continue

      elif self.subSys == cfg.CONS:
#        print(n.get('msg').split("-",2)[0])          
        # Extract the packet index number and convert it into integer
        if ('Reced' in n.get('msg').split("-",2)[0]):
          self.packet_num = int(n.get('msg').split("-",2)[0].strip().split("(",2)[1][:-1])
        else:
          continue

      # Extract the data payload string
      payload_str = n.get('msg').split("-",2)[-1].strip().split(",")
      payload_int = []
      for m in payload_str:
        if m[0:2] == '0x':
          payload_int.append(int(m, 16))

      currentDict = {"timestamp":timestampDict,
                     "timeref":timeref,
                     "subsys":self.subSys,
                     "pkt_num":self.packet_num,
                     "payload":payload_int,
                     "procesed":cfg.NO}
#      print(currentDict)
      dicList.append(currentDict)

      self.packet_num += 1

#    yield currentDict
    return dicList

  ######################################################################
  # P A R S E   A   L O G   F I L E
  ######################################################################
  def parseLogFile(self):
    try:
      logDicList = []
      with open('{}/{}'.format(self.log_dir, self.log_files_dic.get(self.subSys))) as f:
      	# Extract the valid lines from log file and put them into a log dictionary list
        logDicList = list(self.generateDicts(f))
#        print(logDicList)

        # Extract list of events with integer timestamp
        self.eventDicList = list(self.generateEventDicts(logDicList))

        # Extract list of occured errors with integer timestamp
        self.errorDicList = filter(lambda n: n.get('type') == 'ERROR', self.eventDicList)

        # Extract list of warnings with integer timestamp
        self.warningDicList = filter(lambda n: n.get('type') == 'WARNING', self.eventDicList)

        # Extract data payload from the log dictionary list
        if self.subSys == cfg.PROD or \
           self.subSys == cfg.CONS or \
           self.subSys == cfg.SCKT_SERVER or \
           self.subSys == cfg.SCKT_CLIENT or \
           self.subSys == cfg.SHMEM_XMIT or \
           self.subSys == cfg.SHMEM_REC:

          # Filter out only the data payload segment
          transactionList = filter(lambda n: n.get('fun') == self.payloadFunName, logDicList)

          # Extract the raw data payload and convert date into timestamp
          self.payloadDicList = self.generatePayloadDicts(transactionList)
        
    except IOError as e:
      print('Error: parseLogFile : can\'t find file or read data!')
    except Exception as e:
      print('Error: parseLogFile : an exception happened!')
