from Queue import Queue
from CSUtils import DA
import time 
import array
import exceptions
from threading import Thread
try:
    from common import *
except ImportError:
    pass



#######################
#    TARGET THREAD    #
#######################

class Target(Thread):
    '''Target thread which constantly checks the META and the requests queue for new messages and reads/writes any new messages to appropriate locations.
       All calls on DA functions must be completed within this thread to ensure that the system is thread safe'''

############################################################################################################################################
    def __init__(self, requests_queue, replies_queue, common, tlf, loader):
        
        '''Initialisation, starting the exception thread for clean closing'''
        Thread.__init__(self)
        self.replies_queue = replies_queue
        self.requests_queue = requests_queue
        self.time = time.time
        self.sleep = time.sleep
        self.tlfObj = tlf
        self.exception = False
        self.closing = False
        self.Buffer = False
        self.load_queue = loader
        self.writelong = DA.WriteLong
        self.readlong = DA.ReadLong
        self.staticVariables = array.array('L',[0x02000430, 0x02000434, 0x02000438, 0x0200043C, 0x80000000, 0xFF000000, 0x00FFFFFF,
                               0x81000000, 0x82000000, 0x83000000, 0x84000000, 0x85000000, 0x86000000, 0xDEADDEAD])
        self.timeouts = True
        global hang
        hang = False
        global loop
        loop = 1
##############################################################################################################################################
    
    def run(self):
        
        '''messages from meta read and put in replies queue
        messages from requests queue written to meta (no more than 2 at once)
        continuously loops until end message is sent'''
        if not self.Buffer:
            check = self.readlong(self.staticVariables[1])
            while check & self.staticVariables[5] != self.staticVariables[7]:
                if self.exception or self.closing:
                    return
                check = self.readlong(self.staticVariables[1])
            msglen = check & self.staticVariables[6]
            self.writelong(self.staticVariables[2], self.staticVariables[4])
            msglist = []
            i = 0
            while i < msglen:
                check = self.readlong(self.staticVariables[1])
                if check & self.staticVariables[5] != self.staticVariables[8]:
                    if self.exception or self.closing:
                        return
                    check = self.readlong(self.staticVariables[1])
                msgpay = check & self.staticVariables[6]
                msglist.append(msgpay)
                self.writelong(self.staticVariables[2], self.staticVariables[4])
                i += 3
            self.replies_queue.put(msglist)
            self.ActivateBuffer()
        while loop != 0:
            i=0
            if self.exception or self.closing:
                return
            while hang:
                    if self.exception or self.closing:
                        return
                    self.sleep(0.001)
            check = self.readlong(self.staticVariables[1])           #Get message from host port
            while (check & self.staticVariables[4]) != 0:
                while hang:
                    if self.exception or self.closing:
                        return
                    self.sleep(0.001)
                self.replies_queue.put(self.read_msg())               #Give message to replies queue        
                check=self.readlong(self.staticVariables[1])
                if self.exception or self.closing:
                    return
            while not self.requests_queue.empty():
                if i<1:                          
                    if self.exception:
                        return
                    msg=self.requests_queue.get()
                    twrite = self.time()
                    self.requests_queue.task_done()             #Get message from requests queue
                    self.write_msg(msg)                         #Write message to Meta
                    telaps = self.time() - twrite
                    if self.timeouts and (telaps > 30):
                        raise(GenericError('Timeout waiting for response to written message',))
                    i+=1
                else:
                    break

