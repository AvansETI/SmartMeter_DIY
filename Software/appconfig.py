import time, os, signal, sys, json
from logger import logger

class AppConfig(object):

    def __init__(self):
        # Load config file
        ROOT_DIR = os.path.dirname(os.path.realpath(__file__))
        CONFIG_FILE = os.path.abspath(os.path.join(ROOT_DIR, './config.json'))
        try:
            with open(CONFIG_FILE) as config:
                self.app_config = json.load(config)
            config.close()
        except Exception as ex:
            logger.error('Error reading config file')

    def __getitem__(self, item):
        return self.app_config[item]
