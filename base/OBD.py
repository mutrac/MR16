"""
OBD.py - Onboard Diagnostic System

This system behaves as the hub for all controller activity and as such hosts
the ZMQ server.
"""
__author__ = 'Trevor Stanhope'
__version___ = 0.1

# Dependencies
import logging
import zmq
import pymongo
import cherrypy
from cherrypy.process.plugins import Monitor
from cherrypy import tools
import os
from datetime import datetime
import thread
import json

# Useful Functions 
def pretty_print(task, msg):
    date = datetime.strftime(datetime.now(), '[%d/%b/%Y:%H:%M:%S]')
    print("%s %s %s" % (date, task, msg))

# Classes
class WatchDog:

    ## Init
    def __init__(self, config):
        self.config = config
        self.data = {}
        self.init_cmq()
        self.init_db()
        self.init_logging()
        
    def __close__(self):
        thread.exit()
    
    ## Initialize Messenger Query
    def init_cmq(self):
        try:
            Monitor(cherrypy.engine, self.listen, frequency=0.01).subscribe()
            pretty_print('OBD', 'Initialized ZMQ listener')
            self.context = zmq.Context()
            self.socket = self.context.socket(zmq.REP)
            self.socket.bind(self.config['CMQ_SERVER'])
            pretty_print('OBD', 'Initialized ZMQ host')
        except Exception as error:
            pretty_print('OBD_ERR', str(error))
        
    ## Initialize DB
    def init_db(self):
        try:
            addr = self.config['MONGO_ADDR']
            port = self.config['MONGO_PORT']
            self.mongo_client = pymongo.MongoClient(addr, port)
            self.db_name = datetime.strftime(datetime.now(), '%Y%m%d')
            self.db = self.mongo_client[self.db_name]
            pretty_print('OBD', 'Initialized DB on %s:%d' % (addr, port))
        except Exception as error:
            pretty_print('OBD_ERR', str(error))

    ## Initialize Logging
    def init_logging(self):
        try:
            self.log_path = os.path.join(os.getcwd(), 'log', datetime.strftime(datetime.now(), self.config['LOG_FILE']))
            logging.basicConfig(filename=self.log_path, level=logging.DEBUG)
        except Exception as error:
            pretty_print('OBD_ERR', str(error))
   
    ## Make Event
    def make_event(self, task, data):
        e = {
            'type' : task,
            'data' : data,
            'time' : datetime.now()
        }
        return e
        
    ## Add Log Entry
    def add_log_entry(self, event):
        try:
            task = event['type']
            uuid = self.db[task].insert(event)
            pretty_print(task, str(uuid))
        except Exception as error:
            pretty_print('OBD ERR', str(error))
    
    ## Listen for Messages
    #! TODO Include setting warnings for the debugger page
    def listen(self):
        try:
            # Receive message from CAN
            pretty_print('OBD', 'Listening')
            packet = self.socket.recv()
            event = json.loads(packet)
            pretty_print('OBD', 'RECEIVED: %s' % str(event))
            
            # TODO: Handle data or errors
            self.add_log_entry(event)
            
            # Send response to CAN
            if event['type'] == 'HUD':
                response = {
                    'type' : 'OBD',
                    'data' : self.data
                }
            elif event['type'] == 'CMQ':
                # TODO Set incoming data to the global "data" object
                self.data.update(event['data'])
                response = {
                    'type' : 'OBD',
                    'data' : {}
                }
            dump = json.dumps(response)
            self.socket.send(dump)
            pretty_print('OBD', str(response))
        except Exception as error:
            pretty_print('OBD', str(error))
    
    """
    Handler Functions
    """
    ## Render Index
    @cherrypy.expose
    def index(self):
        html = open('static/index.html').read()
        #! Add render of error page
        return html  
    
    ## Handle Posts
    @cherrypy.expose
    def default(self, *args, **kwargs):
        try:
            #! Handle requests, such as for logs of pulls
            pass
        except Exception as error:
            pretty_print('OBD', str(error))
        return None

if __name__ == '__main__':
    with open('config/OBD_v1.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load config file 
    daemon = WatchDog(config) # start watchdog
    cherrypy.server.socket_host = daemon.config['CHERRYPY_ADDR']
    cherrypy.server.socket_port = daemon.config['CHERRYPY_PORT']
    currdir = os.path.dirname(os.path.abspath(__file__))
    conf = {
        '/' : {'tools.staticdir.on':True, 'tools.staticdir.dir':os.path.join(currdir,'static')},
        '/data' : {'tools.staticdir.on':True, 'tools.staticdir.dir':os.path.join(currdir,'data')}, # NEED the '/' before the folder name
    }
    cherrypy.quickstart(daemon, '/', config=conf)