#############################################################################################################################################################

    ##############
    #    LOAD    #
    ##############
    def Load_Program(self, loaded):
        '''Loads a program to the board'''
        global hang
        items = self.load_queue.get()   #If load is sent then reset target and load code
        if items != 'common':           #'common' indicates the script included with this file when the testsystem is being used
            DA.UseTarget(items[1])      #If a common object is not available, it assumes it has been given file and DA ID values
        else:
            DA.UseTarget(common['dashId'])
        if loaded == True:
            hang = True
        self.flush_meta()
        self.tlfObj.record('host_port_control:Received load code command', 1, 1)
        DA.HardReset()
        self.tlfObj.record('host_port_control: resetTarget: HardReset first target: ', 1, 1)
        try:
            if items == 'common':
                DA.LoadProgramFileEx(common['js'], True) # ShowProgress=True as this seems to make the load more reliable
                self.tlfObj.record('host_port_control: code ' + common['js'] + ' loaded', 1, 1)
            else:
                DA.LoadProgramFileEx(items[0], True)
                self.tlfObj.record('host_port_control: code ' + items[0] + 'loaded', 1, 1)
        except:
            self.tlfObj.record('host_port_control:ERROR: loadCodeDA Error: loadcodeDAfailed failure', 1, 1)
            raise(GenericError(('Loading program :', items[0], 'To target :', items[1], 'Failed')))
            DA.HardReset()
            return
        try:
            DA.RunTarget(DA.GetFirstThread())
            try:
                self.tlfObj.record('host_port_control: code ' +common['js'] + ' loaded and running' , 1, 1)
            except:
                self.tlfObj.record('host_port_control: code ' + items[0] + 'loaded and running', 1, 1)
           
        except:
            try:
                self.tlfObj.record('host_port_control: code ' + common['js'] + 'failed to run', 1, 0)
            except:
                self.tlfObj.record('host_port_control: code ' + items[0] + 'failed to run', 1, 0)
                self.tlfObj.exit('DA Failed to run')
            raise(GenericError(('Loading program :', items[0], 'To target :', items[1], 'Failed')))
            return
        
        #clear initial ready message
        self.load_queue.put('loaded') 
        ready = self.readlong(self.staticVariables[1])
        t = self.time()
        
        while ready & self.staticVariables[4] == 0:
            if self.exception:
                return
            ready = self.readlong(self.staticVariables[1])
            telaps = self.time() - t
            if self.timeouts and (telaps > 10):
                self.tlfObj.record('host_port_control: ERROR: time out waiting for asyncronous ready message', 1, 1)
                raise(GenericError('Time out waiting for host_port ready response',))
        if (ready & self.staticVariables[5]) != self.staticVariables[9]:
            self.tlfObj.record( 'host_port_control: ERROR: The ready message has not been read', 1, 1)
            exception_queue.put(self.staticVariables[13])
        else:
            self.writelong(self.staticVariables[2], self.staticVariables[4])
            if loaded:
                check = self.readlong(self.staticVariables[1])
                while check & self.staticVariables[5] != self.staticVariables[7]:
                    if self.exception or self.closing:
                        return
                    self.sleep(0.001)
                    check = self.readlong(self.staticVariables[1])
                msglen = check & self.staticVariables[6]
                self.writelong(self.staticVariables[2], self.staticVariables[4])
                msglist = []
                i = 0
                while i < msglen:
                    check = self.readlong(self.staticVariables[1])
                    if check & self.staticVariables[5] != self.staticVariables[8]:
                        if self.exception or self.closing:
                            return
                        check = self.readlong(self.staticVariables[1])
                    msgpay = check & self.staticVariables[6]
                    msglist.append(msgpay)
                    self.writelong(self.staticVariables[2], self.staticVariables[4])
                    i += 3
                self.replies_queue.put(msglist)
                self.ActivateBuffer()
                hang = False
                
###############################################################################################################################################################
    ####################
    #    FLUSH META    #
    ####################
    def flush_meta(self):
        while (self.readlong(self.staticVariables[1])&0xf0000000) != 0x0:
            self.writelong(self.staticVariables[2], self.staticVariables[4])
            self.sleep(0.001)
        


###############################################################################################################################################################
    ####################
    #    END TARGET    #
    ####################
    def end_target(self):
        self.closing = True
        global loop
        loop = 0
        while self.isAlive():
            if self.exception:
                return
            self.sleep(0.001)
        self.replies_queue.put('end')
        
