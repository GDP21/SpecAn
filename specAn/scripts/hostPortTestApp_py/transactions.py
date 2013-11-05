from host_port_control import Target, GenericError
import threading
import time
import struct
import random
from Queue import Queue
try:
    from common import *
except ImportError:
    pass

class Messages():

    def __init__(self, common = None, GUI = False):
        '''the constructor'''
        self.async_queue = Queue()
        self.replies_queue = Queue()
        self.requests_queue = Queue()
        self.time = time.time
        self.sleep =time.sleep
        self.load_queue = Queue()
        self.IdLi = []
        self.CurId = 0
        self.DoneFlag = True
        self.closing = False
        self.exception = False
        self.timeouts = True
        self.GUI = GUI
        self.loaded = False
        self.message_id = 0                        #increment the message function by 1 every time a message is sent but not above 0x7F        global sourceID
        self.common = common
        try:
            self.tlfObj = common['tlfObj']
        except:
            self.tlfObj = Tlf_Fake()
        self.__targ = Target(self.requests_queue, self.replies_queue, 0, self.tlfObj, self.load_queue)
        self.__targ.daemon = True
        self.Idhandle = threading.Thread(target = self.__IdSort)
        self.Idhandle.daemon = True
        self.Idhandle.start()
    #################################################
    #   Interpretation of read message #
    #################################################
####################################################################################################################################################
    def __interpret_read(self, message):
        '''interprets the decoded message to only return necessary values and comments'''
        try:
            msgfunc = message[4]
            if (msgfunc == 7)and(message[3]==0x10000):
                return (message[6], message[7])
            elif msgfunc == 1:                                                    #Ready message type
                if message [6] != 0:
                    self.tlfObj.record( 'transactions: Messages lost since last ready message:' + message[6], 1, 1)    #Provides a count of lost async messages
                return True
            elif msgfunc ==3:
                if message[1] !=0:
                    self.tlfObj.record( 'Activated', 1, 1)                                           #response to TV_ACTIVATE message need to return source ID as well as this is used in other messages
                    return message[1]
                else:
                    return 0
            elif msgfunc ==5:
                    if message[1] != 0:
                        return True                                        #response to TV_DEACTIVATE message
                    else:
                        return False
            elif msgfunc == 7:
                return message[7]
            elif hex(msgfunc) == 0xffffffff:
                return False                                     #Error message response to invalid message
            else:
                return False
        except Exception, e:
            self.tlfObj.record(str(e), 0, 0)
            return False
####################################################################################################################################################

    def __CheckAsync(self, message):
        try:
            if message[3] == 0x10000:
                self.async_queue.put(self.__interpret_read(message))
                return True
            return False
        except IndexError:
            return True

####################################################################################################################################################

    def img_tv_ping(self):
        '''sends ping message'''
        Id = self.__IdGen()
        while Id != self.CurId:
            if self.CurId == 0x5e4000:
                return 0
            self.sleep(0.001)
        Ping = (0x14, 0x0, 0x0, self.message_id, 0x0, 0x0)
        if self.message_id >=27:
            self.message_id = 0
        else:
            self.message_id+=1
        msg = Ping
        self.requests_queue.put(msg)
        time0 = self.time()
        while self.replies_queue.empty():
            if self.closing or self.exception:
                return False
            if self.timeouts and (self.time() - time0 > 5):
                return False
            self.sleep(0.001)
        reply = self.replies_queue.get()
        while self.__CheckAsync(reply):
            time0 = self.time()
            while self.replies_queue.empty():
                if self.closing or self.exception:
                    return False
                if self.timeouts and (self.time() - time0 > 5):
                    return False
                self.sleep(0.001)
            reply = self.replies_queue.get()
        msg = self.__interpret_read(reply)
        self.DoneFlag = True
        return msg

####################################################################################################################################################
    def img_tv_activate(self, demodID, channelID):
        '''sends activate message'''
        Id = self.__IdGen()
        while Id != self.CurId:
            if self.CurId == 0x5e4000:
                return 0
            self.sleep(0.001)
        try:
            Activate = (0x1c, 0x0, 0x0, self.message_id,
                        0x02, 0x08, demodID, channelID)
        except TypeError:
            self.tlfObj.record( 'transactions: ERROR: demodID and channelID for img_tv_activate function must be either long, int or hex numbers.', 1, 1)
            self.DoneFlag = True
            return False
        if self.message_id >=27:
            self.message_id = 0
        else:
            self.message_id+=1
        msg = Activate
        self.requests_queue.put(msg)
        time0 = self.time()
        while self.replies_queue.empty():
            if self.closing or self.exception:
                return False
            if self.timeouts and (self.time() - time0 > 5):
                return False
            self.sleep(0.001)
        reply = self.replies_queue.get()
        while self.__CheckAsync(reply):
            time0 = self.time()
            while self.replies_queue.empty():
                if self.closing or self.exception:
                    return False
                if self.timeouts and (self.time() - time0 > 5):
                    return False
                self.sleep(0.001)
            reply = self.replies_queue.get()
        msg = self.__interpret_read(reply)
        self.DoneFlag = True
        return msg
