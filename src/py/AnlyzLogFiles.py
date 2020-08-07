'''
 * @file   AnlyzLogFiles.py
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   03 August 2020
 * @version 0.1
 * @brief   This Python script parses and analyzes the ISC log files. 
 *          It writes its report into report_dataflow.log file. 
 *          It draws the time flow in a graph as well.
'''

from __future__ import print_function,unicode_literals
from operator import itemgetter
import Config as cfg
from LogFileParser import Log_File_Parser
import matplotlib.pyplot as plt

# NOTE: Some parts of this script need to be carefully debugged.

######################################################################
# G E N E R A T E   P A Y L O A D   D I C T I O N A R I E S
######################################################################
def generateDataflowDicts(prod, cons, xmit, rec):
  # Concatenate all lists from producer, consumer, xmitter, and receiver
  currentListDic = prod + cons + xmit + rec
  # Sort the list of dictionaries by 'timeref' key
  currentListDic = sorted(currentListDic, key=itemgetter('timeref'))
#  print('=currentListDic==============================')
#  print(currentListDic)
#  print('=currentListDic==============================')
  return currentListDic


########################################################################
# M A I N   P R O C E D U R E
########################################################################
if __name__ == '__main__':
  # ----------------------------------------------
  # Parsing the producer log file
  # ----------------------------------------------
  lfp_prod = Log_File_Parser(cfg.PROD, cfg.HOST_CLIENT_LOG_DIR)
  lfp_prod.parseLogFile()
#  print([n for n in lfp_prod.payloadDicList])


  # ----------------------------------------------
  # Parsing the consumer log file
  # ----------------------------------------------
  lfp_cons = Log_File_Parser(cfg.CONS, cfg.HOST_SERVER_LOG_DIR)
  lfp_cons.parseLogFile()
#  print([n for n in lfp_cons.payloadDicList])


  # ----------------------------------------------
  # Parsing the isc log files
  # ----------------------------------------------
  lfp_isc_client = Log_File_Parser(cfg.ISC, cfg.HOST_CLIENT_LOG_DIR)
  lfp_isc_client.parseLogFile()
#  print([n for n in lfp_isc_client.payloadDicList])


  lfp_isc_server = Log_File_Parser(cfg.ISC, cfg.HOST_SERVER_LOG_DIR)
  lfp_isc_server.parseLogFile()
#  print([n for n in lfp_isc_server.payloadDicList])


  # ----------------------------------------------
  # Parsing the shared memory receiver log file
  # ----------------------------------------------
  lfp_isc_shmem_rec = Log_File_Parser(cfg.SHMEM_REC, cfg.HOST_SERVER_LOG_DIR)
  lfp_isc_shmem_rec.parseLogFile()
#  print([n for n in lfp_isc_shmem_rec.payloadDicList])


  # ----------------------------------------------
  # Parsing the shared memory transmitter log file
  # ----------------------------------------------
  lfp_isc_shmem_xmit = Log_File_Parser(cfg.SHMEM_XMIT, cfg.HOST_CLIENT_LOG_DIR)
  lfp_isc_shmem_xmit.parseLogFile()