###############################################################################################################################################################

    #########################
    #    ACTIVATE_BUFFER    #
    #########################
    def ActivateBuffer(self):
        self.writelong(self.staticVariables[0], self.staticVariables[10])
        Bufloc = self.readlong(self.staticVariables[1])
        while (Bufloc & self.staticVariables[10] != self.staticVariables[10]) or (Bufloc & 0x8f000000 == self.staticVariables[12]):
            self.writelong(self.staticVariables[2], self.staticVariables[4])
            if self.exception:
                return
            self.writelong(self.staticVariables[0], self.staticVariables[10])
            Bufloc = self.readlong(self.staticVariables[1])
        self.BufferH = 0xB7000000 + 3*(Bufloc & 0xffffff)
        self.writelong(self.staticVariables[2], self.staticVariables[4])
        self.writelong(self.staticVariables[0], self.staticVariables[11])
        Bufloc = self.readlong(self.staticVariables[1])
        while Bufloc & self.staticVariables[11] != self.staticVariables[11]:
            self.writelong(self.staticVariables[2], self.staticVariables[4])
            if self.exception:
                return
            self.writelong(self.staticVariables[0], self.staticVariables[11])
            Bufloc = self.readlong(self.staticVariables[1])
        self.writelong(self.staticVariables[2], self.staticVariables[4])
        self.BufferU = 0xB7000000 + 3*(Bufloc & 0xffffff)
        if not self.Buffer:
            self.writelong(self.staticVariables[0], self.staticVariables[12])
            Conf = self.read_msg()
        return True
    
    
###############################################################################################################################################################

    ######################
    #    READ MESSAGE    #
    ######################
    def read_msg (self):        
        
        '''reads message from the meta'''
        msglen = self.readlong(self.BufferU)
        msg = []
        i = 0
        while i <= msglen:
            line = self.readlong(self.BufferU + i)
            msg.append(line)
            i += 4
        self.writelong(self.staticVariables[2], self.staticVariables[4])
        return msg
############################################################################################################################################################### 

    #######################
    #    WRITE MESSAGE    #
    #######################
    def write_msg(self, message):
        '''writes message to meta'''

        #check message is being given in correct format

        if isinstance (message, tuple) ==False:
            self.tlfObj.record( 'host_port_control: ERROR: The message is not in a valid format for the write function.', 1, 1)
            sys.exit()
    
        #calculate size of the message to be sent
        size = int(message[0])
   
        #create a start message which contains the size calculated above
        command = (size+4 & self.staticVariables[6]) |  self.staticVariables[12]
        
        #Procedurally write message to buffers.
        i = 0
        for item in message:
            self.writelong(self.BufferH + i, item)
            i += 4
        
        if not self.check_ready(10):
            return
        #write start message to register
        self.writelong(self.staticVariables[0], command)
    
        #write data message to register
        return True
        
###############################################################################################################################################################

    ############################
    #    CHECK INTERUPT SET    #
    ############################
    def check_ready(self, totm):
        '''Checks the interrupt is clear'''
        time_start = self.time()
        devicecommand = self.readlong(self.staticVariables[0])
        while (devicecommand & self.staticVariables[4]) != 0:
            if self.exception or hang:
                return
            if self.timeouts and (self.time() - time_start > totm):
                raise GenericError('Time out waiting for interrupt to clear')
                return False
            self.sleep(0.001)
        return True
    
###############################################################################################################################################################

    ###########################
    #    EXCEPTION HANDLER    #
    ###########################
    def exception_handler(self):
        self.exception = True
        while self.is_alive():
            self.exception = True
            global loop
            loop = 0
            self.sleep(0.001)
        return True
###############################################################################################################################################################

    #######################
    #    PAUSE/UNPAUSE    #
    #######################
    def Pause_target(self):
        global hang
        hang = True
        return True
        
    def Unpause_target(self):
        global hang
        hang = False
        return True
    
###############################################################################################################################################################

    #########################
    #    GET TARGET LIST    #
    #########################
    def getTargetList(self):
        targlist = []
        print 'Showing targets'
        curtarg = DA.GetFirstTarget()
        num = 1
        while curtarg:
            targinf = DA.GetTargetInfo(curtarg)
            targinf = targinf.split()
            daid = targinf[0] + ' ' + targinf[1].lstrip('0')
            self.tlfObj.record('Target: '+ str(num) + '  Info: ' + ' '.join(targinf) + '  ID: ' + daid, 0, 0 )
            num+=1
            if daid not in targlist:
                targlist.append(daid)
            curtarg = DA.GetNextTarget(curtarg)
        return targlist
            

###############################################################################################################################################################

    #######################
    #    TIMEOUT ERROR    #
    #######################
class GenericError(exceptions.Exception):
    def __init__(self, Descript):
        self.Descript = Descript
    def __str__(self):
        return self.Descript


###############################################################################################################################################################