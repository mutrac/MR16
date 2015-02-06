"""
    OBD
"""
__author__ = 'Trevor Stanhope'
__version___ = 0.1

# Dependencies
import logging
import pymongo
import cherrypy
from cherrypy.process.plugins import Monitor
from cherrypy import tools
import os
from datetime import datetime
import thread
import json

# Classes
class WatchDog:

    ## Init
    def __init__(self, config):
        self.config = config
        self.init_db()
        self.init_logging()
        thread.start_new_thread(self.run, ())
        
    def __close__(self):
        thread.exit()
    
    ## Run
    def run(self):
        cherrypy.server.socket_host = self.config['CHERRYPY_ADDR']
        cherrypy.server.socket_port = self.config['CHERRYPY_PORT']
        currdir = os.path.dirname(os.path.abspath(__file__))
        conf = {
        '/': {'tools.staticdir.on':True, 'tools.staticdir.dir':os.path.join(currdir,'static')},
        '/data': {'tools.staticdir.on':True, 'tools.staticdir.dir':os.path.join(currdir,'data')}, # NEED the '/' before the folder name
        }
        cherrypy.quickstart(self, '/', config=conf)
        
    ## Initialize DB
    def init_db(self):
        try:
            self.mongo_client = pymongo.MongoClient(self.config['MONGO_ADDR'], self.config['MONGO_PORT'])        
            self.add_log_entry(self.make_event('DB', 'OKAY - Initialized DB'))
        except Exception as error:
            self.add_log_entry(self.make_event('ERROR', str(error)))

    ## Initialize Logging
    def init_logging(self):
        try:
            self.log_path = '%s/%s' % (self.config['LOG_DIR'], datetime.strftime(datetime.now(), self.config['LOG_FILE']))
            logging.basicConfig(filename=self.log_path, level=logging.DEBUG)
        except Exception as error:
            self.add_log_entry(self.make_event('ERROR', str(error)))
    
        
    def make_event(self, task, msg):
        e = {
            'type' : task,
            'msg' : msg
        }
        return e
        
    ## Add Log Entry
    def add_log_entry(self, event):
        task = event['type']
        msg = event['msg'] #!
        event['time'] = datetime.strftime(datetime.now(), '%Y/%m/%s %H:%M:%S.f')
        date = datetime.strftime(datetime.now(), '%d/%b/%Y:%H:%M:%S')
        db_name = datetime.strftime(datetime.now(), '%Y%m%d')
        db = self.mongo_client[db_name]
        uuid = db[task].insert(event)
        print('%s %s %s' % (date, task, msg))

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
            self.add_log_entry(self.make_event('ERROR', str(error)))
        return None
    
# Main
if __name__ == '__main__':
    with open('obd_config.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read())
    daemon = WatchDog(config)
    daemon.run()