####################################################################################################################################################        
    def img_tv_deactivate(self, targetID):
        '''sends deactivate message. N.B. the TV demodulator must be put into its intialised state before it is deactivated.'''#
        Id = self.__IdGen()
        while Id != self.CurId:
            if self.CurId == 0x5e4000:
                return 0
            self.sleep(0.001)
        try:
            Deactivate = (0x14, 0x0, targetID, self.message_id, 0x04, 0x0)
        except TypeError:
            self.tlfObj.record('transactions: ERROR: targetID for img_tv_deactivate function must be either long, int or hex numbers.', 1, 1)
            self.DoneFlag = True
            return False
        if self.message_id >=27:
            self.message_id = 0
        else:
            self.message_id+=1
        msg = Deactivate
        self.requests_queue.put(msg)
        time0 = self.time()
        while self.replies_queue.empty():
            if self.closing or self.exception:
                return False
            if self.timeouts and (self.time() - time0 > 5):
                return False
            self.sleep(0.001)
        reply = self.replies_queue.get()
        while self.__CheckAsync(reply):
            time0 = self.time()
            while self.replies_queue.empty():
                if self.closing or self.exception:
                    return False
                if self.timeouts and (self.time() - time0 > 5):
                    return False
                self.sleep(0.001)
            reply = self.replies_queue.get()
        msg = self.__interpret_read(reply)
        self.DoneFlag = True
        return msg
####################################################################################################################################################        
    def img_tv_setreg(self, targetID, regID, value):
        '''sends set register message'''
        Id = self.__IdGen()
        while Id != self.CurId:
            if self.CurId == 0x5e4000:
                return 0
            self.sleep(0.001)
        try:
            SetReg = (0x1c, 0x0, targetID, self.message_id, 0x06, 0x08, regID, value)
        except TypeError:
            self.tlfObj.record('transactions: ERROR: targetID, regID and value for img_tv_setreg function must be either long, int or hex numbers.', 1, 1)
            self.DoneFlag = True
            return False
        if self.message_id >=27:
            self.message_id = 0
        else:
            self.message_id+=1
        msg = SetReg
        self.requests_queue.put(msg)
        time0 = self.time()
        while self.replies_queue.empty():
            if self.closing or self.exception:
                return False
            if self.timeouts and (self.time() - time0 > 5):
                return False
            self.sleep(0.001)
        reply = self.replies_queue.get()
        while self.__CheckAsync(reply):
            time0 = self.time()
            while self.replies_queue.empty():
                if self.closing or self.exception:
                    return False
                if self.timeouts and (self.time() - time0 > 5):
                    return False
                self.sleep(0.001)
            reply = self.replies_queue.get()
        msg = self.__interpret_read(reply)
        self.DoneFlag = True
        return msg
####################################################################################################################################################        
    def img_tv_getreg(self, targetID, regID):
        '''sends get register message'''
        Id = self.__IdGen()
        while Id != self.CurId:
            if self.CurId == 0x5e4000:
                return 0
            self.sleep(0.001)
        try:
            GetReg = (0x18, 0x0, targetID, self.message_id, 0x08, 0x04, regID)
        except TypeError:
            self.tlfObj.record('transactions: ERROR: targetID and regID for img_tv_getreg function must be either long, int or hex numbers.', 1, 1)
            self.DoneFlag = True
            return False
        if self.message_id >=27:
            self.message_id = 0
        else:
            self.message_id+=1
        msg = GetReg
        self.requests_queue.put(msg)
        time0 = self.time()
        while self.replies_queue.empty():
            if self.closing or self.exception:
                return False
            if self.timeouts and (self.time() - time0 > 5):
                return False
            self.sleep(0.001)
        reply = self.replies_queue.get()
        while self.__CheckAsync(reply):
            time0 = self.time()
            while self.replies_queue.empty():
                if self.closing or self.exception:
                    return False
                if self.timeouts and (self.time() - time0 > 5):
                    return False
                self.sleep(0.001)
            reply = self.replies_queue.get()
        msg = self.__interpret_read(reply)
        self.DoneFlag = True
        return msg
