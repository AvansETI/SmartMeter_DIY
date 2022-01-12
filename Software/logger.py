import logging

# Setup logger
logging.basicConfig(level=logging.DEBUG,
                    format="%(asctime)s %(name)s %(levelname)-8s %(thread)d %(message)s",
                    datefmt="%Y-%m-%d %H:%M:%S")
logger = logging.getLogger("emon_mqtt2influx")
logger.setLevel(logging.DEBUG)
