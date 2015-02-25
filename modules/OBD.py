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

# Classes
class WatchDog:

    def __init__(self):
        pass

    """
    Handler Functions
    """
    ## Render Index
    @cherrypy.expose
    def index(self):
        html = open('static/index.html').read()
        return html
    
    ## Handle Posts
    @cherrypy.expose
    def default(self, *args, **kwargs):
        try:
            pretty_print('MSG', str(kwargs))
        except Exception as err:
            pretty_print('ERROR', str(err))
        return None
    
# Main
if __name__ == '__main__':
    daemon = HiveAggregator(CONFIG_FILE)
    cherrypy.server.socket_host = aggregator.CHERRYPY_ADDR
    cherrypy.server.socket_port = aggregator.CHERRYPY_PORT
    currdir = os.path.dirname(os.path.abspath(__file__))
    conf = {
        '/': {'tools.staticdir.on':True, 'tools.staticdir.dir':os.path.join(currdir,'static')},
        '/data': {'tools.staticdir.on':True, 'tools.staticdir.dir':os.path.join(currdir,'data')}, # NEED the '/' before the folder name
    }
    cherrypy.quickstart(aggregator, '/', config=conf)