#  print([n for n in lfp_isc_shmem_xmit.payloadDicList])


  # List of all transactions between transmitter/receiver nodes
  dataflowDicList = generateDataflowDicts(lfp_prod.payloadDicList, 
                                          lfp_cons.payloadDicList, 
                                          lfp_isc_shmem_xmit.payloadDicList, 
                                          lfp_isc_shmem_rec.payloadDicList)


  # write report into output file
  with open(cfg.OUT_DATAFLOW_FILENAME, "w") as out_file:
    out_file.write('########################################################################\n')
    out_file.write('# E r r o r   a n d   w a r n i n g s   r e p o r t\n')
    out_file.write('########################################################################\n')

    out_file.write('\n')

    out_file.write('-------------------------------------\n')
    out_file.write(' P r o d u c e r   S u b s y s t e m\n')
    out_file.write('-------------------------------------\n')

    out_file.write('Errors:')
    if len(list(filter(None, lfp_prod.errorDicList))) > 0:
      out_file.write([n for n in lfp_prod.errorDicList])
    else:
      out_file.write('None.')

    out_file.write('\nWarnings:')
    if len(list(filter(None, lfp_prod.warningDicList))) > 0:
      out_file.write([n for n in lfp_prod.warningDicList])
    else:
      out_file.write('None.')

    out_file.write('\n')

    out_file.write('-------------------------------------\n')
    out_file.write(' C o n s u m e r   S u b s y s t e m\n')
    out_file.write('-------------------------------------\n')

    out_file.write('Errors:')
    if len(list(filter(None, lfp_cons.errorDicList))) > 0:
      out_file.write([n for n in lfp_cons.errorDicList])
    else:
      out_file.write('None.')

    out_file.write('\nWarnings:')
    if len(list(filter(None, lfp_cons.warningDicList))) > 0:
      out_file.write([n for n in lfp_cons.warningDicList])
    else:
      out_file.write('None.')

    out_file.write('\n')

    out_file.write('------------------------------------------\n')
    out_file.write(' I S C   C l i e n t   S u b s y s t e m\n')
    out_file.write('------------------------------------------\n')

    out_file.write('Errors:')
    if len(list(filter(None, lfp_isc_client.errorDicList))) > 0:
      out_file.write([n for n in lfp_isc_client.errorDicList])
    else:
      out_file.write('None.')

    out_file.write('\nWarnings:')
    if len(list(filter(None, lfp_isc_client.warningDicList))) > 0:
      out_file.write([n for n in lfp_isc_client.warningDicList])
    else:
      out_file.write('None.')

    out_file.write('\n')

    out_file.write('------------------------------------------\n')
    out_file.write(' I S C   S e r v e r   S u b s y s t e m\n')
    out_file.write('------------------------------------------\n')

    out_file.write('Errors:')
    if len(list(filter(None, lfp_isc_server.errorDicList))) > 0:
      out_file.write([n for n in lfp_isc_server.errorDicList])
    else:
      out_file.write('None.')

    out_file.write('\nWarnings:')
    if len(list(filter(None, lfp_isc_server.warningDicList))) > 0:
      out_file.write([n for n in lfp_isc_server.warningDicList])
    else:
      out_file.write('None.')

    out_file.write('\n')

    out_file.write('-----------------------------------------\n')
    out_file.write(' I S C   S H M E M   R E C   T h r e a d\n')
    out_file.write('-----------------------------------------\n')

    out_file.write('Errors:')
    if len(list(filter(None, lfp_isc_shmem_rec.errorDicList))) > 0:
      out_file.write([n for n in lfp_isc_shmem_rec.errorDicList])
    else:
      out_file.write('None.')

    out_file.write('\nWarnings:')
    if len(list(filter(None, lfp_isc_shmem_rec.warningDicList))) > 0:
      out_file.write([n for n in lfp_isc_shmem_rec.warningDicList])
    else:
      out_file.write('None.')

    out_file.write('\n')

    out_file.write('-------------------------------------------\n')
    out_file.write(' I S C   S H M E M   X M I T   T h r e a d\n')
    out_file.write('-------------------------------------------\n')

    out_file.write('Errors:')
    if len(list(filter(None, lfp_isc_shmem_xmit.errorDicList))) > 0:
      out_file.write([n for n in lfp_isc_shmem_xmit.errorDicList])
    else:
      out_file.write('None.')

    out_file.write('\nWarnings:')
    if len(list(filter(None, lfp_isc_shmem_xmit.warningDicList))) > 0:
      out_file.write([n for n in lfp_isc_shmem_xmit.warningDicList])
    else:
      out_file.write('None.')


    out_file.write('\n\n')


    out_file.write('########################################################################\n')
    out_file.write('# D a t a f l o w   s e q u e n c e\n')
    out_file.write('########################################################################\n')

    #  print(dataflowDicList)
    # Create table header
    out_file.write('\n{:>4} | {:^20} | {:^12} | {:^8} | {:^10} |'.format('Num', 'Timestamp', 'Subsys', 'Packect#', 'PayloadLen'))
    idx = 0
    x = []
    y = []
    first_time = 1
    x_init = 0
    fig = plt.figure()
    ax1 = plt.subplot2grid((1,1), (0,0))
    for label in ax1.xaxis.get_ticklabels():
      label.set_rotation(45)
    ax1.grid(True, color='g', linestyle='-', linewidth='1')
    ax1.xaxis.label.set_color('c')
    ax1.yaxis.label.set_color('r')
    ax1.set_yticks([0,25,50,75])
    ax1.spines['left'].set_color('c')
    ax1.spines['right'].set_visible(False)
    ax1.spines['top'].set_visible(False)
    ax1.spines['left'].set_linewidth(2)
    ax1.tick_params(axis='x', colors='#f06215')

    for n in dataflowDicList:
      out_file.write('\n{:>4} | {}.{}.{} {}:{}:{},{} | {:^12} | {:^8} | {:^10} |'.format(
                     idx,
                     n.get('timestamp').get('year'), 
                     n.get('timestamp').get('month'), 
                     n.get('timestamp').get('day'), 
                     n.get('timestamp').get('hour'), 
                     n.get('timestamp').get('minute'), 
                     n.get('timestamp').get('second'), 
                     n.get('timestamp').get('microsecond'), 
                     n.get('subsys'),
                     n.get('pkt_num'),
                     len(n.get('payload')))
                    )
      if first_time == 0:
        y.append(idx)
        x.append(n.get('timeref')-x_init)
      else:
        x_init = n.get('timeref')
      y.append(idx+1)
      x.append(n.get('timeref')-x_init)
      # Annotation example with arrow
      ax1.annotate(n.get('subsys'), (x[-1], y[-1]),
                    xytext=(0.1+idx*0.07, 0.1+idx*0.09), textcoords='axes fraction',
                    arrowprops = dict(arrowstyle="->", facecolor='grey', color='grey',connectionstyle="angle3"))
      first_time = 0
      idx += 1

    plt.plot(x,y,label='TFD')
    plt.xlabel('Timestamp')
    plt.ylabel('Subsystem')
    plt.title('Time flow diagram\nInter SoC Communication system')
    plt.legend()
    plt.show()

    out_file.write('\n\n')


    out_file.write('########################################################################\n')
    out_file.write('# P a y l o a d   m a t c h\n')
    out_file.write('########################################################################\n')

    out_file.write('\n')

    out_file.write('-------------------------------------\n')
    out_file.write(' P r o d u c e r   <>   X m i t t e r\n')
    out_file.write('-------------------------------------\n')

    prod_xmit_pair = {}
    out_file.write('ToDo: This section needs further debugging!\n')
    
    '''

    out_file.write('\n{:>4} | {:^21} | {:^8} | {:^21} | {:^8} | {:^15} |'.format('Num', 'Producer Timestamp', 'Packect#', 'Xmitter Timestamp', 'Packect#', 'Payload Match'))
    idx = 0
    for n in dataflowDicList:
      if n.get('subsys') == cfg.PROD and n.get('procesed') == cfg.NO:
        n['procesed'] = cfg.YES
        out_file.write('\n{:>4} | {}.{}.{} {}:{}:{},{} | {:^8} |'.format(
                     idx,
                     n.get('timestamp').get('year'), 
                     n.get('timestamp').get('month'), 
                     n.get('timestamp').get('day'), 
                     n.get('timestamp').get('hour'), 
                     n.get('timestamp').get('minute'), 
                     n.get('timestamp').get('second'), 
                     n.get('timestamp').get('microsecond'), 
                     n.get('pkt_num'))
                    )

        for m in dataflowDicList:
          if m.get('subsys') == cfg.SHMEM_XMIT and m.get('timeref') > n.get('timeref') and m.get('procesed') == cfg.NO:
            m['procesed'] = cfg.YES
            out_file.write(' {}.{}.{} {}:{}:{},{} | {:^8} |'.format(
                     m.get('timestamp').get('year'), 
                     m.get('timestamp').get('month'), 
                     m.get('timestamp').get('day'), 
                     m.get('timestamp').get('hour'), 
                     m.get('timestamp').get('minute'), 
                     m.get('timestamp').get('second'), 
                     m.get('timestamp').get('microsecond'), 
                     m.get('pkt_num'))
                    )
            if m.get('payload') == n.get('payload'):
              out_file.write(' {:^15} |'.format('Passed'))
              prod_xmit_pair[str(idx)] = (n,m,'Passed')
            else:
              out_file.write(' {:^15} |'.format('Failed'))
              prod_xmit_pair[str(idx)] = (n,m,'Failed')
            break
        idx += 1

    out_file.write('\n')

    for key in prod_xmit_pair:
      (n,m,res) = prod_xmit_pair[key]
      if res == 'Failed':
        out_file.write('\nPayload values of producer packet#{}:\n'.format(n.get('pkt_num')))
        for i in n.get('payload'):
          out_file.write(' {},'.format(i))
        out_file.write('\n\nPayload values of xmitter packet#{}:\n'.format(m.get('pkt_num')))
        for i in m.get('payload'):
          out_file.write(' {},'.format(i))

        out_file.write('\n\n')

    '''

    out_file.write('\n\n')


    out_file.write('--------------------------------------\n')
    out_file.write(' R e c e i v e r  <>   C o n s u m e r\n')
    out_file.write('--------------------------------------\n')

    rec_cons_pair = {}

    out_file.write('ToDo: This section needs further debugging!\n')
    
    '''

    out_file.write('\n{:>4} | {:^21} | {:^8} | {:^21} | {:^8} | {:^15} |'.format('Num', 'Rec Timestamp', 'Packect#', 'Consumer Timestamp', 'Packect#', 'Payload Match'))
    idx = 0
    for n in dataflowDicList:
      if n.get('subsys') == cfg.SHMEM_REC and n.get('procesed') == cfg.NO:
        n['procesed'] = cfg.YES
        out_file.write('\n{:>4} | {}.{}.{} {}:{}:{},{} | {:^8} |'.format(
                     idx,
                     n.get('timestamp').get('year'), 
                     n.get('timestamp').get('month'), 
                     n.get('timestamp').get('day'), 
                     n.get('timestamp').get('hour'), 
                     n.get('timestamp').get('minute'), 
                     n.get('timestamp').get('second'), 
                     n.get('timestamp').get('microsecond'), 
                     n.get('pkt_num'))
                    )

        for m in dataflowDicList:
          if m.get('subsys') == cfg.CONS and m.get('timeref') > n.get('timeref') and m.get('procesed') == cfg.NO:
            m['procesed'] = cfg.YES
            out_file.write(' {}.{}.{} {}:{}:{},{} | {:^8} |'.format(
                     m.get('timestamp').get('year'), 
                     m.get('timestamp').get('month'), 
                     m.get('timestamp').get('day'), 
                     m.get('timestamp').get('hour'), 
                     m.get('timestamp').get('minute'), 
                     m.get('timestamp').get('second'), 
                     m.get('timestamp').get('microsecond'), 
                     m.get('pkt_num'))
                    )
            if m.get('payload') == n.get('payload'):
              out_file.write(' {:^15} |'.format('Passed'))
              rec_cons_pair[str(idx)] = (n,m,'Passed')
            else:
              out_file.write(' {:^15} |'.format('Failed'))
              rec_cons_pair[str(idx)] = (n,m,'Failed')
            break
        idx += 1

    out_file.write('\n')

    for key in rec_cons_pair:
      (n,m,res) = rec_cons_pair[key]
      if res == 'Failed':
        out_file.write('\nPayload values of receiver packet#{}:\n'.format(n.get('pkt_num')))
        for i in n.get('payload'):
          out_file.write(' {},'.format(i))
        out_file.write('\n\nPayload values of consumer packet#{}:\n'.format(m.get('pkt_num')))
        for i in m.get('payload'):
          out_file.write(' {},'.format(i))

        out_file.write('\n\n')
    '''