####################################################################################################################################################
    def img_tv_auto_on(self, targetID, regID):
        '''sends start automatic register notifications request message'''
        Id = self.__IdGen()
        while Id != self.CurId:
            if self.CurId == 0x5e4000:
                return 0
            self.sleep(0.001)
        try:
            Auto_on = (0x18, 0x0, targetID, self.message_id, 0x0A, 0x04, regID)
        except TypeError:
            self.tlfObj.record('transactions: targetID and regId for img_tv_auto_on function must be long, int or hex numbers.', 1, 1)
            self.DoneFlag = True
            return False
        if self.message_id >=27:
            self.message_id = 0
        else:
            self.message_id+=1
        msg = Auto_on
        self.requests_queue.put(msg)
        time0 = self.time()
        while self.replies_queue.empty():
            if self.closing or self.exception:
                return False
            if self.timeouts and (self.time() - time0 > 5):
                return False
            self.sleep(0.001)
        reply = self.replies_queue.get()
        while self.__CheckAsync(reply):
            time0 = self.time()
            while self.replies_queue.empty():
                if self.closing or self.exception:
                    return False
                if self.timeouts and (self.time() - time0 > 5):
                    return False
                self.sleep(0.001)
            reply = self.replies_queue.get()
        msg = self.__interpret_read(reply)
        self.DoneFlag = True
        return msg
####################################################################################################################################################        
    def img_tv_auto_off(self, targetID, regID):
        '''sends stop automatic register updates request message'''
        Id = self.__IdGen()
        while Id != self.CurId:
            if self.CurId == 0x5e4000:
                return 0
            self.sleep(0.001)
        try:
            Auto_off = (0x18, 0x0, targetID, self.message_id, 0x0C, 0x04, regID)
        except TypeError:
            self.tlfObj.record('transactions: ERROR: targetID and regID for img_tv_auto_off function must be either long, int or hex numbers.', 1, 1)
            self.DoneFlag = True
            return False
        if self.message_id >=27:
            self.message_id = 0
        else:
            self.message_id+=1
        msg = Auto_off
        self.requests_queue.put(msg)
        time0 = self.time()
        while self.replies_queue.empty():
            if self.closing or self.exception:
                return False
            if self.timeouts and (self.time() - time0 > 5):
                return False
            self.sleep(0.001)
        reply = self.replies_queue.get()
        while self.__CheckAsync(reply):
            time0 = self.time()
            while self.replies_queue.empty():
                if self.closing or self.exception:
                    return False
                if self.timeouts and (self.time() - time0 > 5):
                    return False
                self.sleep(0.001)
            reply = self.replies_queue.get()
        msg = self.__interpret_read(reply)
        t0 = self.time()
        self.DoneFlag = True
        return msg
####################################################################################################################################################
    def __end_thread(self):
        '''ends the Target thread'''
        list =[]
        if self.replies_queue.qsize() != 0:
            size = self.replies_queue.qsize()
            self.tlfObj.record('transactions: messages remaining in reply queue...being erased', 1, 1)
            i=0
            while i<=size:
                list.append(self.replies_queue.get())
                i += 1
        self.__targ.end_target()
        while self.replies_queue.empty():
            if self.exception:
                return
            self.sleep(0.001)
        reply = self.replies_queue.get()
        while reply != 'end':
            if self.exception:
                return
            list.append(reply)
            if reply[3] & 0x10000 != 0:
                self.async_queue.put(self.__interpret_read(reply))
            while self.replies_queue.empty():
                if self.exception:
                    return
                self.sleep(0.001)
            reply = self.replies_queue.get()
        self.closing = True
        self.__targ.closing = True
        while self.__targ.isAlive():
            if self.exception:
                return
            self.sleep(0.001)
        while self.Idhandle.isAlive():
            if self.exception:
                return
            self.sleep(0.001)
        return True
####################################################################################################################################################    
    def load_code(self, prog = 0, daid = 0):
        '''hard resets the target and loads the code'''
        Id = self.__IdGen()
        while Id != self.CurId:
            if self.CurId == 0x5e4000:
                return 0
            self.sleep(0.001)
        if self.replies_queue.qsize() != 0:
            size = self.replies_queue.qsize()
            self.tlfObj.record( 'transactions: messages remaining in reply queue...being erased', 1, 1)
            i=0
            while i<=size:
                self.replies_queue.get()
                i+=1
        #self.requests_queue.put('load')
        if (prog != 0) and (daid != 0):
            self.load_queue.put((prog, daid))
        else:
            self.load_queue.put('common')
        try:
            self.__targ.Load_Program(self.loaded)
        except GenericError:
            return 'Error'   
        while self.load_queue.empty():
            if self.CurId == 0x5e4000:
                return
            self.sleep(0.001)
        response = self.load_queue.get()
        if not self.loaded:
            self.__targ.start()
            self.loaded = True
        while self.replies_queue.empty():
            if self.exception:
                return
            self.sleep(0.001)
        response = self.replies_queue.get()
        if response[4] != 1:
            self.DoneFlag = True
            return False
        self.DoneFlag = True
        return True
        
        
####################################################################################################################################################
    def async(self):
        '''returns the asynchronous messages'''
        while self.async_queue.empty():
            while self.replies_queue.empty():
                if (self.CurId == 0x5e4000) or self.exception:
                    return (0, 0)
                self.sleep(0.001)
            reply = self.replies_queue.get()
            if reply == 'end':
                self.replies_queue.put('end')
            if self.CurId == 0x5e4000 or self.exception:
                    return (0, 0)
            try:
                if reply[3]& 0x10000 != 0:
                    self.async_queue.put(self.__interpret_read(reply))
                else:
                    self.replies_queue.put(reply)
            except IndexError:
                pass
            self.sleep(0.001)
        msg = self.async_queue.get() 
        return msg
####################################################################################################################################################
    def __IdGen(self):
        newId = 0x940000 | random.randint(0, 0xffff)
        while newId in self.IdLi:
            newId = 0x940000 | random.randint(0, 0xffff)
            self.sleep(0.001)
        self.IdLi.append(newId)
        return newId
    
    def __IdSort(self):
        while (self.CurId != 0x5e4000) and not self.exception:
            while len(self.IdLi) < 1:
                if (self.CurId == 0x5e4000) or self.exception:
                    return
                self.sleep(0.001)
            timein = self.time()
            while not self.DoneFlag:
                if (self.CurId == 0x5e4000) or self.exception:
                    return
                if self.timeouts and (self.time() - timein > 5):
                    break
                self.sleep(0.001)
            self.DoneFlag = False
            self.CurId = self.IdLi.pop(0)
####################################################################################################################################################
    def exception_handler(self, kill_GUI = True):
        if self.__targ.is_alive():
            self.__targ.exception_handler()
        if self.GUI and kill_GUI:
            self.GUI.exception_handler()
        self.exception = True
        self.CurId = 0x5e4000
        return
####################################################################################################################################################
    def host_port_shutdown(self):
        self.closing = True
        self.CurId = 0x5e4000
        self.__end_thread()
        return True
    
####################################################################################################################################################
    def External_cmd(self, cmd = 0, tmlim = 0):
        if cmd != 0:
            self.__targ.Pause_target()
            try:
                retval = eval(cmd)
                self.__targ.Unpause_target()
                return retval
            except:
                self.__targ.Unpause_target()
                return False
        elif tmlim != 0:
            self.__targ.Pause_target()
            t0 = self.time()
            while self.time() - t0 < tmlim:
                time.sleep(0.001)
                pass
            self.__targ.Unpause_target()
            return True
        else:
            self.__targ.Pause_target()
            return True
    
    def Resume_targ(self):
        self.__targ.Unpause_target()
        return True
        
####################################################################################################################################################
    def compDA(self, da = None):
        DA_list = self.__targ.getTargetList()
        if da == None:
            return DA_list
        if type(da) == type('str'):
            if da in DA_list:
                return True
            else:
                return False
        cor_DA = []
        for item in da:
            if item in DA_list:
                cor_DA.append(item)
        if len(cor_DA) > 0:
            return cor_DA
        return False
####################################################################################################################################################
    def GetConnection(self):
        self.__targ.Buffer = True
        self.__targ.ActivateBuffer()
        self.__targ.start()
        
####################################################################################################################################################

    def getTargetstatus(self):
        return (self.__targ.BufferH, self.__targ.BufferU, self.__targ.is_alive())
    
####################################################################################################################################################

    def ToggleTimeouts(self):
        if self.timeouts:
            self.__targ.timeouts = False
            self.timeouts = False
            return False
        self.__targ.timeouts = True
        self.timeouts = True
        return True
    
####################################################################################################################################################
    ######################
    #    FAKE TLF OBJ    #
    ######################
class Tlf_Fake:
    def record(self, string, foo, bar):
        pass